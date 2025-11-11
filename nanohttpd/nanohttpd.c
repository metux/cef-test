#define _POSIX_C_SOURCE 200809L
#include "nanohttpd.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>

/* ------------ handler registry ------------ */

struct handler_entry {
    char *prefix;
    nanohttpd_handler_fn fn;
    void *user;
    struct handler_entry *next;
};

static void nanohttpd_init(nanohttpd_server *server) {
    if (server->initialized)
        return;

    server->handlers = NULL;
    pthread_mutex_init(&server->handlers_lock, NULL);
    server->initialized = 1;
}

int nanohttpd_register_handler(nanohttpd_server *server,
                               const char *prefix,
                               nanohttpd_handler_fn fn,
                               void *user_data) {
    nanohttpd_init(server);

    struct handler_entry *h = calloc(1, sizeof(*h));
    if (!h) return -1;
    h->prefix = strdup(prefix);
    h->fn = fn;
    h->user = user_data;
    pthread_mutex_lock(&server->handlers_lock);
    h->next = server->handlers;
    server->handlers = h;
    pthread_mutex_unlock(&server->handlers_lock);
    return 0;
}

/* ------------ utilities ------------ */

void nanohttpd_urldecode(char *s) {
    char *dst = s;
    while (*s) {
        if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
            char hex[3] = { s[1], s[2], 0 };
            *dst++ = (char) strtol(hex, NULL, 16);
            s += 3;
        } else if (*s == '+') {
            *dst++ = ' ';
            s++;
        } else {
            *dst++ = *s++;
        }
    }
    *dst = 0;
}

static ssize_t sendall(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    const char *p = buf;
    while (sent < len) {
        ssize_t rv = write(fd, p + sent, len - sent);
        if (rv < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += rv;
    }
    return sent;
}

/* Send simple text response */
void nanohttpd_xfer_reply_text(nanohttpd_xfer *xfer,
                               const char *status,
                               const char *content_type,
                               const char *text)
{
    if (!content_type)
        content_type = "text/plain";

    char header[256];
    int hdrlen = snprintf(header, sizeof(header),
                          "HTTP/1.0 %s\r\nContent-Length: %zu\r\n"
                          "Content-Type: %s\r\n\r\n",
                          status, strlen(text), content_type);
    sendall(xfer->fd, header, hdrlen);
    sendall(xfer->fd, text, strlen(text));
}

/* ------------ request handler ------------ */

struct client_ctx {
    int fd;
    nanohttpd_server *server;
};

static void free_headers(struct nanohttpd_header *h) {
    while (h) {
        struct nanohttpd_header *n = h->next;
        free(h->name);
        free(h->value);
        free(h);
        h = n;
    }
}

static void add_req_header(nanohttpd_xfer *xfer, const char* name, const char *value) {
    fprintf(stderr, "add_req_header: name=\"%s\" value=\"%s\"\n", name, value);
    nanohttpd_header *hdr = calloc(1, sizeof(nanohttpd_header));
    hdr->name = strdup(name);
    hdr->value = strdup(value);
    hdr->next = xfer->req_headers;
    xfer->req_headers = hdr;

    if (strcasecmp(name, "Content-Length") == 0) {
        char *end;
        long cl = strtol(value, &end, 10);
        if (end != value && *end == '\0' && cl >= 0)
            xfer->req_content_length = (size_t)cl;
    }
}

static void *client_thread(void *arg) {
    struct client_ctx *ctx = arg;

    char buf[4096];
    ssize_t read_size = read(ctx->fd, buf, sizeof(buf)-1);
    if (read_size <= 0)
        goto done;
    buf[read_size] = 0;

    nanohttpd_xfer srv_xfer = {
        .fd = ctx->fd,
        .server = ctx->server,
        .matched_prefix = "",
    };

    char method[16], xpath[1024], vers[32];
    if (sscanf(buf, "%15s %1023s %31s", method, xpath, vers) != 3) {
        nanohttpd_xfer_reply_text(&srv_xfer,
                                  NANOHTTPD_RESPONSE_BAD_REQUEST,
                                  NULL,
                                  "Bad Request\n");
        goto done;
    }

    if ((strcmp(method,"GET") != 0) && (strcmp(method,"POST") !=0) && (strcmp(method,"PUT") != 0)) {
        nanohttpd_xfer_reply_text(&srv_xfer,
                                  NANOHTTPD_RESPONSE_NOT_IMPLEMENTED,
                                  NULL,
                                  "Only GET/POST/PUT supported\n");
        goto done;
    }

    srv_xfer.req_method = method;

    // fix path: strip extra leading slashes
    char *path = xpath;
    while (path[0] == '/' && path[1] == '/') path++;

    char *q = strpbrk(path, "?#"); /* strip query */
    if (q) *q = 0;

    /* parse headers */
    char *hdr_start = strstr(buf, "\r\n");
    if (hdr_start) {
        hdr_start += 2;                     /* skip CRLF after request line */
        char *hdr_end = strstr(hdr_start, "\r\n\r\n");
        if (hdr_end) hdr_end += 2;          /* point to body start (not used) */
        else hdr_end = buf + read_size;     /* fallback */

        char *line = hdr_start;
        while (line < hdr_end) {
            char *next_line = strstr(line, "\r\n");
            if (!next_line) next_line = hdr_end;

            char *colon = strchr(line, ':');
            if (colon && colon < next_line) {
                *colon = '\0';
                char *value = colon + 1;
                while (*value == ' ' || *value == '\t') value++;
                char *eol = next_line;
                while (eol > value && (eol[-1] == ' ' || eol[-1] == '\t' || eol[-1] == '\r')) eol--;
                *eol = '\0';

                add_req_header(&srv_xfer, line, value);
            }
            line = next_line;
            if (line < hdr_end) {
                line += 2;
            }
        }

        char *body_text = hdr_end;
        if ((body_text[0] == '\r') && (body_text[1] == '\n'))
            body_text += 2;

        size_t remaining = (&buf[read_size]) - body_text;
        if (srv_xfer.req_content_length > remaining) {
            nanohttpd_xfer_reply_text(&srv_xfer,
                                      NANOHTTPD_RESPONSE_BAD_REQUEST,
                                      NULL,
                                      "Content-Length too big\n");
            goto done;
        }

        char *new_body = calloc(1, srv_xfer.req_content_length + 1);
        memcpy(new_body, body_text, srv_xfer.req_content_length);
        srv_xfer.req_body = new_body;
    }

    /* check handlers */
    pthread_mutex_lock(&ctx->server->handlers_lock);
    struct handler_entry *h = ctx->server->handlers;
    while (h) {
        if (strncmp(path, h->prefix, strlen(h->prefix)) == 0) {
            nanohttpd_handler_fn fn = h->fn;
            pthread_mutex_unlock(&ctx->server->handlers_lock);

            char *remaining = path + strlen(h->prefix);
            while (remaining[0] == '/') remaining++;

            fprintf(stderr, "-> remaining \"%s\"\n", remaining);

            nanohttpd_xfer xfer = {
                .fd = ctx->fd,
                .path = path,
                .user_data = h->user,
                .server = ctx->server,
                .matched_prefix = h->prefix,
                .remaining = remaining,
                .req_method = srv_xfer.req_method,
                .req_headers = srv_xfer.req_headers,
                .req_content_length = srv_xfer.req_content_length,
                .req_body = srv_xfer.req_body,
            };
            fn(&xfer);
            goto done;
        }
        h = h->next;
    }

    free_headers(srv_xfer.req_headers);
    free((char*)srv_xfer.req_body);

    pthread_mutex_unlock(&ctx->server->handlers_lock);
    nanohttpd_xfer_reply_text(&srv_xfer,
                              NANOHTTPD_RESPONSE_NOT_FOUND,
                              NULL,
                              "Not Found\n");

done:
    close(ctx->fd);
    free(ctx);
    return NULL;
}

/* ------------ server loop ------------ */

int nanohttpd_serve(nanohttpd_server *server) {
    nanohttpd_init(server);

    int port = atoi(server->port_str);
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return -1; }

    int on = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons((uint16_t)port);

    if (bind(srv, (struct sockaddr*)&sa, sizeof(sa)) < 0) { perror("bind"); return -1; }
    if (listen(srv, 64) < 0) { perror("listen"); return -1; }

    server->running = true;
    while (server->running) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int cl = accept(srv, (struct sockaddr*)&cli, &clilen);
        if (cl < 0) { if (errno==EINTR) continue; perror("accept"); break; }

        struct client_ctx *ctx = malloc(sizeof(*ctx));
        ctx->fd = cl;
        ctx->server = server;

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, ctx);
        pthread_detach(tid);
    }

    close(srv);
    return 0;
}

static void *__serverthread(void *arg) {
    nanohttpd_server *server = (nanohttpd_server*)arg;
    nanohttpd_serve(server);
    server->running = false;
}

int nanohttpd_serve_thread(nanohttpd_server *server) {
    server->running = true;
    pthread_t tid;
    pthread_create(&tid, NULL, __serverthread, server);
    pthread_detach(tid);
}

void nanohttpd_xfer_reply_ok_text(nanohttpd_xfer *xfer,
                                  const char *content_type,
                                  const char *data)
{
    nanohttpd_xfer_reply_text(xfer, "200 OK", content_type, data);
}

int nanohttpd_next_elem_int_dec(nanohttpd_xfer *xfer)
{
    while (xfer->remaining[0] == '/') xfer->remaining++;
    char *endptr = NULL;
    uint32_t val = strtol(xfer->remaining, &endptr, 10);
    xfer->remaining = endptr;
    while (xfer->remaining[0] == '/') xfer->remaining++;
    return val;
}

int nanohttpd_next_elem_int_hex(nanohttpd_xfer *xfer)
{
    while (xfer->remaining[0] == '/') xfer->remaining++;
    char *endptr = NULL;
    uint32_t val = strtol(xfer->remaining, &endptr, 16);
    xfer->remaining = endptr;
    while (xfer->remaining[0] == '/') xfer->remaining++;
    return val;
}

const char* nanohttpd_next_elem_path(nanohttpd_xfer *xfer)
{
    while (xfer->remaining[0] == '/') xfer->remaining++;
    char *start = xfer->remaining;

    while ((xfer->remaining[0]) && (xfer->remaining[0] != '/'))
        xfer->remaining++;

    if (xfer->remaining[0]) {
        xfer->remaining[0] = 0;
        xfer->remaining++;
    }

    return start;
}

void nanohttpd_shutdown(nanohttpd_server *server)
{
    server->running = 0;
}

/* find header case-insensitive */
const char *nanohttpd_find_header(nanohttpd_xfer *xfer, const char *name) {
    nanohttpd_header *h = xfer->req_headers;
    while (h) {
        if (strcasecmp(h->name, name) == 0)
            return h->value;
        h = h->next;
    }
    return NULL;
}

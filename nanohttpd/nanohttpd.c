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

static void *client_thread(void *arg) {
    struct client_ctx *ctx = arg;

    char buf[4096];
    ssize_t n = read(ctx->fd, buf, sizeof(buf)-1);
    if (n <= 0)
        goto done;
    buf[n] = 0;

    nanohttpd_xfer srv_xfer = {
        .fd = ctx->fd,
        .server = ctx->server,
        .matched_prefix = "",
    };

    char method[16], path[1024], vers[32];
    if (sscanf(buf, "%15s %1023s %31s", method, path, vers) != 3) {
        nanohttpd_xfer_reply_text(&srv_xfer,
                                  NANOHTTPD_RESPONSE_BAD_REQUEST,
                                  NULL,
                                  "Bad Request\n");
        goto done;
    }
    if (strcmp(method,"GET") != 0) {
        nanohttpd_xfer_reply_text(&srv_xfer,
                                  NANOHTTPD_RESPONSE_NOT_IMPLEMENTED,
                                  NULL,
                                  "Only GET supported\n");
        goto done;
    }

//    urldecode(path);
    char *q = strpbrk(path, "?#"); /* strip query */
    if (q) *q = 0;

    /* check handlers */
    pthread_mutex_lock(&ctx->server->handlers_lock);
    struct handler_entry *h = ctx->server->handlers;
    while (h) {
        if (strncmp(path, h->prefix, strlen(h->prefix)) == 0) {
            nanohttpd_handler_fn fn = h->fn;
            pthread_mutex_unlock(&ctx->server->handlers_lock);
            nanohttpd_xfer xfer = {
                .fd = ctx->fd,
                .path = path,
                .user_data = h->user,
                .server = ctx->server,
                .matched_prefix = h->prefix,
            };
            fn(&xfer);
            goto done;
        }
        h = h->next;
    }
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

    fprintf(stderr,"Serving on port %d\n", port);
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
    fprintf(stderr, "server thread: port=%s\n", server->port_str);
    nanohttpd_serve(server);
    server->running = false;
}

int nanohttpd_serve_thread(nanohttpd_server *server) {
    fprintf(stderr, "starting server thread\n");
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

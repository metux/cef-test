#define _POSIX_C_SOURCE 200809L
#include "minihttp.h"

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
    http_handler_fn fn;
    void *user;
    struct handler_entry *next;
};

static struct handler_entry *handlers = NULL;
static pthread_mutex_t handlers_lock = PTHREAD_MUTEX_INITIALIZER;

int httpd_register_handler(const char *prefix, http_handler_fn fn, void *user_data) {
    struct handler_entry *h = calloc(1, sizeof(*h));
    if (!h) return -1;
    h->prefix = strdup(prefix);
    h->fn = fn;
    h->user = user_data;
    pthread_mutex_lock(&handlers_lock);
    h->next = handlers;
    handlers = h;
    pthread_mutex_unlock(&handlers_lock);
    return 0;
}

/* ------------ utilities ------------ */

static const char *guess_mime(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    ext++;
    if (strcasecmp(ext,"html")==0) return "text/html";
    if (strcasecmp(ext,"txt")==0)  return "text/plain";
    if (strcasecmp(ext,"png")==0)  return "image/png";
    if (strcasecmp(ext,"jpg")==0 || strcasecmp(ext,"jpeg")==0) return "image/jpeg";
    return "application/octet-stream";
}

static void urldecode(char *s) {
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
static void send_text(int fd, const char *status, const char *text) {
    char header[256];
    int hdrlen = snprintf(header, sizeof(header),
                          "HTTP/1.0 %s\r\nContent-Length: %zu\r\n"
                          "Content-Type: text/plain\r\n\r\n",
                          status, strlen(text));
    sendall(fd, header, hdrlen);
    sendall(fd, text, strlen(text));
}

/* ------------ request handler ------------ */

struct client_ctx {
    int fd;
    char docroot[1024];
};

static void *client_thread(void *arg) {
    struct client_ctx *ctx = arg;
    int fd = ctx->fd;
    char docroot[1024];
    strncpy(docroot, ctx->docroot, sizeof(docroot)-1);
    free(ctx);

    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n <= 0) { close(fd); return NULL; }
    buf[n] = 0;

    char method[16], path[1024], vers[32];
    if (sscanf(buf, "%15s %1023s %31s", method, path, vers) != 3) {
        send_text(fd, "400 Bad Request", "Bad Request\n");
        close(fd); return NULL;
    }
    if (strcmp(method,"GET") != 0) {
        send_text(fd, "501 Not Implemented", "Only GET supported\n");
        close(fd); return NULL;
    }

    urldecode(path);
    char *q = strpbrk(path, "?#"); /* strip query */
    if (q) *q = 0;

    /* check handlers */
    pthread_mutex_lock(&handlers_lock);
    struct handler_entry *h = handlers;
    while (h) {
        if (strncmp(path, h->prefix, strlen(h->prefix)) == 0) {
            http_handler_fn fn = h->fn;
            void *ud = h->user;
            pthread_mutex_unlock(&handlers_lock);
            fn(fd, path, ud);
            close(fd);
            return NULL;
        }
        h = h->next;
    }
    pthread_mutex_unlock(&handlers_lock);

    /* default: static file */
    char filepath[2048];
    snprintf(filepath, sizeof(filepath), "%s/%s",
             docroot, path[0]=='/' ? path+1 : path);

    struct stat st;
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) {
        send_text(fd, "404 Not Found", "Not Found\n");
        close(fd); return NULL;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        send_text(fd, "500 Internal Server Error", "Cannot open file\n");
        close(fd); return NULL;
    }

    char header[256];
    int hdrlen = snprintf(header, sizeof(header),
                          "HTTP/1.0 200 OK\r\nContent-Length: %zu\r\n"
                          "Content-Type: %s\r\n\r\n",
                          (size_t)st.st_size, guess_mime(filepath));
    sendall(fd, header, hdrlen);

    while ((n = fread(buf,1,sizeof(buf),f)) > 0) {
        sendall(fd, buf, n);
    }
    fclose(f);
    close(fd);
    return NULL;
}

/* ------------ server loop ------------ */

int httpd_serve(const char *port_str, const char *docroot) {
    int port = atoi(port_str);
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

    fprintf(stderr,"Serving %s on port %d\n", docroot, port);
    for (;;) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int cl = accept(srv, (struct sockaddr*)&cli, &clilen);
        if (cl < 0) { if (errno==EINTR) continue; perror("accept"); break; }

        struct client_ctx *ctx = malloc(sizeof(*ctx));
        ctx->fd = cl;
        strncpy(ctx->docroot, docroot, sizeof(ctx->docroot)-1);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, ctx);
        pthread_detach(tid);
    }

    close(srv);
    return 0;
}

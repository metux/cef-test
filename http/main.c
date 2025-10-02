#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "nanohttpd.h"

/* Shared counter */
static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void write_reply_ok(nanohttpd_xfer *xfer, const char *content_type, const char *data)
{
    char buf[1024] = { 0 };
    snprintf(buf, sizeof(buf),
             "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    write(xfer->fd, buf, strlen(buf));
    write(xfer->fd, data, strlen(data));
}

static void counter_handler(nanohttpd_xfer *xfer) {
    pthread_mutex_lock(&lock);
    int n = ++counter;
    pthread_mutex_unlock(&lock);

    char resp[128];
    snprintf(resp, sizeof(resp), "Count=%d\n", n);
    write_reply_ok(xfer, "text/plain", resp);
}

int main(int argc, char **argv) {
    nanohttpd_server srv = { 0 };
    nanohttpd_register_handler(&srv, "/counter", counter_handler, NULL);
    return nanohttpd_serve(&srv, "8080");
}

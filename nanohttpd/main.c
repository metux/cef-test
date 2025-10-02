#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "nanohttpd.h"

/* Shared counter */
static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void counter_handler(nanohttpd_xfer *xfer) {
    pthread_mutex_lock(&lock);
    int n = ++counter;
    pthread_mutex_unlock(&lock);

    char resp[128];
    snprintf(resp, sizeof(resp), "Count=%d\n", n);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, resp);
    if (n==10) {
        fprintf(stderr, "counter reached limit. terminating server\n");
        xfer->server->running = false;
    }
}

int main(int argc, char **argv) {
    nanohttpd_server srv = { 0 };
    nanohttpd_register_handler(&srv, "/counter", counter_handler, NULL);
    return nanohttpd_serve(&srv, "8080");
}

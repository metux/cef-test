#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "cefhelper.h"
#include "nanohttpd.h"

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void handle_reload(nanohttpd_xfer *xfer) {
    fprintf(stderr, "handle_reload()\n");
    cefhelper_reload();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "reloaded");
}

static void seturl_handler(nanohttpd_xfer *xfer) {
    fprintf(stderr, "seturl request: %s\n", xfer->path);
    size_t prefixlen = strlen(xfer->matched_prefix);
    const char *remaining = xfer->path + prefixlen;
    while (remaining[0] == '/') remaining++;
    fprintf(stderr, "remaining query: %s\n", remaining);

    char *decoded = strdup(remaining);
    nanohttpd_urldecode(decoded);

    fprintf(stderr, "remaining decoded: %s\n", decoded);
    cefhelper_loadurl(decoded);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, decoded);
    free(decoded);
}

static void counter_handler(nanohttpd_xfer *xfer) {
    pthread_mutex_lock(&lock);
    int n = ++counter;
    pthread_mutex_unlock(&lock);

    char resp[128];
    snprintf(resp, sizeof(resp), "cefserver\ncount=%d\n", n);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, resp);

    cefhelper_loadurl("https://www.thur.de/");

    if (n==10) {
        fprintf(stderr, "counter reached limit. terminating server\n");
        exit(0);
    }
}

int main(int argc, char* argv[])
{
    /* just pass control to CEF if we're in a subprocess */
    if (check_cef_subprocess(argc, argv))
        return cefhelper_subprocess(argc, argv);

    uint32_t parent_xid = 0;
    if (argc > 1) {
        parent_xid = strtol(argv[1], NULL, 16);
        printf("parsing window id: %lX\n", parent_xid);
    }

    nanohttpd_server srv = { .port_str = "8080" };
    nanohttpd_register_handler(&srv, "/counter", counter_handler, NULL);
    nanohttpd_register_handler(&srv, "/url", seturl_handler, NULL);
    nanohttpd_register_handler(&srv, "/reload", handle_reload, NULL);
    nanohttpd_serve_thread(&srv);

    return cefhelper_run(parent_xid, 800, 600, "file:///");
}

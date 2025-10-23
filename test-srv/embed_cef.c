#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "cefhelper.h"
#include "nanohttpd.h"

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void handle_goback(nanohttpd_xfer *xfer) {
    cefhelper_goback();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "back");
}

static void handle_goforward(nanohttpd_xfer *xfer) {
    cefhelper_goforward();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "forward");
}

static void handle_reload(nanohttpd_xfer *xfer) {
    cefhelper_reload();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "reloaded");
}

static void handle_stopload(nanohttpd_xfer *xfer) {
    cefhelper_stopload();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "stopped");
}

static void handle_seturl(nanohttpd_xfer *xfer) {
    size_t prefixlen = strlen(xfer->matched_prefix);
    const char *remaining = xfer->path + prefixlen;
    while (remaining[0] == '/') remaining++;

    char *decoded = strdup(remaining);
    nanohttpd_urldecode(decoded);

    cefhelper_loadurl(decoded);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, decoded);
    free(decoded);
}

static void handle_create(nanohttpd_xfer *xfer) {

    char *endptr = NULL;
    uint32_t id = strtol(xfer->remaining, &endptr, 10);
    while (endptr[0] == '/') endptr++;
    uint32_t xid = strtol(endptr, &endptr, 16);
    while (endptr[0] == '/') endptr++;
    uint32_t width = strtol(endptr, &endptr, 10);
    while (endptr[0] == '/') endptr++;
    uint32_t height = strtol(endptr, &endptr, 10);
    while (endptr[0] == '/') endptr++;

    char *url = strdup(endptr);
    nanohttpd_urldecode(url);

    fprintf(stderr, "ID: %d\n", id);
    fprintf(stderr, "XID: %X\n", xid);
    fprintf(stderr, "URL: %s\n", url);
    fprintf(stderr, "WIDTH: %d\n", width);
    fprintf(stderr, "HEIGHT: %d\n", height);

    int ret = cefhelper_create(xid, width, height, url);
    free(url);

    // FIXME: should check for errors and send proper HTTP codes
    char reply[512];
    sprintf(reply, "%d", ret);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, reply);
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
    nanohttpd_register_handler(&srv, "/stopload", handle_stopload, NULL);
    nanohttpd_register_handler(&srv, "/url", handle_seturl, NULL);
    nanohttpd_register_handler(&srv, "/reload", handle_reload, NULL);
    nanohttpd_register_handler(&srv, "/back", handle_goback, NULL);
    nanohttpd_register_handler(&srv, "/forward", handle_goforward, NULL);
    nanohttpd_register_handler(&srv, "/create", handle_create, NULL);
    nanohttpd_serve_thread(&srv);

    return cefhelper_run(parent_xid, 800, 600, "file:///");
}

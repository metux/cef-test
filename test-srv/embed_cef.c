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
    nanohttpd_serve_thread(&srv);

    return cefhelper_run(parent_xid, 800, 600, "file:///");
}

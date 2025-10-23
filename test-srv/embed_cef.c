#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "cefhelper.h"
#include "nanohttpd.h"

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void handle_goback(nanohttpd_xfer *xfer)
{
    int idx = nanohttpd_next_elem_int_dec(xfer);
    cefhelper_goback(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "back");
}

static void handle_goforward(nanohttpd_xfer *xfer)
{
    int idx = nanohttpd_next_elem_int_dec(xfer);
    cefhelper_goforward(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "forward");
}

static void handle_reload(nanohttpd_xfer *xfer)
{
    int idx = nanohttpd_next_elem_int_dec(xfer);
    cefhelper_reload(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "reloaded");
}

static void handle_stopload(nanohttpd_xfer *xfer)
{
    int idx = nanohttpd_next_elem_int_dec(xfer);
    cefhelper_stopload(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "stopped");
}

static void handle_seturl(nanohttpd_xfer *xfer)
{
    int idx = nanohttpd_next_elem_int_dec(xfer);
    char *decoded = strdup(xfer->remaining);
    nanohttpd_urldecode(decoded);

    cefhelper_loadurl(idx, decoded);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, decoded);
    free(decoded);
}

static void handle_create(nanohttpd_xfer *xfer)
{
    uint32_t idx = nanohttpd_next_elem_int_dec(xfer);
    uint32_t xid = nanohttpd_next_elem_int_hex(xfer);
    uint32_t width = nanohttpd_next_elem_int_dec(xfer);
    uint32_t height = nanohttpd_next_elem_int_dec(xfer);

    char *url = strdup(xfer->remaining);
    nanohttpd_urldecode(url);

    if (strlen(url)==0)
        url = strdup("file:///");

    fprintf(stderr, "IDX: %d\n", idx);
    fprintf(stderr, "XID: %X\n", xid);
    fprintf(stderr, "URL: \"%s\"\n", url);
    fprintf(stderr, "WIDTH: %d\n", width);
    fprintf(stderr, "HEIGHT: %d\n", height);

    int ret = cefhelper_create(idx, xid, width, height, url);
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

    return cefhelper_run();
}

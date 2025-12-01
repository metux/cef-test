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
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_goback(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "back");
}

static void handle_goforward(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_goforward(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "forward");
}

static void handle_reload(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_reload(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "reloaded");
}

static void handle_close(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_close(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "closed");
}

static void handle_closeall(nanohttpd_xfer *xfer)
{
    cefhelper_closeall();
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "closed all");
}

static void handle_stopload(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_stopload(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "stopped");
}

static void handle_repaint(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_repaint(idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "repaint");
}

static void handle_zoom_in(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_zoom(idx, true);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "zoom");
}

static void handle_zoom_out(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    cefhelper_zoom(idx, false);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "zoom");
}

static void handle_resize(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    uint32_t width = nanohttpd_next_elem_int_dec(xfer);
    uint32_t height = nanohttpd_next_elem_int_dec(xfer);
    cefhelper_resize(idx, width, height);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "resize");
}

static void handle_seturl(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    char *decoded = strdup(xfer->remaining);
    nanohttpd_urldecode(decoded);

    const char *hdr_url = nanohttpd_find_header(xfer, "Url");
    if (hdr_url) {
        free(decoded);
        decoded = strdup(hdr_url);
    }

    if (decoded[0]) {
        cefhelper_loadurl(idx, decoded);
        nanohttpd_xfer_reply_ok_text(xfer, NULL, decoded);
    } else {
        nanohttpd_xfer_reply_ok_text(xfer, NULL, "FAIL: URL EMPTY");
    }

    free(decoded);
}

static void handle_create(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);
    uint32_t xid = nanohttpd_next_elem_int_hex(xfer);
    uint32_t width = nanohttpd_next_elem_int_dec(xfer);
    uint32_t height = nanohttpd_next_elem_int_dec(xfer);

    const char *hdr_url = nanohttpd_find_header(xfer, "Url");
    const char *hdr_webhook = nanohttpd_find_header(xfer, "Webhook");
    const char *hdr_xid = nanohttpd_find_header(xfer, "XID");

    if (!(hdr_url && (*hdr_url))) {
        hdr_url = "file:///";
    }

    if (!hdr_webhook)
        hdr_webhook = "";

    if (hdr_xid) {
        uint32_t val = strtol(hdr_xid, NULL, 16);
        if (val)
            xid = val;
    }

    char key[34] = { 0 };
    char *walk = key;
    srand(time(NULL) ^ getpid());
    for (int x=0; x<16; x++) {
        unsigned char c = (unsigned char)rand();
        sprintf(walk, "%02X", c);
        walk ++;
        walk ++;
    }

    if (strcmp(idx, "*")==0)
        idx = key;

    int ret = cefhelper_create(idx, xid, width, height, hdr_url, hdr_webhook);

    // FIXME: should check for errors and send proper HTTP codes
    char reply[512];
    sprintf(reply, "%s", idx);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, reply);
}

static void handle_shutdown(nanohttpd_xfer *xfer)
{
    cefhelper_closeall();
    nanohttpd_shutdown(xfer->server);
}

static void handle_script(nanohttpd_xfer *xfer)
{
    const char *idx = nanohttpd_next_elem_path(xfer);

    if (xfer->req_body == NULL) {
        // FIXME: should be an http error
        nanohttpd_xfer_reply_ok_text(xfer, NULL, "no content");
    }
    fprintf(stderr, "SCRIPT: \"%s\"\n", xfer->req_body);

    cefhelper_execjs(idx, xfer->req_body);
    nanohttpd_xfer_reply_ok_text(xfer, NULL, "DONE");
}

static void handle_list(nanohttpd_xfer *xfer)
{
    char buffer[8196];
    cefhelper_list(buffer, sizeof(buffer));
    nanohttpd_xfer_reply_ok_text(xfer, NULL, buffer);
}

int main(int argc, char* argv[])
{
    /* just pass control to CEF if we're in a subprocess */
    if (check_cef_subprocess(argc, argv))
        return cefhelper_subprocess(argc, argv);

    nanohttpd_server srv = { .port_str = "8080" };
    nanohttpd_register_handler(&srv, "/api/v1/browser/script", handle_script, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/stop", handle_stopload, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/url", handle_seturl, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/navigate", handle_seturl, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/reload", handle_reload, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/back", handle_goback, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/forward", handle_goforward, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/create", handle_create, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/shutdown", handle_shutdown, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/closeall", handle_closeall, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/close", handle_close, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/list", handle_list, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/repaint", handle_repaint, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/resize", handle_resize, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/zoom-in", handle_zoom_in, NULL);
    nanohttpd_register_handler(&srv, "/api/v1/browser/zoom-out", handle_zoom_out, NULL);

    nanohttpd_serve_thread(&srv);

    return cefhelper_run();
}

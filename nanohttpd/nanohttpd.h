#ifndef NANO_HTTPD_H
#define NANO_HTTPD_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NANOHTTPD_RESPONSE_OK              "200 OK"
#define NANOHTTPD_RESPONSE_BAD_REQUEST     "400 Bad Request"
#define NANOHTTPD_RESPONSE_NOT_FOUND       "404 Not Found"
#define NANOHTTPD_RESPONSE_NOT_IMPLEMENTED "501 Not Implemented"

typedef struct {
    bool initialized;
    bool running;
    pthread_mutex_t handlers_lock;
    struct handler_entry *handlers;
    const char *port;
} nanohttpd_server;

typedef struct {
    nanohttpd_server *server;
    const char *path;
    int fd;
    void *user_data;
} nanohttpd_xfer;

/* Handler function type:
 * - client_fd: socket to write HTTP response to
 * - path: request path (already URL-decoded, no query/fragment)
 * - user_data: pointer passed during handler registration
 */
typedef void (*nanohttpd_handler_fn)(nanohttpd_xfer *xfer);

/* Register a handler for a fixed path prefix.
 * Example: httpd_register_handler("/counter", counter_fn, NULL);
 */
int nanohttpd_register_handler(nanohttpd_server *server,
                               const char *prefix,
                               nanohttpd_handler_fn fn,
                               void *user_data);

/* Start serving. Blocks forever. */
int nanohttpd_serve(nanohttpd_server *server, const char *port);

int nanohttpd_serve_thread(nanohttpd_server *server, const char *port);

void nanohttpd_xfer_reply_text(nanohttpd_xfer *xfer,
                               const char *response,
                               const char *content_type,
                               const char *data);

void nanohttpd_xfer_reply_ok_text(nanohttpd_xfer *xfer,
                                  const char *content_type,
                                  const char *data);

#ifdef __cplusplus
}
#endif

#endif /* NANO_HTTPD_H */

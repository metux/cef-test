#ifndef NANO_HTTPD_H
#define NANO_HTTPD_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool initialized;
    pthread_mutex_t handlers_lock;
    struct handler_entry *handlers;
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

ssize_t nanohttpd_xfer_write_str(nanohttpd_xfer *xfer, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* NANO_HTTPD_H */

#ifndef MINIHTTP_H
#define MINIHTTP_H

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _httpd_server {
    bool initialized;
    pthread_mutex_t handlers_lock;
    struct handler_entry *handlers;
} nanohttpd_server;

/* Handler function type:
 * - client_fd: socket to write HTTP response to
 * - path: request path (already URL-decoded, no query/fragment)
 * - user_data: pointer passed during handler registration
 */
typedef void (*http_handler_fn)(nanohttpd_server *server, int client_fd, const char *path, void *user_data);

/* Register a handler for a fixed path prefix.
 * Example: httpd_register_handler("/counter", counter_fn, NULL);
 */
int httpd_register_handler(nanohttpd_server *server, const char *prefix, http_handler_fn fn, void *user_data);

/* Start serving. Blocks forever. */
int httpd_serve(nanohttpd_server *server, const char *port);

#ifdef __cplusplus
}
#endif

#endif /* MINIHTTP_H */

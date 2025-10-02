#ifndef MINIHTTP_H
#define MINIHTTP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Handler function type:
 * - client_fd: socket to write HTTP response to
 * - path: request path (already URL-decoded, no query/fragment)
 * - user_data: pointer passed during handler registration
 */
typedef void (*http_handler_fn)(int client_fd, const char *path, void *user_data);

/* Register a handler for a fixed path prefix.
 * Example: httpd_register_handler("/counter", counter_fn, NULL);
 */
int httpd_register_handler(const char *prefix, http_handler_fn fn, void *user_data);

/* Start serving. Blocks forever. */
int httpd_serve(const char *port, const char *docroot);

#ifdef __cplusplus
}
#endif

#endif /* MINIHTTP_H */

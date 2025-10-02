#include "minihttp.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Shared counter */
static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void counter_handler(http_server *server, int fd, const char *path, void *ud) {
    (void)path; (void)ud;
    pthread_mutex_lock(&lock);
    int n = ++counter;
    pthread_mutex_unlock(&lock);

    char resp[128];
    snprintf(resp, sizeof(resp),
             "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nCount=%d\n", n);
    write(fd, resp, strlen(resp));
}

int main(int argc, char **argv) {
    http_server srv = { 0 };
    httpd_register_handler(&srv, "/counter", counter_handler, NULL);
    return httpd_serve(&srv, "8080");
}

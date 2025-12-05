#ifndef __CEFHELPER_H
#define __CEFHELPER_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CEFHELPER_ZOOM_RESET = 0,
    CEFHELPER_ZOOM_SET = 1,
    CEFHELPER_ZOOM_IN = 2,
    CEFHELPER_ZOOM_OUT = 3
} cefhelper_zoom_mode_t;

int cefhelper_subprocess(int argc, char *argv[]);
int cefhelper_run(void);
bool check_cef_subprocess(int argc, char *argv[]);
void cefhelper_loadurl(const char *idx, const char *url);
void cefhelper_reload(const char *idx);
void cefhelper_stopload(const char *idx);
void cefhelper_goback(const char *idx);
void cefhelper_goforward(const char *idx);
void cefhelper_execjs(const char *idx, const char *code);
void cefhelper_print(const char *idx);
void cefhelper_close(const char *idx);
void cefhelper_repaint(const char *idx);
void cefhelper_resize(const char *idx, int w, int h);
void cefhelper_zoom(const char *idx, cefhelper_zoom_mode_t mode, double level);
void cefhelper_closeall(void);
int cefhelper_create(const char *idx, uint32_t parent_xid, int width, int height, const char *url, const char *webhook);
int cefhelper_list(char *buf, size_t bufmax);

#ifdef __cplusplus
}
#endif

#endif /* __CEFHELPER_H */

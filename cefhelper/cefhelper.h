#ifndef __CEFHELPER_H
#define __CEFHELPER_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int cefhelper_subprocess(int argc, char *argv[]);
int cefhelper_run(void);
bool check_cef_subprocess(int argc, char *argv[]);
void cefhelper_loadurl(const char *idx, const char *url);
void cefhelper_reload(const char *idx);
void cefhelper_stopload(const char *idx);
void cefhelper_goback(const char *idx);
void cefhelper_goforward(const char *idx);
void cefhelper_execjs(const char *idx, const char *code);
void cefhelper_close(const char *idx);
void cefhelper_repaint(const char *idx);
void cefhelper_closeall(void);
int cefhelper_create(const char *idx, uint32_t parent_xid, int width, int height, const char *url, const char *webhook);
int cefhelper_list(char *buf, size_t bufmax);

#ifdef __cplusplus
}
#endif

#endif /* __CEFHELPER_H */

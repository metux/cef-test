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
void cefhelper_loadurl(int idx, const char *url);
void cefhelper_reload(int idx);
void cefhelper_stopload(int idx);
void cefhelper_goback(int idx);
void cefhelper_goforward(int idx);
void cefhelper_execjs(int idx, const char *code);
void cefhelper_close(int idx);
void cefhelper_closeall(void);
int cefhelper_create(int idx, uint32_t parent_xid, int width, int height, const char *url);

#ifdef __cplusplus
}
#endif

#endif /* __CEFHELPER_H */

#ifndef __CEFHELPER_H
#define __CEFHELPER_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    int cefhelper_subprocess(int argc, char *argv[]);
    int cefhelper_run(uint32_t parent_xid, int width, int height, const char *url);
    bool check_cef_subprocess(int argc, char *argv[]);
#ifdef __cplusplus
}
#endif

#endif

#include <stdio.h>
#include <stdlib.h>

#include "cefhelper.h"

int main(int argc, char* argv[])
{
    /* just pass control to CEF if we're in a subprocess */
    if (check_cef_subprocess(argc, argv))
        return cefhelper_subprocess(argc, argv);

    uint32_t parent_xid = 0;
    if (argc > 1) {
        parent_xid = strtol(argv[1], NULL, 16);
        printf("parsing window id: %lX\n", parent_xid);
    }

    return cefhelper_run(parent_xid, 800, 600, "file:///");
}

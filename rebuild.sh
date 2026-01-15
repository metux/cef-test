#!/bin/bash

( cd ../cef-build && ./build.sh ) || exit 1

./sync-cef
make clean

if make ; then
    notify-send "CEFTEST compile finished: OKAY" -a "CEFTEST BUILD" -e || true
else
    notify-send "CEFTEST compile finished: FAILED" -a "CEFTEST BUILD" -e || true
fi

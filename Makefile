srcroot := .

# the parent window XID for test run
# you should run some program where the browser shall be embedded
# into and set the target window ID here
TEST_PARENT_WINDOW_ID ?= 0x480000a

include $(srcroot)/conf.mk

DIST_DIR := dist

.PHONY: all clean run

all:
	$(MAKE) -C cefsdk
	$(MAKE) -C cefhelper
	$(MAKE) -C nanohttpd
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C nanohttpd clean
	$(MAKE) -C cefhelper clean
	rm -Rf $(DIST_DIR)

run: $(BIN) bundle
	$(MAKE) -C src run

srcroot := .

# the parent window XID for test run
# you should run some program where the browser shall be embedded
# into and set the target window ID here
TEST_PARENT_WINDOW_ID ?= 0x480000a

include $(srcroot)/conf.mk

DIST_DIR := dist

SUBDIRS := cefsdk cefhelper nanohttpd test-simple test-srv

.PHONY: all clean run

all:
	for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

clean:
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean ; done
	rm -Rf $(DIST_DIR)

run:
	$(MAKE) -C src run

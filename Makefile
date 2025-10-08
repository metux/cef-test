srcroot := .

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
	$(MAKE) -C test-srv run

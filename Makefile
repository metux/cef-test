srcroot := .

include $(srcroot)/conf.mk

DIST_DIR := dist

SUBDIRS := cefsdk cefhelper nanohttpd browserd

export PA_DEBUG:=1
export MALLOC_PERTURB_:=153

.PHONY: all clean run

all:
	for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

clean:
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean ; done
	rm -Rf $(DIST_DIR)

run:
	$(MAKE) -C browserd run

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

gdb:
	$(MAKE) -C browserd gdb

bundle: all
	$(MAKE) -C browserd bundle

install: bundle
	mkdir -p $(DESTDIR)/$(INSTALL_PREFIX)/
	cp -R $(DIST_DIR)/* $(DESTDIR)/$(INSTALL_PREFIX)
	mkdir -p $(DESTDIR)/$(BINDIR)
	cat browserd.in | sed -e 's~__PREFIX__~$(INSTALL_PREFIX)~g;' > $(DESTDIR)/$(BINDIR)/browserd
	chmod ugo+x $(DESTDIR)/$(BINDIR)/browserd

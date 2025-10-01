srcroot := ..

include $(srcroot)/conf.mk

CEFSDK_TARBALL := download/cef.tar.bz2
CEFSDK_DIR := cef
CEFSDK_MARKER := $(CEFSDK_DIR)/.unpacked
CEFSDK_DIST_FILES := \
    $(CEFSDK_DIR)/$(CEF_BUILD_TYPE)/v8_context_snapshot.bin \
    $(CEFSDK_DIR)/$(CEF_BUILD_TYPE)/libEGL.so \
    $(CEFSDK_DIR)/$(CEF_BUILD_TYPE)/libGLESv2.so \
    $(CEFSDK_DIR)/$(CEF_BUILD_TYPE)/libcef.so \
    $(CEFSDK_DIR)/$(CEF_BUILD_TYPE)/libvk_swiftshader.so \
    $(CEFSDK_DIR)/Resources/locales \
    $(CEFSDK_DIR)/Resources/icudtl.dat \
    $(CEFSDK_DIR)/Resources/chrome_100_percent.pak \
    $(CEFSDK_DIR)/Resources/chrome_200_percent.pak \
    $(CEFSDK_DIR)/Resources/resources.pak

all: $(CEFSDK_MARKER)

$(CEFSDK_TARBALL):
	@echo ">>> Downloading CEF $(CEF_VERSION)"
	@mkdir `dirname "$@"`
	@curl -L -o $@ $(CEF_URL)

$(CEFSDK_MARKER): $(CEFSDK_TARBALL)
	@echo ">>> Unpacking CEF"
	@rm -Rf $(CEFSDK_DIR)
	@mkdir -p $(CEFSDK_DIR)
	@tar xjf $(CEFSDK_TARBALL) --strip-components=1 -C $(CEFSDK_DIR)
	@touch $@

clean:
	@rm -Rf $(CEFSDK_DIR)

install-dist:
	@[ "$(DIST_DIR)" ] || (echo "missing DIST_DIR variable" ; exit 1)
	mkdir -p $(DIST_DIR)
	for fn in $(CEFSDK_DIST_FILES) ; do cp -R $$fn $(DIST_DIR) ; done

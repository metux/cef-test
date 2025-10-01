srcroot := ..

include $(srcroot)/conf.mk

CEFSDK_TARBALL := download/cef.tar.bz2
CEFSDK_DIR := cef
CEFSDK_MARKER := $(CEFSDK_DIR)/.unpacked

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

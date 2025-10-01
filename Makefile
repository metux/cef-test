# CEF version to fetch (change if you need another version)
CEF_VERSION := cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185
#CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_$(CEF_VERSION)_linux64.tar.bz2
CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185_linux64.tar.bz2
CEF_TARBALL := cef.tar.bz2
CEF_DIR := third_party/cef
CEF_BUILD_TYPE := Release

PKG_CONFIG ?= pkgconf

X11_LIBS := $(shell $(PKG_CONFIG) --libs x11 xcursor xext)
X11_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 xcursor xext)

CEF_LIBS := -L$(CEF_DIR)/$(CEF_BUILD_TYPE) -lcef
CEF_CFLAGS := -I$(CEF_DIR)

# Compiler / flags
CXX := g++
CXXFLAGS := -g -std=c++17 -O2 -Wno-unused
LDFLAGS := -Wl,-rpath,. -pthread

# Sources
SRCS := src/embed_cef.c src/cefhelper.cpp
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
BIN := embed_cef

# FIXME: this wrapper library could become a separate shared library
WRAPPER_SRCS := $(wildcard $(CEF_DIR)/libcef_dll/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/base/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/wrapper/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/ctocpp/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/cpptoc/*.cc) \

WRAPPER_OBJS := $(WRAPPER_SRCS:.cc=.o)
WRAPPER_LIB := libcef_lib.a
WRAPPER_LIBS := $(WRAPPER_LIB)

PROG_LIBS := $(CEF_LIBS) $(X11_LIBS) $(WRAPPER_LIBS)
PROG_CFLAGS := $(CEF_CFLAGS) $(X11_CFLAGS) $(WRAPPER_CFLAGS)

CFLAGS += $(PROG_CFLAGS)
CXXFLAGS += $(PROG_CFLAGS)

.PHONY: all clean fetch_cef bundle run

all: bundle

# Rule to build the binary
$(BIN): fetch_cef $(OBJS) $(WRAPPER_LIB)
	$(CXX) -o $@ $(OBJS) $(PROG_CFLAGS) $(PROG_LIBS) $(LDFLAGS)

$(CEF_DIR)/libcef_dll/%.o: $(CEF_DIR)/libcef_dll/%.cc
	$(CXX) $(CXXFLAGS) -I$(CEF_DIR) -c $< -o $@ -DWRAPPING_CEF_SHARED

# Build the wrapper static library
$(WRAPPER_LIB): $(WRAPPER_OBJS)
	$(AR) rcs $@ $(WRAPPER_OBJS)

# Fetch and extract CEF if not present
fetch_cef:
	@if [ ! -d "$(CEF_DIR)" ]; then \
            echo ">>> Downloading CEF $(CEF_VERSION)" && \
            mkdir -p third_party && \
            curl -L -o $(CEF_TARBALL) $(CEF_URL) && \
            tar -xjf $(CEF_TARBALL) -C third_party && \
            rm -f $(CEF_TARBALL) && \
            mv third_party/cef_binary_* $(CEF_DIR) && \
            echo ">>> CEF unpacked to $(CEF_DIR)" ; \
        fi

clean:
	rm -Rf src/*.o *.o $(BIN) *.a *.so $(DIST_DIR)

DIST_DIR := dist
DIST_CEF_FILES := \
    $(CEF_BUILD_TYPE)/v8_context_snapshot.bin \
    $(CEF_BUILD_TYPE)/libEGL.so \
    $(CEF_BUILD_TYPE)/libGLESv2.so \
    $(CEF_BUILD_TYPE)/libcef.so \
    $(CEF_BUILD_TYPE)/libvk_swiftshader.so \
    Resources/locales \
    Resources/icudtl.dat \
    Resources/chrome_100_percent.pak \
    Resources/chrome_200_percent.pak \
    Resources/resources.pak

DIST_FILES := $(addprefix $(CEF_DIR)/, $(DIST_CEF_FILES))

bundle: $(BIN)
	rm -Rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	for fn in $(DIST_FILES) ; do cp -R $$fn $(DIST_DIR) ; done
	cp embed_cef $(DIST_DIR)

run: $(BIN) bundle
	strip --strip-unneeded $(BIN)
	cd $(DIST_DIR) && ./$(BIN) 0x480000a

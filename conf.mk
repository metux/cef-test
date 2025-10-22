# CEF version to fetch (change if you need another version)
CEF_VERSION := cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185
#CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_$(CEF_VERSION)_linux64.tar.bz2
CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185_linux64.tar.bz2
CEF_TARBALL := cef.tar.bz2
CEF_DIR := $(srcroot)/cefsdk/cef
CEF_BUILD_TYPE := Release

PKG_CONFIG ?= pkgconf

X11_LIBS := $(shell $(PKG_CONFIG) --libs x11 xcursor xext)
X11_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 xcursor xext)

CEF_LIBS := -L$(CEF_DIR)/$(CEF_BUILD_TYPE) -lcef $(srcroot)/cefsdk/libcefwrapper.a
CEF_CFLAGS := -I$(CEF_DIR)

CEFHELPER_LIBS := $(srcroot)/cefhelper/libcefhelper.a
CEFHELPER_CFLAGS := -I$(srcroot)/cefhelper

NANOHTTPD_LIBS := $(srcroot)/nanohttpd/libnanohttpd.a
NANOHTTPD_CFLAGS := -I$(srcroot)/nanohttpd

# Compiler / flags
CXX ?= g++
CC ?= gcc
CFLAGS += -ffunction-sections -fdata-sections -O2 -Wno-unused
CXXFLAGS += $(CFLAGS) -g -std=c++17
LDFLAGS += -Wl,-rpath,. -pthread -Wl,--gc-sections

TEST_PARENT_WINDOW_ID ?= 0x580000a

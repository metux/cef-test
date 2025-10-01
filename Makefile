srcroot := .

include $(srcroot)/conf.mk

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

.PHONY: prepare_deps all clean bundle run

all: bundle

prepare_deps:
	$(MAKE) -C cefsdk

# Rule to build the binary
$(BIN): prepare_deps $(OBJS) $(WRAPPER_LIB)
	$(CXX) -o $@ $(OBJS) $(PROG_CFLAGS) $(PROG_LIBS) $(LDFLAGS)

$(CEF_DIR)/libcef_dll/%.o: $(CEF_DIR)/libcef_dll/%.cc
	$(CXX) $(CXXFLAGS) -I$(CEF_DIR) -c $< -o $@ -DWRAPPING_CEF_SHARED

# Build the wrapper static library
$(WRAPPER_LIB): $(WRAPPER_OBJS)
	$(AR) rcs $@ $(WRAPPER_OBJS)

clean:
	rm -Rf src/*.o *.o $(BIN) *.a *.so $(DIST_DIR)

DIST_DIR := dist

bundle: $(BIN)
	rm -Rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	$(MAKE) -C cefsdk install-dist DIST_DIR=$(abspath $(DIST_DIR))
	cp embed_cef $(DIST_DIR)

run: $(BIN) bundle
	strip --strip-unneeded $(BIN)
	cd $(DIST_DIR) && ./$(BIN) 0x480000a

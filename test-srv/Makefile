srcroot := ..

# the parent window XID for test run
# you should run some program where the browser shall be embedded
# into and set the target window ID here
TEST_PARENT_WINDOW_ID ?= 0x5800004

include $(srcroot)/conf.mk

DIST_DIR := $(srcroot)/dist

SRCS := embed_cef.c
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
BIN := embed_cef

PROG_LIBS := $(NANOHTTPD_LIBS) $(CEFHELPER_LIBS) $(CEF_LIBS) $(X11_LIBS)
PROG_CFLAGS := $(NANOHTTPD_CFLAGS) $(CEFHELPER_CFLAGS) $(CEF_CFLAGS) $(X11_CFLAGS)

CFLAGS += $(PROG_CFLAGS)
CXXFLAGS += $(PROG_CFLAGS)

.PHONY: prepare_deps all clean bundle run

all: bundle

prepare_deps:
	$(MAKE) -C $(srcroot)/cefsdk
	$(MAKE) -C $(srcroot)/cefhelper
	$(MAKE) -C $(srcroot)/nanohttpd

# Rule to build the binary
$(BIN): prepare_deps $(OBJS)
	$(CXX) -o $@ $(OBJS) $(PROG_CFLAGS) $(PROG_LIBS) $(LDFLAGS) -Wl,--gc-sections 

clean:
	rm -Rf src/*.o *.o $(BIN) *.a *.so $(DIST_DIR)

bundle: $(BIN)
	rm -Rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	$(MAKE) -C $(srcroot)/cefsdk install-dist DIST_DIR=$(abspath $(DIST_DIR))
	cp embed_cef $(DIST_DIR)

run: $(BIN) bundle
	cd $(DIST_DIR) && ./$(BIN) $(TEST_PARENT_WINDOW_ID)

srcroot := ..

include $(srcroot)/conf.mk

DIST_DIR := $(srcroot)/dist

SRCS := browserd.c
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
BIN := browserd

PROG_LIBS := $(NANOHTTPD_LIBS) $(CEFHELPER_LIBS) $(CEF_LIBS) $(X11_LIBS) $(CURL_LIBS)
PROG_CFLAGS := $(NANOHTTPD_CFLAGS) $(CEFHELPER_CFLAGS) $(CEF_CFLAGS) $(X11_CFLAGS) $(CURL_CFLAGS)

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
	cp $(BIN) $(DIST_DIR)

run: $(BIN) bundle
	cd $(DIST_DIR) && ./$(BIN)

gdb: $(BIN) bundle
	cd $(DIST_DIR) && gdb ./$(BIN)

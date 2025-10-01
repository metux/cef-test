srcroot := .

include $(srcroot)/conf.mk

# Sources
SRCS := src/embed_cef.c src/cefhelper.cpp
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
BIN := embed_cef

PROG_LIBS := $(CEF_LIBS) $(X11_LIBS)
PROG_CFLAGS := $(CEF_CFLAGS) $(X11_CFLAGS)

CFLAGS += $(PROG_CFLAGS)
CXXFLAGS += $(PROG_CFLAGS)

.PHONY: prepare_deps all clean bundle run

all: bundle

prepare_deps:
	$(MAKE) -C cefsdk

# Rule to build the binary
$(BIN): prepare_deps $(OBJS)
	$(CXX) -o $@ $(OBJS) $(PROG_CFLAGS) $(PROG_LIBS) $(LDFLAGS)

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

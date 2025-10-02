

srcroot := ..

include $(srcroot)/conf.mk

NANOHTTPD_SRCS := nanohttpd.c
NANOHTTPD_OBJS := $(NANOHTTPD_SRCS:.c=.o)
NANOHTTPD_LIB  := libnanohttpd.a

CFLAGS += $(CEF_CFLAGS)
CXXFLAGS += $(CEF_CFLAGS)
LDFLAGS += $(CEF_LIBS)

all: $(NANOHTTPD_LIB) server

clean:
	@rm -Rf $(CEFSDK_DIR) *.a *.so *.o server

$(NANOHTTPD_LIB): $(NANOHTTPD_OBJS)
	$(AR) rcs $@ $(NANOHTTPD_OBJS)

server: main.c $(NANOHTTPD_LIB)
	$(CC) -std=c11 -O2 -Wall -pthread $^ -o $@

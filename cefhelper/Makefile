srcroot := ..

include $(srcroot)/conf.mk

CEFSDK_DIR := cef

CEFHELPER_SRCS := cefhelper.cpp
CEFHELPER_OBJS := $(CEFHELPER_SRCS:.cpp=.o)
CEFHELPER_LIB  := libcefhelper.a

CFLAGS += $(CEF_CFLAGS) $(CURL_CFLAGS)
CXXFLAGS += $(CEF_CFLAGS) $(CURL_CFLAGS)
LDFLAGS += $(CEF_LIBS) $(CURL_LIBS)

all: $(CEFHELPER_LIB)

clean:
	@rm -Rf $(CEFSDK_DIR) *.a *.so *.o

$(CEFHELPER_LIB): $(CEFHELPER_OBJS)
	$(AR) rcs $@ $(CEFHELPER_OBJS)

cefhelper.o:: task/*.h cefhelper.h cefhelper_priv.h

cefhelper.o:: CefHelperApp.h

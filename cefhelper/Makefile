srcroot := ..

include $(srcroot)/conf.mk

CEFSDK_DIR := cef

CEFHELPER_SRCS := cefhelper.cpp
CEFHELPER_OBJS := $(CEFHELPER_SRCS:.cpp=.o)
CEFHELPER_LIB  := libcefhelper.a

CFLAGS += $(CEF_CFLAGS)
CXXFLAGS += $(CEF_CFLAGS)
LDFLAGS += $(CEF_LIBS)

all: $(CEFHELPER_LIB)

clean:
	@rm -Rf $(CEFSDK_DIR) *.a *.so *.o

$(CEFHELPER_LIB): $(CEFHELPER_OBJS)
	$(AR) rcs $@ $(CEFHELPER_OBJS)

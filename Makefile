# Compiler / flags
CXX := g++
#CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/cef -Wno-unused
CXXFLAGS := -std=c++17 -O2 -Ithird_party/cef -Wno-unused
LDFLAGS := -Wl,-rpath,. -pthread -ldl -lX11 -lXext -lXcursor

# CEF version to fetch (change if you need another version)
CEF_VERSION := cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185
CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_$(CEF_VERSION)_linux64.tar.bz2
CEF_TARBALL := cef.tar.bz2
CEF_DIR := third_party/cef

# Sources
SRCS := embed_cef.cpp
OBJS := $(SRCS:.cpp=.o)
BIN := embed_cef

LIBCEF_DIR := $(CEF_DIR)/libcef_dll

WRAPPER_SRCS := $(wildcard $(CEF_DIR)/libcef_dll/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/base/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/wrapper/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/ctocpp/*.cc) \
                $(wildcard $(CEF_DIR)/libcef_dll/cpptoc/*.cc) \

WRAPPER_OBJS := $(WRAPPER_SRCS:.cc=.o)
WRAPPER_LIB := libcef_lib.a

.PHONY: all clean fetch_cef

all: $(BIN)

dump:
	echo "$(WRAPPER_SRCS)"

#run_resources: $(BIN)
#	@echo "Copying CEF resources..."
#	mkdir -p ./locales
#	cp -r $(CEF_DIR)/Release/$(CEF_RESOURCES) ./

# Rule to build the binary
$(BIN): fetch_cef $(OBJS) $(WRAPPER_LIB)
	$(CXX) -o $@ $(OBJS) $(WRAPPER_LIB) -L$(CEF_DIR)/Release -lcef $(LDFLAGS)

wr:	$(WRAPPER_OBJS)
	echo WR $(WRAPPER_SRCS)

$(CEF_DIR)/libcef_dll/%.o: $(CEF_DIR)/libcef_dll/%.cc
	$(CXX) $(CXXFLAGS) -I$(CEF_DIR) -c $< -o $@ -DWRAPPING_CEF_SHARED

# Build the wrapper static library
$(WRAPPER_LIB): $(WRAPPER_OBJS)
	$(AR) rcs $@ $(WRAPPER_OBJS)

# Fetch and extract CEF if not present
fetch_cef:
	echo "CEF_URL=$(CEF_URL)"
	@if [ ! -d "$(CEF_DIR)" ]; then \
            echo ">>> Downloading CEF $(CEF_VERSION)" && \
            mkdir -p third_party && \
            curl -L -o $(CEF_TARBALL) $(CEF_URL) && \
            tar -xjf $(CEF_TARBALL) -C third_party && \
            rm -f $(CEF_TARBALL) && \
            mv third_party/cef_binary_* $(CEF_DIR) && \
            echo ">>> CEF unpacked to $(CEF_DIR)" ; \
        fi

# Compile .cpp → .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN) *.a

run: embed_cef
#run: run_resources
	ln -sf ./third_party/cef/Debug/libcef.so libcef.so
	ln -sf ./third_party/cef/Resources/locales locales
	ln -sf ./third_party/cef/Resources/icudtl.dat icudtl.dat
	ln -sf ./third_party/cef/Resources/chrome_100_percent.pak chrome_100_percent.pak
	ln -sf ./third_party/cef/Resources/resources.pak resources.pak
	./$(BIN)
#	strace -f ./$(BIN)

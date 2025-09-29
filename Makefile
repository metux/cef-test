# Compiler / flags
CXX := g++
#CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Ithird_party/cef -Wno-unused
CXXFLAGS := -std=c++17 -O2 -Ithird_party/cef -Wno-unused
LDFLAGS := -Wl,-rpath,. -pthread -ldl -lX11 -lXext -lXcursor

# CEF version to fetch (change if you need another version)
CEF_VERSION := cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185
CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_$(CEF_VERSION)_linux64.tar.bz2
#CEF_URL := https://cef-builds.spotifycdn.com/cef_binary_140.1.14%2Bgeb1c06e%2Bchromium-140.0.7339.185_linux64.tar.bz2
CEF_TARBALL := cef.tar.bz2
CEF_DIR := third_party/cef

# Sources
SRCS := embed_cef.cpp
OBJS := $(SRCS:.cpp=.o)
BIN := embed_cef

.PHONY: all clean fetch_cef

all: $(BIN)

# Rule to build the binary
$(BIN): fetch_cef $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CEF_DIR)/Release/libcef_lib.a -L$(CEF_DIR)/Release -lcef $(LDFLAGS)

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

WRAPPER_SRCS := $(wildcard $(CEF_DIR)/libcef_dll/*.cc)
WRAPPER_OBJS := $(WRAPPER_SRCS:.cc=.o)

$(CEF_DIR)/libcef_dll/%.o: $(CEF_DIR)/libcef_dll/%.cc
	$(CXX) $(CXXFLAGS) -I$(CEF_DIR) -c $< -o $@

# Compile .cpp → .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)

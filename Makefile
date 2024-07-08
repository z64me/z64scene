CC=gcc
CXX=g++
MINGW=i686-w64-mingw32.static-
WREN_INCLUDES=-Iwren/src/vm -Iwren/src/include -Iwren/src/optional
Z64CONVERT_CFLAGS=-DGBI_PREFIX=F3DEX2 -DVFILE_VISIBILITY=static -DWOW_OVERLOAD_FILE -Iz64convert/gfxasm/src -Iz64convert/wowlib -Iz64convert/src
Z64CONVERT_SRC=$(wildcard z64convert/src/*.c) $(wildcard z64convert/gfxasm/src/*.c)
Z64CONVERT_SRC:=$(filter-out z64convert/src/stb_image.c z64convert/src/cli.c z64convert/src/n64texconv.c, $(Z64CONVERT_SRC))
CFLAGS_COMMON=-I z64viewer/include -I z64viewer/src -Istb $(WREN_INCLUDES) $(Z64CONVERT_CFLAGS) -Wall -Wno-unused-function -Wno-scalar-storage-order
CXXFLAGS_COMMON=-Iimgui -Iimgui/backends -Iz64viewer/include -Ijson/include -Itoml11 -Istb $(WREN_INCLUDES)
LDFLAGS_COMMON=`$(MINGW)pkg-config --libs glfw3` -Wall -Wno-unused-function -Wno-scalar-storage-order
SRC_C=$(wildcard src/*.c) $(wildcard z64viewer/src/*.c) $(wildcard wren/src/vm/*.c) $(wildcard wren/src/optional/*.c) $(Z64CONVERT_SRC)
SRC_CXX=$(wildcard src/*.cpp)
OBJ_C=$(patsubst %.c,bin/o/$(FOLDER)/%.o,$(SRC_C))
OBJ_CXX=$(patsubst %.cpp,bin/o/$(FOLDER)/%.o,$(SRC_CXX))
IMGUI_SRC=imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp $(wildcard imgui/imgui*.cpp)
IMGUI_OBJ=$(patsubst %.cpp,bin/o/$(FOLDER)/imgui/%.o,$(IMGUI_SRC))
TARGET=bin/z64scene

# defaults
PLATFORM ?= linux
BUILD_TYPE ?= debug
FOLDER=$(PLATFORM)/$(BUILD_TYPE)

ifeq ($(PLATFORM),win32)
	CFLAGS=$(CFLAGS_COMMON) -DNOC_FILE_DIALOG_WIN32
	CXXFLAGS=$(CXXFLAGS_COMMON)
	LDFLAGS=$(LDFLAGS_COMMON) -mconsole -mwindows
	TARGET|=.exe
else ifeq ($(PLATFORM),linux)
	MINGW=
	CFLAGS=$(CFLAGS_COMMON) -DNOC_FILE_DIALOG_GTK `pkg-config --cflags gtk+-3.0`
	CXXFLAGS=$(CXXFLAGS_COMMON)
	LDFLAGS=$(LDFLAGS_COMMON) `pkg-config --libs gtk+-3.0`
endif

ifeq ($(BUILD_TYPE),debug)
	CFLAGS += -Og -g
	CXXFLAGS_COMMON += -Og -g
else ifeq ($(BUILD_TYPE),release)
	CFLAGS += -DNDEBUG -Os -Wno-unused-variable -Wno-strict-aliasing
	LDFLAGS += -s
endif

.PHONY: all clean

all: $(TARGET) copy_toml

# TODO make this less bad
win32:
	$(MAKE) PLATFORM=win32

linux:
	$(MAKE) PLATFORM=linux

win32-release:
	$(MAKE) PLATFORM=win32 BUILD_TYPE=release

linux-release:
	$(MAKE) PLATFORM=linux BUILD_TYPE=release

bin:
	mkdir -p bin

bin/o/$(FOLDER):
	mkdir -p bin/o/$(FOLDER)

bin/o/$(FOLDER)/imgui:
	mkdir -p bin/o/$(FOLDER)/imgui

# copy toml's from toml/ to bin/toml/
# Find all .toml files recursively in toml/ directory
TOML_FILES := $(shell find toml/ -type f -name "*.toml")
# Generate corresponding targets in bin/toml/ while maintaining directory structure
BIN_TOML_FILES := $(patsubst toml/%,bin/toml/%,$(TOML_FILES))
# Copy .toml files to bin/toml/ if they don't already exist
$(BIN_TOML_FILES): bin/toml/% : toml/%
	mkdir -p $(dir $@)
	cp $< $@
# Create bin/toml/ directory if it doesn't exist
bin/toml:
	mkdir -p bin/toml
.PHONY: copy_toml
copy_toml: $(BIN_TOML_FILES)
# end toml copy

# Create necessary subdirectories for object files
$(OBJ_C): | bin/o/$(FOLDER)
$(OBJ_CXX): | bin/o/$(FOLDER)
$(IMGUI_OBJ): | bin/o/$(FOLDER)/imgui

bin/o/$(FOLDER)/%.o: %.c
	@mkdir -p $(dir $@)
	$(MINGW)$(CC) $(CFLAGS) -c $< -o $@

bin/o/$(FOLDER)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(MINGW)$(CXX) $(CXXFLAGS) -c $< -o $@

bin/o/$(FOLDER)/imgui/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(MINGW)$(CXX) -c -DNDEBUG -Os -Iimgui -Iimgui/backends -Iimgui/examples/libs/glfw/include $< -o $@

$(TARGET): bin bin/o/$(FOLDER) bin/o/$(FOLDER)/imgui $(OBJ_C) $(OBJ_CXX) $(IMGUI_OBJ)
	$(MINGW)$(CXX) -o $@ $(OBJ_C) $(OBJ_CXX) $(IMGUI_OBJ) $(LDFLAGS)

clean:
	rm -rf bin/o

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
INCLUDES = -Iinclude -I../backend/include
SRC_DIR = src
BIN = viewer
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/assets.cpp $(SRC_DIR)/render.cpp $(SRC_DIR)/ipc_attach.cpp $(SRC_DIR)/anim.cpp $(SRC_DIR)/layout_config.cpp

EDITOR_BIN = level_editor
EDITOR_SRC = $(SRC_DIR)/level_editor.cpp $(SRC_DIR)/assets.cpp $(SRC_DIR)/layout_config.cpp

RAYLIB_FLAGS := $(shell pkg-config --cflags --libs raylib 2>/dev/null)
ifeq ($(strip $(RAYLIB_FLAGS)),)
RAYLIB_FLAGS = -I/usr/local/include -L/usr/local/lib -lraylib -lm -lpthread -ldl -lrt -lX11
endif
RAYLIB_FLAGS += -Wl,-rpath,/usr/local/lib

all: $(BIN) $(EDITOR_BIN)

$(BIN): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN) $(SOURCES) $(RAYLIB_FLAGS)

$(EDITOR_BIN): $(EDITOR_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(EDITOR_BIN) $(EDITOR_SRC) $(RAYLIB_FLAGS)

clean:
	rm -f $(BIN) $(EDITOR_BIN)

.PHONY: all clean

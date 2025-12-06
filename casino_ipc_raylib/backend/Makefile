CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
LDFLAGS ?= -pthread -lrt
INCLUDES = -Iinclude
SRC_DIR = src
BIN_DIR = .

SRCS_COMMON = $(SRC_DIR)/ipc_shared.cpp

all: casino_server player

casino_server: $(SRC_DIR)/casino_server.cpp $(SRCS_COMMON)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/casino_server $(SRC_DIR)/casino_server.cpp $(SRCS_COMMON) $(LDFLAGS)

player: $(SRC_DIR)/player.cpp $(SRCS_COMMON)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/player $(SRC_DIR)/player.cpp $(SRCS_COMMON) $(LDFLAGS)

clean:
	rm -f $(BIN_DIR)/casino_server $(BIN_DIR)/player

.PHONY: all clean

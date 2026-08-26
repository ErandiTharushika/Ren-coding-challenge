CXX      := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra
SRC      := src/main.cpp
BIN_DIR  := bin
BIN      := $(BIN_DIR)/graph_solution_bin

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

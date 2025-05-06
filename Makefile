# Author: Serhij Čepil (sipxi)
# FIT VUT Student
# https://github.com/sipxi
# Date: 18/05/2025

# Compiler and flags
CC          := gcc
CFLAGS      := -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS     := -lc

# Directories
SRC_DIR     := src
INC_DIR     := includes
BUILD_DIR   := build
TEST_DIR    := tests

# Binaries
BIN         := $(BUILD_DIR)/main

# Source and object files
SRC         := $(wildcard $(SRC_DIR)/*.c)
TEST_SRC    := $(wildcard $(TEST_DIR)/*.c)

OBJ         := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
TEST_OBJ    := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRC))

# Targets
.PHONY: all clean run

# Default build target
all: clean $(BIN)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Link objects
$(BIN): $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Run the main program
run: clean $(BIN)
	./$(BIN) 10000 10000 10 10 10

# Clean build directory
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)

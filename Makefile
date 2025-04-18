# Author: Serhij Čepil (sipxi)
# FIT VUT Student
# https://github.com/sipxi
# Date: 18/05/2025

# Compiler and flags
CC          := gcc
CFLAGS      := -std=gnu99 -pedantic

# Directories
SRC_DIR     := src
INC_DIR     := includes
BUILD_DIR   := build
TEST_DIR    := tests

# File extensions
EXE_SUFFIX  :=
MKDIR       := mkdir -p $(BUILD_DIR)
RM          := rm -rf $(BUILD_DIR)

# Binaries
BIN         := $(BUILD_DIR)/main$(EXE_SUFFIX)

# Source and object files
SRC         := $(wildcard $(SRC_DIR)/*.c)
TEST_SRC    := $(wildcard $(TEST_DIR)/*.c)

OBJ         := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
TEST_OBJ    := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRC))

# Targets
.PHONY: all clean run test

# Default target
all: $(BIN)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Link objects
$(BIN): $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Run the program
run: $(BIN)
	./$(BIN)

# Clean build directory
clean:
	@echo "Cleaning up..."
	$(RM)

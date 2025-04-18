# Author: Serhij Čepil (sipxi)
# FIT VUT Student
# https://github.com/sipxi
# Date: 18/05/2025

# Compiler and flags
CC          := gcc #! Change based on your compiler, mine is GCC
CFLAGS      := -std=gnu99 -pedantic

# Directories
SRC_DIR     := src
INC_DIR     := includes
BUILD_DIR   := build
TEST_DIR    := tests

# OS Detection
ifeq ($(OS),Windows_NT)
    EXE_SUFFIX := .exe
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
    EXE_SUFFIX :=
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
endif

# Binaries
BIN         := $(BUILD_DIR)/main$(EXE_SUFFIX)

# Source and object files (use forward slashes here!)
SRC         := $(wildcard $(SRC_DIR)/*.c)
TEST_SRC    := $(wildcard $(TEST_DIR)/*.c)

OBJ         := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
TEST_OBJ    := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRC))
# Targets
.PHONY: all clean run test

# Default build target
all: $(BIN)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile test object files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Link object files into main binary
$(BIN): $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Run the main program
run: $(BIN)
	$(BIN)

# Clean build directory
clean:
	@echo "Cleaning up..."
	$(RM)

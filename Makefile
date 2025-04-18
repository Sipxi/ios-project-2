# Author: Serhij Čepil (sipxi)
# FIT VUT Student
# https://github.com/sipxi
# Date: 18/05/2025

# Compiler and flags
CC          := gcc #! Change based on your complier, my is GCC
#!CFLAGS      := -std=gnu99 -Wall -Wextra -Werror -pedantic
CFLAGS      := -std=gnu99 -pedantic

# Directories
SRC_DIR     := src
INC_DIR     := includes
BUILD_DIR   := build

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

BIN         := $(BUILD_DIR)/main$(EXE_SUFFIX)

# Source and object files (use forward slashes here!)
SRC         := $(wildcard $(SRC_DIR)/*.c)
OBJ         := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

# Targets
.PHONY: all clean run test

# Default build target
all: $(BIN)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Link object files into binary
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Run the main program
run: $(BIN)
	$(BIN)

# Run with test input
test: $(BIN)
	$(BIN) hline tests/obrazek.txt

# Clean build directory
clean:
	@echo "Cleaning up..."
	$(RM)

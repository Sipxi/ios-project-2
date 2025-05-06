# Author: Serhij Čepil (sipxi)
# FIT VUT Student
# https://github.com/sipxi
# Date: 18/05/2025

# Compiler and flags
CC      := gcc
FLAGS  := -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt

# Files
SRC     := proj2.c
BIN     := proj2

.PHONY: all clean run test

all: $(BIN)

# Compile proj2.c
$(BIN): $(SRC)
	$(CC) $(FLAGS) $(SRC) -o $(BIN)

# Run with args
run: $(BIN)
	./$(BIN) 4 4 10 10 10

# Clean
clean:
	@echo "Cleaning up..."
	rm -f $(BIN)
	rm -f *.out

test:
	@echo "Running tests..."
	python3 ./ios_proj2_test.py run ./$(BIN) 4 4 10 10 10
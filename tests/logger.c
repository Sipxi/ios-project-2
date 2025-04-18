#include <stdio.h>
#include <stdlib.h>

// Log file pointer
static FILE *log_file = NULL;

// Initialize the log file
void init_log(const char *filename) {
    log_file = fopen(filename, "w");
    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "Test Results Log\n");
    fprintf(log_file, "================\n\n");
}

// Write a log entry
void log_test_result(const char *func_name, const char *assertion, int expected, int actual, int passed) {
    if (!log_file) {
        fprintf(stderr, "Log file not initialized.\n");
        return;
    }
    fprintf(log_file, "Test: %s\n", func_name);
    fprintf(log_file, "  Assertion: %s\n", assertion);
    fprintf(log_file, "  Expected:  %d\n", expected);
    fprintf(log_file, "  Actual:    %d\n", actual);
    fprintf(log_file, "  Result:    %s\n", passed ? "✓ PASSED" : "✗ FAILED");
    fprintf(log_file, "\n");
}

// Close the log file
void close_log() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
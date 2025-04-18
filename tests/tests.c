// tests.c
#include <assert.h>
#include <stdio.h> // For printf
#include <stdlib.h>
#include <stdbool.h>
#include "main.h"
#include "logger.h"



// Global flag to track test failures
static bool test_failure = false;
static int tests_passed = 0;
static int tests_failed = 0;

// Custom assertion macro with red-colored output (without file name)
#define ASSERT(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "\033[31mAssertion failed: %s\n" \
                            "  Expected: %d\n" \
                            "  Actual:   %d\n" \
                            "  In function '%s', line %d\n\033[0m", \
                    message, (expected), (actual), __func__, __LINE__); \
            log_test_result(__func__, message, (expected), (actual), 0); \
            tests_failed++; /* Increment failed test counter */ \
            return; /* Exit the current test function */ \
        } else { \
            log_test_result(__func__, message, (expected), (actual), 1); \
            tests_passed++; /* Increment passed test counter */ \
        } \
    } while (0)


void reset_test_counters() {
    test_failure = false;
    tests_passed = 0;
    tests_failed = 0;
}


void test_invalid_num_args() {
    const char *argv[] = {"program", "10", "20", "50", "500"};
    Config cfg;
    int result = parse_args(5, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_arg_values() {
    const char *argv[] = {"program", "abc", "20", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
}

void test_missing_args() {
    const char *argv[] = {"program", "10", "20", "50", "500"};
    Config cfg;
    int result = parse_args(5, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_extra_args() {
    const char *argv[] = {"program", "10", "20", "50", "500", "1000", "extra"};
    Config cfg;
    int result = parse_args(7, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_valid_args() {
    const char *argv[] = {"program", "10", "20", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result, EX_SUCCESS, "result == EX_SUCCESS");
    assert(cfg.num_trucks == 10);
    assert(cfg.num_cars == 20);
    assert(cfg.capacity == 50);
    assert(cfg.max_car_arrival_us == 500);
    assert(cfg.max_truck_arrival_us == 1000);
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_arg_count_too_few() {
    const char *argv[] = {"program", "10", "20", "50", "500"};
    Config cfg;
    int result = parse_args(5, argv, &cfg); // Too few arguments
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_arg_count_too_many() {
    const char *argv[] = {"program", "10", "20", "50", "500", "1000", "extra"};
    Config cfg;
    int result = parse_args(7, argv, &cfg); // Too many arguments
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_num_trucks_negative() {
    const char *argv[] = {"program", "-10", "20", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_num_trucks_exceeds_max() {
    const char *argv[] = {"program", "10001", "20", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_num_cars_negative() {
    const char *argv[] = {"program", "10", "-20", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_num_cars_exceeds_max() {
    const char *argv[] = {"program", "10", "10001", "50", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_capacity_below_min() {
    const char *argv[] = {"program", "10", "20", "2", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_capacity_above_max() {
    const char *argv[] = {"program", "10", "20", "101", "500", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_max_car_arrival_us_below_min() {
    const char *argv[] = {"program", "10", "20", "50", "-1", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result, EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_max_car_arrival_us_above_max() {
    const char *argv[] = {"program", "10", "20", "50", "10001", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_max_truck_arrival_us_below_min() {
    const char *argv[] = {"program", "10", "20", "50", "500", "-1"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_invalid_max_truck_arrival_us_above_max() {
    const char *argv[] = {"program", "10", "20", "50", "500", "1001"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_ERROR_ARGS, "result == EX_ERROR_ARGS");
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_edge_case_min_values() {
    const char *argv[] = {"program", "0", "0", "3", "0", "0"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_SUCCESS, "result == EX_SUCCESS");
    assert(cfg.num_trucks == 0);
    assert(cfg.num_cars == 0);
    assert(cfg.capacity == 3);
    assert(cfg.max_car_arrival_us == 0);
    assert(cfg.max_truck_arrival_us == 0);
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void test_edge_case_max_values() {
    const char *argv[] = {"program", "10000", "10000", "100", "10000", "1000"};
    Config cfg;
    int result = parse_args(6, argv, &cfg);
    ASSERT(result,  EX_SUCCESS, "result == EX_SUCCESS");
    assert(cfg.num_trucks == 10000);
    assert(cfg.num_cars == 10000);
    assert(cfg.capacity == 100);
    assert(cfg.max_car_arrival_us == 10000);
    assert(cfg.max_truck_arrival_us == 1000);
    printf("\033[32mTest '%s' passed.\033[0m\n", __func__); // Print success message in green
}

void run_args_test() {

    init_log("arg_tests.log"); // Initialize the log file

    printf("\033[34mRunning args tests...\033[0m\n");
    test_valid_args();
    test_invalid_arg_count_too_few();
    test_invalid_arg_count_too_many();
    test_invalid_num_trucks_negative();
    test_invalid_num_trucks_exceeds_max();
    test_invalid_num_cars_negative();
    test_invalid_num_cars_exceeds_max();
    test_invalid_capacity_below_min();
    test_invalid_capacity_above_max();
    test_invalid_max_car_arrival_us_below_min();
    test_invalid_max_car_arrival_us_above_max();
    test_invalid_max_truck_arrival_us_below_min();
    test_invalid_max_truck_arrival_us_above_max();
    test_edge_case_min_values();
    test_edge_case_max_values();
    test_extra_args();
    test_missing_args();
    test_invalid_arg_values();
    test_invalid_num_args();

    close_log(); // Close the log file

    // Report overall test status
    printf("\nTest Summary:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);

    if (tests_failed > 0) {
        printf("\033[31mSome tests failed. Check the log file for details.\033[0m\n");
    } else {
        printf("\033[32mAll tests passed!\033[0m\n");
    }
    reset_test_counters();

    
}


void run_all_tests() {
    run_args_test();
   
}
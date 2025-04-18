#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "tests.h" 
#include "main.h"

// Function to parse arguments
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR:%d] Expected %d arguments, got %d\n",
                EX_ERROR_ARGS, EXPECTED_ARGS, argc);
        return EX_ERROR_ARGS;
    }

    char *end;

    cfg->num_trucks = strtol(argv[1], &end, 10);
    if (*end != '\0' || cfg->num_trucks < 0 || cfg->num_trucks > MAX_NUM_TRUCKS) {
        fprintf(stderr, "[ERROR] Invalid number for num_trucks: %s\n", argv[1]);
        return EX_ERROR_ARGS;
    }

    cfg->num_cars = strtol(argv[2], &end, 10);
    if (*end != '\0' || cfg->num_cars < 0 || cfg->num_cars > MAX_NUM_CARS) {
        fprintf(stderr, "[ERROR] Invalid number for num_cars: %s\n", argv[2]);
        return EX_ERROR_ARGS;
    }

    cfg->capacity = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity < MIN_CAPACITY_TRAFFIC 
        || cfg->capacity > MAX_CAPACITY_TRAFFIC) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for capacity: %s\n", argv[3]);
        return EX_ERROR_ARGS;
    }

    cfg->max_car_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_car_arrival_us < MIN_CAR_ARRIVAL_US 
        || cfg->max_car_arrival_us > MAX_CAR_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for max_car_arrival_us: %s\n", argv[4]);
        return EX_ERROR_ARGS;
    }

    cfg->max_truck_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_truck_arrival_us < MIN_TRUCK_ARRIVAL_US
         || cfg->max_truck_arrival_us > MAX_TRUCK_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for max_truck_arrival_us: %s\n", argv[5]);
        return EX_ERROR_ARGS;
    }

    return EX_SUCCESS;
}


int main(int argc, char const *argv[]) {
    run_all_tests();

    return EX_SUCCESS;
}


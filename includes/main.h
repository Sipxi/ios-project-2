#ifndef MAIN_H
#define MAIN_H


// --- Argument count ---
#define EXPECTED_ARGS 6

// --- Limits ---
#define MAX_NUM_TRUCKS         10000
#define MAX_NUM_CARS           10000

// --- Capacity constraints ---
#define MIN_CAPACITY_TRAFFIC   3
#define MAX_CAPACITY_TRAFFIC   100

// --- Timing constraints (microseconds) ---
#define MIN_CAR_ARRIVAL_US     0
#define MAX_CAR_ARRIVAL_US     10000
#define MIN_TRUCK_ARRIVAL_US   0
#define MAX_TRUCK_ARRIVAL_US   1000

typedef enum {
    EX_SUCCESS,
    EX_ERROR_ARGS,
} ReturnCode;

typedef struct {
    int num_trucks;
    int num_cars;
    int capacity;
    int max_car_arrival_us;
    int max_truck_arrival_us;
} Config;

int parse_args(int argc, char const *argv[], Config *cfg);

#endif // MAIN_H

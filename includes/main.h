#ifndef MAIN_H
#define MAIN_H
#include <sys/mman.h>   // mmap
#include <stdio.h> // input output
#include <stdlib.h> //stol
#include <unistd.h> // sleep
#include <sys/types.h> // pid_t
#include <time.h> // for rand
#include <sys/wait.h> // wait
// --- Argument count ---
#define EXPECTED_ARGS 6

// --- Limits ---
#define MAX_NUM_TRUCKS         10000
#define MAX_NUM_CARS           10000

// --- Capacity constraints ---
#define MIN_CAPACITY_PARCEL   3
#define MAX_CAPACITY_PARCEL   100

// --- Timing constraints (microseconds) ---
#define MIN_VEHICLE_ARRIVAL_US     0
#define MAX_VEHICLE_ARRIVAL_US     10000
#define MIN_FERRY_ARRIVAL_US   0
#define MAX_FERRY_ARRIVAL_US   1000



typedef enum {
    EX_SUCCESS,
    EX_ERROR_ARGS,
    EX_ERROR_INIT,
    EX_ERROR_FORK,
} ReturnCode;

// Configuration structure
typedef struct {
    int num_trucks;
    int num_cars;
    int capacity_of_ferry;
    int max_vehicle_arrival_us;
    int max_ferry_arrival_us;
} Config;


typedef struct {
    int action_counter;       // Global action counter (A)
    int ferry_port;           // Current port of the ferry (0 or 1)
    int ferry_capacity;       // Remaining capacity of the ferry
    int waiting_trucks[2];    // Number of trucks waiting at each port
    int waiting_cars[2];      // Number of cars waiting at each port
    int loaded_trucks;        // Number of trucks currently on the ferry
    int loaded_cars;          // Number of cars currently on the ferry
} SharedData;


SharedData *init_shared_data(Config cfg);
int parse_args(int argc, char const *argv[], Config *cfg);


void ferry_procces();

void car_procces(int car_id, int port);
void truck_procces(int truck_id, int port);
void create_ferry_process(SharedData *shared_data, Config cfg);
void create_car_process(SharedData *shared_data, Config cfg);
void create_truck_process(SharedData *shared_data, Config cfg);
void wait_for_children();



#endif
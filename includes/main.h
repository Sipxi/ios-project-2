// Author: Serhij Čepil (sipxi)
// FIT VUT Student
// https://github.com/sipxi
// Date: 18/05/2025
#ifndef MAIN_H
#define MAIN_H
#include <sys/mman.h>   // mmap
#include <stdio.h> // input output
#include <stdlib.h> //stol
#include <unistd.h> // sleep
#include <sys/types.h> // pid_t
#include <time.h> // for rand
#include <sys/wait.h> // wait
#include <semaphore.h> // sem_t
#include <stdbool.h>
#include <string.h>
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

// --- Constants ---
#define TRUCK_SIZE 3
#define CAR_SIZE 1

// --- Structs ---
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
    int vehicles_to_unload;   // Number of vehicles to unload
    int vehicles_unloaded;
    int next_vehicle_is_truck;
    int total_vehicles_unloaded;
    sem_t action_counter_sem;    // Semaphore for synchronizing action counter
    sem_t unload_vehicle;
    sem_t lock_mutex;
    sem_t vehicle_loading;
    sem_t unload_complete_sem;     // ferry waits on this
    sem_t load_truck;
    sem_t load_car;
    sem_t port_ready[2];
    sem_t loading_done;
} SharedData;

//--- Functions ---
SharedData *init_shared_data(Config cfg);
int parse_args(int argc, char const *argv[], Config *cfg);
int rand_range(int min, int max);



void print_action(SharedData *shared_data, const char vehicle_type, int vehicle_id, const char *action, int port);
void print_shared_data(SharedData *shared_data);

void vehicle_process(SharedData *shared_data, Config cfg, char vehicle_type, int id, int port);
void ferry_process(SharedData *shared_data, Config cfg); 
void create_ferry_process(SharedData *shared_data, Config cfg);
void create_vehicle_process(SharedData *shared_data, Config cfg, const char vehicle_type);

void wait_for_children();


#endif
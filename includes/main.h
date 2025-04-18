#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

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

// --- Semaphore names ---
#define SEM_FERRY "sem_ferry"        // Semaphore for ferry synchronization
#define SEM_CARS "sem_cars"          // Semaphore for car synchronization
#define SEM_TRUCKS "sem_trucks"      // Semaphore for truck synchronization
#define SEM_OUTPUT "sem_output"      // Semaphore for writing to proj2.out

typedef enum {
    EX_SUCCESS,
    EX_ERROR_ARGS,
} ReturnCode;

// Configuration structure
typedef struct {
    int num_trucks;
    int num_cars;
    int capacity;
    int max_car_arrival_us;
    int max_truck_arrival_us;
} Config;

// Shared data structure
typedef struct {
    int action_counter;       // Global action counter (A)
    int ferry_port;           // Current port of the ferry (0 or 1)
    int ferry_capacity;       // Remaining capacity of the ferry
    int waiting_cars[2];      // Number of cars waiting at each port
    int waiting_trucks[2];    // Number of trucks waiting at each port
} SharedData;

// Function prototypes
int parse_args(int argc, char const *argv[], Config *cfg);
void init_shared_memory(SharedData **shared_data);
sem_t *init_semaphore(const char *name);
void cleanup_semaphores();
pid_t create_ferry_process(SharedData *shared_data, Config *cfg);
void create_vehicle_processes(SharedData *shared_data, Config *cfg);
void ferry_process(SharedData *shared_data, Config *cfg);
void truck_process(int id, SharedData *shared_data, Config *cfg);
void car_process(int id, SharedData *shared_data, Config *cfg);
void cleanup_resources(SharedData *shared_data);

#endif // MAIN_H
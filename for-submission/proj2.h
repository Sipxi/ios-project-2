/**
 * Author: Serhij Čepil (sipxi)
 * FIT VUT Student
 * https://github.com/sipxi
 * Date: 18/05/2025
 * Time spent: 63h
 */
#ifndef PROJ2_H
#define PROJ2_H
#include <semaphore.h>  // sem_t
#include <stdio.h>      // input output
#include <stdlib.h>     //stol
#include <string.h>
#include <sys/mman.h>   // mmap
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <time.h>       // for rand
#include <unistd.h>     // sleep
// --- Argument count ---
#define EXPECTED_ARGS 6

// --- Limits ---
#define MAX_NUM_TRUCKS 10000
#define MAX_NUM_CARS 10000

// --- Capacity constraints ---
#define MIN_CAPACITY_PARCEL 3
#define MAX_CAPACITY_PARCEL 100

// --- Timing constraints (microseconds) ---
#define MIN_VEHICLE_ARRIVAL_US 0
#define MAX_VEHICLE_ARRIVAL_US 10000
#define MIN_FERRY_ARRIVAL_US 0
#define MAX_FERRY_ARRIVAL_US 1000

// --- Constants ---
#define TRUCK_SIZE 3
#define CAR_SIZE 1
#define PARSE_BASE_DECIMAL 10
// --- Structs ---
typedef struct {
    int num_trucks;
    int num_cars;
    int capacity_of_ferry;
    int max_vehicle_arrival_us;
    int max_ferry_arrival_us;
    FILE *log_file;
} Config;

typedef struct {
    int action_counter;      // Global action counter
    int ferry_port;          // Current port of the ferry (0 or 1)
    int ferry_capacity;      // Ferry capacity
    int waiting_trucks[2];   // Number of trucks waiting at each port
    int waiting_cars[2];     // Number of cars waiting at each port
    int loaded_trucks;       // Number of trucks currently on the ferry
    int loaded_cars;         // Number of cars currently on the ferry
    int vehicles_to_unload;  // Number of vehicles to unload
    int vehicles_unloaded;   // Number of vehicles unloaded
    int next_vehicle_is_truck; // Next vehicle to unload 
    int total_vehicles_unloaded; // Total number of vehicles unloaded
    sem_t action_counter_sem;  // Semaphore for synchronizing action counter
    sem_t unload_vehicle;     // Semaphore for unloading vehicles
    sem_t lock_mutex;  // Semaphore for synchronizing shared data
    sem_t vehicle_boarding; // Semaphore for vehicle boarding
    sem_t unload_complete_sem;  // Semaphore for unload completion
    sem_t load_truck[2]; // Semaphores for loading trucks at each port
    sem_t load_car[2]; // Semaphores for loading cars at each port
    sem_t loading_done; // Semaphore for loading completion
} SharedData;

//--- Helpers ---

int parse_uint(const char *value_str, int min, int max, const char *arg_name,
               int *result);
int destroy_semaphore(sem_t *sem, const char *sem_name);
int rand_range(int min, int max);
int init_semaphore(sem_t *sem, int pshared, unsigned int value,
                   const char *sem_name);
void wait_for_children();
FILE *file_init(const char *filename);
void wait_for_loading_signal(SharedData *shared_data, char vehicle_type,
                             int port);
void board_vehicle(SharedData *shared_data, Config cfg, char vehicle_type,
                   int id);
void add_vehicle_to_port(SharedData *shared_data, char vehicle_type, int port);
void ferry_to_another_port(SharedData *shared_data, FILE *log_file);
int unload_vehicles(SharedData *shared_data);
int try_load_vehicle(SharedData *shared_data, int port, int is_truck,
                     int *remaining_capacity, int *vehicle_count);
//--- Functions ---

int cleanup(SharedData *shared_data);
int load_ferry(SharedData *shared_data, Config cfg);
int parse_args(int argc, char const *argv[], Config *cfg);
void print_action(SharedData *shared_data, FILE *log_file,
                  const char vehicle_type, int vehicle_id, const char *action,
                  int port);
void ferry_process(SharedData *shared_data, Config cfg);
void vehicle_process(SharedData *shared_data, Config cfg, char vehicle_type,
                     int id, int port);

SharedData *init_shared_data(Config cfg);
void print_shared_data(SharedData *shared_data);

void create_ferry_process(SharedData *shared_data, Config cfg);
void create_vehicle_process(SharedData *shared_data, Config cfg,
                            const char vehicle_type);

#endif
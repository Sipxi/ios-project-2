/**
 * Author: Serhij Čepil (sipxi)
 * FIT VUT Student
 * https://github.com/sipxi
 * Date: 18/05/2025
 * Time spent: 61h
 */
#include "main.h"

/**
 * @brief Helper function to parse and validate an unsigned integer argument.
 * @param value_str The value that has to be parsed.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @param arg_name Name of the argument for error messages.
 * @param result Pointer to store the parsed value.
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise.
 */
int parse_uint(const char *value_str, unsigned short min, unsigned short max,
               const char *arg_name, unsigned short *result) {
    char *end;
    unsigned long value = strtoul(value_str, &end, 10);

    // Check for parsing errors or out-of-range values
    if (*end != '\0' || value < min || value > max) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for %s: %s "
                "(allowed range: %d-%d)\n",
                arg_name, value_str, min, max);
        return EXIT_FAILURE;
    }

    *result = value;
    return EXIT_SUCCESS;
}

/**
 * @brief Function to parse arguments
 * @param argc Number of arguments
 * @param argv Arguments list
 * @param cfg Configuration structure
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR] Expected %d arguments, got %d\n",
                EXPECTED_ARGS, argc);
        return EXIT_FAILURE;
    }

    // Parse and validate each argument
    if (parse_uint(argv[1], 0, MAX_NUM_TRUCKS, "num_trucks", &cfg->num_trucks) 
    || parse_uint(argv[2], 0, MAX_NUM_CARS, "num_cars", &cfg->num_cars) 
    || parse_uint(argv[3], MIN_CAPACITY_PARCEL, MAX_CAPACITY_PARCEL,
                   "capacity_of_ferry", &cfg->capacity_of_ferry) 
    || parse_uint(argv[4], MIN_VEHICLE_ARRIVAL_US, MAX_VEHICLE_ARRIVAL_US,
                   "max_vehicle_arrival_us", &cfg->max_vehicle_arrival_us)
    ||  parse_uint(argv[5], MIN_FERRY_ARRIVAL_US, MAX_FERRY_ARRIVAL_US,
                   "max_ferry_arrival_us", &cfg->max_ferry_arrival_us)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Initializes shared data structure and semaphores
 * @param cfg Configuration structure.
 * @return Pointer to the initialized Shareddata structure if successful,
 *         otherwise NULL on failure.
 *
 * Allocates shared memory for the SharedData structure with mmap,
 * initializes semaphores for synchronization,
 * sets initial values for shared data.
 * TODO Helper func, to reduce code?
 */
SharedData *init_shared_data(Config cfg) {
    SharedData *shared_data =
        mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (shared_data == MAP_FAILED) {
        fprintf(stderr, "[ERROR] mmap failed\n");
        return NULL;
    }

    // Initialize unnamed semaphore in shared memory
    if (sem_init(&shared_data->action_counter_sem, 1, 1) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for action_counter_sem\n");
        return NULL;
    }
    // Initialize unnamed semaphore in shared memory
    if (sem_init(&shared_data->unload_vehicle, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for unload_vehicle\n");
        return NULL;
    }
    // Initialize unnamed semaphore in shared memory
    if (sem_init(&shared_data->lock_mutex, 1, 1) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for lock_mutex\n");
        return NULL;
    }
    // Initialize unnamed semaphore in shared memory
    if (sem_init(&shared_data->unload_complete_sem, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for unload_complete_sem\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_car[0], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_car[0]\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_car[1], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_car[1]\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_truck[0], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_truck[0]\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_truck[1], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_truck[1]\n");
        return NULL;
    }

    if (sem_init(&shared_data->vehicle_loading, 1, 1) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for vehicle_loading\n");
        return NULL;
    }

    if (sem_init(&shared_data->loading_done, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for loading_done\n");
        return NULL;
    }

    shared_data->action_counter = 1;
    shared_data->ferry_port = 0;
    shared_data->ferry_capacity = cfg.capacity_of_ferry;
    shared_data->waiting_trucks[0] = 0;
    shared_data->waiting_trucks[1] = 0;
    shared_data->waiting_cars[0] = 0;
    shared_data->waiting_cars[1] = 0;
    shared_data->loaded_cars = 0;
    shared_data->loaded_trucks = 0;
    shared_data->total_vehicles_unloaded = 0;
    return shared_data;
}

/**
 * @brief Generates a random number within a given range inclusive
 * @param min Lower bound of the range
 * @param max Upper bound of the range
 * @return A random int in the range (min , max)
 */
int rand_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/**
 * @brief Initializes and opens a file with write permissions.
 * @param filename The name of the file to be opened.
 * @return A pointer to the opened FILE. Exits the program if the file cannot be
 * opened.
 */
FILE *file_init(const char *filename) {
    FILE *file = fopen("proj2.out", "w");
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Failed to open file\n");
        exit(EXIT_FAILURE);
    }
    return file;
}

/**
 * @brief Logs an action performed by a vehicle or ferry to a log file.
 *
 * @param shared_data Pointer to the shared data.
 * @param log_file File pointer to the log file where actions are recorded.
 * @param vehicle_type Character representing the type of vehicle ('O' for car,
 * 'N' for truck, 'P' for ferry).
 * @param id Identifier of the vehicle. Pass 0 for ferry.
 * @param action Description of the action being logged.
 * @param port The port number related to the action. If you don't have one,
 * pass -1.
 *
 * This function records an action and its relevant details in a log file.
 * It ensures synchronized access to the action counter using a semaphore.
 */
void print_action(SharedData *shared_data, FILE *log_file,
                  const char vehicle_type, int id, const char *action,
                  int port) {
    sem_wait(&shared_data->action_counter_sem);

    fprintf(log_file, "%d: ", shared_data->action_counter++);
    //If id is 0, it's a ferry
    if (id == 0) {
        fprintf(log_file, "%c: %s", vehicle_type, action);
    } else {
        fprintf(log_file, "%c %d: %s", vehicle_type, id, action);
    }
    //If port is not -1, print it
    if (port != -1) {
        fprintf(log_file, " %d", port);
    }
    fprintf(log_file, "\n");
    // Flush the log file
    fflush(log_file);
    sem_post(&shared_data->action_counter_sem);
}

int unload_vehicles(SharedData *shared_data, Config cfg) {
    int to_unload = 0;

    sem_wait(&shared_data->lock_mutex);
    to_unload = shared_data->loaded_cars + shared_data->loaded_trucks;
    shared_data->vehicles_to_unload = to_unload;
    shared_data->vehicles_unloaded = 0;
    sem_post(&shared_data->lock_mutex);

    for (int i = 0; i < to_unload; i++) {
        sem_post(&shared_data->unload_vehicle);  // allow one vehicle to unload

        // To count total unloaded vehicles
        sem_wait(&shared_data->lock_mutex);
        shared_data->total_vehicles_unloaded++;
        sem_post(&shared_data->lock_mutex);
    }

    return to_unload;
}

int load_ferry(SharedData *shared_data, Config cfg) {
    int port = shared_data->ferry_port;
    int remaining_capacity = cfg.capacity_of_ferry;
    int vehicle_count = 0;

    while (remaining_capacity > 0) {
        sem_wait(&shared_data->lock_mutex);
        int loaded = 0;

        // Try to load expected vehicle type
        if (shared_data->next_vehicle_is_truck) {
            if (shared_data->waiting_trucks[port] > 0 &&
                remaining_capacity >= 3) {
                shared_data->waiting_trucks[port]--;
                remaining_capacity -= 3;
                shared_data->vehicles_to_unload++;
                sem_post(&shared_data->load_truck[port]);
                sem_wait(&shared_data->vehicle_loading);
                vehicle_count++;
                loaded = 1;
            }
        } else {
            if (shared_data->waiting_cars[port] > 0 &&
                remaining_capacity >= 1) {
                shared_data->waiting_cars[port]--;
                remaining_capacity -= 1;
                shared_data->vehicles_to_unload++;
                sem_post(&shared_data->load_car[port]);
                sem_wait(&shared_data->vehicle_loading);
                vehicle_count++;
                loaded = 1;
            }
        }

        // Try alternate vehicle if expected couldn't load
        if (!loaded) {
            if (!shared_data->next_vehicle_is_truck) {
                if (shared_data->waiting_trucks[port] > 0 &&
                    remaining_capacity >= 3) {
                    shared_data->waiting_trucks[port]--;
                    remaining_capacity -= 3;
                    shared_data->vehicles_to_unload++;
                    sem_post(&shared_data->load_truck[port]);
                    sem_wait(&shared_data->vehicle_loading);
                    vehicle_count++;
                    loaded = 1;
                }
            } else {
                if (shared_data->waiting_cars[port] > 0 &&
                    remaining_capacity >= 1) {
                    shared_data->waiting_cars[port]--;
                    remaining_capacity -= 1;
                    shared_data->vehicles_to_unload++;
                    sem_post(&shared_data->load_car[port]);
                    sem_wait(&shared_data->vehicle_loading);
                    vehicle_count++;
                    loaded = 1;
                }
            }
        }

        // No vehicle could be loaded
        if (!loaded) {
            sem_post(&shared_data->lock_mutex);
            break;
        }

        // Alternate type for next expected vehicle
        shared_data->next_vehicle_is_truck =
            !shared_data->next_vehicle_is_truck;
        sem_post(&shared_data->lock_mutex);
    }

    return vehicle_count;
}
//! Do i need config file
void ferry_to_another_port(SharedData *shared_data, Config cfg) {
    print_action(shared_data, cfg.log_file, 'P', 0, "leaving",
                 shared_data->ferry_port);
    sem_wait(&shared_data->lock_mutex);
    shared_data->ferry_port = (shared_data->ferry_port + 1) % 2;
    sem_post(&shared_data->lock_mutex);
}

// Function for ferry process
void ferry_process(SharedData *shared_data, Config cfg) {
    print_action(shared_data, cfg.log_file, 'P', 0, "started", -1);

    while (1) {
        // Wait for ferry  to arrive
        usleep(rand_range(0, cfg.max_ferry_arrival_us));
        print_action(shared_data, cfg.log_file, 'P', 0, "arrived to",
                     shared_data->ferry_port);

        // If there are vehicles to unload unload them
        sem_wait(&shared_data->lock_mutex);
        int to_unload = shared_data->vehicles_to_unload;
        sem_post(&shared_data->lock_mutex);
        if (to_unload > 0) {
            unload_vehicles(shared_data, cfg);  // signal vehicles to unload
            // Wait until all of them reported back
            sem_wait(&shared_data->unload_complete_sem);
        }
        //  Check if there are no more vehicles to work with
        sem_wait(&shared_data->lock_mutex);
        if (shared_data->total_vehicles_unloaded ==
            cfg.num_cars + cfg.num_trucks) {
            print_action(shared_data, cfg.log_file, 'P', 0, "leaving",
                         shared_data->ferry_port);
            print_action(shared_data, cfg.log_file, 'P', 0, "finish", -1);
            break;
        }
        sem_post(&shared_data->lock_mutex);

        // reset counters
        sem_wait(&shared_data->lock_mutex);
        shared_data->vehicles_to_unload = 0;
        shared_data->loaded_cars = 0;
        shared_data->loaded_trucks = 0;
        sem_post(&shared_data->lock_mutex);

        // Signal vehicles to load
        int to_load = load_ferry(shared_data, cfg);

        // Wait for all vehicles to load
        for (int i = 0; i < to_load; i++) {
            sem_wait(&shared_data->loading_done);
        }
        // Go to another port
        ferry_to_another_port(shared_data, cfg);
    }
}

// Function for vehicle process (truck or car)
void vehicle_process(SharedData *shared_data, Config cfg, char vehicle_type,
                     int id, int port) {
    print_action(shared_data, cfg.log_file, vehicle_type, id, "started", -1);
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));
    print_action(shared_data, cfg.log_file, vehicle_type, id, "arrived to",
                 port);

    // Modify waiting amount at port
    sem_wait(&shared_data->lock_mutex);
    if (vehicle_type == 'N') {
        shared_data->waiting_trucks[port]++;
    } else {
        shared_data->waiting_cars[port]++;
    }
    sem_post(&shared_data->lock_mutex);

    // Wait for port to be ready
    // Based on vehicle type wait for signal
    if (vehicle_type == 'N') {
        sem_wait(&shared_data->load_truck[port]);
    } else {
        sem_wait(&shared_data->load_car[port]);
    }
    // Signal to ferry that I'm loading
    sem_post(&shared_data->vehicle_loading);

    // Edit shared data
    sem_wait(&shared_data->lock_mutex);
    if (vehicle_type == 'N') {
        shared_data->loaded_trucks++;
    } else {
        shared_data->loaded_cars++;
    }
    print_action(shared_data, cfg.log_file, vehicle_type, id, "boarding", -1);
    // Signal to the ferry that I'm done
    sem_post(&shared_data->loading_done);
    sem_post(&shared_data->lock_mutex);

    // Wait until ferry says go ahead
    sem_wait(&shared_data->unload_vehicle);

    // Now I'm unloading...
    print_action(shared_data, cfg.log_file, vehicle_type, id, "leaving in",
                 (port + 1) % 2);

    // Notify ferry I’m done
    sem_wait(&shared_data->lock_mutex);
    // Edit shared data
    shared_data->vehicles_unloaded++;
    // If all vehicles unloaded, signal to ferry
    if (shared_data->vehicles_unloaded == shared_data->vehicles_to_unload) {
        sem_post(&shared_data->unload_complete_sem);
    }
    sem_post(&shared_data->lock_mutex);
}

void create_ferry_process(SharedData *shared_data, Config cfg) {
    pid_t ferry_pid = fork();
    if (ferry_pid == 0) {
        // Ferry process

        ferry_process(shared_data, cfg);
        exit(EXIT_SUCCESS);
    } else if (ferry_pid < 0) {
        fprintf(stderr, "[ERROR] fork failed\n");
        exit(EXIT_FAILURE);
    }
}

void create_vehicle_process(SharedData *shared_data, Config cfg,
                            const char vehicle_type) {
    int num_vehicles = vehicle_type == 'O' ? cfg.num_cars : cfg.num_trucks;
    for (int i = 0; i < num_vehicles; i++) {
        pid_t vehicle_pid = fork();
        if (vehicle_pid == 0) {
            srand(getpid());  // Seed the random number generator using the
                              // process ID
            int port = rand() % 2;
            int id = i + 1;
            vehicle_process(shared_data, cfg, vehicle_type, id, port);  //
            // Process vehicle
            exit(EXIT_SUCCESS);
        } else if (vehicle_pid < 0) {
            fprintf(stderr, "[ERROR] fork failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Helper function to destroy a semaphore
 * @param sem Pointer to the semaphore
 * @param sem_name Name of the semaphore for error messages
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int destroy_semaphore(sem_t *sem, const char *sem_name) {
    if (sem_destroy(sem) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed for %s\n", sem_name);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Function to clean up shared data and semaphores
 * @param shared_data Pointer to the shared data structure
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int cleanup(SharedData *shared_data) {
    int result = EXIT_SUCCESS;

    // Destroy semaphores
    if (destroy_semaphore(&shared_data->action_counter_sem, "action_counter_sem") ||
        destroy_semaphore(&shared_data->unload_vehicle, "unload_vehicle") ||
        destroy_semaphore(&shared_data->lock_mutex, "lock_mutex") ||
        destroy_semaphore(&shared_data->unload_complete_sem, "unload_complete_sem") ||
        destroy_semaphore(&shared_data->load_car[0], "load_car") ||
        destroy_semaphore(&shared_data->load_car[1], "load_car") ||
        destroy_semaphore(&shared_data->load_truck[0], "load_truck") ||
        destroy_semaphore(&shared_data->load_truck[1], "load_truck") ||
        destroy_semaphore(&shared_data->vehicle_loading, "vehicle_loading") ||
        destroy_semaphore(&shared_data->loading_done, "loading_done")) {
        result = EXIT_FAILURE; // Mark failure but continue cleanup
    }

    // Unmap shared memory
    if (munmap(shared_data, sizeof(SharedData)) == -1) {
        fprintf(stderr, "[ERROR] munmap failed\n");
        result = EXIT_FAILURE;
    }
    return result;
}

/**
 * @brief Wait for all child processes to finish
 */
void wait_for_children() {
    while (wait(NULL) > 0);  // Wait for all child processes to finish
}

// Main function
int main(int argc, char const *argv[]) {
    Config cfg;
    if (parse_args(argc, argv, &cfg) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    cfg.log_file = file_init("proj2.out");
    SharedData *shared_data = init_shared_data(cfg);
    if (shared_data == NULL) {
        return EXIT_FAILURE;
    }
    srand(time(NULL));  // Initialize random number generator for random port
                        // assignment

    // 1. Create ferry process
    create_ferry_process(shared_data, cfg);
    // 2. Create personal car processes
    create_vehicle_process(shared_data, cfg, 'O');
    // 3. Create truck processes
    create_vehicle_process(shared_data, cfg, 'N');
    // 4. Wait for all processes to finish
    wait_for_children();
    // 5. Clean up
    if (cleanup(shared_data) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    fclose(cfg.log_file);
    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EXIT_SUCCESS;
}
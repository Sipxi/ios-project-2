/**
 * Author: Serhij Čepil (sipxi)
 * FIT VUT Student
 * https://github.com/sipxi
 * Date: 18/05/2025
 * Time spent: 63h
 */
#include "main.h"

/**
 * @brief Helper function to parse and validate an argument
 * @param value_str The value that has to be parsed
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @param arg_name Name of the argument for error messages
 * @param result Pointer to store the parsed value
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int parse_uint(const char *value_str, int min, int max, const char *arg_name,
               int *result) {
    char *end;
    int value = strtoul(value_str, &end, 10);

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
 * 
 * This function parses the command line arguments and validates them.
 * If any argument is invalid, it prints an error message and returns EXIT_FAILURE.
 */
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR] Expected %d arguments, got %d\n",
                EXPECTED_ARGS, argc);
        return EXIT_FAILURE;
    }

    // Parse and validate each argument
    if (parse_uint(argv[1], 0, MAX_NUM_TRUCKS, "num_trucks",
                   &cfg->num_trucks) ||
        parse_uint(argv[2], 0, MAX_NUM_CARS, "num_cars", &cfg->num_cars) ||
        parse_uint(argv[3], MIN_CAPACITY_PARCEL, MAX_CAPACITY_PARCEL,
                   "capacity_of_ferry", &cfg->capacity_of_ferry) ||
        parse_uint(argv[4], MIN_VEHICLE_ARRIVAL_US, MAX_VEHICLE_ARRIVAL_US,
                   "max_vehicle_arrival_us", &cfg->max_vehicle_arrival_us) ||
        parse_uint(argv[5], MIN_FERRY_ARRIVAL_US, MAX_FERRY_ARRIVAL_US,
                   "max_ferry_arrival_us", &cfg->max_ferry_arrival_us)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Helper function to initialize a semaphore
 * @param sem Pointer to the semaphore
 * @param pshared Whether the semaphore is shared between processes (1 = shared)
 * @param init_value Initial value of the semaphore
 * @param sem_name Name of the semaphore for error messages
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int init_semaphore(sem_t *sem, int pshared, unsigned int init_value,
                   const char *sem_name) {
    if (sem_init(sem, pshared, init_value) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for %s\n", sem_name);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Function to initialize shared data
 * @param cfg Configuration structure
 * @return Pointer to initialized SharedData, or NULL on failure
 *
 * Initializes shared data using mmap and semaphores
 */
SharedData *init_shared_data(Config cfg) {
    // Allocate shared memory using mmap
    SharedData *shared_data =
        mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (shared_data == MAP_FAILED) {
        fprintf(stderr, "[ERROR] mmap failed\n");
        return NULL;
    }

    // Initialize semaphores
    if (init_semaphore(&shared_data->action_counter_sem, 1, 1,
                       "action_counter_sem") ||
        init_semaphore(&shared_data->unload_vehicle, 1, 0, "unload_vehicle") ||
        init_semaphore(&shared_data->lock_mutex, 1, 1, "lock_mutex") ||
        init_semaphore(&shared_data->unload_complete_sem, 1, 0,
                       "unload_complete_sem") ||
        init_semaphore(&shared_data->load_car[0], 1, 0, "load_car[0]") ||
        init_semaphore(&shared_data->load_car[1], 1, 0, "load_car[1]") ||
        init_semaphore(&shared_data->load_truck[0], 1, 0, "load_truck[0]") ||
        init_semaphore(&shared_data->load_truck[1], 1, 0, "load_truck[1]") ||
        init_semaphore(&shared_data->vehicle_boarding, 1, 1,
                       "vehicle_boarding") ||
        init_semaphore(&shared_data->loading_done, 1, 0, "loading_done")) {
        // Clean up shared memory
        munmap(shared_data, sizeof(SharedData));
        return NULL;
    }

    // Initialize shared data
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
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Failed to open file\n");
        exit(EXIT_FAILURE);
    }
    return file;
}

/**
 * @brief Logs an action performed by a vehicle or ferry to a log file.
 * @param shared_data Pointer to the shared data.
 * @param log_file File pointer to the log file where actions are recorded.
 * @param vehicle_type Character representing the type of vehicle ('O' for car,
 * 'N' for truck, 'P' for ferry).
 * @param id ID of the vehicle. Pass 0 for ferry.
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
    // If id is 0, it's a ferry
    if (id == 0) {
        fprintf(log_file, "%c: %s", vehicle_type, action);
    } else {
        fprintf(log_file, "%c %d: %s", vehicle_type, id, action);
    }
    // If port is not -1, print it
    if (port != -1) {
        fprintf(log_file, " %d", port);
    }
    fprintf(log_file, "\n");
    // Flush the log file
    fflush(log_file);
    sem_post(&shared_data->action_counter_sem);
}

/**
 * @brief Unloads vehicles from the ferry.
 * @param shared_data Pointer to the shared data.
 * @param cfg Configuration parameters.
 * @return The number of vehicles unloaded.
 *
 * This function unloads vehicles from the ferry and returns the number of
 * vehicles unloaded. It also updates shared data and the total number of
 * vehicles unloaded.
 */
int unload_vehicles(SharedData *shared_data) {
    // Update shared data and calculate vehicles to unload
    sem_wait(&shared_data->lock_mutex);
    int vehicles_to_unload =
        shared_data->loaded_cars + shared_data->loaded_trucks;
    shared_data->vehicles_to_unload = vehicles_to_unload;
    shared_data->vehicles_unloaded = 0;
    sem_post(&shared_data->lock_mutex);

    for (int vehicle = 0; vehicle < vehicles_to_unload; vehicle++) {
        // allow one vehicle to unload
        sem_post(&shared_data->unload_vehicle);

        // To count total unloaded vehicles
        sem_wait(&shared_data->lock_mutex);
        shared_data->total_vehicles_unloaded++;
        sem_post(&shared_data->lock_mutex);
    }

    return vehicles_to_unload;
}

/**
 * @brief Helper function to try to load a vehicle from the port onto the ferry.
 * @param shared_data Pointer to shared data.
 * @param port The port to load the vehicle from.
 * @param is_truck Whether the vehicle is a truck or a car.
 * @param remaining_capacity Pointer to the remaining capacity of the ferry.
 * @param vehicle_count Pointer to the number of vehicles currently loaded.
 * @return 1 if the vehicle was loaded, 0 otherwise.
 *
 * This function tries to load a vehicle from the given port onto the ferry.
 * It checks if there are vehicles waiting at the port and if the ferry has
 * enough capacity to load the vehicle. If so, it reduces the waiting count
 * at the port and the remaining capacity of the ferry, and signals the
 * vehicle to board the ferry. The function then waits for the vehicle to
 * finish boarding and increments the vehicle count. If the vehicle is not
 * loaded, the function returns 0
 */
int try_load_vehicle(SharedData *shared_data, int port, int is_truck,
                     int *remaining_capacity, int *vehicle_count) {
    int required_space = is_truck ? TRUCK_SIZE : CAR_SIZE;
    // Make a pointer to easily change varible at shared data
    int *waiting = is_truck ? &shared_data->waiting_trucks[port]
                            : &shared_data->waiting_cars[port];
    // Make a pointer to semaphor to easily access it
    sem_t *load_sem = is_truck ? &shared_data->load_truck[port]
                               : &shared_data->load_car[port];

    if (*waiting > 0 && *remaining_capacity >= required_space) {
        (*waiting)--;
        *remaining_capacity -= required_space;
        shared_data->vehicles_to_unload++;
        sem_post(load_sem);
        sem_wait(&shared_data->vehicle_boarding);
        (*vehicle_count)++;
        return 1;
    }

    return 0;
}

/**
 * @brief Loads vehicles onto the ferry.
 * @param shared_data Pointer to shared data
 * @param cfg Configuration struct
 *
 * Loads vehicles onto the ferry in alternating order
 */
int load_ferry(SharedData *shared_data, Config cfg) {
    sem_wait(&shared_data->lock_mutex);
    int port = shared_data->ferry_port;
    int remaining_capacity = cfg.capacity_of_ferry;
    int vehicle_count = 0;
    sem_post(&shared_data->lock_mutex);

    while (remaining_capacity > 0) {
        sem_wait(&shared_data->lock_mutex);

        // Try expected type first
        int loaded = try_load_vehicle(shared_data, port,
                                      shared_data->next_vehicle_is_truck,
                                      &remaining_capacity, &vehicle_count);
        // If not successful, try the other type
        if (!loaded) {
            loaded = try_load_vehicle(shared_data, port,
                                      !shared_data->next_vehicle_is_truck,
                                      &remaining_capacity, &vehicle_count);
        }
        // No vehicle could be loaded
        if (!loaded) {
            sem_post(&shared_data->lock_mutex);
            break;
        }
        // Alternate vehicle type for next time
        shared_data->next_vehicle_is_truck =
            !shared_data->next_vehicle_is_truck;
        sem_post(&shared_data->lock_mutex);
    }

    return vehicle_count;
}

/**
 * @brief Helper function to translate ferry to another port
 * @param shared_data Pointer to shared data
 * @param log_file For logging
 */
void ferry_to_another_port(SharedData *shared_data, FILE *log_file) {
    print_action(shared_data, log_file, 'P', 0, "leaving",
                 shared_data->ferry_port);
    sem_wait(&shared_data->lock_mutex);
    shared_data->ferry_port = (shared_data->ferry_port + 1) % 2;
    sem_post(&shared_data->lock_mutex);
}

/**
 * @brief Main function for ferry process
 * @param shared_data Pointer to shared data
 * @param cfg Config struct
 *
 * Main function for ferry process. This function is responsible for
 * loading and unloading vehicles from the ferry
 */
void ferry_process(SharedData *shared_data, Config cfg) {
    print_action(shared_data, cfg.log_file, 'P', 0, "started", -1);

    while (1) {
        // Wait for ferry to arrive
        usleep(rand_range(0, cfg.max_ferry_arrival_us));
        print_action(shared_data, cfg.log_file, 'P', 0, "arrived to",
                     shared_data->ferry_port);

        sem_wait(&shared_data->lock_mutex);
        int curr_vehicles_to_unload = shared_data->vehicles_to_unload;
        sem_post(&shared_data->lock_mutex);
        // If there are vehicles to unload unload them
        if (curr_vehicles_to_unload > 0) {
            unload_vehicles(shared_data);
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

        // Reset counters
        sem_wait(&shared_data->lock_mutex);
        shared_data->vehicles_to_unload = 0;
        shared_data->loaded_cars = 0;
        shared_data->loaded_trucks = 0;
        sem_post(&shared_data->lock_mutex);

        // Signal vehicles to load
        int vehicles_to_load = load_ferry(shared_data, cfg);

        // Wait for all vehicles to load
        for (int i = 0; i < vehicles_to_load; i++) {
            sem_wait(&shared_data->loading_done);
        }
        // Go to another port
        ferry_to_another_port(shared_data, cfg.log_file);
    }
}

/**
 * @brief Helper function to wait for loading signal
 * @param shared_data Pointer to shared data
 * @param vehicle_type The type of vehicle to add, either 'O' for cars or
 * 'N' for trucks.
 * @param port The port of the ferry
 */
void wait_for_loading_signal(SharedData *shared_data, char vehicle_type,
                             int port) {
    // Based on vehicle type wait for signal
    if (vehicle_type == 'N') {
        sem_wait(&shared_data->load_truck[port]);
    } else {
        sem_wait(&shared_data->load_car[port]);
    }
}

/**
 * @brief Helper function to board a vehicle
 * @param shared_data Pointer to shared data
 * @param cfg Configuration structure containing the parameters for the
 * vehicles.
 * @param vehicle_type The type of vehicle to add, either 'O' for cars or
 * 'N' for trucks.
 * @param id The id of the vehicle
 */
void board_vehicle(SharedData *shared_data, Config cfg, char vehicle_type,
                   int id) {
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
}

/**
 * @brief Helper function to add vehicle to port
 * @param shared_data Pointer to shared data
 * @param cfg Configuration structure.
 * @param vehicle_type The type of vehicle to add, either 'O' for cars or
 * 'N' for trucks.
 * @param port The port to add the vehicle to.
 */
void add_vehicle_to_port(SharedData *shared_data, char vehicle_type, int port) {
    sem_wait(&shared_data->lock_mutex);
    if (vehicle_type == 'N') {
        shared_data->waiting_trucks[port]++;
    } else {
        shared_data->waiting_cars[port]++;
    }
    sem_post(&shared_data->lock_mutex);
}

/**
 * @brief Process a vehicle
 * @param shared_data Pointer to shared data
 * @param cfg Configuration structure
 * @param vehicle_type The type of vehicle to process, either 'O' for cars or
 * 'N' for trucks.
 * @param id The ID of the vehicle.
 * @param port The port the vehicle is heading to.
 *
 * Main function for vehicle process that using the shared data and semaphores
 * to communicate with ferry
 */
void vehicle_process(SharedData *shared_data, Config cfg, char vehicle_type,
                     int id, int port) {
    print_action(shared_data, cfg.log_file, vehicle_type, id, "started", -1);
    // Wait for vehicle to arrive
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));
    print_action(shared_data, cfg.log_file, vehicle_type, id, "arrived to",
                 port);

    // Modify waiting amount at port
    add_vehicle_to_port(shared_data, vehicle_type, port);

    // Wait for loading signal
    wait_for_loading_signal(shared_data, vehicle_type, port);

    // Signal to ferry that I'm boarding
    sem_post(&shared_data->vehicle_boarding);

    board_vehicle(shared_data, cfg, vehicle_type, id);

    // Wait until ferry says go ahead
    sem_wait(&shared_data->unload_vehicle);

    // Now I'm leaving
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

/**
 * @brief Creates a new process for the ferry operation.
 * @param shared_data Pointer to the shared data.
 * @param cfg Configuration structure.
 */
void create_ferry_process(SharedData *shared_data, Config cfg) {
    pid_t ferry_pid = fork();
    if (ferry_pid == 0) {
        ferry_process(shared_data, cfg);
        exit(EXIT_SUCCESS);
    } else if (ferry_pid < 0) {
        fprintf(stderr, "[ERROR] fork failed\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Creates a specified number of vehicle processes.
 * @param shared_data Pointer to the shared data.
 * @param cfg Configuration structure containing the parameters for the
 * vehicles.
 * @param vehicle_type The type of vehicles to create, either 'O' for cars or
 * 'N' for trucks.
 *
 * Forks a specified number of processes which execute the vehicle
 * process function. Each process is seeded with the process ID to generate
 * random port numbers.
 */
void create_vehicle_process(SharedData *shared_data, Config cfg,
                            const char vehicle_type) {
    int num_vehicles = vehicle_type == 'O' ? cfg.num_cars : cfg.num_trucks;
    for (int idx = 0; idx < num_vehicles; idx++) {
        pid_t vehicle_pid = fork();
        if (vehicle_pid == 0) {
            // Seed the random number generator
            srand(getpid());

            int port = rand() % 2;
            int id = idx + 1;

            vehicle_process(shared_data, cfg, vehicle_type, id, port);
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
 * @param shared_data Pointer to the shared data
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int cleanup(SharedData *shared_data) {
    int result = EXIT_SUCCESS;

    // Destroy semaphores
    if (destroy_semaphore(&shared_data->action_counter_sem,
                          "action_counter_sem") ||
        destroy_semaphore(&shared_data->unload_vehicle, "unload_vehicle") ||
        destroy_semaphore(&shared_data->lock_mutex, "lock_mutex") ||
        destroy_semaphore(&shared_data->unload_complete_sem,
                          "unload_complete_sem") ||
        destroy_semaphore(&shared_data->load_car[0], "load_car") ||
        destroy_semaphore(&shared_data->load_car[1], "load_car") ||
        destroy_semaphore(&shared_data->load_truck[0], "load_truck") ||
        destroy_semaphore(&shared_data->load_truck[1], "load_truck") ||
        destroy_semaphore(&shared_data->vehicle_boarding, "vehicle_boarding") ||
        destroy_semaphore(&shared_data->loading_done, "loading_done")) {
        result = EXIT_FAILURE;  // Mark failure but continue cleanup
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

// --- Main function ---
int main(int argc, char const *argv[]) {
    Config cfg;
    // Parse arguments
    if (parse_args(argc, argv, &cfg) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    // Initialize shared data and config
    cfg.log_file = file_init("proj2.out");
    SharedData *shared_data = init_shared_data(cfg);
    if (shared_data == NULL) {
        return EXIT_FAILURE;
    }
    create_ferry_process(shared_data, cfg);
    create_vehicle_process(shared_data, cfg, 'O');
    create_vehicle_process(shared_data, cfg, 'N');
    //  Wait for all processes to finish
    wait_for_children();
    // Cleanup
    if (cleanup(shared_data) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    // Close log file
    fclose(cfg.log_file);
    return EXIT_SUCCESS;
}
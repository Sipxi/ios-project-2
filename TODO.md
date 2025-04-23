Helper args
---------
/**
 * @brief Helper function to parse and validate an unsigned integer argument
 * @param value_str String representation of the value
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @param arg_name Name of the argument for error messages
 * @param result Pointer to store the parsed value
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int parse_uint(const char *value_str, unsigned long min, unsigned long max,
               const char *arg_name, unsigned long *result) {
    char *end;
    unsigned long value = strtoul(value_str, &end, 10);

    // Check for parsing errors or out-of-range values
    if (*end != '\0' || value < min || value > max) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for %s: %s "
                        "(allowed range: %lu-%lu)\n",
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
    if (parse_uint(argv[1], 0, MAX_NUM_TRUCKS, "num_trucks", &cfg->num_trucks) ||
        parse_uint(argv[2], 0, MAX_NUM_CARS, "num_cars", &cfg->num_cars) ||
        parse_uint(argv[3], MIN_CAPACITY_PARCEL, MAX_CAPACITY_PARCEL, "capacity_of_ferry", &cfg->capacity_of_ferry) ||
        parse_uint(argv[4], MIN_VEHICLE_ARRIVAL_US, MAX_VEHICLE_ARRIVAL_US, "max_vehicle_arrival_us", &cfg->max_vehicle_arrival_us) ||
        parse_uint(argv[5], MIN_FERRY_ARRIVAL_US, MAX_FERRY_ARRIVAL_US, "max_ferry_arrival_us", &cfg->max_ferry_arrival_us)) {
        return EXIT_FAILURE;
    }

    // Debugging: Print parsed values
    printf("[DEBUG] Parsed arguments:\n");
    printf("  num_trucks: %lu\n", cfg->num_trucks);
    printf("  num_cars: %lu\n", cfg->num_cars);
    printf("  capacity_of_ferry: %lu\n", cfg->capacity_of_ferry);
    printf("  max_vehicle_arrival_us: %lu\n", cfg->max_vehicle_arrival_us);
    printf("  max_ferry_arrival_us: %lu\n", cfg->max_ferry_arrival_us);

    return EXIT_SUCCESS;
}
---------

Helper shared_init
---------
/**
 * @brief Helper function to initialize a semaphore
 * @param sem Pointer to the semaphore
 * @param pshared Whether the semaphore is shared between processes (1 = shared)
 * @param value Initial value of the semaphore
 * @param sem_name Name of the semaphore for error messages
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int init_semaphore(sem_t *sem, int pshared, unsigned int value, const char *sem_name) {
    if (sem_init(sem, pshared, value) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for %s: %s\n", sem_name, strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Function to initialize shared data
 * @param cfg Configuration structure
 * @return Pointer to initialized SharedData, or NULL on failure
 */
SharedData *init_shared_data(Config cfg) {
    // Allocate shared memory using mmap
    SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (shared_data == MAP_FAILED) {
        fprintf(stderr, "[ERROR] mmap failed: %s\n", strerror(errno));
        return NULL;
    }

    // Initialize semaphores
    if (init_semaphore(&shared_data->action_counter_sem, 1, 1, "action_counter_sem") ||
        init_semaphore(&shared_data->unload_vehicle, 1, 0, "unload_vehicle") ||
        init_semaphore(&shared_data->lock_mutex, 1, 1, "lock_mutex") ||
        init_semaphore(&shared_data->unload_complete_sem, 1, 0, "unload_complete_sem") ||
        init_semaphore(&shared_data->load_car, 1, 0, "load_car") ||
        init_semaphore(&shared_data->load_truck, 1, 0, "load_truck") ||
        init_semaphore(&shared_data->port_ready[0], 1, 0, "port_ready[0]") ||
        init_semaphore(&shared_data->port_ready[1], 1, 0, "port_ready[1]") ||
        init_semaphore(&shared_data->vehicle_loading, 1, 1, "vehicle_loading") ||
        init_semaphore(&shared_data->loading_done, 1, 0, "loading_done")) {
        // Cleanup on failure
        munmap(shared_data, sizeof(SharedData));
        return NULL;
    }

    // Initialize shared data fields
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

    // Debugging: Print initialization status
    printf("[DEBUG] Shared data and semaphores initialized successfully\n");

    return shared_data;
}
---------
cleanup helper
------------
/**
 * @brief Helper function to destroy a semaphore
 * @param sem Pointer to the semaphore
 * @param sem_name Name of the semaphore for error messages
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
 */
int destroy_semaphore(sem_t *sem, const char *sem_name) {
    if (sem_destroy(sem) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed for %s: %s\n", sem_name, strerror(errno));
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
        destroy_semaphore(&shared_data->load_car, "load_car") ||
        destroy_semaphore(&shared_data->load_truck, "load_truck") ||
        destroy_semaphore(&shared_data->port_ready[0], "port_ready[0]") ||
        destroy_semaphore(&shared_data->port_ready[1], "port_ready[1]") ||
        destroy_semaphore(&shared_data->vehicle_loading, "vehicle_loading") ||
        destroy_semaphore(&shared_data->loading_done, "loading_done")) {
        result = EXIT_FAILURE; // Mark failure but continue cleanup
    }

    // Unmap shared memory
    if (munmap(shared_data, sizeof(SharedData)) == -1) {
        fprintf(stderr, "[ERROR] munmap failed: %s\n", strerror(errno));
        result = EXIT_FAILURE;
    }

    // Debugging: Print cleanup status
    if (result == EXIT_SUCCESS) {
        printf("[DEBUG] Cleanup completed successfully\n");
    } else {
        printf("[DEBUG] Cleanup encountered errors\n");
    }

    return result;
}
----------
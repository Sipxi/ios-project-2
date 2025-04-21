// Author: Serhij Čepil (sipxi)
// FIT VUT Student
// https://github.com/sipxi
// Date: 18/05/2025
// Time spent: 30h

#include "main.h"

//* Function to parse arguments
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR] Expected %d arguments, got %d\n",
                EXPECTED_ARGS, argc);
        return EXIT_FAILURE;
    }

    char *end;

    cfg->num_trucks = strtol(argv[1], &end, 10);
    if (*end != '\0' || cfg->num_trucks < 0 ||
        cfg->num_trucks > MAX_NUM_TRUCKS) {
        fprintf(stderr, "[ERROR] Invalid number for num_trucks: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    cfg->num_cars = strtol(argv[2], &end, 10);
    if (*end != '\0' || cfg->num_cars < 0 || cfg->num_cars > MAX_NUM_CARS) {
        fprintf(stderr, "[ERROR] Invalid number for num_cars: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    cfg->capacity_of_ferry = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity_of_ferry < MIN_CAPACITY_PARCEL ||
        cfg->capacity_of_ferry > MAX_CAPACITY_PARCEL) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for parcel "
                "capacity_of_ferry: %s\n",
                argv[3]);
        return EXIT_FAILURE;
    }

    cfg->max_vehicle_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_vehicle_arrival_us < MIN_VEHICLE_ARRIVAL_US ||
        cfg->max_vehicle_arrival_us > MAX_VEHICLE_ARRIVAL_US) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for vehicle_arrival_us: "
                "%s\n",
                argv[4]);
        return EXIT_FAILURE;
    }

    cfg->max_ferry_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_ferry_arrival_us < MIN_FERRY_ARRIVAL_US ||
        cfg->max_ferry_arrival_us > MAX_FERRY_ARRIVAL_US) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for "
                "min_parcel_arrival_us: %s\n",
                argv[5]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

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
    if (sem_init(&shared_data->protection_mutex, 1, 1) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for protection_mutex\n");
        return NULL;
    }
        // Initialize unnamed semaphore in shared memory
    if (sem_init(&shared_data->unload_complete_sem, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for unload_complete_sem\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_car, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_car\n");
        return NULL;
    }
    if (sem_init(&shared_data->load_truck, 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for load_truck\n");
        return NULL;
    }
    if (sem_init(&shared_data->port_ready[0], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for port_ready[0]\n");
        return NULL;
    }
    if (sem_init(&shared_data->port_ready[1], 1, 0) == -1) {
        fprintf(stderr, "[ERROR] sem_init failed for port_ready[1]\n");
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
    return shared_data;
}

//* Function to generate a random number within a range
int rand_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

//* Print and update action counter in shared_data
void print_action(SharedData *shared_data, const char vehicle_type, int id,
                  const char *action, int port) {
    sem_wait(&shared_data->action_counter_sem);

    if (id == 0) {
        printf("%d: %c: %s, Port: %d\n", shared_data->action_counter++,
               vehicle_type, action, port);
    } else {
        printf("%d: %c %d: %s, Port: %d\n", shared_data->action_counter++,
               vehicle_type, id, action, port);
    }

    sem_post(&shared_data->action_counter_sem);
}

int unload_vehicles(SharedData *shared_data, Config cfg) {
    int to_unload;

    sem_wait(&shared_data->protection_mutex);
    to_unload = shared_data->loaded_cars + shared_data->loaded_trucks;
    shared_data->vehicles_to_unload = to_unload;
    shared_data->vehicles_unloaded = 0;
    sem_post(&shared_data->protection_mutex);

    for (int i = 0; i < to_unload; i++) {
        sem_post(&shared_data->unload_vehicle);  // allow one vehicle to unload
    }

    return to_unload;
}

int load_vehicles(SharedData *shared_data, Config cfg) {
    int loaded_vehicles = 0;
    int space_left = cfg.capacity_of_ferry;  // Available space on the ferry

    // Alternate between loading a cargo truck and a personal vehicle
    while (space_left > 0) {
        // Try to load a cargo truck if there is space
        if (shared_data->next_vehicle_is_truck) {
            if (space_left >= TRUCK_SIZE) {  // Check if there is enough space for a truck
                sem_post(&shared_data->load_truck);  // Signal that a truck can be loaded
                shared_data->next_vehicle_is_truck = 0;  // Next vehicle will be a car
                loaded_vehicles++;
                space_left -= TRUCK_SIZE;  // Reduce the space by the truck size
                printf("Loaded cargo truck, remaining space: %d\n", space_left);
            } else {
                // No more space for a truck, try loading a car if possible
                if (space_left >= CAR_SIZE) {  // Check if there is space for a car
                    sem_post(&shared_data->load_car);  // Signal that a car can be loaded
                    shared_data->next_vehicle_is_truck = 1;  // Next vehicle will be a truck
                    loaded_vehicles++;
                    space_left -= CAR_SIZE;  // Reduce space by the car size
                    printf("Loaded personal vehicle, remaining space: %d\n", space_left);
                } else {
                    break;  // No more space for any vehicle
                }
            }
        }
        // Try to load a personal car
        else {
            if (space_left >= CAR_SIZE) {  // Check if there is space for a car
                sem_post(&shared_data->load_car);  // Signal that a car can be loaded
                shared_data->next_vehicle_is_truck = 1;  // Next vehicle will be a truck
                loaded_vehicles++;
                space_left -= CAR_SIZE;  // Reduce space by the car size
                printf("Loaded personal vehicle, remaining space: %d\n", space_left);
            } else {
                // No more space for a car, try loading a truck if possible
                if (space_left >= TRUCK_SIZE) {  // Check if there is enough space for a truck
                    sem_post(&shared_data->load_truck);  // Signal that a truck can be loaded
                    shared_data->next_vehicle_is_truck = 0;  // Next vehicle will be a car
                    loaded_vehicles++;
                    space_left -= TRUCK_SIZE;  // Reduce space by the truck size
                    printf("Loaded cargo truck, remaining space: %d\n", space_left);
                } else {
                    break;  // No more space for any vehicle
                }
            }
        }
    }
    return loaded_vehicles;
}

int test_load_vehicles(SharedData *shared_data, Config cfg) {
    int loaded_vehicles = 0;
    sem_post(&shared_data->protection_mutex);
    for (int i = 0; i < shared_data->waiting_cars[shared_data->ferry_port]; i++) {
        sem_post(&shared_data->load_car);  // Signal that a car can be loaded
        loaded_vehicles++;
    }
    sem_wait(&shared_data->protection_mutex);



    return loaded_vehicles;
}


// Function for ferry process
void ferry_process(SharedData *shared_data, Config cfg) {
    print_action(shared_data, 'P', 0, "started", shared_data->ferry_port);
    usleep(rand_range(0, cfg.max_ferry_arrival_us));
    print_action(shared_data, 'P', 0, "arrived", shared_data->ferry_port);


    while (1) {
        //printf("Waiting for vehicles to unload...\n");
        if (shared_data->vehicles_to_unload > 0) {
            unload_vehicles(shared_data, cfg);  // signal vehicles to unload
            // Wait until all of them reported back
            //printf("Waiting for everybody to unload...\n");
            sem_wait(&shared_data->unload_complete_sem);
        }
        //printf("All vehicles unloaded\n");

        // reset counters
        sem_wait(&shared_data->protection_mutex);
        shared_data->loaded_cars = 0;
        shared_data->loaded_trucks = 0;
        sem_post(&shared_data->protection_mutex);

        //printf("Reset counters\n");

        printf("Port %d is ready for loading\n", shared_data->ferry_port);
        //ferry ready for loading
        int to_load = test_load_vehicles(shared_data, cfg);
        for (int i = 0; i < to_load; i++) {
            sem_post(&shared_data->port_ready[shared_data->ferry_port]);
        }
        // TODO: load logic goes here (just like unload, but reversed)
        printf("%d vehicles can be loaded\n", to_load);
        break;
    }

}

// Function for vehicle process (truck or car)
void vehicle_process(SharedData *shared_data, Config cfg, char vehicle_type,
                     int id, int port) {
    print_action(shared_data, vehicle_type, id, "started", port);
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));
    print_action(shared_data, vehicle_type, id, "arrived", port);

    // Modify waiting amount at port
    sem_wait(&shared_data->protection_mutex);
    if (vehicle_type == 'N') {
        shared_data->waiting_trucks[port]++;
    } else {
        shared_data->waiting_cars[port]++;
    }
    sem_post(&shared_data->protection_mutex);

    // Wait for ferry to be ready in specific port
    //printf("ID: %d Waiting for port %d \n", id, port);
    sem_wait(&shared_data->port_ready[port]);

    // Now I'm loading...
    sem_wait(&shared_data->protection_mutex);
    if (vehicle_type == 'N') {
        shared_data->loaded_trucks++;
    } else {
        shared_data->loaded_cars++;
    }
    print_action(shared_data, vehicle_type, id, "loading", port);
    sem_post(&shared_data->protection_mutex);


    // Wait until ferry says "go ahead"
    //printf("ID: %d Waiting for unloading \n", id);
    sem_wait(&shared_data->unload_vehicle);

    // Now I'm unloading...
    print_action(shared_data, vehicle_type, id, "leaving", (port + 1) % 2);

    // Notify ferry I’m done
    sem_wait(&shared_data->protection_mutex);
    shared_data->vehicles_unloaded++;
    if (shared_data->vehicles_unloaded == shared_data->vehicles_to_unload) {
        sem_post(&shared_data->unload_complete_sem);
    }
    sem_post(&shared_data->protection_mutex);
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
            int port = 0;
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

int cleanup(SharedData *shared_data) {
    // Clean up the semaphore, print error if it fails
    if (sem_destroy(&shared_data->action_counter_sem) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->unload_vehicle) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->protection_mutex) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->unload_complete_sem) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->load_car) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->load_truck) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->port_ready[0]) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }
    if (sem_destroy(&shared_data->port_ready[1]) == -1) {
        fprintf(stderr, "[ERROR] sem_destroy failed\n");
        return EXIT_FAILURE;
    }

    // Unmap shared memory, print error if it fails
    if (munmap(shared_data, sizeof(SharedData)) == -1) {
        fprintf(stderr, "[ERROR] munmap failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void wait_for_children() {
    while (wait(NULL) > 0);  // Wait for all child processes to finish
}

void print_shared_data(SharedData *shared_data) {
    printf("\nShared data: {\n");
    printf("A: action_counter: %d\n", shared_data->action_counter);
    printf("ferry_port: %d\n", shared_data->ferry_port);
    printf("ferry_capacity: %d\n", shared_data->ferry_capacity);
    printf("N: waiting_trucks[0]: %d\n", shared_data->waiting_trucks[0]);
    printf("N: waiting_trucks[1]: %d\n", shared_data->waiting_trucks[1]);
    printf("O: waiting_cars[0]: %d\n", shared_data->waiting_cars[0]);
    printf("O: waiting_cars[1]: %d\n", shared_data->waiting_cars[1]);
    printf("loaded_cars: %d\n", shared_data->loaded_cars);
    printf("loaded_trucks: %d\n}\n", shared_data->loaded_trucks);
}

// Main function
int main(int argc, char const *argv[]) {
    Config cfg;
    if (parse_args(argc, argv, &cfg) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

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
    //create_vehicle_process(shared_data, cfg, 'N');

    // 4. Wait for all processes to finish
    wait_for_children();

    // 5. Clean up
    if (cleanup(shared_data) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EXIT_SUCCESS;
}

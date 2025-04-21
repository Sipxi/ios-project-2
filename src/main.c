// Author: Serhij Čepil (sipxi)
// FIT VUT Student
// https://github.com/sipxi
// Date: 18/05/2025
// Time spent: 15h
#include "main.h"

// Function to parse arguments
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR] Expected %d arguments, got %d\n",
                EXPECTED_ARGS, argc);
        return EX_ERROR;
    }

    char *end;

    cfg->num_trucks = strtol(argv[1], &end, 10);
    if (*end != '\0' || cfg->num_trucks < 0 ||
        cfg->num_trucks > MAX_NUM_TRUCKS) {
        fprintf(stderr, "[ERROR] Invalid number for num_trucks: %s\n", argv[1]);
        return EX_ERROR;
    }

    cfg->num_cars = strtol(argv[2], &end, 10);
    if (*end != '\0' || cfg->num_cars < 0 || cfg->num_cars > MAX_NUM_CARS) {
        fprintf(stderr, "[ERROR] Invalid number for num_cars: %s\n", argv[2]);
        return EX_ERROR;
    }

    cfg->capacity_of_ferry = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity_of_ferry < MIN_CAPACITY_PARCEL ||
        cfg->capacity_of_ferry > MAX_CAPACITY_PARCEL) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for parcel "
                "capacity_of_ferry: %s\n",
                argv[3]);
        return EX_ERROR;
    }

    cfg->max_vehicle_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_vehicle_arrival_us < MIN_VEHICLE_ARRIVAL_US ||
        cfg->max_vehicle_arrival_us > MAX_VEHICLE_ARRIVAL_US) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for vehicle_arrival_us: "
                "%s\n",
                argv[4]);
        return EX_ERROR;
    }

    cfg->max_ferry_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_ferry_arrival_us < MIN_FERRY_ARRIVAL_US ||
        cfg->max_ferry_arrival_us > MAX_FERRY_ARRIVAL_US) {
        fprintf(stderr,
                "[ERROR] Invalid or out-of-range value for "
                "min_parcel_arrival_us: %s\n",
                argv[5]);
        return EX_ERROR;
    }

    return EX_SUCCESS;
}

SharedData *init_shared_data(Config cfg) {
    SharedData *shared_data =
        mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (shared_data == MAP_FAILED) {
        fprintf(stderr, "[ERROR] mmap failed\n");
        return NULL;
    }
    // Initialize semaphores
    shared_data->ferry_loaded_sem =
        sem_open("/ferry_loaded_sem", O_CREAT, 0644, 0);
    if (shared_data->ferry_loaded_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /ferry_loaded_sem\n");
        return NULL;
    }
    // Initialize semaphores
    shared_data->ferry_arrived_sem =
        sem_open("/ferry_arrived_sem", O_CREAT, 0644, 0);
    if (shared_data->ferry_arrived_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /ferry_arrived_sem\n");
        return NULL;
    }
    // Initialize semaphores
    shared_data->action_counter_sem =
        sem_open("/action_counter_sem", O_CREAT, 0644, 1);
    if (shared_data->action_counter_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /action_counter_sem\n");
        return NULL;
    }
    shared_data->vehicle_unload_sem =
        sem_open("/vehicle_unload_sem", O_CREAT, 0644, 0);
    if (shared_data->vehicle_unload_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /vehicle_unload_sem\n");
        return NULL;
    }
    shared_data->helper_sem = sem_open("/helper_sem", O_CREAT, 0644, 1);
    if (shared_data->helper_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /vehicle_unload_sem\n");
        return NULL;
    }
    shared_data->port_ready_sem[0] =
        sem_open("/port_ready_sem_0", O_CREAT, 0644, 0);
    if (shared_data->port_ready_sem[0] == SEM_FAILED) {
        fprintf(stderr, "[ERROR] Failed to open semaphore: /ferry_sem\n");
        return NULL;
    }
    shared_data->port_ready_sem[1] =
        sem_open("/port_ready_sem_1", O_CREAT, 0644, 0);
    if (shared_data->port_ready_sem[1] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: //port_ready_sem_1\n");
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

int no_more_vehicles_to_transport(SharedData *shared_data, Config cfg) {
    return false;
}

int rand_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// Print and update action counter in shared_data
void print_action(SharedData *shared_data, const char vehicle_type, int id,
                  const char *action, int port) {
    sem_wait(shared_data->action_counter_sem);
    printf("%d: %c %d: %s, Port: %d\n", shared_data->action_counter++,
           vehicle_type, id, action, port);
    sem_post(shared_data->action_counter_sem);
}

void ferry_process(SharedData *shared_data, Config cfg) {
    int done = 0;

    print_action(shared_data, 'P', 0, "started", shared_data->ferry_port);

    while (!done) {
        usleep(rand_range(0, cfg.max_ferry_arrival_us));
        print_action(shared_data, 'P', 0, "arrived", shared_data->ferry_port);

        printf("Port %d is ready\n", shared_data->ferry_port);

        // Notify vehicles that ferry has arrived at destination
        for (int i = 0;
             i < shared_data->loaded_cars; i++) {
            sem_post(shared_data->ferry_arrived_sem);
        }

        // Allow vehicles to unload
        for (int i = 0;
             i < shared_data->loaded_cars + shared_data->loaded_trucks; i++) {
            sem_post(shared_data->vehicle_unload_sem);
        }

        shared_data->loaded_cars = 0;
        shared_data->loaded_trucks = 0;

        printf("Port %d waiting to be loaded\n", shared_data->ferry_port);

        // Allow vehicles to board
        for (int i = 0; i < 1; i++) {
            sem_post(shared_data->port_ready_sem[shared_data->ferry_port]);
        }

        // Wait for a vehicle to board
        sem_wait(shared_data->ferry_loaded_sem);

        print_action(shared_data, 'P', 0, "leaving", shared_data->ferry_port);

        shared_data->ferry_port = (shared_data->ferry_port + 1) % 2;

        if (no_more_vehicles_to_transport(shared_data, cfg)) {
            print_action(shared_data, 'P', 0, "finish",
                         shared_data->ferry_port);
            done = 1;
        }

        printf("Another round\n");
    }
}

void vehicle_process(SharedData *shared_data, int vehicle_id, int port,
                     Config cfg, const char vehicle_type) {
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));  // Simulate arrival time
    print_action(shared_data, vehicle_type, vehicle_id, "arrived", port);

    // Mark as waiting
    if (vehicle_type == 'O') {
        shared_data->waiting_cars[port]++;
    } else {
        shared_data->waiting_trucks[port]++;
    }

    // Wait for ferry to be ready at this port
    sem_wait(shared_data->port_ready_sem[port]);

    sem_wait(shared_data->helper_sem);
    // Boarding
    print_action(shared_data, vehicle_type, vehicle_id, "boarding", port);

    // Update shared info
    if (vehicle_type == 'O') {
        shared_data->waiting_cars[port]--;
        shared_data->loaded_cars++;
    } else {
        shared_data->waiting_trucks[port]--;
        shared_data->loaded_trucks++;
    }
    sem_post(shared_data->helper_sem);

    // Inform ferry we boarded
    sem_post(shared_data->ferry_loaded_sem);

    // 🚨 Wait for the ferry to arrive at the other port
    sem_wait(shared_data->ferry_arrived_sem);

    // 🚪 Wait for permission to unload
    sem_wait(shared_data->vehicle_unload_sem);

    int other_port = (port + 1) % 2;
    print_action(shared_data, vehicle_type, vehicle_id, "leaving", other_port);
}

void create_ferry_process(SharedData *shared_data, Config cfg) {
    pid_t ferry_pid = fork();
    if (ferry_pid == 0) {
        // Ferry process
        ferry_process(shared_data, cfg);
        exit(EX_SUCCESS);
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
            print_action(shared_data, vehicle_type, id, "started", port);
            vehicle_process(shared_data, id, port, cfg,
                            vehicle_type);  // Process vehicle
            exit(EX_SUCCESS);
        } else if (vehicle_pid < 0) {
            fprintf(stderr, "[ERROR] fork failed\n");
            exit(EX_ERROR);
        }
    }
}

int cleanup(SharedData *shared_data) {
    // Close and unlink semaphores
    if (sem_close(shared_data->ferry_loaded_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /ferry_loaded_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/ferry_loaded_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /ferry_loaded_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->helper_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /helper_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/helper_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /helper_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->vehicle_unload_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /vehicle_unload_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/vehicle_unload_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /vehicle_unload_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->action_counter_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /action_counter_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/action_counter_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /action_counter_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->port_ready_sem[0]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /port_ready_sem_0\n");
        return EX_ERROR;
    }
    if (sem_unlink("/port_ready_sem_0") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /port_ready_sem_0\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->port_ready_sem[1]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /port_ready_sem_1\n");
        return EX_ERROR;
    }
    if (sem_unlink("/port_ready_sem_1") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /port_ready_sem_1\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->ferry_arrived_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /ferry_arrived_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/ferry_arrived_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /ferry_arrived_sem\n");
        return EX_ERROR;
    }
    // Clean up shared memory
    munmap(shared_data,
           sizeof(SharedData));  // Unmap the shared memory when done
    return EX_SUCCESS;
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
    if (parse_args(argc, argv, &cfg) != EX_SUCCESS) {
        return EX_ERROR;
    }

    SharedData *shared_data = init_shared_data(cfg);
    if (shared_data == NULL) {
        return EX_ERROR;
    }
    srand(time(NULL));  // Initialize random number generator for random port
                        // assignment

    // 1. Create ferry process
    create_ferry_process(shared_data, cfg);

    // 2. Create personal car processes
    create_vehicle_process(shared_data, cfg, 'O');

    // 3. Create truck processes
    // create_vehicle_process(shared_data, cfg, 'N');

    // 4. Wait for all processes to finish
    wait_for_children();

    // 5. Print shared data
    print_shared_data(shared_data);
    // 6. Clean up
    if (cleanup(shared_data) != EX_SUCCESS) {
        return EX_ERROR;
    }

    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EX_SUCCESS;
}

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
    shared_data->action_counter_sem =
        sem_open("/action_counter_sem", O_CREAT, 0644, 1);
    if (shared_data->action_counter_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /action_counter_sem\n");
        return NULL;
    }

    shared_data->waiting_cars_sem =
        sem_open("/waiting_cars_sem", O_CREAT, 0644, 1);
    if (shared_data->waiting_cars_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /waiting_cars_sem\n");
        return NULL;
    }

    // Initialize semaphores
    shared_data->vehicle_loaded_sem =
        sem_open("/vehicle_loaded_sem", O_CREAT, 0644, 0);
    if (shared_data->vehicle_loaded_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /vehicle_loaded_sem\n");
        return NULL;
    }
    // Initialize semaphores
    shared_data->vehicle_unloaded_sem =
        sem_open("/vehicle_unloaded_sem", O_CREAT, 0644, 0);
    if (shared_data->vehicle_unloaded_sem == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /vehicle_unloaded_sem\n");
        return NULL;
    }
    shared_data->unload_vehicle_sem[0] =
        sem_open("/unload_vehicle_sem_0", O_CREAT, 0644, 0);
    if (shared_data->unload_vehicle_sem[0] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /unload_vehicle_sem_0\n");
        return NULL;
    }

    shared_data->unload_vehicle_sem[1] =
        sem_open("/unload_vehicle_sem_1", O_CREAT, 0644, 0);
    if (shared_data->unload_vehicle_sem[1] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /unload_vehicle_sem_1\n");
        return NULL;
    }
    shared_data->unload_complete[0] =
        sem_open("/unload_complete_0", O_CREAT, 0644, 0);
    if (shared_data->unload_complete[0] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /unload_complete_0\n");
        return NULL;
    }
    shared_data->unload_complete[1] =
        sem_open("/unload_complete_1", O_CREAT, 0644, 0);
    if (shared_data->unload_complete[1] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /unload_complete_1\n");
        return NULL;
    }
    shared_data->port_ready_sem[0] =
        sem_open("/port_ready_sem_0", O_CREAT, 0644, 0);
    if (shared_data->port_ready_sem[0] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /port_ready_sem_0\n");
        return NULL;
    }
    shared_data->port_ready_sem[1] =
        sem_open("/port_ready_sem_1", O_CREAT, 0644, 0);
    if (shared_data->port_ready_sem[1] == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /port_ready_sem_1\n");
        return NULL;
    }
    shared_data->unloaded_from_ferry =
        sem_open("/unloaded_from_ferry", O_CREAT, 0644, 1);
    if (shared_data->unloaded_from_ferry == SEM_FAILED) {
        fprintf(stderr,
                "[ERROR] Failed to open semaphore: /unloaded_from_ferry\n");
        return NULL;
    }
    shared_data->load_car = sem_open("/load_car", O_CREAT, 0644, 1);
    if (shared_data->load_car == SEM_FAILED) {
        fprintf(stderr, "[ERROR] Failed to open semaphore: /load_car\n");
        return NULL;
    }
    shared_data->load_truck = sem_open("/load_truck", O_CREAT, 0644, 1);
    if (shared_data->load_truck == SEM_FAILED) {
        fprintf(stderr, "[ERROR] Failed to open semaphore: /load_truck\n");
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
    return 0;
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

int remaining_cars_to_load() {
    return 0;
}


int unload_all_vehicles(SharedData *shared_data) {
    int unloaded_vehicles = 0;
    for (int i = 0; i < shared_data->loaded_cars + shared_data->loaded_trucks; i++) {

        sem_post(shared_data->unload_vehicle_sem[shared_data->ferry_port]);

        sem_wait(shared_data->vehicle_unloaded_sem);
        unloaded_vehicles++;

    }
    return unloaded_vehicles;
}

void ferry_to_another_port(SharedData *shared_data){
    print_action(shared_data, 'P', 0, "leaving", shared_data->ferry_port);
    shared_data->ferry_port = (shared_data->ferry_port + 1) % 2;

}

// void load_ferry(Config cfg, SharedData *shared_data) {
//     int space_used = 0;
//     int load_truck_next = 1; // Start with truck

//     while (space_used < cfg.capacity_of_ferry) {
//         if (load_truck_next) {
//             if (space_used + 3 <= cfg.capacity_of_ferry) {
//                 sem_post(shared_data->load_truck);
//                 space_used += 3;
//             } else if (space_used + 1 <= cfg.capacity_of_ferry) {
//                 sem_post(shared_data->load_car);
//                 space_used += 1;
//             } else {
//                 break;
//             }
//         } else {
//             if (space_used + 1 <= cfg.capacity_of_ferry) {
//                 sem_post(shared_data->load_car);
//                 space_used += 1;
//             } else if (space_used + 3 <= cfg.capacity_of_ferry) {
//                 sem_post(shared_data->load_truck);
//                 space_used += 3;
//             } else {
//                 break;
//             }
//         }
//         load_truck_next = !load_truck_next; // Alternate
//     }
// }


int load_ferry(Config cfg, SharedData *shared_data) {
    int space_used = 0;
    int vehicle_loaded = 0;
    int waiting_cars = shared_data->waiting_cars[shared_data->ferry_port];

    while (space_used < cfg.capacity_of_ferry && waiting_cars != 0) {
        if (space_used + 1 <= cfg.capacity_of_ferry) {
            sem_post(shared_data->load_car);
            space_used += 1;
            waiting_cars--;
            vehicle_loaded++;
        } else {
            break;
        }
    }
    return vehicle_loaded;
}

void ferry_process(SharedData *shared_data, Config cfg) {
    int done = 0;
    int unloaded_vehicles = 0;

    print_action(shared_data, 'P', 0, "started", shared_data->ferry_port);
    while (!done) {
        usleep(rand_range(0, cfg.max_ferry_arrival_us));
        print_action(shared_data, 'P', 0, "arrived", shared_data->ferry_port);

        unloaded_vehicles += unload_all_vehicles(shared_data);

        if (shared_data->waiting_cars[shared_data->ferry_port] == 0 &&
            shared_data->waiting_trucks[shared_data->ferry_port] == 0) {
            //! CHANGE TO NUM_CARS + NUM_TRUCKS
            if (unloaded_vehicles == cfg.num_cars) {
                print_action(shared_data, 'P', 0, "finish", shared_data->ferry_port);
                break;
            }
            ferry_to_another_port(shared_data);
        } else {

            sem_post(shared_data->unload_complete[shared_data->ferry_port]);

            sem_post(shared_data->port_ready_sem[shared_data->ferry_port]);

            int vehicle_loaded = load_ferry(cfg, shared_data);
            printf("%d loaded\n", vehicle_loaded);
            printf("%d were waiting\n", shared_data->waiting_cars[shared_data->ferry_port]);
            for (int i = 0; i < vehicle_loaded; ++i) {
                printf("waiting for {%d}\n", i);
                sem_wait(shared_data->vehicle_loaded_sem);
                printf("Done {%d}\n", i);
            }
            ferry_to_another_port(shared_data);

        }
    }
}

void add_to_queue(SharedData *shared_data, const char vehicle_type, int port){
    sem_wait(shared_data->waiting_cars_sem);
    if (vehicle_type == 'O') {
        shared_data->waiting_cars[port]++;
    } else {
        shared_data->waiting_trucks[port]++;
    }
    sem_post(shared_data->waiting_cars_sem);

}

void load_vehicle_on_ferry(SharedData *shared_data, const char vehicle_type, int port) {
    if (vehicle_type == 'O') {
        sem_wait(shared_data->load_car);
        shared_data->waiting_cars[port]--;
        shared_data->loaded_cars++;
        sem_post(shared_data->load_car);
    }
    else {
        sem_wait(shared_data->load_truck);
        shared_data->waiting_trucks[port]--;
        shared_data->loaded_trucks++;
        sem_post(shared_data->load_truck);
    }
}

void unload_vehicle_from_ferry(SharedData *shared_data, const char vehicle_type, int port) {
    sem_wait(shared_data->unloaded_from_ferry);
    if (vehicle_type == 'O') {
        shared_data->loaded_cars--;
    } else {
        shared_data->loaded_trucks--;
    }
    sem_post(shared_data->unloaded_from_ferry);
}

void vehicle_process(SharedData *shared_data, int vehicle_id, int port,
                     Config cfg, const char vehicle_type) {
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));  // Simulate arrival time
    print_action(shared_data, vehicle_type, vehicle_id, "arrived", port);
    
    // Add to vehicle to queue
    add_to_queue(shared_data, vehicle_type, port);
    // Wait for unload
    sem_wait(shared_data->unload_complete[port]);
    // Wait for port to be ready
    sem_wait(shared_data->port_ready_sem[port]);
    // Load vehicle

    if (vehicle_type == 'O') {
        sem_wait(shared_data->load_car);
    } else {
        sem_wait(shared_data->load_truck);
    }

    load_vehicle_on_ferry(shared_data, vehicle_type, port);

    print_action(shared_data, vehicle_type, vehicle_id, "loaded", port);
    sem_post(shared_data->vehicle_loaded_sem);
    
    // Wait for unload
    sem_wait(shared_data->unload_vehicle_sem[(port + 1) % 2]);

    // Unload vehicle
    unload_vehicle_from_ferry(shared_data, vehicle_type, port);

    port = (port + 1) % 2;
    print_action(shared_data, vehicle_type, vehicle_id, "unloaded", port);
    sem_post(shared_data->vehicle_unloaded_sem);
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
            int port = rand() % 2;
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
    if (sem_close(shared_data->waiting_cars_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /waiting_cars_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/waiting_cars_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /waiting_cars_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->vehicle_loaded_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /vehicle_loaded_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/vehicle_loaded_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /vehicle_loaded_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->vehicle_unloaded_sem) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /vehicle_unloaded_sem\n");
        return EX_ERROR;
    }
    if (sem_unlink("/vehicle_unloaded_sem") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /vehicle_unloaded_sem\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->unload_complete[0]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /unload_complete_0\n");
        return EX_ERROR;
    }
    if (sem_unlink("/unload_complete_0") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /unload_complete_0\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->unload_complete[1]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /unload_complete_1\n");
        return EX_ERROR;
    }
    if (sem_unlink("/unload_complete_1") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /unload_complete_1\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->unload_vehicle_sem[0]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /unload_vehicle_sem_0\n");
        return EX_ERROR;
    }
    if (sem_unlink("/unload_vehicle_sem_0") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /unload_vehicle_sem_0\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->unload_vehicle_sem[1]) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /unload_vehicle_sem_1\n");
        return EX_ERROR;
    }
    if (sem_unlink("/unload_vehicle_sem_1") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /unload_vehicle_sem_1\n");
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
    if (sem_close(shared_data->unloaded_from_ferry) == -1) {
        fprintf(stderr,
                "[ERROR] Failed to close semaphore: /unloaded_from_ferry\n");
        return EX_ERROR;
    }
    if (sem_unlink("/unloaded_from_ferry") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /unloaded_from_ferry\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->load_car) == -1) {
        fprintf(stderr, "[ERROR] Failed to close semaphore: /load_car\n");
        return EX_ERROR;
    }
    if (sem_unlink("/load_car") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /load_car\n");
        return EX_ERROR;
    }
    if (sem_close(shared_data->load_truck) == -1) {
        fprintf(stderr, "[ERROR] Failed to close semaphore: /load_truck\n");
        return EX_ERROR;
    }
    if (sem_unlink("/load_truck") == -1) {
        fprintf(stderr,
                "[ERROR] Failed to unlink semaphore: /load_truck\n");
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

    // 5. Clean up
    if (cleanup(shared_data) != EX_SUCCESS) {
        return EX_ERROR;
    }

    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EX_SUCCESS;
}

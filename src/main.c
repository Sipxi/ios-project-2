// Author: Serhij Čepil (sipxi)
// FIT VUT Student
// https://github.com/sipxi
// Date: 18/05/2025
// Time spent: 15h
#include "main.h"

//TODO: Exit code should be only 0 or 1

// Function to parse arguments
int parse_args(int argc, char const *argv[], Config *cfg) {
    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "[ERROR:%d] Expected %d arguments, got %d\n",
                EX_ERROR_ARGS, EXPECTED_ARGS, argc);
        return EX_ERROR_ARGS;
    }

    char *end;

    cfg->num_trucks = strtol(argv[1], &end, 10);
    if (*end != '\0' || cfg->num_trucks < 0 || cfg->num_trucks > MAX_NUM_TRUCKS) {
        fprintf(stderr, "[ERROR:%d] Invalid number for num_trucks: %s\n", EX_ERROR_ARGS,argv[1]);
        return EX_ERROR_ARGS;
    }

    cfg->num_cars = strtol(argv[2], &end, 10);
    if (*end != '\0' || cfg->num_cars < 0 || cfg->num_cars > MAX_NUM_CARS) {
        fprintf(stderr, "[ERROR:%d] Invalid number for num_cars: %s\n",EX_ERROR_ARGS, argv[2]);
        return EX_ERROR_ARGS;
    }

    cfg->capacity_of_ferry = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity_of_ferry < MIN_CAPACITY_PARCEL || cfg->capacity_of_ferry > MAX_CAPACITY_PARCEL) {
        fprintf(stderr, "[ERROR:%d] Invalid or out-of-range value for parcel capacity_of_ferry: %s\n",EX_ERROR_ARGS, argv[3]);
        return EX_ERROR_ARGS;
    }

    cfg->max_vehicle_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_vehicle_arrival_us < MIN_VEHICLE_ARRIVAL_US 
        || cfg->max_vehicle_arrival_us > MAX_VEHICLE_ARRIVAL_US) {
        fprintf(stderr, "[ERROR:%d] Invalid or out-of-range value for vehicle_arrival_us: %s\n",EX_ERROR_ARGS, argv[4]);
        return EX_ERROR_ARGS;
    }

    cfg->max_ferry_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_ferry_arrival_us < MIN_FERRY_ARRIVAL_US 
        || cfg->max_ferry_arrival_us > MAX_FERRY_ARRIVAL_US) {
        fprintf(stderr, "[ERROR:%d] Invalid or out-of-range value for min_parcel_arrival_us: %s\n",EX_ERROR_ARGS, argv[5]);
        return EX_ERROR_ARGS;
    }

    return EX_SUCCESS;
}

SharedData *init_shared_data(Config cfg) {
    SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (shared_data == MAP_FAILED) {
        fprintf(stderr, "[ERROR:%d] mmap failed\n", EX_ERROR_INIT);
        return NULL;
    }
        // Initialize semaphores
    shared_data->action_counter_sem = sem_open("/action_counter_sem", O_CREAT, 0644, 1);
    if (shared_data->action_counter_sem == SEM_FAILED) {
        fprintf(stderr, "[ERROR:%d] sem_open failed for action_counter_sem\n", EX_ERROR_INIT);
        return NULL;
    }
    shared_data->ferry_sem = sem_open("/ferry_sem", O_CREAT, 0644, 1);
    if (shared_data->ferry_sem == SEM_FAILED) {
        fprintf(stderr, "[ERROR:%d] sem_open failed for ferry_sem\n", EX_ERROR_INIT);
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

int rand_range(int min, int max){
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
 }

// Print and update action counter in shared_data
void print_action(SharedData *shared_data, const char vehicle_type, int id, const char * action, int port) {
    printf("%d: %c %d: %s, Port: %d\n", shared_data->action_counter++, vehicle_type, id, action, port);
}

void ferry_process(SharedData *shared_data, Config cfg){
    print_action(shared_data, 'P', 0, "started", shared_data->ferry_port);
    usleep(rand_range(0, cfg.max_ferry_arrival_us));
    print_action(shared_data, 'P', 0, "arrived", shared_data->ferry_port);

}

void vehicle_process(SharedData *shared_data, int vehicle_id, int port, Config cfg, const char vehicle_type) {
    usleep(rand_range(0, cfg.max_vehicle_arrival_us));  // Simulate arrival time
    print_action(shared_data, vehicle_type, vehicle_id, "arrived", port);  // Log arrival
    vehicle_type == 'O' ? shared_data->waiting_cars[port]++ : shared_data->waiting_trucks[port]++;

}

void create_ferry_process(SharedData *shared_data, Config cfg){
    pid_t ferry_pid = fork();
    if (ferry_pid == 0){
        // Ferry process
        ferry_process(shared_data, cfg);
        exit(0);
    }
    else if (ferry_pid < 0){
        fprintf(stderr, "[ERROR] fork failed\n");
        exit(1);
    }
    
}

void create_vehicle_process(SharedData *shared_data, Config cfg, const char vehicle_type) {
    int num_vehicles = vehicle_type == 'O' ? cfg.num_cars : cfg.num_trucks;
    for (int i = 0; i < num_vehicles; i++) {
        pid_t vehicle_pid = fork();
        if (vehicle_pid == 0) {
            srand(getpid()); // Seed the random number generator using the process ID
            int port = rand() % 2;
            print_action(shared_data, vehicle_type, i + 1, "started", port);
            vehicle_process(shared_data, i + 1, port, cfg, vehicle_type);  // Process vehicle
            exit(0);
        } else if (vehicle_pid < 0) {
            fprintf(stderr, "[ERROR] fork failed\n");
            exit(1);
        }
    }
}

int cleanup(SharedData *shared_data) {
    // Clean up shared memory/semaphores
    munmap(shared_data, sizeof(SharedData)); // Unmap the shared memory when done
    // Close and unlink semaphores
    if (sem_close(shared_data->action_counter_sem) == -1) {
        fprintf(stderr, "[ERROR] sem_close failed for action_counter_sem\n");
        return 1;
    }

    if (sem_unlink("/action_counter_sem") == -1) {
        fprintf(stderr, "[ERROR] sem_unlink failed for action_counter_sem\n");
        return 1;
    }
    if (sem_close(shared_data->ferry_sem) == -1) {
        fprintf(stderr, "[ERROR] sem_close failed for ferry_sem\n");
    }

    if (sem_unlink("/ferry_sem") == -1) {
        fprintf(stderr, "[ERROR] sem_unlink failed for ferry_sem\n");
    }
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
    printf("loaded_trucks: %d\n}", shared_data->loaded_trucks);
}

// Main function
int main(int argc, char const *argv[]) {
    Config cfg;
    if (parse_args(argc, argv, &cfg) != EX_SUCCESS) {
        return EX_ERROR_ARGS;
    }

    SharedData *shared_data = init_shared_data(cfg);
    if (shared_data == NULL) {
        return EX_ERROR_INIT;
    }
    srand(time(NULL));  // Initialize random number generator for random port assignment

    // 1. Create ferry process
    create_ferry_process(shared_data, cfg);

    // 2. Create personal car processes
    create_vehicle_process(shared_data, cfg, 'O');

    // 3. Create truck processes
    create_vehicle_process(shared_data, cfg, 'N');

    // 4. Wait for all processes to finish
    wait_for_children();

    // 5. Print shared data
    print_shared_data(shared_data);

    // 6. Clean up
    if (cleanup != EX_SUCCESS){
        return 0;
    }

    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EX_SUCCESS;
}

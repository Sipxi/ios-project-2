#include "main.h"

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
        fprintf(stderr, "[ERROR] Invalid number for num_trucks: %s\n", argv[1]);
        return EX_ERROR_ARGS;
    }

    cfg->num_cars = strtol(argv[2], &end, 10);
    if (*end != '\0' || cfg->num_cars < 0 || cfg->num_cars > MAX_NUM_CARS) {
        fprintf(stderr, "[ERROR] Invalid number for num_cars: %s\n", argv[2]);
        return EX_ERROR_ARGS;
    }

    cfg->capacity = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity < MIN_CAPACITY_TRAFFIC || cfg->capacity > MAX_CAPACITY_TRAFFIC) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for capacity: %s\n", argv[3]);
        return EX_ERROR_ARGS;
    }

    cfg->max_car_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_car_arrival_us < MIN_CAR_ARRIVAL_US || cfg->max_car_arrival_us > MAX_CAR_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for max_car_arrival_us: %s\n", argv[4]);
        return EX_ERROR_ARGS;
    }

    cfg->max_truck_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_truck_arrival_us < MIN_TRUCK_ARRIVAL_US || cfg->max_truck_arrival_us > MAX_TRUCK_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for max_truck_arrival_us: %s\n", argv[5]);
        return EX_ERROR_ARGS;
    }

    return EX_SUCCESS;
}

// Initialize shared memory
void init_shared_memory(SharedData **shared_data) {
    int shm_fd = shm_open("/ferry_shared", O_CREAT | O_RDWR, 0666);
    assert(shm_fd != -1);

    ftruncate(shm_fd, sizeof(SharedData));
    *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    assert(*shared_data != MAP_FAILED);

    // Initialize shared data
    (*shared_data)->action_counter = 1;
    (*shared_data)->ferry_port = 0;
    (*shared_data)->ferry_capacity = 0;
    (*shared_data)->waiting_cars[0] = 0;
    (*shared_data)->waiting_cars[1] = 0;
    (*shared_data)->waiting_trucks[0] = 0;
    (*shared_data)->waiting_trucks[1] = 0;
}

// Initialize semaphores
sem_t *init_semaphore(const char *name) {
    sem_t *sem = sem_open(name, O_CREAT, 0666, 1);
    assert(sem != SEM_FAILED);
    return sem;
}

// Cleanup semaphores
void cleanup_semaphores() {
    sem_unlink(SEM_FERRY);
    sem_unlink(SEM_CARS);
    sem_unlink(SEM_TRUCKS);
    sem_unlink(SEM_OUTPUT);
}

// Create ferry process
pid_t create_ferry_process(SharedData *shared_data, Config *cfg) {
    pid_t pid = fork();
    if (pid == 0) {
        ferry_process(shared_data, cfg);
        exit(0);
    }
    return pid;
}

// Create vehicle processes
void create_vehicle_processes(SharedData *shared_data, Config *cfg) {
    for (int i = 0; i < cfg->num_trucks; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            truck_process(i + 1, shared_data, cfg);
            exit(0);
        }
    }

    for (int i = 0; i < cfg->num_cars; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            car_process(i + 1, shared_data, cfg);
            exit(0);
        }
    }
}

// Ferry process logic
void ferry_process(SharedData *shared_data, Config *cfg) {
    FILE *output = fopen("proj2.out", "w");
    assert(output != NULL);

    while (1) {
        sem_wait(init_semaphore(SEM_FERRY));

        // Arrive at the current port
        fprintf(output, "%d: P: arrived to %d\n", shared_data->action_counter++, shared_data->ferry_port);

        // Unload all vehicles
        // TODO: Implement unloading logic

        // Load waiting vehicles
        // TODO: Implement loading logic

        // Leave the current port
        fprintf(output, "%d: P: leaving %d\n", shared_data->action_counter++, shared_data->ferry_port);

        sem_post(init_semaphore(SEM_FERRY));

        // Travel to the next port
        shared_data->ferry_port = (shared_data->ferry_port + 1) % 2;
        usleep(rand() % (cfg->max_truck_arrival_us + 1));

        // Check termination condition
        // TODO: Implement termination logic
    }

    // Finish the ferry process
    fprintf(output, "%d: P: finish\n", shared_data->action_counter++);
    fclose(output);
}

// Truck process logic
void truck_process(int id, SharedData *shared_data, Config *cfg) {
    FILE *output = fopen("proj2.out", "a");
    assert(output != NULL);

    int start_port = rand() % 2;
    fprintf(output, "%d: N %d: started\n", shared_data->action_counter++, id);

    // Travel to the starting port
    usleep(rand() % (cfg->max_truck_arrival_us + 1));
    fprintf(output, "%d: N %d: arrived to %d\n", shared_data->action_counter++, id, start_port);

    // Wait for the ferry
    sem_wait(init_semaphore(SEM_TRUCKS));
    shared_data->waiting_trucks[start_port]++;
    sem_post(init_semaphore(SEM_TRUCKS));

    // Board the ferry
    sem_wait(init_semaphore(SEM_FERRY));
    shared_data->waiting_trucks[start_port]--;
    shared_data->ferry_capacity--;
    fprintf(output, "%d: N %d: boarding\n", shared_data->action_counter++, id);
    sem_post(init_semaphore(SEM_FERRY));

    // Travel to the destination port
    int dest_port = (start_port + 1) % 2;

    // Disembark
    sem_wait(init_semaphore(SEM_FERRY));
    fprintf(output, "%d: N %d: leaving in %d\n", shared_data->action_counter++, id, dest_port);
    sem_post(init_semaphore(SEM_FERRY));

    fclose(output);
}

// Car process logic
void car_process(int id, SharedData *shared_data, Config *cfg) {
    FILE *output = fopen("proj2.out", "a");
    assert(output != NULL);

    int start_port = rand() % 2;
    fprintf(output, "%d: O %d: started\n", shared_data->action_counter++, id);

    // Travel to the starting port
    usleep(rand() % (cfg->max_car_arrival_us + 1));
    fprintf(output, "%d: O %d: arrived to %d\n", shared_data->action_counter++, id, start_port);

    // Wait for the ferry
    sem_wait(init_semaphore(SEM_CARS));
    shared_data->waiting_cars[start_port]++;
    sem_post(init_semaphore(SEM_CARS));

    // Board the ferry
    sem_wait(init_semaphore(SEM_FERRY));
    shared_data->waiting_cars[start_port]--;
    shared_data->ferry_capacity--;
    fprintf(output, "%d: O %d: boarding\n", shared_data->action_counter++, id);
    sem_post(init_semaphore(SEM_FERRY));

    // Travel to the destination port
    int dest_port = (start_port + 1) % 2;

    // Disembark
    sem_wait(init_semaphore(SEM_FERRY));
    fprintf(output, "%d: O %d: leaving in %d\n", shared_data->action_counter++, id, dest_port);
    sem_post(init_semaphore(SEM_FERRY));

    fclose(output);
}

// Cleanup resources
void cleanup_resources(SharedData *shared_data) {
    munmap(shared_data, sizeof(SharedData));
    shm_unlink("/ferry_shared");
    cleanup_semaphores();
}

// Main function
int main(int argc, char const *argv[]) {
    srand(time(NULL)); // Seed random number generator

    Config cfg;
    if (parse_args(argc, argv, &cfg) != EX_SUCCESS) {
        return EX_ERROR_ARGS;
    }

    SharedData *shared_data;
    init_shared_memory(&shared_data);
    shared_data->ferry_capacity = cfg.capacity;

    sem_t *sem_ferry = init_semaphore(SEM_FERRY);
    sem_t *sem_cars = init_semaphore(SEM_CARS);
    sem_t *sem_trucks = init_semaphore(SEM_TRUCKS);
    sem_t *sem_output = init_semaphore(SEM_OUTPUT);

    pid_t ferry_pid = create_ferry_process(shared_data, &cfg);
    create_vehicle_processes(shared_data, &cfg);

    waitpid(ferry_pid, NULL, 0);

    cleanup_resources(shared_data);
    return EX_SUCCESS;
}
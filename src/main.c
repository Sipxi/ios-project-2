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




// Print and update action counter in shared_data
void print_action(SharedData *shared_data, const char *vehicle_type, int id, int port) {
    printf("%d: %s %d: started, Port: %d\n", shared_data->action_counter++, vehicle_type, id, port);
}

void ferry_process(SharedData *shared_data){
    print_action(shared_data, "P", 0, shared_data->ferry_port);
    // Simulate ferry process (loading, unloading, etc.)
}

void car_process(SharedData *shared_data, int car_id, int port){
    print_action(shared_data, "O", car_id, port);
    // Simulate car arriving at the port and boarding the ferry
}

void truck_process(SharedData *shared_data, int truck_id, int port){
    print_action(shared_data, "N", truck_id, port);
    // Simulate truck arriving at the port and boarding the ferry
}

void create_ferry_process(SharedData *shared_data, Config cfg){
    pid_t ferry_pid = fork();
    if (ferry_pid == 0){
        // Ferry process
        ferry_process(shared_data);
        exit(0);
    }
    else if (ferry_pid < 0){
        fprintf(stderr, "[ERROR] fork failed\n");
        exit(1);
    }
}

void create_car_process(SharedData *shared_data, Config cfg){
    for (int i = 0; i < cfg.num_cars; i++) {
        pid_t car_pid = fork();
        if (car_pid == 0) {
            // Seed the random number generator using the process ID (getpid())
            srand(getpid());
            // Each car gets a random port (0 or 1)
            int port = rand() % 2; 
            car_process(shared_data, i + 1, port);  // car_id = i + 1
            shared_data->waiting_cars[port]++;
            exit(0);
        } else if (car_pid < 0) {
            fprintf(stderr, "[ERROR] fork failed\n");
            exit(1);
        }
    }
}

void create_truck_process(SharedData *shared_data, Config cfg){
    for (int i = 0; i < cfg.num_trucks; i++) {
        pid_t truck_pid = fork();
        if (truck_pid == 0) {
            // Seed the random number generator using the process ID (getpid())
            srand(getpid());
            // Each truck gets a random port (0 or 1)
            int port = rand() % 2; 
            truck_process(shared_data, i + 1, port);  // truck_id = i + 1
            shared_data->waiting_trucks[port]++;
            exit(0);
        } else if (truck_pid < 0) {
            fprintf(stderr, "[ERROR] fork failed\n");
            exit(1);
        }
    }
}



void wait_for_children() {
    while (wait(NULL) > 0);  // Wait for all child processes to finish
}

void print_shared_data(SharedData *shared_data) {
    printf("\nShared data: {\n");
    printf("action_counter: %d\n", shared_data->action_counter);
    printf("ferry_port: %d\n", shared_data->ferry_port);
    printf("ferry_capacity: %d\n", shared_data->ferry_capacity);
    printf("waiting_trucks[0]: %d\n", shared_data->waiting_trucks[0]);
    printf("waiting_trucks[1]: %d\n", shared_data->waiting_trucks[1]);
    printf("waiting_cars[0]: %d\n", shared_data->waiting_cars[0]);
    printf("waiting_cars[1]: %d\n", shared_data->waiting_cars[1]);
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
    create_car_process(shared_data, cfg);

    // 3. Create truck processes
    create_truck_process(shared_data, cfg);

    // 4. Wait for all processes to finish
    wait_for_children();

    // 5. Print shared data
    print_shared_data(shared_data);

    // Clean up shared memory/semaphores


    printf("\n \033[32mEverything done successfully.\033[0m\n");
    return EX_SUCCESS;
}

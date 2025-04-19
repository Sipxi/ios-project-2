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

    cfg->capacity_of_parcel = strtol(argv[3], &end, 10);
    if (*end != '\0' || cfg->capacity_of_parcel < MIN_CAPACITY_PARCEL || cfg->capacity_of_parcel > MAX_CAPACITY_PARCEL) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for parcel capacity_of_parcel: %s\n", argv[3]);
        return EX_ERROR_ARGS;
    }

    cfg->max_vehicle_arrival_us = strtol(argv[4], &end, 10);
    if (*end != '\0' || cfg->max_vehicle_arrival_us < MIN_VEHICLE_ARRIVAL_US 
        || cfg->max_vehicle_arrival_us > MAX_VEHICLE_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for vehicle_arrival_us: %s\n", argv[4]);
        return EX_ERROR_ARGS;
    }

    cfg->max_parcel_arrival_us = strtol(argv[5], &end, 10);
    if (*end != '\0' || cfg->max_parcel_arrival_us < MIN_PARCEL_ARRIVAL_US 
        || cfg->max_parcel_arrival_us > MAX_PARCEL_ARRIVAL_US) {
        fprintf(stderr, "[ERROR] Invalid or out-of-range value for min_parcel_arrival_us: %s\n", argv[5]);
        return EX_ERROR_ARGS;
    }

    return EX_SUCCESS;
}


//Dummy child logic
void start_ferry() {
    printf("Ferry: started\n");
    usleep(100000); // simulate some work
    printf("Ferry: finished\n");
}

void start_car(int id) {
    srand(getpid() ^ time(NULL));  // Add this
    int port = rand() % 2;
    printf("Car %d: started, port %d\n", id, port);
    usleep(100000);
    printf("Car %d: done\n", id);
}

void start_truck(int id) {
    srand(getpid() ^ time(NULL));  // Add this
    int port = rand() % 2;
    printf("Truck %d: started, port %d\n", id, port);
    usleep(100000);
    printf("Truck %d: done\n", id);
}




// Main function
int main(int argc, char const *argv[]) {
    Config cfg;
    if (parse_args(argc, argv, &cfg) != EX_SUCCESS) {
        return EX_ERROR_ARGS;
    }

    srand(time(NULL)); // for port randomization

    // Start ferry process
    pid_t ferry_pid = fork();
    if (ferry_pid == 0) {
        start_ferry();
        exit(0);
    }

    // Start N trucks
    for (int i = 1; i <= cfg.num_trucks; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            start_truck(i);
            exit(0);
        }
    }
    for (int i = 1; i <= cfg.num_cars; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            start_car(i);
            exit(0);
        }
    }

    // Wait for all child processes
    while (wait(NULL) > 0);
    return EX_SUCCESS;
}

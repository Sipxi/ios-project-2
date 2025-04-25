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
                sem_wait(&shared_data->vehicle_boarding);
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
                sem_wait(&shared_data->vehicle_boarding);
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
                    sem_wait(&shared_data->vehicle_boarding);
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
                    sem_wait(&shared_data->vehicle_boarding);
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
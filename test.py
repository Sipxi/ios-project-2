def load_ferry(ferry_capacity):
    space_used = 0
    load_truck_next = True  # Start with loading a truck

    while space_used < ferry_capacity:
        if load_truck_next:
            if space_used + 3 <= ferry_capacity:
                print("signal_load_truck")
                space_used += 3
            elif space_used + 1 <= ferry_capacity:
                print("signal_load_car")
                space_used += 1
            else:
                break
        else:
            if space_used + 1 <= ferry_capacity:
                print("signal_load_car")
                space_used += 1
            elif space_used + 3 <= ferry_capacity:
                print("signal_load_truck")
                space_used += 3
            else:
                break
        load_truck_next = not load_truck_next  # Alternate next load

load_ferry(10)
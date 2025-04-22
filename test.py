def load_ferry(ferry_capacity, port):
    ferry = []
    remaining_capacity = ferry_capacity
    expected_type = "N"  # Start with truck

    while remaining_capacity > 0 and port:
        loaded = False

        # Try to find expected vehicle type that fits
        for i in range(len(port)):
            vehicle = port[i]
            vehicle_size = 3 if vehicle == "N" else 1
            if vehicle == expected_type and vehicle_size <= remaining_capacity:
                ferry.append(vehicle)
                remaining_capacity -= vehicle_size
                port.pop(i)
                loaded = True
                break

        # If no expected type was loaded, try alternate type
        if not loaded:
            alt_type = "O" if expected_type == "N" else "N"
            for i in range(len(port)):
                vehicle = port[i]
                vehicle_size = 3 if vehicle == "N" else 1
                if vehicle == alt_type and vehicle_size <= remaining_capacity:
                    ferry.append(vehicle)
                    remaining_capacity -= vehicle_size
                    port.pop(i)
                    loaded = True
                    break

        # If still nothing loaded, exit
        if not loaded:
            break

        # Alternate expected vehicle type
        expected_type = "O" if expected_type == "N" else "N"

    return ferry

# Example usage:
shared_port = []
loaded = load_ferry(10, shared_port)
print("Loaded onto ferry:", loaded)
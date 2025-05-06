import re
from typing import List, Dict, Optional

# Constants
TRUCK_SIZE = 3
CAR_SIZE = 1
TRUCKS = 4
CARS = 4
MAX_CAPACITY = 10

# Event type definition
Event = Dict[str, Optional[any]]


def parse_output(lines: List[str]) -> List[Event]:
    pattern = re.compile(r"(\d+): (.*?)(?: (\d+))?: (.+?)(?: (\d+))?$")
    events = []

    for line in lines:
        match = pattern.match(line.strip())
        if match:
            action_num, vehicle_type, vehicle_id, action, port = match.groups()
            events.append({
                "action_num": int(action_num),
                "type": vehicle_type,
                "id": int(vehicle_id) if vehicle_id else 0,
                "action": action.strip(),
                "port": int(port) if port else None
            })
    return events

def is_ordered(events: List[Event]) -> bool:
    for i in range(1, len(events)):
        if events[i]['action_num'] < events[i - 1]['action_num']:
            print("❌ Actions are not ordered")
            print(f"{events[i]}\n{events[i - 1]}")
            return False
    return True

def is_port_valid(events: List[Event]) -> bool:
    for event in events:
        if event['port'] is not None and event['port'] not in (0, 1):
            print(f"❌ Invalid port number: {event['port']}")
            print(event)
            return False
    return True

def check_ids(events: List[Event], num_trucks: int, num_cars: int) -> bool:
    ids_truck = {event['id'] for event in events if event['type'] == 'N'}
    ids_car = {event['id'] for event in events if event['type'] == 'O'}

    valid_truck_ids = ids_truck == set(range(1, num_trucks + 1))
    valid_car_ids = ids_car == set(range(1, num_cars + 1))

    if not valid_truck_ids:
        print("❌ Truck IDs mismatch:")
        for event in events:
            if event['type'] == 'N' and event['id'] not in range(1, num_trucks + 1):
                print(event)

    if not valid_car_ids:
        print("❌ Car IDs mismatch:")
        for event in events:
            if event['type'] == 'O' and event['id'] not in range(1, num_cars + 1):
                print(event)

    return valid_truck_ids and valid_car_ids

def check_types(events: List[Event]) -> bool:
    valid_types = {'P', 'O', 'N'}
    for event in events:
        if event['type'] not in valid_types:
            print(f"❌ Invalid type: {event['type']}")
            print(event)
            return False
    return True

def check_actions(events: List[Event]) -> bool:
    valid_actions = {
        'N': {'started', 'arrived to', 'boarding', 'leaving in'},
        'O': {'started', 'arrived to', 'boarding', 'leaving in'},
        'P': {'started', 'arrived to', 'leaving', 'finish'}
    }

    for event in events:
        etype = event.get('type')
        action = event.get('action')
        if etype in valid_actions and action not in valid_actions[etype]:
            print(f"❌ Invalid action for type '{etype}': {action}")
            print(event)
            return False
    return True

def validate_process_flow(events, X_N, X_O):
    expected_N_O_steps = {'started', 'arrived to', 'boarding', 'leaving in'}
    expected_P_steps = {'started', 'arrived to', 'leaving', 'finish'}

    # Organize events by type and id
    process_log = {
        'N': {i: set() for i in range(1, X_N + 1)},
        'O': {i: set() for i in range(1, X_O + 1)},
        'P': {0: set()}  # Only one P, id = 0
    }

    # Collect the actions taken by each id
    for event in events:
        etype = event['type']
        eid = event['id']
        action = event['action']

        if etype in process_log and eid in process_log[etype]:
            process_log[etype][eid].add(action)

    all_valid = True

    # Check N and O
    for etype in ('N', 'O'):
        expected_steps = expected_N_O_steps
        for eid, actions in process_log[etype].items():
            if actions != expected_steps:
                print(f"❌ {etype} {eid} is missing steps. Got: {actions}")
                print(f"Expected: {expected_steps}")
                all_valid = False

    # Check P
    p_actions = process_log['P'][0]
    if not expected_P_steps.issubset(p_actions):
        print(f"❌ Process P is missing steps. Got: {p_actions}")
        print(f"Expected: {expected_P_steps}")
        all_valid = False

    return all_valid

def check_boarding_after_leaving(events):
    p_state = 'idle'
    buffer = []
    all_good = True

    for event in events:
        if event['type'] == 'P' and event['action'] == 'arrived to':
            # Start tracking a new ferry trip
            buffer = []
            p_state = 'at_port'

        elif event['type'] == 'P' and event['action'] == 'leaving':
            if p_state == 'at_port':
                # Validate the buffer
                found_boarding = False
                for e in buffer:
                    if e['action'] == 'boarding':
                        found_boarding = True
                    elif e['action'] == 'leaving in':
                        if found_boarding:
                            print("❌ Invalid sequence: 'leaving in' after 'boarding'")
                            print(f"Event: {e}")
                            all_good = False
                buffer = []
                p_state = 'moving'

        elif p_state == 'at_port' and event['type'] in ('N', 'O') and event['action'] in ('boarding', 'leaving in'):
            buffer.append(event)


    return all_good

def check_ferry_capacity(events, car_size, truck_size, max_capacity):
    current_trip = []
    tracking = False
    boarded_ids = set()
    all_good = True

    for event in events:
        if event['type'] == 'P' and event['action'] == 'arrived to':
            tracking = True
            current_trip = []
            boarded_ids = set()

        elif event['type'] == 'P' and event['action'] == 'leaving':
            if tracking:
                # Calculate total load
                load = 0
                for e in current_trip:
                    vehicle_key = (e['type'], e['id'])
                    if vehicle_key in boarded_ids:
                        print(f"❌ Duplicate boarding detected: {e}")
                        all_good = False
                        continue
                    boarded_ids.add(vehicle_key)

                    if e['type'] == 'N':
                        load += truck_size
                    elif e['type'] == 'O':
                        load += car_size

                if load > max_capacity:
                    print(f"❌ Ferry overloaded: {load} > {max_capacity}")
                    print("Trip events:")
                    for e in current_trip:
                        print(e)
                    all_good = False
                tracking = False

        elif tracking and event['action'] == 'boarding' and event['type'] in ['N', 'O']:
            current_trip.append(event)


    return all_good

def check_vehicles_and_finish(events):
    """
    Validates vehicle events to ensure:
    - The ferry finishes its journey.
    - All vehicles switch ports correctly.
    - Ferries ('P') are not required to leave.
    """
    start_ports = {}  # Tracks the first port each vehicle arrives at
    end_ports = {}    # Tracks the last port each vehicle leaves from
    ferry_finished = False  # Tracks if the ferry has completed its journey
    valid = True  # Flag to track overall validity

    for event in events:
        vehicle_key = (event['type'], event['id'])

        # Check for the ferry's finish action
        if event['type'] == 'P' and event['action'] == 'finish':
            ferry_finished = True

        # Record arrival events
        if event['action'] == 'arrived to':
            if vehicle_key not in start_ports:
                start_ports[vehicle_key] = event['port']

        # Record departure events
        elif event['action'] == 'leaving in':
            end_ports[vehicle_key] = event['port']

    # Validate that the ferry finished its journey
    if not ferry_finished:
        print("❌ Ferry did not finish its journey.")
        valid = False

    # Validate vehicle port switching (excluding ferries)
    for vehicle, start_port in start_ports.items():
        vehicle_type, _ = vehicle

        # Skip validation for ferries ('P')
        if vehicle_type == 'P':
            continue

        end_port = end_ports.get(vehicle)
        if end_port is None:
            print(f"❌ {vehicle} never left")
            valid = False
        elif start_port == end_port:
            print(f"❌ {vehicle} left from the same port they arrived to: {start_port}")
            valid = False

    # If all checks passed
    if valid:
        print("✅ All vehicles switched ports correctly, and ferry finished its journey")

    return valid

def check_vehicle_order(events):
    arrived_vehicles = set()
    boarded_vehicles = set()
    valid = True

    for event in events:
        vehicle_key = (event['type'], event['id'])

        if event['action'] == 'arrived to':
            # Mark vehicle as arrived
            arrived_vehicles.add(vehicle_key)

        elif event['action'] == 'boarding':
            # Vehicle should not board before arriving
            if vehicle_key not in arrived_vehicles:
                print(f"❌ Vehicle {vehicle_key} tried to board before arriving!")
                valid = False
            else:
                # Mark vehicle as boarded
                boarded_vehicles.add(vehicle_key)

        elif event['action'] == 'leaving in':
            # Vehicle should not leave before boarding
            if vehicle_key not in boarded_vehicles:
                print(f"❌ Vehicle {vehicle_key} tried to leave before boarding!")
                valid = False
            # Vehicle must have arrived before leaving
            if vehicle_key not in arrived_vehicles:
                print(f"❌ Vehicle {vehicle_key} tried to leave before arriving!")
                valid = False
    return valid


# ---------------- Main Execution ----------------

def main(filename: str = "proj2.out") -> bool:
    with open(filename, "r") as f:
        lines = f.readlines()

    events = parse_output(lines)

    checks = [
        (check_ids, "✅ IDs are correct", "❌ IDs are incorrect", TRUCKS, CARS),
        (check_types, "✅ Types are correct", "❌ Types are incorrect"),
        (check_actions, "✅ Actions are correct", "❌ Actions are incorrect"),
        (is_ordered, "✅ Actions are ordered", "❌ Actions are not ordered"),
        (is_port_valid, "✅ Ports are valid", "❌ Ports are invalid"),
        (validate_process_flow, "✅ Process flow is valid", "❌ Process flow is invalid", TRUCKS, CARS),
        (check_boarding_after_leaving, "✅ Boarding after leaving is valid", "❌ Boarding after leaving is invalid"),
        (check_ferry_capacity, "✅ Ferry capacity is valid", "❌ Ferry capacity is invalid", CAR_SIZE, TRUCK_SIZE, MAX_CAPACITY),
        (check_vehicles_and_finish, "✅ Vehicles switched ports", "❌ Vehicles did not switch ports correctly"),
        (check_vehicle_order, "✅ Vehicles arrived before leaving", "❌ Vehicles did not arrive before leaving correctly")
    ]

    all_tests_passed = True

    for check_func, success_msg, failure_msg, *args in checks:
        if not check_func(events, *args):
            print(failure_msg)
            all_tests_passed = False
        else:
            print(success_msg)

    return all_tests_passed

if __name__ == "__main__":
    main()






import subprocess
import os
from concurrent.futures import ThreadPoolExecutor

def compile_c_program(c_file):
    """Compile the C program."""
    compile_command = f"gcc -I./includes {c_file} -o main"
    result = subprocess.run(compile_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print("Compilation failed:")
        print(result.stderr.decode())
        return False
    return True

def run_program_with_args(args, timeout=5):
    """Run the compiled C program with arguments and monitor for deadlocks."""
    try:
        result = subprocess.run(
            ['./main'] + args,  # Arguments passed to the compiled C program
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout  # Timeout to detect if the program hangs
        )
        return result.returncode, result.stdout.decode(), result.stderr.decode()
    except subprocess.TimeoutExpired:
        # Timeout expired means a possible deadlock or infinite loop
        return None, "", "Timeout expired (possible deadlock)"

def run_test(i, test_args):
    """Run a single test and check for deadlock or timeout."""
    print(f"Running test {i + 1}/1000")
    returncode, stdout, stderr = run_program_with_args(test_args)
    
    if returncode is None:
        # If there's a timeout (possible deadlock), print the output
        print(f"Deadlock or timeout detected in test {i + 1}!")
        print(stdout)
        print(stderr)

def main():
    c_file = "src/main.c"
    if not compile_c_program(c_file):
        return
    
    # Test parameters for the C program
    test_args = ['4', '4', '10', '10', '10']
    
    # Use ThreadPoolExecutor to run tests concurrently
    with ThreadPoolExecutor(max_workers=10) as executor:
        # Run 1000 tests in parallel
        futures = [executor.submit(run_test, i, test_args) for i in range(1000)]
        
        # Wait for all futures to complete
        for future in futures:
            future.result()
    print("Test run completed!")
    os.remove('main')
    print("Removed main")
    

if __name__ == "__main__":
    main()

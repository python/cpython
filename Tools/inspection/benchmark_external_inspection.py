import _remote_debugging
import time
import subprocess
import sys
import contextlib
import tempfile
import os
import argparse
from _colorize import get_colors, can_colorize

CODE = '''\
import time
import os
import sys
import math

def slow_fibonacci(n):
    """Intentionally slow recursive fibonacci - should show up prominently in profiler"""
    if n <= 1:
        return n
    return slow_fibonacci(n-1) + slow_fibonacci(n-2)

def medium_computation():
    """Medium complexity function"""
    result = 0
    for i in range(1000):
        result += math.sqrt(i) * math.sin(i)
    return result

def fast_loop():
    """Fast simple loop"""
    total = 0
    for i in range(100):
        total += i
    return total

def string_operations():
    """String manipulation that should be visible in profiler"""
    text = "hello world " * 100
    words = text.split()
    return " ".join(reversed(words))

def nested_calls():
    """Nested function calls to test call stack depth"""
    def level1():
        def level2():
            def level3():
                return medium_computation()
            return level3()
        return level2()
    return level1()

def main_loop():
    """Main computation loop with different execution paths"""
    iteration = 0

    while True:
        iteration += 1

        # Different execution paths with different frequencies
        if iteration % 50 == 0:
            # Expensive operation - should show high per-call time
            result = slow_fibonacci(20)

        elif iteration % 10 == 0:
            # Medium operation
            result = nested_calls()

        elif iteration % 5 == 0:
            # String operations
            result = string_operations()

        else:
            # Fast operation - most common
            result = fast_loop()

        # Small delay to make sampling more interesting
        time.sleep(0.001)

if __name__ == "__main__":
    main_loop()
'''

DEEP_STATIC_CODE = """\
import time
def factorial(n):
    if n <= 1:
        time.sleep(10000)
        return 1
    return n * factorial(n-1)

factorial(900)
"""

CODE_WITH_TONS_OF_THREADS = '''\
import time
import threading
import random
import math

def cpu_intensive_work():
    """Do some CPU intensive calculations"""
    result = 0
    for _ in range(10000):
        result += math.sin(random.random()) * math.cos(random.random())
    return result

def io_intensive_work():
    """Simulate IO intensive work with sleeps"""
    time.sleep(0.1)

def mixed_workload():
    """Mix of CPU and IO work"""
    while True:
        if random.random() < 0.3:
            cpu_intensive_work()
        else:
            io_intensive_work()

def create_threads(n):
    """Create n threads doing mixed workloads"""
    threads = []
    for _ in range(n):
        t = threading.Thread(target=mixed_workload, daemon=True)
        t.start()
        threads.append(t)
    return threads

# Start with 5 threads
active_threads = create_threads(5)
thread_count = 5

# Main thread manages threads and does work
while True:
    # Randomly add or remove threads
    if random.random() < 0.1:  # 10% chance each iteration
        if random.random() < 0.5 and thread_count < 100:
            # Add 1-5 new threads
            new_count = random.randint(1, 5)
            new_threads = create_threads(new_count)
            active_threads.extend(new_threads)
            thread_count += new_count
        elif thread_count > 10:
            # Remove 1-3 threads
            remove_count = random.randint(1, 5)
            # The threads will terminate naturally since they're daemons
            active_threads = active_threads[remove_count:]
            thread_count -= remove_count

    cpu_intensive_work()
    time.sleep(0.05)
'''

CODE_EXAMPLES = {
    "basic": {
        "code": CODE,
        "description": "Mixed workload with fibonacci, computations, and string operations",
    },
    "deep_static": {
        "code": DEEP_STATIC_CODE,
        "description": "Deep recursive call stack with 900+ frames (factorial)",
    },
    "threads": {
        "code": CODE_WITH_TONS_OF_THREADS,
        "description": "Tons of threads doing mixed CPU/IO work",
    },
}


def benchmark(unwinder, duration_seconds=10):
    """Benchmark mode - measure raw sampling speed for specified duration"""
    sample_count = 0
    fail_count = 0
    total_work_time = 0.0
    start_time = time.perf_counter()
    end_time = start_time + duration_seconds
    total_attempts = 0

    colors = get_colors(can_colorize())

    print(
        f"{colors.BOLD_BLUE}Benchmarking sampling speed for {duration_seconds} seconds...{colors.RESET}"
    )

    try:
        while time.perf_counter() < end_time:
            total_attempts += 1
            work_start = time.perf_counter()
            try:
                stack_trace = unwinder.get_stack_trace()
                if stack_trace:
                    sample_count += 1
            except (OSError, RuntimeError, UnicodeDecodeError) as e:
                fail_count += 1

            work_end = time.perf_counter()
            total_work_time += work_end - work_start

            if total_attempts % 10000 == 0:
                avg_work_time_us = (total_work_time / total_attempts) * 1e6
                work_rate = (
                    total_attempts / total_work_time if total_work_time > 0 else 0
                )
                success_rate = (sample_count / total_attempts) * 100

                # Color code the success rate
                if success_rate >= 95:
                    success_color = colors.GREEN
                elif success_rate >= 80:
                    success_color = colors.YELLOW
                else:
                    success_color = colors.RED

                print(
                    f"{colors.CYAN}Attempts:{colors.RESET} {total_attempts} | "
                    f"{colors.CYAN}Success:{colors.RESET} {success_color}{success_rate:.1f}%{colors.RESET} | "
                    f"{colors.CYAN}Rate:{colors.RESET} {colors.MAGENTA}{work_rate:.1f}Hz{colors.RESET} | "
                    f"{colors.CYAN}Avg:{colors.RESET} {colors.YELLOW}{avg_work_time_us:.2f}µs{colors.RESET}"
                )
    except KeyboardInterrupt:
        print(f"\n{colors.YELLOW}Benchmark interrupted by user{colors.RESET}")

    actual_end_time = time.perf_counter()
    wall_time = actual_end_time - start_time

    # Return final statistics
    return {
        "wall_time": wall_time,
        "total_attempts": total_attempts,
        "sample_count": sample_count,
        "fail_count": fail_count,
        "success_rate": (
            (sample_count / total_attempts) * 100 if total_attempts > 0 else 0
        ),
        "total_work_time": total_work_time,
        "avg_work_time_us": (
            (total_work_time / total_attempts) * 1e6 if total_attempts > 0 else 0
        ),
        "work_rate_hz": total_attempts / total_work_time if total_work_time > 0 else 0,
        "samples_per_sec": sample_count / wall_time if wall_time > 0 else 0,
    }


def print_benchmark_results(results):
    """Print comprehensive benchmark results"""
    colors = get_colors(can_colorize())

    print(f"\n{colors.BOLD_GREEN}{'='*60}{colors.RESET}")
    print(f"{colors.BOLD_GREEN}get_stack_trace() Benchmark Results{colors.RESET}")
    print(f"{colors.BOLD_GREEN}{'='*60}{colors.RESET}")

    # Basic statistics
    print(f"\n{colors.BOLD_CYAN}Basic Statistics:{colors.RESET}")
    print(
        f"  {colors.CYAN}Wall time:{colors.RESET}           {colors.YELLOW}{results['wall_time']:.3f}{colors.RESET} seconds"
    )
    print(
        f"  {colors.CYAN}Total attempts:{colors.RESET}      {colors.MAGENTA}{results['total_attempts']:,}{colors.RESET}"
    )
    print(
        f"  {colors.CYAN}Successful samples:{colors.RESET}  {colors.GREEN}{results['sample_count']:,}{colors.RESET}"
    )
    print(
        f"  {colors.CYAN}Failed samples:{colors.RESET}      {colors.RED}{results['fail_count']:,}{colors.RESET}"
    )

    # Color code the success rate
    success_rate = results["success_rate"]
    if success_rate >= 95:
        success_color = colors.BOLD_GREEN
    elif success_rate >= 80:
        success_color = colors.BOLD_YELLOW
    else:
        success_color = colors.BOLD_RED

    print(
        f"  {colors.CYAN}Success rate:{colors.RESET}        {success_color}{success_rate:.2f}%{colors.RESET}"
    )

    # Performance metrics
    print(f"\n{colors.BOLD_CYAN}Performance Metrics:{colors.RESET}")
    print(
        f"  {colors.CYAN}Average call time:{colors.RESET}   {colors.YELLOW}{results['avg_work_time_us']:.2f}{colors.RESET} µs"
    )
    print(
        f"  {colors.CYAN}Work rate:{colors.RESET}           {colors.MAGENTA}{results['work_rate_hz']:.1f}{colors.RESET} calls/sec"
    )
    print(
        f"  {colors.CYAN}Sample rate:{colors.RESET}         {colors.MAGENTA}{results['samples_per_sec']:.1f}{colors.RESET} samples/sec"
    )
    print(
        f"  {colors.CYAN}Total work time:{colors.RESET}     {colors.YELLOW}{results['total_work_time']:.3f}{colors.RESET} seconds"
    )

    # Color code work efficiency
    efficiency = (results["total_work_time"] / results["wall_time"]) * 100
    if efficiency >= 80:
        efficiency_color = colors.GREEN
    elif efficiency >= 50:
        efficiency_color = colors.YELLOW
    else:
        efficiency_color = colors.RED

    print(
        f"  {colors.CYAN}Work efficiency:{colors.RESET}     {efficiency_color}{efficiency:.1f}%{colors.RESET}"
    )


def parse_arguments():
    """Parse command line arguments"""
    # Build the code examples description
    examples_desc = "\n".join(
        [f"  {name}: {info['description']}" for name, info in CODE_EXAMPLES.items()]
    )

    parser = argparse.ArgumentParser(
        description="Benchmark get_stack_trace() performance",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Examples:
  %(prog)s                           # Run basic benchmark for 10 seconds (default)
  %(prog)s --duration 30             # Run basic benchmark for 30 seconds
  %(prog)s -d 60                     # Run basic benchmark for 60 seconds
  %(prog)s --code deep_static        # Run deep static call stack benchmark
  %(prog)s --code deep_static -d 30  # Run deep static benchmark for 30 seconds

Available code examples:
{examples_desc}
        """,
        color=True,
    )

    parser.add_argument(
        "--duration",
        "-d",
        type=int,
        default=10,
        help="Benchmark duration in seconds (default: 10)",
    )

    parser.add_argument(
        "--code",
        "-c",
        choices=list(CODE_EXAMPLES.keys()),
        default="basic",
        help="Code example to benchmark (default: basic)",
    )

    parser.add_argument(
        "--threads",
        choices=["all", "main", "only_active"],
        default="all",
        help="Which threads to include in the benchmark (default: all)",
    )

    return parser.parse_args()


def create_target_process(temp_file, code_example="basic"):
    """Create and start the target process for benchmarking"""
    example_info = CODE_EXAMPLES.get(code_example, {"code": CODE})
    selected_code = example_info["code"]
    temp_file.write(selected_code)
    temp_file.flush()

    process = subprocess.Popen(
        [sys.executable, temp_file.name], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )

    # Give it time to start
    time.sleep(1.0)

    # Check if it's still running
    if process.poll() is not None:
        stdout, stderr = process.communicate()
        raise RuntimeError(
            f"Target process exited unexpectedly:\nSTDOUT: {stdout.decode()}\nSTDERR: {stderr.decode()}"
        )

    return process, temp_file.name


def cleanup_process(process, temp_file_path):
    """Clean up the target process and temporary file"""
    with contextlib.suppress(Exception):
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def main():
    """Main benchmark function"""
    colors = get_colors(can_colorize())
    args = parse_arguments()

    print(f"{colors.BOLD_MAGENTA}External Inspection Benchmark Tool{colors.RESET}")
    print(f"{colors.BOLD_MAGENTA}{'=' * 34}{colors.RESET}")

    example_info = CODE_EXAMPLES.get(args.code, {"description": "Unknown"})
    print(
        f"\n{colors.CYAN}Code Example:{colors.RESET} {colors.GREEN}{args.code}{colors.RESET}"
    )
    print(f"{colors.CYAN}Description:{colors.RESET} {example_info['description']}")
    print(
        f"{colors.CYAN}Benchmark Duration:{colors.RESET} {colors.YELLOW}{args.duration}{colors.RESET} seconds"
    )

    process = None
    temp_file_path = None

    try:
        # Create target process
        print(f"\n{colors.BLUE}Creating and starting target process...{colors.RESET}")
        with tempfile.NamedTemporaryFile(mode="w", suffix=".py") as temp_file:
            process, temp_file_path = create_target_process(temp_file, args.code)
            print(
                f"{colors.GREEN}Target process started with PID: {colors.BOLD_WHITE}{process.pid}{colors.RESET}"
            )

            # Run benchmark with specified duration
            with process:
                # Create unwinder and run benchmark
                print(f"{colors.BLUE}Initializing unwinder...{colors.RESET}")
                try:
                    kwargs = {}
                    if args.threads == "all":
                        kwargs["all_threads"] = True
                    elif args.threads == "main":
                        kwargs["all_threads"] = False
                    elif args.threads == "only_active":
                        kwargs["only_active_thread"] = True
                    unwinder = _remote_debugging.RemoteUnwinder(
                        process.pid, **kwargs
                    )
                    results = benchmark(unwinder, duration_seconds=args.duration)
                finally:
                    cleanup_process(process, temp_file_path)

            # Print results
            print_benchmark_results(results)

    except PermissionError as e:
        print(
            f"{colors.BOLD_RED}Error: Insufficient permissions to read stack trace: {e}{colors.RESET}"
        )
        print(
            f"{colors.YELLOW}Try running with appropriate privileges (e.g., sudo){colors.RESET}"
        )
        return 1
    except Exception as e:
        print(f"{colors.BOLD_RED}Error during benchmarking: {e}{colors.RESET}")
        if process:
            with contextlib.suppress(Exception):
                stdout, stderr = process.communicate(timeout=1)
                if stdout:
                    print(
                        f"{colors.CYAN}Process STDOUT:{colors.RESET} {stdout.decode()}"
                    )
                if stderr:
                    print(
                        f"{colors.RED}Process STDERR:{colors.RESET} {stderr.decode()}"
                    )
        raise

    return 0


if __name__ == "__main__":
    sys.exit(main())

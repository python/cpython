"""Memory watchdog: periodically read the memory usage of the main test process
and print it out, until terminated."""


import sys
import time
from test.libregrtest.utils import get_process_memory_usage


ONE_GIB = (1024 ** 3)


def watchdog(pid):
    while True:
        mem = get_process_memory_usage(pid)
        if mem is None:
            # get_process_memory_usage() is not supported on the platform,
            # or something went wrong. Exit since the next call is likely to
            # fail the same way.
            return

        # Prefer sys.stdout.write() to print() to use a single write() syscall.
        # print(msg) calls write(msg.encode()) and then write(b"\n").
        sys.stdout.write(f" ... process data size: {mem / ONE_GIB:.1f} GiB\n")
        sys.stdout.flush()
        time.sleep(1)

def main():
    if len(sys.argv) != 2:
        print(f"usage: python {sys.argv[0]} pid")
        sys.exit(1)
    pid = int(sys.argv[1])

    try:
        watchdog(pid)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()

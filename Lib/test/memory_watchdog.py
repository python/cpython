"""Memory watchdog: periodically read the memory usage of the main test process
and print it out, until terminated."""
# stdin should refer to the process' /proc/<PID>/statm: we don't pass the
# process' PID to avoid a race condition in case of - unlikely - PID recycling.
# If the process crashes, reading from the /proc entry will fail with ESRCH.


import os
import sys
import time


try:
    page_size = os.sysconf('SC_PAGESIZE')
except (ValueError, AttributeError):
    try:
        page_size = os.sysconf('SC_PAGE_SIZE')
    except (ValueError, AttributeError):
        page_size = 4096

while True:
    sys.stdin.seek(0)
    statm = sys.stdin.read()
    data = int(statm.split()[5])
    sys.stdout.write(" ... process data size: {data:.1f}G\n"
                     .format(data=data * page_size / (1024 ** 3)))
    sys.stdout.flush()
    time.sleep(1)

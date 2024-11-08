"""Memory watchdog: periodically read the memory usage of the main test process
and print it out, until terminated."""
# stdin should refer to the process' /proc/<PID>/statm: we don't pass the
# process' PID to avoid a race condition in case of - unlikely - PID recycling.
# If the process crashes, reading from the /proc entry will fail with ESRCH.


import sys
import time
from test.support import get_pagesize


while True:
    page_size = get_pagesize()
    sys.stdin.seek(0)
    statm = sys.stdin.read()
    data = int(statm.split()[5])
    sys.stdout.write(" ... process data size: {data:.1f}G\n"
                     .format(data=data * page_size / (1024 ** 3)))
    sys.stdout.flush()
    time.sleep(1)

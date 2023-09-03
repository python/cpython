import os
import sys
import time

from . import RunTests


class Logger:
    def __init__(self, pgo: bool):
        self.start_time = time.perf_counter()
        self.win_load_tracker = None
        self.pgo = pgo

        # used to display the progress bar "[ 3/100]"
        self.test_count_text = ''
        self.test_count_width = 1

    def set_tests(self, runtests: RunTests):
        if runtests.forever:
            self.test_count_text = ''
            self.test_count_width = 3
        else:
            self.test_count_text = '/{}'.format(len(runtests.tests))
            self.test_count_width = len(self.test_count_text) - 1

    def start_load_tracker(self):
        if sys.platform != 'win32':
            return

        # If we're on windows and this is the parent runner (not a worker),
        # track the load average.
        from .win_utils import WindowsLoadTracker

        try:
            self.win_load_tracker = WindowsLoadTracker()
        except PermissionError as error:
            # Standard accounts may not have access to the performance
            # counters.
            print(f'Failed to create WindowsLoadTracker: {error}')

    def stop_load_tracker(self):
        if self.win_load_tracker is not None:
            self.win_load_tracker.close()
            self.win_load_tracker = None

    def get_time(self):
        return time.perf_counter() - self.start_time

    def getloadavg(self):
        if self.win_load_tracker is not None:
            return self.win_load_tracker.getloadavg()

        if hasattr(os, 'getloadavg'):
            return os.getloadavg()[0]

        return None

    def log(self, line=''):
        empty = not line

        # add the system load prefix: "load avg: 1.80 "
        load_avg = self.getloadavg()
        if load_avg is not None:
            line = f"load avg: {load_avg:.2f} {line}"

        # add the timestamp prefix:  "0:01:05 "
        test_time = self.get_time()

        mins, secs = divmod(int(test_time), 60)
        hours, mins = divmod(mins, 60)
        test_time = "%d:%02d:%02d" % (hours, mins, secs)

        line = f"{test_time} {line}"
        if empty:
            line = line[:-1]

        print(line, flush=True)

    def display_progress(self, test_index, text, results, runtests):
        quiet = runtests.quiet
        if quiet:
            return

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count_text}"
        fails = len(results.bad) + len(results.environment_changed)
        if fails and not self.pgo:
            line = f"{line}/{fails}"
        self.log(f"[{line}] {text}")

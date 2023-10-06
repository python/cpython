import os
import time

from test.support import MS_WINDOWS
from .results import TestResults
from .runtests import RunTests
from .utils import print_warning

if MS_WINDOWS:
    from .win_utils import WindowsLoadTracker


class Logger:
    def __init__(self, results: TestResults, quiet: bool, pgo: bool):
        self.start_time = time.perf_counter()
        self.test_count_text = ''
        self.test_count_width = 3
        self.win_load_tracker: WindowsLoadTracker | None = None
        self._results: TestResults = results
        self._quiet: bool = quiet
        self._pgo: bool = pgo

    def log(self, line: str = '') -> None:
        empty = not line

        # add the system load prefix: "load avg: 1.80 "
        load_avg = self.get_load_avg()
        if load_avg is not None:
            line = f"load avg: {load_avg:.2f} {line}"

        # add the timestamp prefix:  "0:01:05 "
        log_time = time.perf_counter() - self.start_time

        mins, secs = divmod(int(log_time), 60)
        hours, mins = divmod(mins, 60)
        formatted_log_time = "%d:%02d:%02d" % (hours, mins, secs)

        line = f"{formatted_log_time} {line}"
        if empty:
            line = line[:-1]

        print(line, flush=True)

    def get_load_avg(self) -> float | None:
        if hasattr(os, 'getloadavg'):
            return os.getloadavg()[0]
        if self.win_load_tracker is not None:
            return self.win_load_tracker.getloadavg()
        return None

    def display_progress(self, test_index: int, text: str) -> None:
        if self._quiet:
            return
        results = self._results

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count_text}"
        fails = len(results.bad) + len(results.env_changed)
        if fails and not self._pgo:
            line = f"{line}/{fails}"
        self.log(f"[{line}] {text}")

    def set_tests(self, runtests: RunTests) -> None:
        if runtests.forever:
            self.test_count_text = ''
            self.test_count_width = 3
        else:
            self.test_count_text = '/{}'.format(len(runtests.tests))
            self.test_count_width = len(self.test_count_text) - 1

    def start_load_tracker(self) -> None:
        if not MS_WINDOWS:
            return

        try:
            self.win_load_tracker = WindowsLoadTracker()
        except PermissionError as error:
            # Standard accounts may not have access to the performance
            # counters.
            print_warning(f'Failed to create WindowsLoadTracker: {error}')

    def stop_load_tracker(self) -> None:
        if self.win_load_tracker is None:
            return
        self.win_load_tracker.close()
        self.win_load_tracker = None

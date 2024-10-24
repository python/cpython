import _colorize
import io
import os
import signal
import sys
import time

from test.support import MS_WINDOWS
from .cmdline import Namespace
from .result import TestResult, State
from .results import TestResults
from .runtests import RunTests
from .single import PROGRESS_MIN_TIME
from .utils import print_warning, format_duration

if MS_WINDOWS:
    from .win_utils import WindowsLoadTracker

STATE_OK = (State.PASSED,)
STATE_SKIP = (State.SKIPPED, State.RESOURCE_DENIED)

class Logger:
    # Bold red for errors and high load.
    ERROR_COLOR = _colorize.ANSIColors.BOLD_RED
    # Regular yellow for info/warnings and expected load.
    INFO_COLOR = _colorize.ANSIColors.YELLOW
    # Bold green for passing tests and low load.
    GOOD_COLOR = _colorize.ANSIColors.BOLD_GREEN
    RESET_COLOR = _colorize.ANSIColors.RESET

    def __init__(self, results: TestResults, ns: Namespace):
        self.start_time = time.perf_counter()
        self.test_count_text = ''
        self.test_count_width = 3
        self.win_load_tracker: WindowsLoadTracker | None = None
        self._results: TestResults = results
        self._quiet: bool = ns.quiet
        self._pgo: bool = ns.pgo
        self.color = ns.color
        if self.color is None:
            self.color = _colorize.can_colorize()
        self.load_threshold = os.process_cpu_count()

    def error(self, s) -> str:
        if not self.color:
            return s
        return f'{self.ERROR_COLOR}{s}{self.RESET_COLOR}'

    def warning(self, s) -> str:
        if not self.color:
            return s
        return f'{self.INFO_COLOR}{s}{self.RESET_COLOR}'

    def good(self, s) -> str:
        if not self.color:
            return s
        return f'{self.GOOD_COLOR}{s}{self.RESET_COLOR}'

    def load_color(self, load_avg: float):
        load = f"{load_avg:.2f}"
        if load_avg < self.load_threshold:
            load = self.good(load)
        elif load_avg < self.load_threshold * 2:
            load = self.warning(load)
        else:
            load = self.error(load)
        return load

    def state_color(self, text: str, state: str | None):
        if state is None or not self.color:
            return text
        if state in STATE_OK:
            return self.good(text)
        elif state in STATE_SKIP:
            return self.warning(text)
        else:
            return self.error(text)

    def log(self, line: str = '') -> int:
        empty = not line

        # add the system load prefix: "load avg: 1.80 "

        load_avg = self.get_load_avg()
        if load_avg is not None:
            load = self.load_color(load_avg)
            line = f"load avg: {load} {line}"

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
            try:
                return os.getloadavg()[0]
            except OSError:
                pass
        if self.win_load_tracker is not None:
            return self.win_load_tracker.getloadavg()
        return None

    def display_progress(self, test_index: int, text: str,
                         stdout: str|None = None) -> None:
        results = self._results
        # "[ 51/405/1] test_tcl passed"
        passed = self.good(f"{test_index:{self.test_count_width}}")
        line = f"{passed}{self.test_count_text}"
        fails = len(results.bad) + len(results.env_changed)
        if fails and not self._pgo:
            line = f"{line}/{self.error(fails)}"
        self.log(f"[{line}] {text}")
        if stdout:
            print(stdout, flush=True)

    def update_progress(self, test_index: int, result: TestResult|None,
                         error_text: str|None = None,
                         running: list[tuple[float, str]]|None = None,
                         stdout: str|None = None) -> None:
        if self._quiet:
            return
        text = ''
        state = None
        if result is not None:
            text = str(result)
            state = result.state
        text = self.state_color(text, state)
        if (result is not None and not self._pgo and
            result.duration is not None and
            result.duration >= PROGRESS_MIN_TIME):
            text = f"{text} ({self.warning(format_duration(result.duration))})"
        if error_text:
            text = f"{text} ({self.error(error_text)})"

        # To avoid spamming the output, only report running tests if they
        # are taking a long time.
        if running:
            lrt = [(secs, test) for secs, test in running if secs >= PROGRESS_MIN_TIME]
            if lrt:
                test_text = ', '.join(
                    f'{test} {self.warning(format_duration(secs))}'
                    for secs, test in lrt
                )
                if text:
                    text += ' -- '
                text += f'running ({len(lrt)}): {test_text}'
        self.display_progress(test_index, text, stdout)

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


class StatusRemovingWrapper(io.TextIOWrapper):
    # Set status_size here to avoid having to override __init__.
    status_size = 0
    def write(self, msg):
        if self.status_size:
            # Clear the last status report, by moving up the number of lines
            # of the status report and clearing the screen below it. This
            # will mess up if something wrote new lines to the screen since
            # the last status report without going through here (e.g.
            # writing directly to the underlying buffer, or the fd). That
            # shouldn't happen when using multiprocessing to run the tests
            # in isolation.
            super().write(f'\033[{self.status_size}A' + '\033[0J')
        self.status_size = 0
        super().write(msg)


def replace_stdout():
    assert type(sys.stdout) is io.TextIOWrapper
    old = sys.stdout
    new = StatusRemovingWrapper(old.buffer, encoding=old.encoding,
                                errors=old.errors,
                                line_buffering=old.line_buffering,
                                write_through=old.write_through)
    sys.stdout = new


class FancyLogger(Logger):
    """A logger with more compact, screen-updating output."""
    def __init__(self, results: TestResults, ns: Namespace):
        self.report_skip_reason = ns.fancy_report_skip_reason
        self.setup_terminal()
        # Import here to avoid circular import issues.
        from . import run_workers
        run_workers.PROGRESS_UPDATE = 5
        super().__init__(results, ns)

    def setup_terminal(self):
        import termios
        def set_columns(unused_sig, unused_frame):
            self.lines, self.columns = termios.tcgetwinsize(sys.stdout.fileno())
        set_columns(None, None)
        signal.signal(signal.SIGWINCH, set_columns)
        replace_stdout()

    def update_progress(self, test_index: int, result: TestResult|None,
                        error_text: str|None = None,
                        running: list[tuple[float, str]]|None = None,
                        stdout: str|None = None) -> None:
        if self._quiet:
            return

        text = ''
        state = None
        if result is not None:
            text = str(result)
            state = result.state
        # We don't want any of the text to wrap (or it'll mess up the
        # in-place update logic), so if the log line is too long, emit it on
        # its own line. The logging process probably adds no more than 35
        # visible characters, or just 4 characters of indentation when
        # printed on its own.
        linelen = len(text)
        text = self.state_color(text, state)
        # If the test is skipped, extra info might be in the test output. We
        # want to display just the skip reason, though, so that needs to be
        # extracted.
        if (state in STATE_SKIP and stdout and
            ' skipped -- ' in stdout and stdout.count('\n') <= 1):
            if self.report_skip_reason:
                _, _, extra_text = stdout.partition(' skipped -- ')
                linelen += len(extra_text) + 3
                text = f'{text} ({self.warning(extra_text)})'
            stdout = None
        elif result is not None and result.duration is not None:
            duration = format_duration(result.duration)
            linelen += len(duration) + 3
            text = f'{text} ({self.warning(duration)})'
        if error_text:
            linelen += len(error_text) + 3
            text = f'{text} ({self.error(error_text)})'

        # If the report line is too long (it might wrap even on a line of
        # its own, with 4-space indent), or there's test stdout to show
        # (which should be preceded by the test report so we know where it
        # came from), or it's a test skip and we need to report those on
        # their own, or it's otherwise not a passing test, display the
        # report with the test's stdout, then display the regular progress
        # report below it.
        if (linelen + 4 > self.columns or
            stdout or
            (self.report_skip_reason and state in STATE_SKIP) or
            (state is not None and state not in STATE_OK + STATE_SKIP)):
            self.display_progress(test_index, text, stdout=stdout)
            text = ''
            linelen = 0

        # Logging adds ~35 characters (timestamp, load avg, test count), so
        # if that would make the report line wrap, print it on a line of its own.
        if linelen + 35 > self.columns:
            lines = ['', f'     {text}']
        else:
            lines = [text]
        if running:
            # Don't fill more than half the screen with running tests (in
            # case someone runs with `-j 100` or has a very short screen).
            # The list is already sorted (descending) by running time, so
            # the longest running ones will be shown.
            for secs, test in running[:self.lines // 2]:
                duration = format_duration(secs)
                visible_line = f" ... running: {test} ({duration})"
                duration = self.warning(duration)
                # Truncate the status lines in case very long test names
                # don't fit, so we don't end up wrapping lines.
                diff = len(visible_line) - self.columns
                if diff > 1:
                    if diff > 5:
                        test = test[:-diff] + '[...]'
                    line = f" ... r: {test} ({duration})"
                else:
                    line = f" ... running: {test} ({duration})"
                lines.append(line)
        report = '\n'.join(lines)
        self.display_progress(test_index, report)
        sys.stdout.status_size = report.count('\n') + 1  # type: ignore[attr-defined]


def detect_vt100_capability():
    """Rough, "good enough" vt100 detection."""
    if not sys.stdout.isatty():
        return False
    try:
        import termios
    except ImportError:
        return False
    try:
        termios.tcgetwinsize(sys.stdout.fileno())
    except termios.error:
        return False
    term = os.environ.get('TERM')
    if not term.startswith(('vt100', 'xterm', 'screen')):
        # Should we parse terminfo capabilities? Or just add terminals to
        # the list as they come up? We don't want too many dependencies.
        return False
    return True


def create_logger(results: TestResults, ns: Namespace) -> Logger:
    reporter = ns.progress_reporter
    logger_class: type[Logger]
    if (reporter == 'detect' and ns.use_mp is not None and
        detect_vt100_capability()):
        logger_class = FancyLogger
    elif reporter == 'fancy':
        logger_class = FancyLogger
    else:
        logger_class = Logger
    return logger_class(results, ns)

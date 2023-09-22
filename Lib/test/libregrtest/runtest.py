import dataclasses
import doctest
import faulthandler
import functools
import gc
import importlib
import io
import os
import sys
import time
import traceback
import unittest

from test import support
from test.support import TestStats
from test.support import os_helper
from test.support import threading_helper
from test.libregrtest.cmdline import Namespace
from test.libregrtest.save_env import saved_test_environment
from test.libregrtest.utils import clear_caches, format_duration, print_warning


# Avoid enum.Enum to reduce the number of imports when tests are run
class State:
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    UNCAUGHT_EXC = "UNCAUGHT_EXC"
    REFLEAK = "REFLEAK"
    ENV_CHANGED = "ENV_CHANGED"
    RESOURCE_DENIED = "RESOURCE_DENIED"
    INTERRUPTED = "INTERRUPTED"
    MULTIPROCESSING_ERROR = "MULTIPROCESSING_ERROR"
    DID_NOT_RUN = "DID_NOT_RUN"
    TIMEOUT = "TIMEOUT"

    @staticmethod
    def is_failed(state):
        return state in {
            State.FAILED,
            State.UNCAUGHT_EXC,
            State.REFLEAK,
            State.MULTIPROCESSING_ERROR,
            State.TIMEOUT}

    @staticmethod
    def has_meaningful_duration(state):
        # Consider that the duration is meaningless for these cases.
        # For example, if a whole test file is skipped, its duration
        # is unlikely to be the duration of executing its tests,
        # but just the duration to execute code which skips the test.
        return state not in {
            State.SKIPPED,
            State.RESOURCE_DENIED,
            State.INTERRUPTED,
            State.MULTIPROCESSING_ERROR,
            State.DID_NOT_RUN}


@dataclasses.dataclass(slots=True)
class TestResult:
    test_name: str
    state: str | None = None
    # Test duration in seconds
    duration: float | None = None
    xml_data: list[str] | None = None
    stats: TestStats | None = None

    # errors and failures copied from support.TestFailedWithDetails
    errors: list[tuple[str, str]] | None = None
    failures: list[tuple[str, str]] | None = None

    def is_failed(self, fail_env_changed: bool) -> bool:
        if self.state == State.ENV_CHANGED:
            return fail_env_changed
        return State.is_failed(self.state)

    def _format_failed(self):
        if self.errors and self.failures:
            le = len(self.errors)
            lf = len(self.failures)
            error_s = "error" + ("s" if le > 1 else "")
            failure_s = "failure" + ("s" if lf > 1 else "")
            return f"{self.test_name} failed ({le} {error_s}, {lf} {failure_s})"

        if self.errors:
            le = len(self.errors)
            error_s = "error" + ("s" if le > 1 else "")
            return f"{self.test_name} failed ({le} {error_s})"

        if self.failures:
            lf = len(self.failures)
            failure_s = "failure" + ("s" if lf > 1 else "")
            return f"{self.test_name} failed ({lf} {failure_s})"

        return f"{self.test_name} failed"

    def __str__(self) -> str:
        match self.state:
            case State.PASSED:
                return f"{self.test_name} passed"
            case State.FAILED:
                return self._format_failed()
            case State.SKIPPED:
                return f"{self.test_name} skipped"
            case State.UNCAUGHT_EXC:
                return f"{self.test_name} failed (uncaught exception)"
            case State.REFLEAK:
                return f"{self.test_name} failed (reference leak)"
            case State.ENV_CHANGED:
                return f"{self.test_name} failed (env changed)"
            case State.RESOURCE_DENIED:
                return f"{self.test_name} skipped (resource denied)"
            case State.INTERRUPTED:
                return f"{self.test_name} interrupted"
            case State.MULTIPROCESSING_ERROR:
                return f"{self.test_name} process crashed"
            case State.DID_NOT_RUN:
                return f"{self.test_name} ran no tests"
            case State.TIMEOUT:
                return f"{self.test_name} timed out ({format_duration(self.duration)})"
            case _:
                raise ValueError("unknown result state: {state!r}")

    def has_meaningful_duration(self):
        return State.has_meaningful_duration(self.state)

    def set_env_changed(self):
        if self.state is None or self.state == State.PASSED:
            self.state = State.ENV_CHANGED


# Minimum duration of a test to display its duration or to mention that
# the test is running in background
PROGRESS_MIN_TIME = 30.0   # seconds

#If these test directories are encountered recurse into them and treat each
# test_ .py or dir as a separate test module. This can increase parallelism.
# Beware this can't generally be done for any directory with sub-tests as the
# __init__.py may do things which alter what tests are to be run.

SPLITTESTDIRS = {
    "test_asyncio",
    "test_concurrent_futures",
    "test_future_stmt",
    "test_multiprocessing_fork",
    "test_multiprocessing_forkserver",
    "test_multiprocessing_spawn",
}

# Storage of uncollectable objects
FOUND_GARBAGE = []


def findtestdir(path=None):
    return path or os.path.dirname(os.path.dirname(__file__)) or os.curdir


def findtests(*, testdir=None, exclude=(),
              split_test_dirs=SPLITTESTDIRS, base_mod=""):
    """Return a list of all applicable test modules."""
    testdir = findtestdir(testdir)
    tests = []
    for name in os.listdir(testdir):
        mod, ext = os.path.splitext(name)
        if (not mod.startswith("test_")) or (mod in exclude):
            continue
        if mod in split_test_dirs:
            subdir = os.path.join(testdir, mod)
            mod = f"{base_mod or 'test'}.{mod}"
            tests.extend(findtests(testdir=subdir, exclude=exclude,
                                   split_test_dirs=split_test_dirs, base_mod=mod))
        elif ext in (".py", ""):
            tests.append(f"{base_mod}.{mod}" if base_mod else mod)
    return sorted(tests)


def split_test_packages(tests, *, testdir=None, exclude=(),
                        split_test_dirs=SPLITTESTDIRS):
    testdir = findtestdir(testdir)
    splitted = []
    for name in tests:
        if name in split_test_dirs:
            subdir = os.path.join(testdir, name)
            splitted.extend(findtests(testdir=subdir, exclude=exclude,
                                      split_test_dirs=split_test_dirs,
                                      base_mod=name))
        else:
            splitted.append(name)
    return splitted


def get_abs_module(ns: Namespace, test_name: str) -> str:
    if test_name.startswith('test.') or ns.testdir:
        return test_name
    else:
        # Import it from the test package
        return 'test.' + test_name


def _runtest_capture_output_timeout_junit(result: TestResult, ns: Namespace) -> None:
    # Capture stdout and stderr, set faulthandler timeout,
    # and create JUnit XML report.

    output_on_failure = ns.verbose3

    use_timeout = (
        ns.timeout is not None and threading_helper.can_start_thread
    )
    if use_timeout:
        faulthandler.dump_traceback_later(ns.timeout, exit=True)

    try:
        support.set_match_tests(ns.match_tests, ns.ignore_tests)
        support.junit_xml_list = xml_list = [] if ns.xmlpath else None
        if ns.failfast:
            support.failfast = True

        if output_on_failure:
            support.verbose = True

            stream = io.StringIO()
            orig_stdout = sys.stdout
            orig_stderr = sys.stderr
            print_warning = support.print_warning
            orig_print_warnings_stderr = print_warning.orig_stderr

            output = None
            try:
                sys.stdout = stream
                sys.stderr = stream
                # print_warning() writes into the temporary stream to preserve
                # messages order. If support.environment_altered becomes true,
                # warnings will be written to sys.stderr below.
                print_warning.orig_stderr = stream

                _runtest_env_changed_exc(result, ns, display_failure=False)
                # Ignore output if the test passed successfully
                if result.state != State.PASSED:
                    output = stream.getvalue()
            finally:
                sys.stdout = orig_stdout
                sys.stderr = orig_stderr
                print_warning.orig_stderr = orig_print_warnings_stderr

            if output is not None:
                sys.stderr.write(output)
                sys.stderr.flush()
        else:
            # Tell tests to be moderately quiet
            support.verbose = ns.verbose

            _runtest_env_changed_exc(result, ns,
                                     display_failure=not ns.verbose)

        if xml_list:
            import xml.etree.ElementTree as ET
            result.xml_data = [ET.tostring(x).decode('us-ascii')
                               for x in xml_list]
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()
        support.junit_xml_list = None


def runtest(ns: Namespace, test_name: str) -> TestResult:
    """Run a single test.

    ns -- regrtest namespace of options
    test_name -- the name of the test

    Returns a TestResult.

    If ns.xmlpath is not None, xml_data is a list containing each
    generated testsuite element.
    """
    start_time = time.perf_counter()
    result = TestResult(test_name)
    try:
        _runtest_capture_output_timeout_junit(result, ns)
    except:
        if not ns.pgo:
            msg = traceback.format_exc()
            print(f"test {test_name} crashed -- {msg}",
                  file=sys.stderr, flush=True)
        result.state = State.UNCAUGHT_EXC
    result.duration = time.perf_counter() - start_time
    return result


def _test_module(the_module):
    loader = unittest.TestLoader()
    tests = loader.loadTestsFromModule(the_module)
    for error in loader.errors:
        print(error, file=sys.stderr)
    if loader.errors:
        raise Exception("errors while loading tests")
    return support.run_unittest(tests)


def save_env(ns: Namespace, test_name: str):
    return saved_test_environment(test_name, ns.verbose, ns.quiet, pgo=ns.pgo)


def regrtest_runner(result, test_func, ns) -> None:
    # Run test_func(), collect statistics, and detect reference and memory
    # leaks.

    if ns.huntrleaks:
        from test.libregrtest.refleak import dash_R
        refleak, test_result = dash_R(ns, result.test_name, test_func)
    else:
        test_result = test_func()
        refleak = False

    if refleak:
        result.state = State.REFLEAK

    match test_result:
        case TestStats():
            stats = test_result
        case unittest.TestResult():
            stats = TestStats.from_unittest(test_result)
        case doctest.TestResults():
            stats = TestStats.from_doctest(test_result)
        case None:
            print_warning(f"{result.test_name} test runner returned None: {test_func}")
            stats = None
        case _:
            print_warning(f"Unknown test result type: {type(test_result)}")
            stats = None

    result.stats = stats


def _load_run_test(result: TestResult, ns: Namespace) -> None:
    # Load the test function, run the test function.

    abstest = get_abs_module(ns, result.test_name)

    # remove the module from sys.module to reload it if it was already imported
    try:
        del sys.modules[abstest]
    except KeyError:
        pass

    the_module = importlib.import_module(abstest)

    if hasattr(the_module, "test_main"):
        # https://github.com/python/cpython/issues/89392
        raise Exception(f"Module {result.test_name} defines test_main() which is no longer supported by regrtest")
    test_func = functools.partial(_test_module, the_module)

    try:
        with save_env(ns, result.test_name):
            regrtest_runner(result, test_func, ns)
    finally:
        # First kill any dangling references to open files etc.
        # This can also issue some ResourceWarnings which would otherwise get
        # triggered during the following test run, and possibly produce
        # failures.
        support.gc_collect()

        cleanup_test_droppings(result.test_name, ns.verbose)

    if gc.garbage:
        support.environment_altered = True
        print_warning(f"{result.test_name} created {len(gc.garbage)} "
                      f"uncollectable object(s).")

        # move the uncollectable objects somewhere,
        # so we don't see them again
        FOUND_GARBAGE.extend(gc.garbage)
        gc.garbage.clear()

    support.reap_children()


def _runtest_env_changed_exc(result: TestResult, ns: Namespace,
                             display_failure: bool = True) -> None:
    # Detect environment changes, handle exceptions.

    # Reset the environment_altered flag to detect if a test altered
    # the environment
    support.environment_altered = False

    if ns.pgo:
        display_failure = False

    test_name = result.test_name
    try:
        clear_caches()
        support.gc_collect()

        with save_env(ns, test_name):
            _load_run_test(result, ns)
    except support.ResourceDenied as msg:
        if not ns.quiet and not ns.pgo:
            print(f"{test_name} skipped -- {msg}", flush=True)
        result.state = State.RESOURCE_DENIED
        return
    except unittest.SkipTest as msg:
        if not ns.quiet and not ns.pgo:
            print(f"{test_name} skipped -- {msg}", flush=True)
        result.state = State.SKIPPED
        return
    except support.TestFailedWithDetails as exc:
        msg = f"test {test_name} failed"
        if display_failure:
            msg = f"{msg} -- {exc}"
        print(msg, file=sys.stderr, flush=True)
        result.state = State.FAILED
        result.errors = exc.errors
        result.failures = exc.failures
        result.stats = exc.stats
        return
    except support.TestFailed as exc:
        msg = f"test {test_name} failed"
        if display_failure:
            msg = f"{msg} -- {exc}"
        print(msg, file=sys.stderr, flush=True)
        result.state = State.FAILED
        result.stats = exc.stats
        return
    except support.TestDidNotRun:
        result.state = State.DID_NOT_RUN
        return
    except KeyboardInterrupt:
        print()
        result.state = State.INTERRUPTED
        return
    except:
        if not ns.pgo:
            msg = traceback.format_exc()
            print(f"test {test_name} crashed -- {msg}",
                  file=sys.stderr, flush=True)
        result.state = State.UNCAUGHT_EXC
        return

    if support.environment_altered:
        result.set_env_changed()
    # Don't override the state if it was already set (REFLEAK or ENV_CHANGED)
    if result.state is None:
        result.state = State.PASSED


def cleanup_test_droppings(test_name: str, verbose: int) -> None:
    # Try to clean up junk commonly left behind.  While tests shouldn't leave
    # any files or directories behind, when a test fails that can be tedious
    # for it to arrange.  The consequences can be especially nasty on Windows,
    # since if a test leaves a file open, it cannot be deleted by name (while
    # there's nothing we can do about that here either, we can display the
    # name of the offending test, which is a real help).
    for name in (os_helper.TESTFN,):
        if not os.path.exists(name):
            continue

        if os.path.isdir(name):
            import shutil
            kind, nuker = "directory", shutil.rmtree
        elif os.path.isfile(name):
            kind, nuker = "file", os.unlink
        else:
            raise RuntimeError(f"os.path says {name!r} exists but is neither "
                               f"directory nor file")

        if verbose:
            print_warning(f"{test_name} left behind {kind} {name!r}")
            support.environment_altered = True

        try:
            import stat
            # fix possible permissions problems that might prevent cleanup
            os.chmod(name, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
            nuker(name)
        except Exception as exc:
            print_warning(f"{test_name} left behind {kind} {name!r} "
                          f"and it couldn't be removed: {exc}")

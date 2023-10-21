import doctest
import faulthandler
import gc
import importlib
import io
import sys
import time
import traceback
import unittest

from test import support
from test.support import TestStats
from test.support import threading_helper

from .result import State, TestResult
from .runtests import RunTests
from .save_env import saved_test_environment
from .setup import setup_tests
from .utils import (
    TestName,
    clear_caches, remove_testfn, abs_module_name, print_warning)


# Minimum duration of a test to display its duration or to mention that
# the test is running in background
PROGRESS_MIN_TIME = 30.0   # seconds


def run_unittest(test_mod):
    loader = unittest.TestLoader()
    tests = loader.loadTestsFromModule(test_mod)
    for error in loader.errors:
        print(error, file=sys.stderr)
    if loader.errors:
        raise Exception("errors while loading tests")
    return support.run_unittest(tests)


def regrtest_runner(result: TestResult, test_func, runtests: RunTests) -> None:
    # Run test_func(), collect statistics, and detect reference and memory
    # leaks.
    if runtests.hunt_refleak:
        from .refleak import runtest_refleak
        refleak, test_result = runtest_refleak(result.test_name, test_func,
                                               runtests.hunt_refleak,
                                               runtests.quiet)
    else:
        test_result = test_func()
        refleak = False

    if refleak:
        result.state = State.REFLEAK

    stats: TestStats | None

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


# Storage of uncollectable GC objects (gc.garbage)
GC_GARBAGE = []


def _load_run_test(result: TestResult, runtests: RunTests) -> None:
    # Load the test module and run the tests.
    test_name = result.test_name
    module_name = abs_module_name(test_name, runtests.test_dir)

    # Remove the module from sys.module to reload it if it was already imported
    sys.modules.pop(module_name, None)

    test_mod = importlib.import_module(module_name)

    if hasattr(test_mod, "test_main"):
        # https://github.com/python/cpython/issues/89392
        raise Exception(f"Module {test_name} defines test_main() which "
                        f"is no longer supported by regrtest")
    def test_func():
        return run_unittest(test_mod)

    try:
        regrtest_runner(result, test_func, runtests)
    finally:
        # First kill any dangling references to open files etc.
        # This can also issue some ResourceWarnings which would otherwise get
        # triggered during the following test run, and possibly produce
        # failures.
        support.gc_collect()

        remove_testfn(test_name, runtests.verbose)

    if gc.garbage:
        support.environment_altered = True
        print_warning(f"{test_name} created {len(gc.garbage)} "
                      f"uncollectable object(s)")

        # move the uncollectable objects somewhere,
        # so we don't see them again
        GC_GARBAGE.extend(gc.garbage)
        gc.garbage.clear()

    support.reap_children()


def _runtest_env_changed_exc(result: TestResult, runtests: RunTests,
                             display_failure: bool = True) -> None:
    # Handle exceptions, detect environment changes.

    # Reset the environment_altered flag to detect if a test altered
    # the environment
    support.environment_altered = False

    pgo = runtests.pgo
    if pgo:
        display_failure = False
    quiet = runtests.quiet

    test_name = result.test_name
    try:
        clear_caches()
        support.gc_collect()

        with saved_test_environment(test_name,
                                    runtests.verbose, quiet, pgo=pgo):
            _load_run_test(result, runtests)
    except support.ResourceDenied as exc:
        if not quiet and not pgo:
            print(f"{test_name} skipped -- {exc}", flush=True)
        result.state = State.RESOURCE_DENIED
        return
    except unittest.SkipTest as exc:
        if not quiet and not pgo:
            print(f"{test_name} skipped -- {exc}", flush=True)
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
        if not pgo:
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


def _runtest(result: TestResult, runtests: RunTests) -> None:
    # Capture stdout and stderr, set faulthandler timeout,
    # and create JUnit XML report.
    verbose = runtests.verbose
    output_on_failure = runtests.output_on_failure
    timeout = runtests.timeout

    use_timeout = (
        timeout is not None and threading_helper.can_start_thread
    )
    if use_timeout:
        faulthandler.dump_traceback_later(timeout, exit=True)

    try:
        setup_tests(runtests)

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

                _runtest_env_changed_exc(result, runtests, display_failure=False)
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
            support.verbose = verbose
            _runtest_env_changed_exc(result, runtests,
                                     display_failure=not verbose)

        xml_list = support.junit_xml_list
        if xml_list:
            import xml.etree.ElementTree as ET
            result.xml_data = [ET.tostring(x).decode('us-ascii')
                               for x in xml_list]
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()
        support.junit_xml_list = None


def run_single_test(test_name: TestName, runtests: RunTests) -> TestResult:
    """Run a single test.

    test_name -- the name of the test

    Returns a TestResult.

    If runtests.use_junit, xml_data is a list containing each generated
    testsuite element.
    """
    start_time = time.perf_counter()
    result = TestResult(test_name)
    pgo = runtests.pgo
    try:
        _runtest(result, runtests)
    except:
        if not pgo:
            msg = traceback.format_exc()
            print(f"test {test_name} crashed -- {msg}",
                  file=sys.stderr, flush=True)
        result.state = State.UNCAUGHT_EXC

    sys.stdout.flush()
    sys.stderr.flush()

    result.duration = time.perf_counter() - start_time
    return result

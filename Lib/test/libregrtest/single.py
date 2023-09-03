import doctest
import faulthandler
import gc
import importlib
import sys
import time
import traceback
import types
import unittest

from test import support
from test.support import threading_helper

from . import TestStats, State, TestResult, RunTests
from .setup import setup_support
from .save_env import saved_test_environment
from .utils import (
    clear_caches, print_warning,
    abs_module_name, remove_testfn, capture_output)


# Minimum duration of a test to display its duration or to mention that
# the test is running in background
PROGRESS_MIN_TIME = 30.0   # seconds


def _run_unittest(test_mod):
    loader = unittest.TestLoader()
    tests = loader.loadTestsFromModule(test_mod)
    for error in loader.errors:
        print(error, file=sys.stderr)
    if loader.errors:
        raise Exception("errors while loading tests")
    return support.run_unittest(tests)


def _save_env(test_name: str, runtests: RunTests):
    return saved_test_environment(test_name, runtests.verbose, runtests.quiet,
                                  pgo=runtests.pgo)


def _regrtest_runner(result: TestResult, test_mod: types.ModuleType,
                     runtests: RunTests) -> None:
    # Run test_func(), collect statistics, and detect reference and memory
    # leaks.

    if runtests.huntrleaks:
        from .refleak import runtest_refleak
        def test_func():
            return _run_unittest(test_mod)
        leak, test_result = runtest_refleak(result.test_name, test_func,
                                   runtests.huntrleaks, runtests.quiet)
    else:
        test_result = _run_unittest(test_mod)
        leak = False

    if leak:
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


# Storage of uncollectable objects
FOUND_GARBAGE = []


def _load_run_test(result: TestResult, runtests: RunTests) -> None:
    # Load the test function, run the test function.
    module_name = abs_module_name(result.test_name, runtests.test_dir)

    # Remove the module from sys.module to reload it if it was already imported
    sys.modules.pop(module_name, None)

    test_mod = importlib.import_module(module_name)

    if hasattr(test_mod, "test_main"):
        # https://github.com/python/cpython/issues/89392
        raise Exception(f"Module {result.test_name} defines test_main() which is no longer supported by regrtest")

    try:
        with _save_env(result.test_name, runtests):
            _regrtest_runner(result, test_mod, runtests)
    finally:
        # First kill any dangling references to open files etc.
        # This can also issue some ResourceWarnings which would otherwise get
        # triggered during the following test run, and possibly produce
        # failures.
        support.gc_collect()

        remove_testfn(result.test_name, runtests.verbose)

    if gc.garbage:
        support.environment_altered = True
        print_warning(f"{result.test_name} created {len(gc.garbage)} "
                      f"uncollectable object(s)")

        # move the uncollectable objects somewhere,
        # so we don't see them again
        FOUND_GARBAGE.extend(gc.garbage)
        gc.garbage.clear()

    support.reap_children()


def _runtest_env_changed_exc(result: TestResult, runtests: RunTests,
                             display_failure: bool = True) -> None:
    # Detect environment changes, handle exceptions.
    pgo = runtests.pgo

    # Reset the environment_altered flag to detect if a test altered
    # the environment
    support.environment_altered = False

    if pgo:
        display_failure = False

    test_name = result.test_name
    try:
        clear_caches()
        support.gc_collect()

        with _save_env(test_name, runtests):
            _load_run_test(result, runtests)
    except support.ResourceDenied as msg:
        if not runtests.quiet and not pgo:
            print(f"{test_name} skipped -- {msg}", flush=True)
        result.state = State.RESOURCE_DENIED
        return
    except unittest.SkipTest as msg:
        if not runtests.quiet and not pgo:
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


def _run_single_test(result: TestResult, runtests: RunTests) -> None:
    # Capture stdout and stderr, set faulthandler timeout,
    # and create JUnit XML report.
    timeout = runtests.timeout
    use_timeout = (bool(timeout) and threading_helper.can_start_thread)
    if use_timeout:
        faulthandler.dump_traceback_later(timeout, exit=True)

    try:
        setup_support(runtests)
        if runtests.use_junit:
            support.junit_xml_list = []
        else:
            support.junit_xml_list = None

        if runtests.output_on_failure:
            support.verbose = True

            with capture_output() as stream:
                _runtest_env_changed_exc(result, runtests,
                                         display_failure=False)

            # Ignore output if the test passed successfully
            if result.state != State.PASSED:
                output = stream.getvalue()
                sys.stderr.write(output)
                sys.stderr.flush()
        else:
            # Tell tests to be moderately quiet
            verbose = runtests.verbose
            support.verbose = verbose
            _runtest_env_changed_exc(result, runtests,
                                     display_failure=not verbose)

        if runtests.use_junit:
            xml_list = support.junit_xml_list
            if xml_list:
                import xml.etree.ElementTree as ET
                result.xml_data = [ET.tostring(x).decode('us-ascii')
                                   for x in xml_list]
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()
        support.junit_xml_list = None


def run_single_test(test_name: str, runtests: RunTests) -> TestResult:
    """Run a single test.

    If runtests.xmlpath is not None, xml_data is a list containing each
    generated testsuite element.
    """
    start_time = time.perf_counter()
    result = TestResult(test_name)
    try:
        _run_single_test(result, runtests)
    except:
        if not runtests.pgo:
            msg = traceback.format_exc()
            print(f"test {test_name} crashed -- {msg}",
                  file=sys.stderr, flush=True)
        result.state = State.UNCAUGHT_EXC
    result.duration = time.perf_counter() - start_time
    return result

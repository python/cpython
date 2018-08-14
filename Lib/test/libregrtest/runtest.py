import faulthandler
import importlib
import io
import os
import sys
import time
import traceback
import unittest
from test import support
from test.libregrtest.refleak import dash_R, clear_caches
from test.libregrtest.save_env import saved_test_environment


# Test result constants.
PASSED = 1
FAILED = 0
ENV_CHANGED = -1
SKIPPED = -2
RESOURCE_DENIED = -3
INTERRUPTED = -4
CHILD_ERROR = -5   # error in a child process

_FORMAT_TEST_RESULT = {
    PASSED: '%s passed',
    FAILED: '%s failed',
    ENV_CHANGED: '%s failed (env changed)',
    SKIPPED: '%s skipped',
    RESOURCE_DENIED: '%s skipped (resource denied)',
    INTERRUPTED: '%s interrupted',
    CHILD_ERROR: '%s crashed',
}

# Minimum duration of a test to display its duration or to mention that
# the test is running in background
PROGRESS_MIN_TIME = 30.0   # seconds

# small set of tests to determine if we have a basically functioning interpreter
# (i.e. if any of these fail, then anything else is likely to follow)
STDTESTS = [
    'test_grammar',
    'test_opcodes',
    'test_dict',
    'test_builtin',
    'test_exceptions',
    'test_types',
    'test_unittest',
    'test_doctest',
    'test_doctest2',
    'test_support'
]

# set of tests that we don't want to be executed when using regrtest
NOTTESTS = set()


def format_test_result(test_name, result):
    fmt = _FORMAT_TEST_RESULT.get(result, "%s")
    return fmt % test_name


def findtests(testdir=None, stdtests=STDTESTS, nottests=NOTTESTS):
    """Return a list of all applicable test modules."""
    testdir = findtestdir(testdir)
    names = os.listdir(testdir)
    tests = []
    others = set(stdtests) | nottests
    for name in names:
        mod, ext = os.path.splitext(name)
        if mod[:5] == "test_" and ext in (".py", "") and mod not in others:
            tests.append(mod)
    return stdtests + sorted(tests)


def get_abs_module(ns, test):
    if test.startswith('test.') or ns.testdir:
        return test
    else:
        # Always import it from the test package
        return 'test.' + test


def runtest(ns, test):
    """Run a single test.

    ns -- regrtest namespace of options
    test -- the name of the test

    Returns the tuple (result, test_time), where result is one of the
    constants:

        INTERRUPTED      KeyboardInterrupt when run under -j
        RESOURCE_DENIED  test skipped because resource denied
        SKIPPED          test skipped for some other reason
        ENV_CHANGED      test failed because it changed the execution environment
        FAILED           test failed
        PASSED           test passed
    """

    output_on_failure = ns.verbose3

    use_timeout = (ns.timeout is not None)
    if use_timeout:
        faulthandler.dump_traceback_later(ns.timeout, exit=True)
    try:
        support.set_match_tests(ns.match_tests)
        # reset the environment_altered flag to detect if a test altered
        # the environment
        support.environment_altered = False
        if ns.failfast:
            support.failfast = True
        if output_on_failure:
            support.verbose = True

            # Reuse the same instance to all calls to runtest(). Some
            # tests keep a reference to sys.stdout or sys.stderr
            # (eg. test_argparse).
            if runtest.stringio is None:
                stream = io.StringIO()
                runtest.stringio = stream
            else:
                stream = runtest.stringio
                stream.seek(0)
                stream.truncate()

            orig_stdout = sys.stdout
            orig_stderr = sys.stderr
            try:
                sys.stdout = stream
                sys.stderr = stream
                result = runtest_inner(ns, test, display_failure=False)
                if result[0] != PASSED:
                    output = stream.getvalue()
                    orig_stderr.write(output)
                    orig_stderr.flush()
            finally:
                sys.stdout = orig_stdout
                sys.stderr = orig_stderr
        else:
            support.verbose = ns.verbose  # Tell tests to be moderately quiet
            result = runtest_inner(ns, test, display_failure=not ns.verbose)
        return result
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()
        cleanup_test_droppings(test, ns.verbose)
runtest.stringio = None


def post_test_cleanup():
    support.reap_children()


def runtest_inner(ns, test, display_failure=True):
    support.unload(test)

    test_time = 0.0
    refleak = False  # True if the test leaked references.
    try:
        abstest = get_abs_module(ns, test)
        clear_caches()
        with saved_test_environment(test, ns.verbose, ns.quiet, pgo=ns.pgo) as environment:
            start_time = time.time()
            the_module = importlib.import_module(abstest)
            # If the test has a test_main, that will run the appropriate
            # tests.  If not, use normal unittest test loading.
            test_runner = getattr(the_module, "test_main", None)
            if test_runner is None:
                def test_runner():
                    loader = unittest.TestLoader()
                    tests = loader.loadTestsFromModule(the_module)
                    for error in loader.errors:
                        print(error, file=sys.stderr)
                    if loader.errors:
                        raise Exception("errors while loading tests")
                    support.run_unittest(tests)
            if ns.huntrleaks:
                refleak = dash_R(the_module, test, test_runner, ns.huntrleaks)
            else:
                test_runner()
            test_time = time.time() - start_time
        post_test_cleanup()
    except support.ResourceDenied as msg:
        if not ns.quiet and not ns.pgo:
            print(test, "skipped --", msg, flush=True)
        return RESOURCE_DENIED, test_time
    except unittest.SkipTest as msg:
        if not ns.quiet and not ns.pgo:
            print(test, "skipped --", msg, flush=True)
        return SKIPPED, test_time
    except KeyboardInterrupt:
        raise
    except support.TestFailed as msg:
        if not ns.pgo:
            if display_failure:
                print("test", test, "failed --", msg, file=sys.stderr,
                      flush=True)
            else:
                print("test", test, "failed", file=sys.stderr, flush=True)
        return FAILED, test_time
    except:
        msg = traceback.format_exc()
        if not ns.pgo:
            print("test", test, "crashed --", msg, file=sys.stderr,
                  flush=True)
        return FAILED, test_time
    else:
        if refleak:
            return FAILED, test_time
        if environment.changed:
            return ENV_CHANGED, test_time
        return PASSED, test_time


def cleanup_test_droppings(testname, verbose):
    import shutil
    import stat
    import gc

    # First kill any dangling references to open files etc.
    # This can also issue some ResourceWarnings which would otherwise get
    # triggered during the following test run, and possibly produce failures.
    gc.collect()

    # Try to clean up junk commonly left behind.  While tests shouldn't leave
    # any files or directories behind, when a test fails that can be tedious
    # for it to arrange.  The consequences can be especially nasty on Windows,
    # since if a test leaves a file open, it cannot be deleted by name (while
    # there's nothing we can do about that here either, we can display the
    # name of the offending test, which is a real help).
    for name in (support.TESTFN,
                 "db_home",
                ):
        if not os.path.exists(name):
            continue

        if os.path.isdir(name):
            kind, nuker = "directory", shutil.rmtree
        elif os.path.isfile(name):
            kind, nuker = "file", os.unlink
        else:
            raise SystemError("os.path says %r exists but is neither "
                              "directory nor file" % name)

        if verbose:
            print("%r left behind %s %r" % (testname, kind, name))
        try:
            # if we have chmod, fix possible permissions problems
            # that might prevent cleanup
            if (hasattr(os, 'chmod')):
                os.chmod(name, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
            nuker(name)
        except Exception as msg:
            print(("%r left behind %s %r and it couldn't be "
                "removed: %s" % (testname, kind, name, msg)), file=sys.stderr)


def findtestdir(path=None):
    return path or os.path.dirname(os.path.dirname(__file__)) or os.curdir

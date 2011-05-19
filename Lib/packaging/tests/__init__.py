"""Test suite for packaging.

This test suite consists of a collection of test modules in the
packaging.tests package.  Each test module has a name starting with
'test' and contains a function test_suite().  The function is expected
to return an initialized unittest.TestSuite instance.

Utility code is included in packaging.tests.support.
"""

# Put this text back for the backport
#Always import unittest from this module, it will be the right version
#(standard library unittest for 3.2 and higher, third-party unittest2
#elease for older versions).

import os
import sys
import unittest
from test.support import TESTFN

# XXX move helpers to support, add tests for them, remove things that
# duplicate test.support (or keep them for the backport; needs thinking)

here = os.path.dirname(__file__) or os.curdir
verbose = 1

def test_suite():
    suite = unittest.TestSuite()
    for fn in os.listdir(here):
        if fn.startswith("test") and fn.endswith(".py"):
            modname = "packaging.tests." + fn[:-3]
            __import__(modname)
            module = sys.modules[modname]
            suite.addTest(module.test_suite())
    return suite


class Error(Exception):
    """Base class for regression test exceptions."""


class TestFailed(Error):
    """Test failed."""


class BasicTestRunner:
    def run(self, test):
        result = unittest.TestResult()
        test(result)
        return result


def _run_suite(suite, verbose_=1):
    """Run tests from a unittest.TestSuite-derived class."""
    global verbose
    verbose = verbose_
    if verbose_:
        runner = unittest.TextTestRunner(sys.stdout, verbosity=2)
    else:
        runner = BasicTestRunner()

    result = runner.run(suite)
    if not result.wasSuccessful():
        if len(result.errors) == 1 and not result.failures:
            err = result.errors[0][1]
        elif len(result.failures) == 1 and not result.errors:
            err = result.failures[0][1]
        else:
            err = "errors occurred; run in verbose mode for details"
        raise TestFailed(err)


def run_unittest(classes, verbose_=1):
    """Run tests from unittest.TestCase-derived classes.

    Originally extracted from stdlib test.test_support and modified to
    support unittest2.
    """
    valid_types = (unittest.TestSuite, unittest.TestCase)
    suite = unittest.TestSuite()
    for cls in classes:
        if isinstance(cls, str):
            if cls in sys.modules:
                suite.addTest(unittest.findTestCases(sys.modules[cls]))
            else:
                raise ValueError("str arguments must be keys in sys.modules")
        elif isinstance(cls, valid_types):
            suite.addTest(cls)
        else:
            suite.addTest(unittest.makeSuite(cls))
    _run_suite(suite, verbose_)


def reap_children():
    """Use this function at the end of test_main() whenever sub-processes
    are started.  This will help ensure that no extra children (zombies)
    stick around to hog resources and create problems when looking
    for refleaks.

    Extracted from stdlib test.support.
    """

    # Reap all our dead child processes so we don't leave zombies around.
    # These hog resources and might be causing some of the buildbots to die.
    if hasattr(os, 'waitpid'):
        any_process = -1
        while True:
            try:
                # This will raise an exception on Windows.  That's ok.
                pid, status = os.waitpid(any_process, os.WNOHANG)
                if pid == 0:
                    break
            except:
                break


def captured_stdout(func, *args, **kw):
    import io
    orig_stdout = getattr(sys, 'stdout')
    setattr(sys, 'stdout', io.StringIO())
    try:
        res = func(*args, **kw)
        sys.stdout.seek(0)
        return res, sys.stdout.read()
    finally:
        setattr(sys, 'stdout', orig_stdout)


def unload(name):
    try:
        del sys.modules[name]
    except KeyError:
        pass

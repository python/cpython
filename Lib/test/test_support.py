"""Supporting definitions for the Python regression test."""

import sys

class Error(Exception):
    """Base class for regression test exceptions."""

class TestFailed(Error):
    """Test failed."""

class TestSkipped(Error):
    """Test skipped.

    This can be raised to indicate that a test was deliberatly
    skipped, but not because a feature wasn't available.  For
    example, if some resource can't be used, such as the network
    appears to be unavailable, this should be raised instead of
    TestFailed.
    """

verbose = 1              # Flag set to 0 by regrtest.py
use_resources = None       # Flag set to [] by regrtest.py

def unload(name):
    try:
        del sys.modules[name]
    except KeyError:
        pass

def forget(modname):
    unload(modname)
    import os
    for dirname in sys.path:
        try:
            os.unlink(os.path.join(dirname, modname + '.pyc'))
        except os.error:
            pass

def requires(resource, msg=None):
    if use_resources is not None and resource not in use_resources:
        if msg is None:
            msg = "Use of the `%s' resource not enabled" % resource
        raise TestSkipped(msg)

FUZZ = 1e-6

def fcmp(x, y): # fuzzy comparison function
    if type(x) == type(0.0) or type(y) == type(0.0):
        try:
            x, y = coerce(x, y)
            fuzz = (abs(x) + abs(y)) * FUZZ
            if abs(x-y) <= fuzz:
                return 0
        except:
            pass
    elif type(x) == type(y) and type(x) in (type(()), type([])):
        for i in range(min(len(x), len(y))):
            outcome = fcmp(x[i], y[i])
            if outcome != 0:
                return outcome
        return cmp(len(x), len(y))
    return cmp(x, y)

try:
    unicode
    have_unicode = 1
except NameError:
    have_unicode = 0

import os
# Filename used for testing
if os.name == 'java':
    # Jython disallows @ in module names
    TESTFN = '$test'
elif os.name != 'riscos':
    TESTFN = '@test'
    # Unicode name only used if TEST_FN_ENCODING exists for the platform.
    if have_unicode:
        TESTFN_UNICODE=unicode("@test-\xe0\xf2", "latin-1") # 2 latin characters.
        if os.name=="nt":
            TESTFN_ENCODING="mbcs"
else:
    TESTFN = 'test'
del os

from os import unlink

def findfile(file, here=__file__):
    import os
    if os.path.isabs(file):
        return file
    path = sys.path
    path = [os.path.dirname(here)] + path
    for dn in path:
        fn = os.path.join(dn, file)
        if os.path.exists(fn): return fn
    return file

def verify(condition, reason='test failed'):
    """Verify that condition is true. If not, raise TestFailed.

       The optional argument reason can be given to provide
       a better error text.
    """

    if not condition:
        raise TestFailed(reason)

def sortdict(dict):
    "Like repr(dict), but in sorted order."
    items = dict.items()
    items.sort()
    reprpairs = ["%r: %r" % pair for pair in items]
    withcommas = ", ".join(reprpairs)
    return "{%s}" % withcommas

def check_syntax(statement):
    try:
        compile(statement, '<string>', 'exec')
    except SyntaxError:
        pass
    else:
        print 'Missing SyntaxError: "%s"' % statement



#=======================================================================
# Preliminary PyUNIT integration.

import unittest


class BasicTestRunner:
    def run(self, test):
        result = unittest.TestResult()
        test(result)
        return result


def run_suite(suite):
    """Run tests from a unittest.TestSuite-derived class."""
    if verbose:
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
            raise TestFailed("errors occurred in %s.%s"
                             % (testclass.__module__, testclass.__name__))
        raise TestFailed(err)


def run_unittest(testclass):
    """Run tests from a unittest.TestCase-derived class."""
    run_suite(unittest.makeSuite(testclass))


#=======================================================================
# doctest driver.

def run_doctest(module, verbosity=None):
    """Run doctest on the given module.

    If optional argument verbosity is not specified (or is None), pass
    test_support's belief about verbosity on to doctest.  Else doctest's
    usual behavior is used (it searches sys.argv for -v).
    """

    import doctest

    if verbosity is None:
        verbosity = verbose
    else:
        verbosity = None

    # Direct doctest output (normally just errors) to real stdout; doctest
    # output shouldn't be compared by regrtest.
    save_stdout = sys.stdout
    sys.stdout = sys.__stdout__
    try:
        f, t = doctest.testmod(module, verbose=verbosity)
        if f:
            raise TestFailed("%d of %d doctests failed" % (f, t))
    finally:
        sys.stdout = save_stdout

"""Supporting definitions for the Python regression tests."""

if __name__ != 'test.test_support':
    raise ImportError, 'test_support must be imported from the test package'

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

class ResourceDenied(TestSkipped):
    """Test skipped because it requested a disallowed resource.

    This is raised when a test calls requires() for a resource that
    has not be enabled.  It is used to distinguish between expected
    and unexpected skips.
    """

verbose = 1              # Flag set to 0 by regrtest.py
use_resources = None       # Flag set to [] by regrtest.py

# _original_stdout is meant to hold stdout at the time regrtest began.
# This may be "the real" stdout, or IDLE's emulation of stdout, or whatever.
# The point is to have some flavor of stdout the user can actually see.
_original_stdout = None
def record_original_stdout(stdout):
    global _original_stdout
    _original_stdout = stdout

def get_original_stdout():
    return _original_stdout or sys.stdout

def unload(name):
    try:
        del sys.modules[name]
    except KeyError:
        pass

def forget(modname):
    '''"Forget" a module was ever imported by removing it from sys.modules and
    deleting any .pyc and .pyo files.'''
    unload(modname)
    import os
    for dirname in sys.path:
        try:
            os.unlink(os.path.join(dirname, modname + os.extsep + 'pyc'))
        except os.error:
            pass
        # Deleting the .pyo file cannot be within the 'try' for the .pyc since
        # the chance exists that there is no .pyc (and thus the 'try' statement
        # is exited) but there is a .pyo file.
        try:
            os.unlink(os.path.join(dirname, modname + os.extsep + 'pyo'))
        except os.error:
            pass

def is_resource_enabled(resource):
    """Test whether a resource is enabled.  Known resources are set by
    regrtest.py."""
    return use_resources is not None and resource in use_resources

def requires(resource, msg=None):
    """Raise ResourceDenied if the specified resource is not available.

    If the caller's module is __main__ then automatically return True.  The
    possibility of False being returned occurs when regrtest.py is executing."""
    # see if the caller's module is __main__ - if so, treat as if
    # the resource was set
    if sys._getframe().f_back.f_globals.get("__name__") == "__main__":
        return
    if not is_resource_enabled(resource):
        if msg is None:
            msg = "Use of the `%s' resource not enabled" % resource
        raise ResourceDenied(msg)

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

is_jython = sys.platform.startswith('java')

import os
# Filename used for testing
if os.name == 'java':
    # Jython disallows @ in module names
    TESTFN = '$test'
elif os.name == 'riscos':
    TESTFN = 'testfile'
else:
    TESTFN = '@test'
    # Unicode name only used if TEST_FN_ENCODING exists for the platform.
    if have_unicode:
        # Assuming sys.getfilesystemencoding()!=sys.getdefaultencoding()
        # TESTFN_UNICODE is a filename that can be encoded using the
        # file system encoding, but *not* with the default (ascii) encoding
        if isinstance('', unicode):
            # python -U
            # XXX perhaps unicode() should accept Unicode strings?
            TESTFN_UNICODE = "@test-\xe0\xf2"
        else:
            # 2 latin characters.
            TESTFN_UNICODE = unicode("@test-\xe0\xf2", "latin-1")
        TESTFN_ENCODING = sys.getfilesystemencoding()
        # TESTFN_UNICODE_UNENCODEABLE is a filename that should *not* be
        # able to be encoded by *either* the default or filesystem encoding.
        # This test really only makes sense on Windows NT platforms
        # which have special Unicode support in posixmodule.
        if (not hasattr(sys, "getwindowsversion") or
                sys.getwindowsversion()[3] < 2): #  0=win32s or 1=9x/ME
            TESTFN_UNICODE_UNENCODEABLE = None
        else:
            # Japanese characters (I think - from bug 846133)
            TESTFN_UNICODE_UNENCODEABLE = eval('u"@test-\u5171\u6709\u3055\u308c\u308b"')
            try:
                # XXX - Note - should be using TESTFN_ENCODING here - but for
                # Windows, "mbcs" currently always operates as if in
                # errors=ignore' mode - hence we get '?' characters rather than
                # the exception.  'Latin1' operates as we expect - ie, fails.
                # See [ 850997 ] mbcs encoding ignores errors
                TESTFN_UNICODE_UNENCODEABLE.encode("Latin1")
            except UnicodeEncodeError:
                pass
            else:
                print \
                'WARNING: The filename %r CAN be encoded by the filesystem.  ' \
                'Unicode filename tests may not be effective' \
                % TESTFN_UNICODE_UNENCODEABLE

# Make sure we can write to TESTFN, try in /tmp if we can't
fp = None
try:
    fp = open(TESTFN, 'w+')
except IOError:
    TMP_TESTFN = os.path.join('/tmp', TESTFN)
    try:
        fp = open(TMP_TESTFN, 'w+')
        TESTFN = TMP_TESTFN
        del TMP_TESTFN
    except IOError:
        print ('WARNING: tests will fail, unable to write to: %s or %s' %
                (TESTFN, TMP_TESTFN))
if fp is not None:
    fp.close()
    try:
        os.unlink(TESTFN)
    except:
        pass
del os, fp

from os import unlink

def findfile(file, here=__file__):
    """Try to find a file on sys.path and the working directory.  If it is not
    found the argument passed to the function is returned (this does not
    necessarily signal failure; could still be the legitimate path)."""
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

def vereq(a, b):
    """Raise TestFailed if a == b is false.

    This is better than verify(a == b) because, in case of failure, the
    error message incorporates repr(a) and repr(b) so you can see the
    inputs.

    Note that "not (a == b)" isn't necessarily the same as "a != b"; the
    former is tested.
    """

    if not (a == b):
        raise TestFailed, "%r == %r" % (a, b)

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


def run_suite(suite, testclass=None):
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
            if testclass is None:
                msg = "errors occurred; run in verbose mode for details"
            else:
                msg = "errors occurred in %s.%s" \
                      % (testclass.__module__, testclass.__name__)
            raise TestFailed(msg)
        raise TestFailed(err)


def run_unittest(*classes):
    """Run tests from unittest.TestCase-derived classes."""
    suite = unittest.TestSuite()
    for cls in classes:
        if isinstance(cls, (unittest.TestSuite, unittest.TestCase)):
            suite.addTest(cls)
        else:
            suite.addTest(unittest.makeSuite(cls))
    if len(classes)==1:
        testclass = classes[0]
    else:
        testclass = None
    run_suite(suite, testclass)


#=======================================================================
# doctest driver.

def run_doctest(module, verbosity=None):
    """Run doctest on the given module.  Return (#failures, #tests).

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
    sys.stdout = get_original_stdout()
    try:
        f, t = doctest.testmod(module, verbose=verbosity)
        if f:
            raise TestFailed("%d of %d doctests failed" % (f, t))
    finally:
        sys.stdout = save_stdout
    if verbose:
        print 'doctest (%s) ... %d tests with zero failures' % (module.__name__, t)
    return f, t

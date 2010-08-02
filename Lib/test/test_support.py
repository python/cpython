"""Supporting definitions for the Python regression tests."""

if __name__ != 'test.test_support':
    raise ImportError('test_support must be imported from the test package')

import contextlib
import errno
import socket
import sys
import os
import shutil
import warnings
import unittest
import re

__all__ = ["Error", "TestFailed", "TestSkipped", "ResourceDenied", "import_module",
           "verbose", "use_resources", "max_memuse", "record_original_stdout",
           "get_original_stdout", "unload", "unlink", "rmtree", "forget",
           "is_resource_enabled", "requires", "find_unused_port", "bind_port",
           "fcmp", "have_unicode", "is_jython", "TESTFN", "HOST", "FUZZ",
           "findfile", "verify", "vereq", "sortdict", "check_syntax_error",
           "open_urlresource", "check_warnings", "_check_py3k_warnings",
           "CleanImport", "EnvironmentVarGuard", "captured_output",
           "captured_stdout", "TransientResource", "transient_internet",
           "run_with_locale", "set_memlimit", "bigmemtest", "bigaddrspacetest",
           "BasicTestRunner", "run_unittest", "run_doctest", "threading_setup",
           "threading_cleanup", "reap_children"]

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

def import_module(name, deprecated=False):
    """Import the module to be tested, raising TestSkipped if it is not
    available."""
    with warnings.catch_warnings():
        if deprecated:
            warnings.filterwarnings("ignore", ".+ (module|package)",
                                    DeprecationWarning)
        try:
            module = __import__(name, level=0)
        except ImportError:
            raise TestSkipped("No module named " + name)
        else:
            return module

verbose = 1              # Flag set to 0 by regrtest.py
use_resources = None     # Flag set to [] by regrtest.py
max_memuse = 0           # Disable bigmem tests (they will still be run with
                         # small sizes, to make sure they work.)
real_max_memuse = 0

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

def unlink(filename):
    try:
        os.unlink(filename)
    except OSError:
        pass

def rmtree(path):
    try:
        shutil.rmtree(path)
    except OSError, e:
        # Unix returns ENOENT, Windows returns ESRCH.
        if e.errno not in (errno.ENOENT, errno.ESRCH):
            raise

def forget(modname):
    '''"Forget" a module was ever imported by removing it from sys.modules and
    deleting any .pyc and .pyo files.'''
    unload(modname)
    for dirname in sys.path:
        unlink(os.path.join(dirname, modname + os.extsep + 'pyc'))
        # Deleting the .pyo file cannot be within the 'try' for the .pyc since
        # the chance exists that there is no .pyc (and thus the 'try' statement
        # is exited) but there is a .pyo file.
        unlink(os.path.join(dirname, modname + os.extsep + 'pyo'))

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

HOST = 'localhost'

def find_unused_port(family=socket.AF_INET, socktype=socket.SOCK_STREAM):
    """Returns an unused port that should be suitable for binding.  This is
    achieved by creating a temporary socket with the same family and type as
    the 'sock' parameter (default is AF_INET, SOCK_STREAM), and binding it to
    the specified host address (defaults to 0.0.0.0) with the port set to 0,
    eliciting an unused ephemeral port from the OS.  The temporary socket is
    then closed and deleted, and the ephemeral port is returned.

    Either this method or bind_port() should be used for any tests where a
    server socket needs to be bound to a particular port for the duration of
    the test.  Which one to use depends on whether the calling code is creating
    a python socket, or if an unused port needs to be provided in a constructor
    or passed to an external program (i.e. the -accept argument to openssl's
    s_server mode).  Always prefer bind_port() over find_unused_port() where
    possible.  Hard coded ports should *NEVER* be used.  As soon as a server
    socket is bound to a hard coded port, the ability to run multiple instances
    of the test simultaneously on the same host is compromised, which makes the
    test a ticking time bomb in a buildbot environment. On Unix buildbots, this
    may simply manifest as a failed test, which can be recovered from without
    intervention in most cases, but on Windows, the entire python process can
    completely and utterly wedge, requiring someone to log in to the buildbot
    and manually kill the affected process.

    (This is easy to reproduce on Windows, unfortunately, and can be traced to
    the SO_REUSEADDR socket option having different semantics on Windows versus
    Unix/Linux.  On Unix, you can't have two AF_INET SOCK_STREAM sockets bind,
    listen and then accept connections on identical host/ports.  An EADDRINUSE
    socket.error will be raised at some point (depending on the platform and
    the order bind and listen were called on each socket).

    However, on Windows, if SO_REUSEADDR is set on the sockets, no EADDRINUSE
    will ever be raised when attempting to bind two identical host/ports. When
    accept() is called on each socket, the second caller's process will steal
    the port from the first caller, leaving them both in an awkwardly wedged
    state where they'll no longer respond to any signals or graceful kills, and
    must be forcibly killed via OpenProcess()/TerminateProcess().

    The solution on Windows is to use the SO_EXCLUSIVEADDRUSE socket option
    instead of SO_REUSEADDR, which effectively affords the same semantics as
    SO_REUSEADDR on Unix.  Given the propensity of Unix developers in the Open
    Source world compared to Windows ones, this is a common mistake.  A quick
    look over OpenSSL's 0.9.8g source shows that they use SO_REUSEADDR when
    openssl.exe is called with the 's_server' option, for example. See
    http://bugs.python.org/issue2550 for more info.  The following site also
    has a very thorough description about the implications of both REUSEADDR
    and EXCLUSIVEADDRUSE on Windows:
    http://msdn2.microsoft.com/en-us/library/ms740621(VS.85).aspx)

    XXX: although this approach is a vast improvement on previous attempts to
    elicit unused ports, it rests heavily on the assumption that the ephemeral
    port returned to us by the OS won't immediately be dished back out to some
    other process when we close and delete our temporary socket but before our
    calling code has a chance to bind the returned port.  We can deal with this
    issue if/when we come across it."""
    tempsock = socket.socket(family, socktype)
    port = bind_port(tempsock)
    tempsock.close()
    del tempsock
    return port

def bind_port(sock, host=HOST):
    """Bind the socket to a free port and return the port number.  Relies on
    ephemeral ports in order to ensure we are using an unbound port.  This is
    important as many tests may be running simultaneously, especially in a
    buildbot environment.  This method raises an exception if the sock.family
    is AF_INET and sock.type is SOCK_STREAM, *and* the socket has SO_REUSEADDR
    or SO_REUSEPORT set on it.  Tests should *never* set these socket options
    for TCP/IP sockets.  The only case for setting these options is testing
    multicasting via multiple UDP sockets.

    Additionally, if the SO_EXCLUSIVEADDRUSE socket option is available (i.e.
    on Windows), it will be set on the socket.  This will prevent anyone else
    from bind()'ing to our host/port for the duration of the test.
    """
    if sock.family == socket.AF_INET and sock.type == socket.SOCK_STREAM:
        if hasattr(socket, 'SO_REUSEADDR'):
            if sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR) == 1:
                raise TestFailed("tests should never set the SO_REUSEADDR "   \
                                 "socket option on TCP/IP sockets!")
        if hasattr(socket, 'SO_REUSEPORT'):
            if sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT) == 1:
                raise TestFailed("tests should never set the SO_REUSEPORT "   \
                                 "socket option on TCP/IP sockets!")
        if hasattr(socket, 'SO_EXCLUSIVEADDRUSE'):
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)

    sock.bind((host, 0))
    port = sock.getsockname()[1]
    return port

FUZZ = 1e-6

def fcmp(x, y): # fuzzy comparison function
    if isinstance(x, float) or isinstance(y, float):
        try:
            fuzz = (abs(x) + abs(y)) * FUZZ
            if abs(x-y) <= fuzz:
                return 0
        except:
            pass
    elif type(x) == type(y) and isinstance(x, (tuple, list)):
        for i in range(min(len(x), len(y))):
            outcome = fcmp(x[i], y[i])
            if outcome != 0:
                return outcome
        return (len(x) > len(y)) - (len(x) < len(y))
    return (x > y) - (x < y)

try:
    unicode
    have_unicode = True
except NameError:
    have_unicode = False

is_jython = sys.platform.startswith('java')

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
    unlink(TESTFN)
del fp

def findfile(file, here=__file__):
    """Try to find a file on sys.path and the working directory.  If it is not
    found the argument passed to the function is returned (this does not
    necessarily signal failure; could still be the legitimate path)."""
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
        raise TestFailed("%r == %r" % (a, b))

def sortdict(dict):
    "Like repr(dict), but in sorted order."
    items = dict.items()
    items.sort()
    reprpairs = ["%r: %r" % pair for pair in items]
    withcommas = ", ".join(reprpairs)
    return "{%s}" % withcommas

def make_bad_fd():
    """
    Create an invalid file descriptor by opening and closing a file and return
    its fd.
    """
    file = open(TESTFN, "wb")
    try:
        return file.fileno()
    finally:
        file.close()
        unlink(TESTFN)

def check_syntax_error(testcase, statement):
    try:
        compile(statement, '<test string>', 'exec')
    except SyntaxError:
        pass
    else:
        testcase.fail('Missing SyntaxError: "%s"' % statement)

def open_urlresource(url):
    import urlparse, urllib2

    requires('urlfetch')
    filename = urlparse.urlparse(url)[2].split('/')[-1] # '/': it's URL!

    for path in [os.path.curdir, os.path.pardir]:
        fn = os.path.join(path, filename)
        if os.path.exists(fn):
            return open(fn)

    print >> get_original_stdout(), '\tfetching %s ...' % url
    f = urllib2.urlopen(url, timeout=15)
    try:
        with open(filename, "wb") as out:
            s = f.read()
            while s:
                out.write(s)
                s = f.read()
    finally:
        f.close()
    return open(filename)


class WarningsRecorder(object):
    """Convenience wrapper for the warnings list returned on
       entry to the warnings.catch_warnings() context manager.
    """
    def __init__(self, warnings_list):
        self._warnings = warnings_list
        self._last = 0

    def __getattr__(self, attr):
        if len(self._warnings) > self._last:
            return getattr(self._warnings[-1], attr)
        elif attr in warnings.WarningMessage._WARNING_DETAILS:
            return None
        raise AttributeError("%r has no attribute %r" % (self, attr))

    @property
    def warnings(self):
        return self._warnings[self._last:]

    def reset(self):
        self._last = len(self._warnings)


def _filterwarnings(filters, quiet=False):
    """Catch the warnings, then check if all the expected
    warnings have been raised and re-raise unexpected warnings.
    If 'quiet' is True, only re-raise the unexpected warnings.
    """
    # Clear the warning registry of the calling module
    # in order to re-raise the warnings.
    frame = sys._getframe(2)
    registry = frame.f_globals.get('__warningregistry__')
    if registry:
        registry.clear()
    with warnings.catch_warnings(record=True) as w:
        # Set filter "always" to record all warnings.  Because
        # test_warnings swap the module, we need to look up in
        # the sys.modules dictionary.
        sys.modules['warnings'].simplefilter("always")
        yield WarningsRecorder(w)
    # Filter the recorded warnings
    reraise = [warning.message for warning in w]
    missing = []
    for msg, cat in filters:
        seen = False
        for exc in reraise[:]:
            message = str(exc)
            # Filter out the matching messages
            if (re.match(msg, message, re.I) and
                issubclass(exc.__class__, cat)):
                seen = True
                reraise.remove(exc)
        if not seen and not quiet:
            # This filter caught nothing
            missing.append((msg, cat.__name__))
    if reraise:
        raise AssertionError("unhandled warning %r" % reraise[0])
    if missing:
        raise AssertionError("filter (%r, %s) did not catch any warning" %
                             missing[0])


@contextlib.contextmanager
def check_warnings(*filters, **kwargs):
    """Context manager to silence warnings.

    Accept 2-tuples as positional arguments:
        ("message regexp", WarningCategory)

    Optional argument:
     - if 'quiet' is True, it does not fail if a filter catches nothing
        (default True without argument,
         default False if some filters are defined)

    Without argument, it defaults to:
        check_warnings(("", Warning), quiet=True)
    """
    quiet = kwargs.get('quiet')
    if not filters:
        filters = (("", Warning),)
        # Preserve backward compatibility
        if quiet is None:
            quiet = True
    return _filterwarnings(filters, quiet)


@contextlib.contextmanager
def _check_py3k_warnings(*filters, **kwargs):
    """Context manager to silence py3k warnings.

    Accept 2-tuples as positional arguments:
        ("message regexp", WarningCategory)

    Optional argument:
     - if 'quiet' is True, it does not fail if a filter catches nothing
        (default False)

    Without argument, it defaults to:
        _check_py3k_warnings(("", DeprecationWarning), quiet=False)
    """
    if sys.py3kwarning:
        if not filters:
            filters = (("", DeprecationWarning),)
    else:
        # It should not raise any py3k warning
        filters = ()
    return _filterwarnings(filters, kwargs.get('quiet'))


class CleanImport(object):
    """Context manager to force import to return a new module reference.

    This is useful for testing module-level behaviours, such as
    the emission of a DeprecationWarning on import.

    Use like this:

        with CleanImport("foo"):
            __import__("foo") # new reference
    """

    def __init__(self, *module_names):
        self.original_modules = sys.modules.copy()
        for module_name in module_names:
            if module_name in sys.modules:
                module = sys.modules[module_name]
                # It is possible that module_name is just an alias for
                # another module (e.g. stub for modules renamed in 3.x).
                # In that case, we also need delete the real module to clear
                # the import cache.
                if module.__name__ != module_name:
                    del sys.modules[module.__name__]
                del sys.modules[module_name]

    def __enter__(self):
        return self

    def __exit__(self, *ignore_exc):
        sys.modules.update(self.original_modules)


class EnvironmentVarGuard(object):

    """Class to help protect the environment variable properly.  Can be used as
    a context manager."""

    def __init__(self):
        self._changed = {}

    def set(self, envvar, value):
        # Remember the initial value on the first access
        if envvar not in self._changed:
            self._changed[envvar] = os.environ.get(envvar)
        os.environ[envvar] = value

    def unset(self, envvar):
        # Remember the initial value on the first access
        if envvar not in self._changed:
            self._changed[envvar] = os.environ.get(envvar)
        if envvar in os.environ:
            del os.environ[envvar]

    def __enter__(self):
        return self

    def __exit__(self, *ignore_exc):
        for (k, v) in self._changed.items():
            if v is None:
                if k in os.environ:
                    del os.environ[k]
            else:
                os.environ[k] = v


class TransientResource(object):

    """Raise ResourceDenied if an exception is raised while the context manager
    is in effect that matches the specified exception and attributes."""

    def __init__(self, exc, **kwargs):
        self.exc = exc
        self.attrs = kwargs

    def __enter__(self):
        return self

    def __exit__(self, type_=None, value=None, traceback=None):
        """If type_ is a subclass of self.exc and value has attributes matching
        self.attrs, raise ResourceDenied.  Otherwise let the exception
        propagate (if any)."""
        if type_ is not None and issubclass(self.exc, type_):
            for attr, attr_value in self.attrs.iteritems():
                if not hasattr(value, attr):
                    break
                if getattr(value, attr) != attr_value:
                    break
            else:
                raise ResourceDenied("an optional resource is not available")


def transient_internet():
    """Return a context manager that raises ResourceDenied when various issues
    with the Internet connection manifest themselves as exceptions."""
    time_out = TransientResource(IOError, errno=errno.ETIMEDOUT)
    socket_peer_reset = TransientResource(socket.error, errno=errno.ECONNRESET)
    ioerror_peer_reset = TransientResource(IOError, errno=errno.ECONNRESET)
    return contextlib.nested(time_out, socket_peer_reset, ioerror_peer_reset)


@contextlib.contextmanager
def captured_output(stream_name):
    """Run the 'with' statement body using a StringIO object in place of a
    specific attribute on the sys module.
    Example use (with 'stream_name=stdout')::

       with captured_stdout() as s:
           print "hello"
       assert s.getvalue() == "hello"
    """
    import StringIO
    orig_stdout = getattr(sys, stream_name)
    setattr(sys, stream_name, StringIO.StringIO())
    try:
        yield getattr(sys, stream_name)
    finally:
        setattr(sys, stream_name, orig_stdout)

def captured_stdout():
    return captured_output("stdout")


#=======================================================================
# Decorator for running a function in a different locale, correctly resetting
# it afterwards.

def run_with_locale(catstr, *locales):
    def decorator(func):
        def inner(*args, **kwds):
            try:
                import locale
                category = getattr(locale, catstr)
                orig_locale = locale.setlocale(category)
            except AttributeError:
                # if the test author gives us an invalid category string
                raise
            except:
                # cannot retrieve original locale, so do nothing
                locale = orig_locale = None
            else:
                for loc in locales:
                    try:
                        locale.setlocale(category, loc)
                        break
                    except:
                        pass

            # now run the function, resetting the locale on exceptions
            try:
                return func(*args, **kwds)
            finally:
                if locale and orig_locale:
                    locale.setlocale(category, orig_locale)
        inner.func_name = func.func_name
        inner.__doc__ = func.__doc__
        return inner
    return decorator

#=======================================================================
# Big-memory-test support. Separate from 'resources' because memory use should be configurable.

# Some handy shorthands. Note that these are used for byte-limits as well
# as size-limits, in the various bigmem tests
_1M = 1024*1024
_1G = 1024 * _1M
_2G = 2 * _1G
_4G = 4 * _1G

MAX_Py_ssize_t = sys.maxsize

def set_memlimit(limit):
    global max_memuse
    global real_max_memuse
    sizes = {
        'k': 1024,
        'm': _1M,
        'g': _1G,
        't': 1024*_1G,
    }
    m = re.match(r'(\d+(\.\d+)?) (K|M|G|T)b?$', limit,
                 re.IGNORECASE | re.VERBOSE)
    if m is None:
        raise ValueError('Invalid memory limit %r' % (limit,))
    memlimit = int(float(m.group(1)) * sizes[m.group(3).lower()])
    real_max_memuse = memlimit
    if memlimit > MAX_Py_ssize_t:
        memlimit = MAX_Py_ssize_t
    if memlimit < _2G - 1:
        raise ValueError('Memory limit %r too low to be useful' % (limit,))
    max_memuse = memlimit

def bigmemtest(minsize, memuse, overhead=5*_1M):
    """Decorator for bigmem tests.

    'minsize' is the minimum useful size for the test (in arbitrary,
    test-interpreted units.) 'memuse' is the number of 'bytes per size' for
    the test, or a good estimate of it. 'overhead' specifies fixed overhead,
    independent of the testsize, and defaults to 5Mb.

    The decorator tries to guess a good value for 'size' and passes it to
    the decorated test function. If minsize * memuse is more than the
    allowed memory use (as defined by max_memuse), the test is skipped.
    Otherwise, minsize is adjusted upward to use up to max_memuse.
    """
    def decorator(f):
        def wrapper(self):
            if not max_memuse:
                # If max_memuse is 0 (the default),
                # we still want to run the tests with size set to a few kb,
                # to make sure they work. We still want to avoid using
                # too much memory, though, but we do that noisily.
                maxsize = 5147
                self.failIf(maxsize * memuse + overhead > 20 * _1M)
            else:
                maxsize = int((max_memuse - overhead) / memuse)
                if maxsize < minsize:
                    # Really ought to print 'test skipped' or something
                    if verbose:
                        sys.stderr.write("Skipping %s because of memory "
                                         "constraint\n" % (f.__name__,))
                    return
                # Try to keep some breathing room in memory use
                maxsize = max(maxsize - 50 * _1M, minsize)
            return f(self, maxsize)
        wrapper.minsize = minsize
        wrapper.memuse = memuse
        wrapper.overhead = overhead
        return wrapper
    return decorator

def precisionbigmemtest(size, memuse, overhead=5*_1M):
    def decorator(f):
        def wrapper(self):
            if not real_max_memuse:
                maxsize = 5147
            else:
                maxsize = size

                if real_max_memuse and real_max_memuse < maxsize * memuse:
                    if verbose:
                        sys.stderr.write("Skipping %s because of memory "
                                         "constraint\n" % (f.__name__,))
                    return

            return f(self, maxsize)
        wrapper.size = size
        wrapper.memuse = memuse
        wrapper.overhead = overhead
        return wrapper
    return decorator

def bigaddrspacetest(f):
    """Decorator for tests that fill the address space."""
    def wrapper(self):
        if max_memuse < MAX_Py_ssize_t:
            if verbose:
                sys.stderr.write("Skipping %s because of memory "
                                 "constraint\n" % (f.__name__,))
        else:
            return f(self)
    return wrapper

#=======================================================================
# unittest integration.

class BasicTestRunner:
    def run(self, test):
        result = unittest.TestResult()
        test(result)
        return result


def _run_suite(suite):
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
            err = "multiple errors occurred"
            if not verbose:
                err += "; run in verbose mode for details"
        raise TestFailed(err)


def run_unittest(*classes):
    """Run tests from unittest.TestCase-derived classes."""
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
    _run_suite(suite)


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

#=======================================================================
# Threading support to prevent reporting refleaks when running regrtest.py -R

def threading_setup():
    import threading
    return len(threading._active), len(threading._limbo)

def threading_cleanup(num_active, num_limbo):
    import threading
    import time

    _MAX_COUNT = 10
    count = 0
    while len(threading._active) != num_active and count < _MAX_COUNT:
        count += 1
        time.sleep(0.1)

    count = 0
    while len(threading._limbo) != num_limbo and count < _MAX_COUNT:
        count += 1
        time.sleep(0.1)

def reap_children():
    """Use this function at the end of test_main() whenever sub-processes
    are started.  This will help ensure that no extra children (zombies)
    stick around to hog resources and create problems when looking
    for refleaks.
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

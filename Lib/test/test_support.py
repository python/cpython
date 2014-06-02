"""Supporting definitions for the Python regression tests."""

if __name__ != 'test.test_support':
    raise ImportError('test_support must be imported from the test package')

import contextlib
import errno
import functools
import gc
import socket
import sys
import os
import platform
import shutil
import warnings
import unittest
import importlib
import UserDict
import re
import time
import struct
import sysconfig
try:
    import thread
except ImportError:
    thread = None

__all__ = ["Error", "TestFailed", "ResourceDenied", "import_module",
           "verbose", "use_resources", "max_memuse", "record_original_stdout",
           "get_original_stdout", "unload", "unlink", "rmtree", "forget",
           "is_resource_enabled", "requires", "find_unused_port", "bind_port",
           "fcmp", "have_unicode", "is_jython", "TESTFN", "HOST", "FUZZ",
           "SAVEDCWD", "temp_cwd", "findfile", "sortdict", "check_syntax_error",
           "open_urlresource", "check_warnings", "check_py3k_warnings",
           "CleanImport", "EnvironmentVarGuard", "captured_output",
           "captured_stdout", "TransientResource", "transient_internet",
           "run_with_locale", "set_memlimit", "bigmemtest", "bigaddrspacetest",
           "BasicTestRunner", "run_unittest", "run_doctest", "threading_setup",
           "threading_cleanup", "reap_children", "cpython_only",
           "check_impl_detail", "get_attribute", "py3k_bytes",
           "import_fresh_module", "threading_cleanup", "reap_children",
           "strip_python_stderr"]

class Error(Exception):
    """Base class for regression test exceptions."""

class TestFailed(Error):
    """Test failed."""

class ResourceDenied(unittest.SkipTest):
    """Test skipped because it requested a disallowed resource.

    This is raised when a test calls requires() for a resource that
    has not been enabled.  It is used to distinguish between expected
    and unexpected skips.
    """

@contextlib.contextmanager
def _ignore_deprecated_imports(ignore=True):
    """Context manager to suppress package and module deprecation
    warnings when importing them.

    If ignore is False, this context manager has no effect."""
    if ignore:
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", ".+ (module|package)",
                                    DeprecationWarning)
            yield
    else:
        yield


def import_module(name, deprecated=False):
    """Import and return the module to be tested, raising SkipTest if
    it is not available.

    If deprecated is True, any module or package deprecation messages
    will be suppressed."""
    with _ignore_deprecated_imports(deprecated):
        try:
            return importlib.import_module(name)
        except ImportError, msg:
            raise unittest.SkipTest(str(msg))


def _save_and_remove_module(name, orig_modules):
    """Helper function to save and remove a module from sys.modules

       Raise ImportError if the module can't be imported."""
    # try to import the module and raise an error if it can't be imported
    if name not in sys.modules:
        __import__(name)
        del sys.modules[name]
    for modname in list(sys.modules):
        if modname == name or modname.startswith(name + '.'):
            orig_modules[modname] = sys.modules[modname]
            del sys.modules[modname]

def _save_and_block_module(name, orig_modules):
    """Helper function to save and block a module in sys.modules

       Return True if the module was in sys.modules, False otherwise."""
    saved = True
    try:
        orig_modules[name] = sys.modules[name]
    except KeyError:
        saved = False
    sys.modules[name] = None
    return saved


def import_fresh_module(name, fresh=(), blocked=(), deprecated=False):
    """Imports and returns a module, deliberately bypassing the sys.modules cache
    and importing a fresh copy of the module. Once the import is complete,
    the sys.modules cache is restored to its original state.

    Modules named in fresh are also imported anew if needed by the import.
    If one of these modules can't be imported, None is returned.

    Importing of modules named in blocked is prevented while the fresh import
    takes place.

    If deprecated is True, any module or package deprecation messages
    will be suppressed."""
    # NOTE: test_heapq, test_json, and test_warnings include extra sanity
    # checks to make sure that this utility function is working as expected
    with _ignore_deprecated_imports(deprecated):
        # Keep track of modules saved for later restoration as well
        # as those which just need a blocking entry removed
        orig_modules = {}
        names_to_remove = []
        _save_and_remove_module(name, orig_modules)
        try:
            for fresh_name in fresh:
                _save_and_remove_module(fresh_name, orig_modules)
            for blocked_name in blocked:
                if not _save_and_block_module(blocked_name, orig_modules):
                    names_to_remove.append(blocked_name)
            fresh_module = importlib.import_module(name)
        except ImportError:
            fresh_module = None
        finally:
            for orig_name, module in orig_modules.items():
                sys.modules[orig_name] = module
            for name_to_remove in names_to_remove:
                del sys.modules[name_to_remove]
        return fresh_module


def get_attribute(obj, name):
    """Get an attribute, raising SkipTest if AttributeError is raised."""
    try:
        attribute = getattr(obj, name)
    except AttributeError:
        raise unittest.SkipTest("module %s has no attribute %s" % (
            obj.__name__, name))
    else:
        return attribute


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

if sys.platform.startswith("win"):
    def _waitfor(func, pathname, waitall=False):
        # Perform the operation
        func(pathname)
        # Now setup the wait loop
        if waitall:
            dirname = pathname
        else:
            dirname, name = os.path.split(pathname)
            dirname = dirname or '.'
        # Check for `pathname` to be removed from the filesystem.
        # The exponential backoff of the timeout amounts to a total
        # of ~1 second after which the deletion is probably an error
        # anyway.
        # Testing on a i7@4.3GHz shows that usually only 1 iteration is
        # required when contention occurs.
        timeout = 0.001
        while timeout < 1.0:
            # Note we are only testing for the existence of the file(s) in
            # the contents of the directory regardless of any security or
            # access rights.  If we have made it this far, we have sufficient
            # permissions to do that much using Python's equivalent of the
            # Windows API FindFirstFile.
            # Other Windows APIs can fail or give incorrect results when
            # dealing with files that are pending deletion.
            L = os.listdir(dirname)
            if not (L if waitall else name in L):
                return
            # Increase the timeout and try again
            time.sleep(timeout)
            timeout *= 2
        warnings.warn('tests may fail, delete still pending for ' + pathname,
                      RuntimeWarning, stacklevel=4)

    def _unlink(filename):
        _waitfor(os.unlink, filename)

    def _rmdir(dirname):
        _waitfor(os.rmdir, dirname)

    def _rmtree(path):
        def _rmtree_inner(path):
            for name in os.listdir(path):
                fullname = os.path.join(path, name)
                if os.path.isdir(fullname):
                    _waitfor(_rmtree_inner, fullname, waitall=True)
                    os.rmdir(fullname)
                else:
                    os.unlink(fullname)
        _waitfor(_rmtree_inner, path, waitall=True)
        _waitfor(os.rmdir, path)
else:
    _unlink = os.unlink
    _rmdir = os.rmdir
    _rmtree = shutil.rmtree

def unlink(filename):
    try:
        _unlink(filename)
    except OSError:
        pass

def rmdir(dirname):
    try:
        _rmdir(dirname)
    except OSError as error:
        # The directory need not exist.
        if error.errno != errno.ENOENT:
            raise

def rmtree(path):
    try:
        _rmtree(path)
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

# Check whether a gui is actually available
def _is_gui_available():
    if hasattr(_is_gui_available, 'result'):
        return _is_gui_available.result
    reason = None
    if sys.platform.startswith('win'):
        # if Python is running as a service (such as the buildbot service),
        # gui interaction may be disallowed
        import ctypes
        import ctypes.wintypes
        UOI_FLAGS = 1
        WSF_VISIBLE = 0x0001
        class USEROBJECTFLAGS(ctypes.Structure):
            _fields_ = [("fInherit", ctypes.wintypes.BOOL),
                        ("fReserved", ctypes.wintypes.BOOL),
                        ("dwFlags", ctypes.wintypes.DWORD)]
        dll = ctypes.windll.user32
        h = dll.GetProcessWindowStation()
        if not h:
            raise ctypes.WinError()
        uof = USEROBJECTFLAGS()
        needed = ctypes.wintypes.DWORD()
        res = dll.GetUserObjectInformationW(h,
            UOI_FLAGS,
            ctypes.byref(uof),
            ctypes.sizeof(uof),
            ctypes.byref(needed))
        if not res:
            raise ctypes.WinError()
        if not bool(uof.dwFlags & WSF_VISIBLE):
            reason = "gui not available (WSF_VISIBLE flag not set)"
    elif sys.platform == 'darwin':
        # The Aqua Tk implementations on OS X can abort the process if
        # being called in an environment where a window server connection
        # cannot be made, for instance when invoked by a buildbot or ssh
        # process not running under the same user id as the current console
        # user.  To avoid that, raise an exception if the window manager
        # connection is not available.
        from ctypes import cdll, c_int, pointer, Structure
        from ctypes.util import find_library

        app_services = cdll.LoadLibrary(find_library("ApplicationServices"))

        if app_services.CGMainDisplayID() == 0:
            reason = "gui tests cannot run without OS X window manager"
        else:
            class ProcessSerialNumber(Structure):
                _fields_ = [("highLongOfPSN", c_int),
                            ("lowLongOfPSN", c_int)]
            psn = ProcessSerialNumber()
            psn_p = pointer(psn)
            if (  (app_services.GetCurrentProcess(psn_p) < 0) or
                  (app_services.SetFrontProcess(psn_p) < 0) ):
                reason = "cannot run without OS X gui process"

    # check on every platform whether tkinter can actually do anything
    # but skip the test on OS X because it can cause segfaults in Cocoa Tk
    # when running regrtest with the -j option (multiple threads/subprocesses)
    if (not reason) and (sys.platform != 'darwin'):
        try:
            from Tkinter import Tk
            root = Tk()
            root.destroy()
        except Exception as e:
            err_string = str(e)
            if len(err_string) > 50:
                err_string = err_string[:50] + ' [...]'
            reason = 'Tk unavailable due to {}: {}'.format(type(e).__name__,
                                                           err_string)

    _is_gui_available.reason = reason
    _is_gui_available.result = not reason

    return _is_gui_available.result

def is_resource_enabled(resource):
    """Test whether a resource is enabled.

    Known resources are set by regrtest.py.  If not running under regrtest.py,
    all resources are assumed enabled unless use_resources has been set.
    """
    return use_resources is None or resource in use_resources

def requires(resource, msg=None):
    """Raise ResourceDenied if the specified resource is not available."""
    if resource == 'gui' and not _is_gui_available():
        raise ResourceDenied(_is_gui_available.reason)
    if not is_resource_enabled(resource):
        if msg is None:
            msg = "Use of the `%s' resource not enabled" % resource
        raise ResourceDenied(msg)


# Don't use "localhost", since resolving it uses the DNS under recent
# Windows versions (see issue #18792).
HOST = "127.0.0.1"
HOSTv6 = "::1"


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
            try:
                if sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT) == 1:
                    raise TestFailed("tests should never set the SO_REUSEPORT "   \
                                     "socket option on TCP/IP sockets!")
            except EnvironmentError:
                # Python's socket module was compiled using modern headers
                # thus defining SO_REUSEPORT but this process is running
                # under an older kernel that does not support SO_REUSEPORT.
                pass
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


# A constant likely larger than the underlying OS pipe buffer size, to
# make writes blocking.
# Windows limit seems to be around 512 B, and many Unix kernels have a
# 64 KiB pipe buffer size or 16 * PAGE_SIZE: take a few megs to be sure.
# (see issue #17835 for a discussion of this number).
PIPE_MAX_SIZE = 4 * 1024 * 1024 + 1

# A constant likely larger than the underlying OS socket buffer size, to make
# writes blocking.
# The socket buffer sizes can usually be tuned system-wide (e.g. through sysctl
# on Linux), or on a per-socket basis (SO_SNDBUF/SO_RCVBUF). See issue #18643
# for a discussion of this number).
SOCK_MAX_SIZE = 16 * 1024 * 1024 + 1

try:
    unicode
    have_unicode = True
except NameError:
    have_unicode = False

is_jython = sys.platform.startswith('java')

# FS_NONASCII: non-ASCII Unicode character encodable by
# sys.getfilesystemencoding(), or None if there is no such character.
FS_NONASCII = None
if have_unicode:
    for character in (
        # First try printable and common characters to have a readable filename.
        # For each character, the encoding list are just example of encodings able
        # to encode the character (the list is not exhaustive).

        # U+00E6 (Latin Small Letter Ae): cp1252, iso-8859-1
        unichr(0x00E6),
        # U+0130 (Latin Capital Letter I With Dot Above): cp1254, iso8859_3
        unichr(0x0130),
        # U+0141 (Latin Capital Letter L With Stroke): cp1250, cp1257
        unichr(0x0141),
        # U+03C6 (Greek Small Letter Phi): cp1253
        unichr(0x03C6),
        # U+041A (Cyrillic Capital Letter Ka): cp1251
        unichr(0x041A),
        # U+05D0 (Hebrew Letter Alef): Encodable to cp424
        unichr(0x05D0),
        # U+060C (Arabic Comma): cp864, cp1006, iso8859_6, mac_arabic
        unichr(0x060C),
        # U+062A (Arabic Letter Teh): cp720
        unichr(0x062A),
        # U+0E01 (Thai Character Ko Kai): cp874
        unichr(0x0E01),

        # Then try more "special" characters. "special" because they may be
        # interpreted or displayed differently depending on the exact locale
        # encoding and the font.

        # U+00A0 (No-Break Space)
        unichr(0x00A0),
        # U+20AC (Euro Sign)
        unichr(0x20AC),
    ):
        try:
            character.encode(sys.getfilesystemencoding())\
                     .decode(sys.getfilesystemencoding())
        except UnicodeError:
            pass
        else:
            FS_NONASCII = character
            break

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
        # TESTFN_UNENCODABLE is a filename that should *not* be
        # able to be encoded by *either* the default or filesystem encoding.
        # This test really only makes sense on Windows NT platforms
        # which have special Unicode support in posixmodule.
        if (not hasattr(sys, "getwindowsversion") or
                sys.getwindowsversion()[3] < 2): #  0=win32s or 1=9x/ME
            TESTFN_UNENCODABLE = None
        else:
            # Japanese characters (I think - from bug 846133)
            TESTFN_UNENCODABLE = eval('u"@test-\u5171\u6709\u3055\u308c\u308b"')
            try:
                # XXX - Note - should be using TESTFN_ENCODING here - but for
                # Windows, "mbcs" currently always operates as if in
                # errors=ignore' mode - hence we get '?' characters rather than
                # the exception.  'Latin1' operates as we expect - ie, fails.
                # See [ 850997 ] mbcs encoding ignores errors
                TESTFN_UNENCODABLE.encode("Latin1")
            except UnicodeEncodeError:
                pass
            else:
                print \
                'WARNING: The filename %r CAN be encoded by the filesystem.  ' \
                'Unicode filename tests may not be effective' \
                % TESTFN_UNENCODABLE


# Disambiguate TESTFN for parallel testing, while letting it remain a valid
# module name.
TESTFN = "{}_{}_tmp".format(TESTFN, os.getpid())

# Save the initial cwd
SAVEDCWD = os.getcwd()

@contextlib.contextmanager
def temp_cwd(name='tempcwd', quiet=False):
    """
    Context manager that creates a temporary directory and set it as CWD.

    The new CWD is created in the current directory and it's named *name*.
    If *quiet* is False (default) and it's not possible to create or change
    the CWD, an error is raised.  If it's True, only a warning is raised
    and the original CWD is used.
    """
    if have_unicode and isinstance(name, unicode):
        try:
            name = name.encode(sys.getfilesystemencoding() or 'ascii')
        except UnicodeEncodeError:
            if not quiet:
                raise unittest.SkipTest('unable to encode the cwd name with '
                                        'the filesystem encoding.')
    saved_dir = os.getcwd()
    is_temporary = False
    try:
        os.mkdir(name)
        os.chdir(name)
        is_temporary = True
    except OSError:
        if not quiet:
            raise
        warnings.warn('tests may fail, unable to change the CWD to ' + name,
                      RuntimeWarning, stacklevel=3)
    try:
        yield os.getcwd()
    finally:
        os.chdir(saved_dir)
        if is_temporary:
            rmtree(name)


def findfile(file, here=__file__, subdir=None):
    """Try to find a file on sys.path and the working directory.  If it is not
    found the argument passed to the function is returned (this does not
    necessarily signal failure; could still be the legitimate path)."""
    if os.path.isabs(file):
        return file
    if subdir is not None:
        file = os.path.join(subdir, file)
    path = sys.path
    path = [os.path.dirname(here)] + path
    for dn in path:
        fn = os.path.join(dn, file)
        if os.path.exists(fn): return fn
    return file

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
    testcase.assertRaises(SyntaxError, compile, statement,
                          '<test string>', 'exec')

def open_urlresource(url, check=None):
    import urlparse, urllib2

    filename = urlparse.urlparse(url)[2].split('/')[-1] # '/': it's URL!

    fn = os.path.join(os.path.dirname(__file__), "data", filename)

    def check_valid_file(fn):
        f = open(fn)
        if check is None:
            return f
        elif check(f):
            f.seek(0)
            return f
        f.close()

    if os.path.exists(fn):
        f = check_valid_file(fn)
        if f is not None:
            return f
        unlink(fn)

    # Verify the requirement before downloading the file
    requires('urlfetch')

    print >> get_original_stdout(), '\tfetching %s ...' % url
    f = urllib2.urlopen(url, timeout=15)
    try:
        with open(fn, "wb") as out:
            s = f.read()
            while s:
                out.write(s)
                s = f.read()
    finally:
        f.close()

    f = check_valid_file(fn)
    if f is not None:
        return f
    raise TestFailed('invalid resource "%s"' % fn)


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
def check_py3k_warnings(*filters, **kwargs):
    """Context manager to silence py3k warnings.

    Accept 2-tuples as positional arguments:
        ("message regexp", WarningCategory)

    Optional argument:
     - if 'quiet' is True, it does not fail if a filter catches nothing
        (default False)

    Without argument, it defaults to:
        check_py3k_warnings(("", DeprecationWarning), quiet=False)
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
            importlib.import_module("foo") # new reference
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


class EnvironmentVarGuard(UserDict.DictMixin):

    """Class to help protect the environment variable properly.  Can be used as
    a context manager."""

    def __init__(self):
        self._environ = os.environ
        self._changed = {}

    def __getitem__(self, envvar):
        return self._environ[envvar]

    def __setitem__(self, envvar, value):
        # Remember the initial value on the first access
        if envvar not in self._changed:
            self._changed[envvar] = self._environ.get(envvar)
        self._environ[envvar] = value

    def __delitem__(self, envvar):
        # Remember the initial value on the first access
        if envvar not in self._changed:
            self._changed[envvar] = self._environ.get(envvar)
        if envvar in self._environ:
            del self._environ[envvar]

    def keys(self):
        return self._environ.keys()

    def set(self, envvar, value):
        self[envvar] = value

    def unset(self, envvar):
        del self[envvar]

    def __enter__(self):
        return self

    def __exit__(self, *ignore_exc):
        for (k, v) in self._changed.items():
            if v is None:
                if k in self._environ:
                    del self._environ[k]
            else:
                self._environ[k] = v
        os.environ = self._environ


class DirsOnSysPath(object):
    """Context manager to temporarily add directories to sys.path.

    This makes a copy of sys.path, appends any directories given
    as positional arguments, then reverts sys.path to the copied
    settings when the context ends.

    Note that *all* sys.path modifications in the body of the
    context manager, including replacement of the object,
    will be reverted at the end of the block.
    """

    def __init__(self, *paths):
        self.original_value = sys.path[:]
        self.original_object = sys.path
        sys.path.extend(paths)

    def __enter__(self):
        return self

    def __exit__(self, *ignore_exc):
        sys.path = self.original_object
        sys.path[:] = self.original_value


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


@contextlib.contextmanager
def transient_internet(resource_name, timeout=30.0, errnos=()):
    """Return a context manager that raises ResourceDenied when various issues
    with the Internet connection manifest themselves as exceptions."""
    default_errnos = [
        ('ECONNREFUSED', 111),
        ('ECONNRESET', 104),
        ('EHOSTUNREACH', 113),
        ('ENETUNREACH', 101),
        ('ETIMEDOUT', 110),
    ]
    default_gai_errnos = [
        ('EAI_AGAIN', -3),
        ('EAI_FAIL', -4),
        ('EAI_NONAME', -2),
        ('EAI_NODATA', -5),
        # Windows defines EAI_NODATA as 11001 but idiotic getaddrinfo()
        # implementation actually returns WSANO_DATA i.e. 11004.
        ('WSANO_DATA', 11004),
    ]

    denied = ResourceDenied("Resource '%s' is not available" % resource_name)
    captured_errnos = errnos
    gai_errnos = []
    if not captured_errnos:
        captured_errnos = [getattr(errno, name, num)
                           for (name, num) in default_errnos]
        gai_errnos = [getattr(socket, name, num)
                      for (name, num) in default_gai_errnos]

    def filter_error(err):
        n = getattr(err, 'errno', None)
        if (isinstance(err, socket.timeout) or
            (isinstance(err, socket.gaierror) and n in gai_errnos) or
            n in captured_errnos):
            if not verbose:
                sys.stderr.write(denied.args[0] + "\n")
            raise denied

    old_timeout = socket.getdefaulttimeout()
    try:
        if timeout is not None:
            socket.setdefaulttimeout(timeout)
        yield
    except IOError as err:
        # urllib can wrap original socket errors multiple times (!), we must
        # unwrap to get at the original error.
        while True:
            a = err.args
            if len(a) >= 1 and isinstance(a[0], IOError):
                err = a[0]
            # The error can also be wrapped as args[1]:
            #    except socket.error as msg:
            #        raise IOError('socket error', msg).with_traceback(sys.exc_info()[2])
            elif len(a) >= 2 and isinstance(a[1], IOError):
                err = a[1]
            else:
                break
        filter_error(err)
        raise
    # XXX should we catch generic exceptions and look for their
    # __cause__ or __context__?
    finally:
        socket.setdefaulttimeout(old_timeout)


@contextlib.contextmanager
def captured_output(stream_name):
    """Return a context manager used by captured_stdout and captured_stdin
    that temporarily replaces the sys stream *stream_name* with a StringIO."""
    import StringIO
    orig_stdout = getattr(sys, stream_name)
    setattr(sys, stream_name, StringIO.StringIO())
    try:
        yield getattr(sys, stream_name)
    finally:
        setattr(sys, stream_name, orig_stdout)

def captured_stdout():
    """Capture the output of sys.stdout:

       with captured_stdout() as s:
           print "hello"
       self.assertEqual(s.getvalue(), "hello")
    """
    return captured_output("stdout")

def captured_stderr():
    return captured_output("stderr")

def captured_stdin():
    return captured_output("stdin")

def gc_collect():
    """Force as many objects as possible to be collected.

    In non-CPython implementations of Python, this is needed because timely
    deallocation is not guaranteed by the garbage collector.  (Even in CPython
    this can be the case in case of reference cycles.)  This means that __del__
    methods may be called later than expected and weakrefs may remain alive for
    longer than expected.  This function tries its best to force all garbage
    objects to disappear.
    """
    gc.collect()
    if is_jython:
        time.sleep(0.1)
    gc.collect()
    gc.collect()


_header = '2P'
if hasattr(sys, "gettotalrefcount"):
    _header = '2P' + _header
_vheader = _header + 'P'

def calcobjsize(fmt):
    return struct.calcsize(_header + fmt + '0P')

def calcvobjsize(fmt):
    return struct.calcsize(_vheader + fmt + '0P')


_TPFLAGS_HAVE_GC = 1<<14
_TPFLAGS_HEAPTYPE = 1<<9

def check_sizeof(test, o, size):
    import _testcapi
    result = sys.getsizeof(o)
    # add GC header size
    if ((type(o) == type) and (o.__flags__ & _TPFLAGS_HEAPTYPE) or\
        ((type(o) != type) and (type(o).__flags__ & _TPFLAGS_HAVE_GC))):
        size += _testcapi.SIZEOF_PYGC_HEAD
    msg = 'wrong size for %s: got %d, expected %d' \
            % (type(o), result, size)
    test.assertEqual(result, size, msg)


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
                self.assertFalse(maxsize * memuse + overhead > 20 * _1M)
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

def precisionbigmemtest(size, memuse, overhead=5*_1M, dry_run=True):
    def decorator(f):
        def wrapper(self):
            if not real_max_memuse:
                maxsize = 5147
            else:
                maxsize = size

            if ((real_max_memuse or not dry_run)
                and real_max_memuse < maxsize * memuse):
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

def _id(obj):
    return obj

def requires_resource(resource):
    if resource == 'gui' and not _is_gui_available():
        return unittest.skip(_is_gui_available.reason)
    if is_resource_enabled(resource):
        return _id
    else:
        return unittest.skip("resource {0!r} is not enabled".format(resource))

def cpython_only(test):
    """
    Decorator for tests only applicable on CPython.
    """
    return impl_detail(cpython=True)(test)

def impl_detail(msg=None, **guards):
    if check_impl_detail(**guards):
        return _id
    if msg is None:
        guardnames, default = _parse_guards(guards)
        if default:
            msg = "implementation detail not available on {0}"
        else:
            msg = "implementation detail specific to {0}"
        guardnames = sorted(guardnames.keys())
        msg = msg.format(' or '.join(guardnames))
    return unittest.skip(msg)

def _parse_guards(guards):
    # Returns a tuple ({platform_name: run_me}, default_value)
    if not guards:
        return ({'cpython': True}, False)
    is_true = guards.values()[0]
    assert guards.values() == [is_true] * len(guards)   # all True or all False
    return (guards, not is_true)

# Use the following check to guard CPython's implementation-specific tests --
# or to run them only on the implementation(s) guarded by the arguments.
def check_impl_detail(**guards):
    """This function returns True or False depending on the host platform.
       Examples:
          if check_impl_detail():               # only on CPython (default)
          if check_impl_detail(jython=True):    # only on Jython
          if check_impl_detail(cpython=False):  # everywhere except on CPython
    """
    guards, default = _parse_guards(guards)
    return guards.get(platform.python_implementation().lower(), default)



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
# Check for the presence of docstrings.

HAVE_DOCSTRINGS = (check_impl_detail(cpython=False) or
                   sys.platform == 'win32' or
                   sysconfig.get_config_var('WITH_DOC_STRINGS'))

requires_docstrings = unittest.skipUnless(HAVE_DOCSTRINGS,
                                          "test requires docstrings")


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

# NOTE: we use thread._count() rather than threading.enumerate() (or the
# moral equivalent thereof) because a threading.Thread object is still alive
# until its __bootstrap() method has returned, even after it has been
# unregistered from the threading module.
# thread._count(), on the other hand, only gets decremented *after* the
# __bootstrap() method has returned, which gives us reliable reference counts
# at the end of a test run.

def threading_setup():
    if thread:
        return thread._count(),
    else:
        return 1,

def threading_cleanup(nb_threads):
    if not thread:
        return

    _MAX_COUNT = 10
    for count in range(_MAX_COUNT):
        n = thread._count()
        if n == nb_threads:
            break
        time.sleep(0.1)
    # XXX print a warning in case of failure?

def reap_threads(func):
    """Use this function when threads are being used.  This will
    ensure that the threads are cleaned up even when the test fails.
    If threading is unavailable this function does nothing.
    """
    if not thread:
        return func

    @functools.wraps(func)
    def decorator(*args):
        key = threading_setup()
        try:
            return func(*args)
        finally:
            threading_cleanup(*key)
    return decorator

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

@contextlib.contextmanager
def swap_attr(obj, attr, new_val):
    """Temporary swap out an attribute with a new object.

    Usage:
        with swap_attr(obj, "attr", 5):
            ...

        This will set obj.attr to 5 for the duration of the with: block,
        restoring the old value at the end of the block. If `attr` doesn't
        exist on `obj`, it will be created and then deleted at the end of the
        block.
    """
    if hasattr(obj, attr):
        real_val = getattr(obj, attr)
        setattr(obj, attr, new_val)
        try:
            yield
        finally:
            setattr(obj, attr, real_val)
    else:
        setattr(obj, attr, new_val)
        try:
            yield
        finally:
            delattr(obj, attr)

def py3k_bytes(b):
    """Emulate the py3k bytes() constructor.

    NOTE: This is only a best effort function.
    """
    try:
        # memoryview?
        return b.tobytes()
    except AttributeError:
        try:
            # iterable of ints?
            return b"".join(chr(x) for x in b)
        except TypeError:
            return bytes(b)

def args_from_interpreter_flags():
    """Return a list of command-line arguments reproducing the current
    settings in sys.flags."""
    import subprocess
    return subprocess._args_from_interpreter_flags()

def strip_python_stderr(stderr):
    """Strip the stderr of a Python process from potential debug output
    emitted by the interpreter.

    This will typically be run on the result of the communicate() method
    of a subprocess.Popen object.
    """
    stderr = re.sub(br"\[\d+ refs\]\r?\n?$", b"", stderr).strip()
    return stderr

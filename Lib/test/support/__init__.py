"""Supporting definitions for the Python regression tests."""

if __name__ != 'test.support':
    raise ImportError('support must be imported from the test package')

import contextlib
import dataclasses
import functools
import _opcode
import os
import re
import stat
import sys
import sysconfig
import textwrap
import time
import types
import unittest
import warnings


__all__ = [
    # globals
    "PIPE_MAX_SIZE", "verbose", "max_memuse", "use_resources", "failfast",
    # exceptions
    "Error", "TestFailed", "TestDidNotRun", "ResourceDenied",
    # io
    "record_original_stdout", "get_original_stdout", "captured_stdout",
    "captured_stdin", "captured_stderr",
    # unittest
    "is_resource_enabled", "requires", "requires_freebsd_version",
    "requires_linux_version", "requires_mac_ver",
    "check_syntax_error",
    "requires_gzip", "requires_bz2", "requires_lzma",
    "bigmemtest", "bigaddrspacetest", "cpython_only", "get_attribute",
    "requires_IEEE_754", "requires_zlib",
    "has_fork_support", "requires_fork",
    "has_subprocess_support", "requires_subprocess",
    "has_socket_support", "requires_working_socket",
    "anticipate_failure", "load_package_tests", "detect_api_mismatch",
    "check__all__", "skip_if_buggy_ucrt_strfptime",
    "check_disallow_instantiation", "check_sanitizer", "skip_if_sanitizer",
    "requires_limited_api", "requires_specialization",
    # sys
    "MS_WINDOWS", "is_jython", "is_android", "is_emscripten", "is_wasi",
    "is_apple_mobile", "check_impl_detail", "unix_shell", "setswitchinterval",
    # os
    "get_pagesize",
    # network
    "open_urlresource",
    # processes
    "reap_children",
    # miscellaneous
    "run_with_locale", "swap_item", "findfile", "infinite_recursion",
    "swap_attr", "Matcher", "set_memlimit", "SuppressCrashReport", "sortdict",
    "run_with_tz", "PGO", "missing_compiler_executable",
    "ALWAYS_EQ", "NEVER_EQ", "LARGEST", "SMALLEST",
    "LOOPBACK_TIMEOUT", "INTERNET_TIMEOUT", "SHORT_TIMEOUT", "LONG_TIMEOUT",
    "Py_DEBUG", "EXCEEDS_RECURSION_LIMIT", "Py_C_RECURSION_LIMIT",
    "skip_on_s390x",
    "without_optimizer",
    ]


# Timeout in seconds for tests using a network server listening on the network
# local loopback interface like 127.0.0.1.
#
# The timeout is long enough to prevent test failure: it takes into account
# that the client and the server can run in different threads or even different
# processes.
#
# The timeout should be long enough for connect(), recv() and send() methods
# of socket.socket.
LOOPBACK_TIMEOUT = 10.0

# Timeout in seconds for network requests going to the internet. The timeout is
# short enough to prevent a test to wait for too long if the internet request
# is blocked for whatever reason.
#
# Usually, a timeout using INTERNET_TIMEOUT should not mark a test as failed,
# but skip the test instead: see transient_internet().
INTERNET_TIMEOUT = 60.0

# Timeout in seconds to mark a test as failed if the test takes "too long".
#
# The timeout value depends on the regrtest --timeout command line option.
#
# If a test using SHORT_TIMEOUT starts to fail randomly on slow buildbots, use
# LONG_TIMEOUT instead.
SHORT_TIMEOUT = 30.0

# Timeout in seconds to detect when a test hangs.
#
# It is long enough to reduce the risk of test failure on the slowest Python
# buildbots. It should not be used to mark a test as failed if the test takes
# "too long". The timeout value depends on the regrtest --timeout command line
# option.
LONG_TIMEOUT = 5 * 60.0

# TEST_HOME_DIR refers to the top level directory of the "test" package
# that contains Python's regression test suite
TEST_SUPPORT_DIR = os.path.dirname(os.path.abspath(__file__))
TEST_HOME_DIR = os.path.dirname(TEST_SUPPORT_DIR)
STDLIB_DIR = os.path.dirname(TEST_HOME_DIR)
REPO_ROOT = os.path.dirname(STDLIB_DIR)


class Error(Exception):
    """Base class for regression test exceptions."""

class TestFailed(Error):
    """Test failed."""
    def __init__(self, msg, *args, stats=None):
        self.msg = msg
        self.stats = stats
        super().__init__(msg, *args)

    def __str__(self):
        return self.msg

class TestFailedWithDetails(TestFailed):
    """Test failed."""
    def __init__(self, msg, errors, failures, stats):
        self.errors = errors
        self.failures = failures
        super().__init__(msg, errors, failures, stats=stats)

class TestDidNotRun(Error):
    """Test did not run any subtests."""

class ResourceDenied(unittest.SkipTest):
    """Test skipped because it requested a disallowed resource.

    This is raised when a test calls requires() for a resource that
    has not be enabled.  It is used to distinguish between expected
    and unexpected skips.
    """

def anticipate_failure(condition):
    """Decorator to mark a test that is known to be broken in some cases

       Any use of this decorator should have a comment identifying the
       associated tracker issue.
    """
    if condition:
        return unittest.expectedFailure
    return lambda f: f

def load_package_tests(pkg_dir, loader, standard_tests, pattern):
    """Generic load_tests implementation for simple test packages.

    Most packages can implement load_tests using this function as follows:

       def load_tests(*args):
           return load_package_tests(os.path.dirname(__file__), *args)
    """
    if pattern is None:
        pattern = "test*"
    top_dir = STDLIB_DIR
    package_tests = loader.discover(start_dir=pkg_dir,
                                    top_level_dir=top_dir,
                                    pattern=pattern)
    standard_tests.addTests(package_tests)
    return standard_tests


def get_attribute(obj, name):
    """Get an attribute, raising SkipTest if AttributeError is raised."""
    try:
        attribute = getattr(obj, name)
    except AttributeError:
        raise unittest.SkipTest("object %r has no attribute %r" % (obj, name))
    else:
        return attribute

verbose = 1              # Flag set to 0 by regrtest.py
use_resources = None     # Flag set to [] by regrtest.py
max_memuse = 0           # Disable bigmem tests (they will still be run with
                         # small sizes, to make sure they work.)
real_max_memuse = 0
junit_xml_list = None    # list of testsuite XML elements
failfast = False

# _original_stdout is meant to hold stdout at the time regrtest began.
# This may be "the real" stdout, or IDLE's emulation of stdout, or whatever.
# The point is to have some flavor of stdout the user can actually see.
_original_stdout = None
def record_original_stdout(stdout):
    global _original_stdout
    _original_stdout = stdout

def get_original_stdout():
    return _original_stdout or sys.stdout


def _force_run(path, func, *args):
    try:
        return func(*args)
    except FileNotFoundError as err:
        # chmod() won't fix a missing file.
        if verbose >= 2:
            print('%s: %s' % (err.__class__.__name__, err))
        raise
    except OSError as err:
        if verbose >= 2:
            print('%s: %s' % (err.__class__.__name__, err))
            print('re-run %s%r' % (func.__name__, args))
        os.chmod(path, stat.S_IRWXU)
        return func(*args)


# Check whether a gui is actually available
def _is_gui_available():
    if hasattr(_is_gui_available, 'result'):
        return _is_gui_available.result
    import platform
    reason = None
    if sys.platform.startswith('win') and platform.win32_is_iot():
        reason = "gui is not available on Windows IoT Core"
    elif sys.platform.startswith('win'):
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
    if not reason:
        try:
            from tkinter import Tk
            root = Tk()
            root.withdraw()
            root.update()
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
    if not is_resource_enabled(resource):
        if msg is None:
            msg = "Use of the %r resource not enabled" % resource
        raise ResourceDenied(msg)
    if resource in {"network", "urlfetch"} and not has_socket_support:
        raise ResourceDenied("No socket support")
    if resource == 'gui' and not _is_gui_available():
        raise ResourceDenied(_is_gui_available.reason)

def _requires_unix_version(sysname, min_version):
    """Decorator raising SkipTest if the OS is `sysname` and the version is less
    than `min_version`.

    For example, @_requires_unix_version('FreeBSD', (7, 2)) raises SkipTest if
    the FreeBSD version is less than 7.2.
    """
    import platform
    min_version_txt = '.'.join(map(str, min_version))
    version_txt = platform.release().split('-', 1)[0]
    if platform.system() == sysname:
        try:
            version = tuple(map(int, version_txt.split('.')))
        except ValueError:
            skip = False
        else:
            skip = version < min_version
    else:
        skip = False

    return unittest.skipIf(
        skip,
        f"{sysname} version {min_version_txt} or higher required, not "
        f"{version_txt}"
    )


def requires_freebsd_version(*min_version):
    """Decorator raising SkipTest if the OS is FreeBSD and the FreeBSD version is
    less than `min_version`.

    For example, @requires_freebsd_version(7, 2) raises SkipTest if the FreeBSD
    version is less than 7.2.
    """
    return _requires_unix_version('FreeBSD', min_version)

def requires_linux_version(*min_version):
    """Decorator raising SkipTest if the OS is Linux and the Linux version is
    less than `min_version`.

    For example, @requires_linux_version(2, 6, 32) raises SkipTest if the Linux
    version is less than 2.6.32.
    """
    return _requires_unix_version('Linux', min_version)

def requires_mac_ver(*min_version):
    """Decorator raising SkipTest if the OS is Mac OS X and the OS X
    version if less than min_version.

    For example, @requires_mac_ver(10, 5) raises SkipTest if the OS X version
    is lesser than 10.5.
    """
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kw):
            if sys.platform == 'darwin':
                import platform
                version_txt = platform.mac_ver()[0]
                try:
                    version = tuple(map(int, version_txt.split('.')))
                except ValueError:
                    pass
                else:
                    if version < min_version:
                        min_version_txt = '.'.join(map(str, min_version))
                        raise unittest.SkipTest(
                            "Mac OS X %s or higher required, not %s"
                            % (min_version_txt, version_txt))
            return func(*args, **kw)
        wrapper.min_version = min_version
        return wrapper
    return decorator


def skip_if_buildbot(reason=None):
    """Decorator raising SkipTest if running on a buildbot."""
    import getpass
    if not reason:
        reason = 'not suitable for buildbots'
    try:
        isbuildbot = getpass.getuser().lower() == 'buildbot'
    except (KeyError, EnvironmentError) as err:
        warnings.warn(f'getpass.getuser() failed {err}.', RuntimeWarning)
        isbuildbot = False
    return unittest.skipIf(isbuildbot, reason)

def check_sanitizer(*, address=False, memory=False, ub=False, thread=False):
    """Returns True if Python is compiled with sanitizer support"""
    if not (address or memory or ub or thread):
        raise ValueError('At least one of address, memory, ub or thread must be True')


    cflags = sysconfig.get_config_var('CFLAGS') or ''
    config_args = sysconfig.get_config_var('CONFIG_ARGS') or ''
    memory_sanitizer = (
        '-fsanitize=memory' in cflags or
        '--with-memory-sanitizer' in config_args
    )
    address_sanitizer = (
        '-fsanitize=address' in cflags or
        '--with-address-sanitizer' in config_args
    )
    ub_sanitizer = (
        '-fsanitize=undefined' in cflags or
        '--with-undefined-behavior-sanitizer' in config_args
    )
    thread_sanitizer = (
        '-fsanitize=thread' in cflags or
        '--with-thread-sanitizer' in config_args
    )
    return (
        (memory and memory_sanitizer) or
        (address and address_sanitizer) or
        (ub and ub_sanitizer) or
        (thread and thread_sanitizer)
    )


def skip_if_sanitizer(reason=None, *, address=False, memory=False, ub=False, thread=False):
    """Decorator raising SkipTest if running with a sanitizer active."""
    if not reason:
        reason = 'not working with sanitizers active'
    skip = check_sanitizer(address=address, memory=memory, ub=ub, thread=thread)
    return unittest.skipIf(skip, reason)

# gh-89363: True if fork() can hang if Python is built with Address Sanitizer
# (ASAN): libasan race condition, dead lock in pthread_create().
HAVE_ASAN_FORK_BUG = check_sanitizer(address=True)


def set_sanitizer_env_var(env, option):
    for name in ('ASAN_OPTIONS', 'MSAN_OPTIONS', 'UBSAN_OPTIONS', 'TSAN_OPTIONS'):
        if name in env:
            env[name] += f':{option}'
        else:
            env[name] = option


def system_must_validate_cert(f):
    """Skip the test on TLS certificate validation failures."""
    @functools.wraps(f)
    def dec(*args, **kwargs):
        try:
            f(*args, **kwargs)
        except OSError as e:
            if "CERTIFICATE_VERIFY_FAILED" in str(e):
                raise unittest.SkipTest("system does not contain "
                                        "necessary certificates")
            raise
    return dec

# A constant likely larger than the underlying OS pipe buffer size, to
# make writes blocking.
# Windows limit seems to be around 512 B, and many Unix kernels have a
# 64 KiB pipe buffer size or 16 * PAGE_SIZE: take a few megs to be sure.
# (see issue #17835 for a discussion of this number).
PIPE_MAX_SIZE = 4 * 1024 * 1024 + 1

# A constant likely larger than the underlying OS socket buffer size, to make
# writes blocking.
# The socket buffer sizes can usually be tuned system-wide (e.g. through sysctl
# on Linux), or on a per-socket basis (SO_SNDBUF/SO_RCVBUF).  See issue #18643
# for a discussion of this number.
SOCK_MAX_SIZE = 16 * 1024 * 1024 + 1

# decorator for skipping tests on non-IEEE 754 platforms
requires_IEEE_754 = unittest.skipUnless(
    float.__getformat__("double").startswith("IEEE"),
    "test requires IEEE 754 doubles")

def requires_zlib(reason='requires zlib'):
    try:
        import zlib
    except ImportError:
        zlib = None
    return unittest.skipUnless(zlib, reason)

def requires_gzip(reason='requires gzip'):
    try:
        import gzip
    except ImportError:
        gzip = None
    return unittest.skipUnless(gzip, reason)

def requires_bz2(reason='requires bz2'):
    try:
        import bz2
    except ImportError:
        bz2 = None
    return unittest.skipUnless(bz2, reason)

def requires_lzma(reason='requires lzma'):
    try:
        import lzma
    except ImportError:
        lzma = None
    return unittest.skipUnless(lzma, reason)

def has_no_debug_ranges():
    try:
        import _testinternalcapi
    except ImportError:
        raise unittest.SkipTest("_testinternalcapi required")
    config = _testinternalcapi.get_config()
    return not bool(config['code_debug_ranges'])

def requires_debug_ranges(reason='requires co_positions / debug_ranges'):
    return unittest.skipIf(has_no_debug_ranges(), reason)

MS_WINDOWS = (sys.platform == 'win32')

# Is not actually used in tests, but is kept for compatibility.
is_jython = sys.platform.startswith('java')

is_android = hasattr(sys, 'getandroidapilevel')

if sys.platform not in {"win32", "vxworks", "ios", "tvos", "watchos"}:
    unix_shell = '/system/bin/sh' if is_android else '/bin/sh'
else:
    unix_shell = None

# wasm32-emscripten and -wasi are POSIX-like but do not
# have subprocess or fork support.
is_emscripten = sys.platform == "emscripten"
is_wasi = sys.platform == "wasi"

is_apple_mobile = sys.platform in {"ios", "tvos", "watchos"}
is_apple = is_apple_mobile or sys.platform == "darwin"

has_fork_support = hasattr(os, "fork") and not (
    # WASM and Apple mobile platforms do not support subprocesses.
    is_emscripten
    or is_wasi
    or is_apple_mobile

    # Although Android supports fork, it's unsafe to call it from Python because
    # all Android apps are multi-threaded.
    or is_android
)

def requires_fork():
    return unittest.skipUnless(has_fork_support, "requires working os.fork()")

has_subprocess_support = not (
    # WASM and Apple mobile platforms do not support subprocesses.
    is_emscripten
    or is_wasi
    or is_apple_mobile

    # Although Android supports subproceses, they're almost never useful in
    # practice (see PEP 738). And most of the tests that use them are calling
    # sys.executable, which won't work when Python is embedded in an Android app.
    or is_android
)

def requires_subprocess():
    """Used for subprocess, os.spawn calls, fd inheritance"""
    return unittest.skipUnless(has_subprocess_support, "requires subprocess support")

# Emscripten's socket emulation and WASI sockets have limitations.
has_socket_support = not (
    is_emscripten
    or is_wasi
)

def requires_working_socket(*, module=False):
    """Skip tests or modules that require working sockets

    Can be used as a function/class decorator or to skip an entire module.
    """
    msg = "requires socket support"
    if module:
        if not has_socket_support:
            raise unittest.SkipTest(msg)
    else:
        return unittest.skipUnless(has_socket_support, msg)

# Does strftime() support glibc extension like '%4Y'?
has_strftime_extensions = False
if sys.platform != "win32":
    # bpo-47037: Windows debug builds crash with "Debug Assertion Failed"
    try:
        has_strftime_extensions = time.strftime("%4Y") != "%4Y"
    except ValueError:
        pass

# Define the URL of a dedicated HTTP server for the network tests.
# The URL must use clear-text HTTP: no redirection to encrypted HTTPS.
TEST_HTTP_URL = "http://www.pythontest.net"

# Set by libregrtest/main.py so we can skip tests that are not
# useful for PGO
PGO = False

# Set by libregrtest/main.py if we are running the extended (time consuming)
# PGO task.  If this is True, PGO is also True.
PGO_EXTENDED = False

# TEST_DATA_DIR is used as a target download location for remote resources
TEST_DATA_DIR = os.path.join(TEST_HOME_DIR, "data")


def darwin_malloc_err_warning(test_name):
    """Assure user that loud errors generated by macOS libc's malloc are
    expected."""
    if sys.platform != 'darwin':
        return

    import shutil
    msg = ' NOTICE '
    detail = (f'{test_name} may generate "malloc can\'t allocate region"\n'
              'warnings on macOS systems. This behavior is known. Do not\n'
              'report a bug unless tests are also failing.\n'
              'See https://github.com/python/cpython/issues/85100')

    padding, _ = shutil.get_terminal_size()
    print(msg.center(padding, '-'))
    print(detail)
    print('-' * padding)


def findfile(filename, subdir=None):
    """Try to find a file on sys.path or in the test directory.  If it is not
    found the argument passed to the function is returned (this does not
    necessarily signal failure; could still be the legitimate path).

    Setting *subdir* indicates a relative path to use to find the file
    rather than looking directly in the path directories.
    """
    if os.path.isabs(filename):
        return filename
    if subdir is not None:
        filename = os.path.join(subdir, filename)
    path = [TEST_HOME_DIR] + sys.path
    for dn in path:
        fn = os.path.join(dn, filename)
        if os.path.exists(fn): return fn
    return filename


def sortdict(dict):
    "Like repr(dict), but in sorted order."
    items = sorted(dict.items())
    reprpairs = ["%r: %r" % pair for pair in items]
    withcommas = ", ".join(reprpairs)
    return "{%s}" % withcommas


def run_code(code: str) -> dict[str, object]:
    """Run a piece of code after dedenting it, and return its global namespace."""
    ns = {}
    exec(textwrap.dedent(code), ns)
    return ns


def check_syntax_error(testcase, statement, errtext='', *, lineno=None, offset=None):
    with testcase.assertRaisesRegex(SyntaxError, errtext) as cm:
        compile(statement, '<test string>', 'exec')
    err = cm.exception
    testcase.assertIsNotNone(err.lineno)
    if lineno is not None:
        testcase.assertEqual(err.lineno, lineno)
    testcase.assertIsNotNone(err.offset)
    if offset is not None:
        testcase.assertEqual(err.offset, offset)


def open_urlresource(url, *args, **kw):
    import urllib.request, urllib.parse
    from .os_helper import unlink
    try:
        import gzip
    except ImportError:
        gzip = None

    check = kw.pop('check', None)

    filename = urllib.parse.urlparse(url)[2].split('/')[-1] # '/': it's URL!

    fn = os.path.join(TEST_DATA_DIR, filename)

    def check_valid_file(fn):
        f = open(fn, *args, **kw)
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

    if verbose:
        print('\tfetching %s ...' % url, file=get_original_stdout())
    opener = urllib.request.build_opener()
    if gzip:
        opener.addheaders.append(('Accept-Encoding', 'gzip'))
    f = opener.open(url, timeout=INTERNET_TIMEOUT)
    if gzip and f.headers.get('Content-Encoding') == 'gzip':
        f = gzip.GzipFile(fileobj=f)
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
    raise TestFailed('invalid resource %r' % fn)


@contextlib.contextmanager
def captured_output(stream_name):
    """Return a context manager used by captured_stdout/stdin/stderr
    that temporarily replaces the sys stream *stream_name* with a StringIO."""
    import io
    orig_stdout = getattr(sys, stream_name)
    setattr(sys, stream_name, io.StringIO())
    try:
        yield getattr(sys, stream_name)
    finally:
        setattr(sys, stream_name, orig_stdout)

def captured_stdout():
    """Capture the output of sys.stdout:

       with captured_stdout() as stdout:
           print("hello")
       self.assertEqual(stdout.getvalue(), "hello\\n")
    """
    return captured_output("stdout")

def captured_stderr():
    """Capture the output of sys.stderr:

       with captured_stderr() as stderr:
           print("hello", file=sys.stderr)
       self.assertEqual(stderr.getvalue(), "hello\\n")
    """
    return captured_output("stderr")

def captured_stdin():
    """Capture the input to sys.stdin:

       with captured_stdin() as stdin:
           stdin.write('hello\\n')
           stdin.seek(0)
           # call test code that consumes from sys.stdin
           captured = input()
       self.assertEqual(captured, "hello")
    """
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
    import gc
    gc.collect()
    gc.collect()
    gc.collect()

@contextlib.contextmanager
def disable_gc():
    import gc
    have_gc = gc.isenabled()
    gc.disable()
    try:
        yield
    finally:
        if have_gc:
            gc.enable()


def python_is_optimized():
    """Find if Python was built with optimizations."""
    cflags = sysconfig.get_config_var('PY_CFLAGS') or ''
    final_opt = ""
    for opt in cflags.split():
        if opt.startswith('-O'):
            final_opt = opt
    return final_opt not in ('', '-O0', '-Og')


def check_cflags_pgo():
    # Check if Python was built with ./configure --enable-optimizations:
    # with Profile Guided Optimization (PGO).
    cflags_nodist = sysconfig.get_config_var('PY_CFLAGS_NODIST') or ''
    pgo_options = [
        # GCC
        '-fprofile-use',
        # clang: -fprofile-instr-use=code.profclangd
        '-fprofile-instr-use',
        # ICC
        "-prof-use",
    ]
    PGO_PROF_USE_FLAG = sysconfig.get_config_var('PGO_PROF_USE_FLAG')
    if PGO_PROF_USE_FLAG:
        pgo_options.append(PGO_PROF_USE_FLAG)
    return any(option in cflags_nodist for option in pgo_options)


Py_GIL_DISABLED = bool(sysconfig.get_config_var('Py_GIL_DISABLED'))
if Py_GIL_DISABLED:
    _header = 'PHBBInP'
else:
    _header = 'nP'
_align = '0n'
_vheader = _header + 'n'

def calcobjsize(fmt):
    import struct
    return struct.calcsize(_header + fmt + _align)

def calcvobjsize(fmt):
    import struct
    return struct.calcsize(_vheader + fmt + _align)


_TPFLAGS_HAVE_GC = 1<<14
_TPFLAGS_HEAPTYPE = 1<<9

def check_sizeof(test, o, size):
    try:
        import _testinternalcapi
    except ImportError:
        raise unittest.SkipTest("_testinternalcapi required")
    result = sys.getsizeof(o)
    # add GC header size
    if ((type(o) == type) and (o.__flags__ & _TPFLAGS_HEAPTYPE) or\
        ((type(o) != type) and (type(o).__flags__ & _TPFLAGS_HAVE_GC))):
        size += _testinternalcapi.SIZEOF_PYGC_HEAD
    msg = 'wrong size for %s: got %d, expected %d' \
            % (type(o), result, size)
    test.assertEqual(result, size, msg)

#=======================================================================
# Decorator for running a function in a different locale, correctly resetting
# it afterwards.

@contextlib.contextmanager
def run_with_locale(catstr, *locales):
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

    try:
        yield
    finally:
        if locale and orig_locale:
            locale.setlocale(category, orig_locale)

#=======================================================================
# Decorator for running a function in a specific timezone, correctly
# resetting it afterwards.

def run_with_tz(tz):
    def decorator(func):
        def inner(*args, **kwds):
            try:
                tzset = time.tzset
            except AttributeError:
                raise unittest.SkipTest("tzset required")
            if 'TZ' in os.environ:
                orig_tz = os.environ['TZ']
            else:
                orig_tz = None
            os.environ['TZ'] = tz
            tzset()

            # now run the function, resetting the tz on exceptions
            try:
                return func(*args, **kwds)
            finally:
                if orig_tz is None:
                    del os.environ['TZ']
                else:
                    os.environ['TZ'] = orig_tz
                time.tzset()

        inner.__name__ = func.__name__
        inner.__doc__ = func.__doc__
        return inner
    return decorator

#=======================================================================
# Big-memory-test support. Separate from 'resources' because memory use
# should be configurable.

# Some handy shorthands. Note that these are used for byte-limits as well
# as size-limits, in the various bigmem tests
_1M = 1024*1024
_1G = 1024 * _1M
_2G = 2 * _1G
_4G = 4 * _1G

MAX_Py_ssize_t = sys.maxsize

def _parse_memlimit(limit: str) -> int:
    sizes = {
        'k': 1024,
        'm': _1M,
        'g': _1G,
        't': 1024*_1G,
    }
    m = re.match(r'(\d+(?:\.\d+)?) (K|M|G|T)b?$', limit,
                 re.IGNORECASE | re.VERBOSE)
    if m is None:
        raise ValueError(f'Invalid memory limit: {limit!r}')
    return int(float(m.group(1)) * sizes[m.group(2).lower()])

def set_memlimit(limit: str) -> None:
    global max_memuse
    global real_max_memuse
    memlimit = _parse_memlimit(limit)
    if memlimit < _2G - 1:
        raise ValueError('Memory limit {limit!r} too low to be useful')

    real_max_memuse = memlimit
    memlimit = min(memlimit, MAX_Py_ssize_t)
    max_memuse = memlimit


class _MemoryWatchdog:
    """An object which periodically watches the process' memory consumption
    and prints it out.
    """

    def __init__(self):
        self.procfile = '/proc/{pid}/statm'.format(pid=os.getpid())
        self.started = False

    def start(self):
        import warnings
        try:
            f = open(self.procfile, 'r')
        except OSError as e:
            warnings.warn('/proc not available for stats: {}'.format(e),
                          RuntimeWarning)
            sys.stderr.flush()
            return

        import subprocess
        with f:
            watchdog_script = findfile("memory_watchdog.py")
            self.mem_watchdog = subprocess.Popen([sys.executable, watchdog_script],
                                                 stdin=f,
                                                 stderr=subprocess.DEVNULL)
        self.started = True

    def stop(self):
        if self.started:
            self.mem_watchdog.terminate()
            self.mem_watchdog.wait()


def bigmemtest(size, memuse, dry_run=True):
    """Decorator for bigmem tests.

    'size' is a requested size for the test (in arbitrary, test-interpreted
    units.) 'memuse' is the number of bytes per unit for the test, or a good
    estimate of it. For example, a test that needs two byte buffers, of 4 GiB
    each, could be decorated with @bigmemtest(size=_4G, memuse=2).

    The 'size' argument is normally passed to the decorated test method as an
    extra argument. If 'dry_run' is true, the value passed to the test method
    may be less than the requested value. If 'dry_run' is false, it means the
    test doesn't support dummy runs when -M is not specified.
    """
    def decorator(f):
        def wrapper(self):
            size = wrapper.size
            memuse = wrapper.memuse
            if not real_max_memuse:
                maxsize = 5147
            else:
                maxsize = size

            if ((real_max_memuse or not dry_run)
                and real_max_memuse < maxsize * memuse):
                raise unittest.SkipTest(
                    "not enough memory: %.1fG minimum needed"
                    % (size * memuse / (1024 ** 3)))

            if real_max_memuse and verbose:
                print()
                print(" ... expected peak memory use: {peak:.1f}G"
                      .format(peak=size * memuse / (1024 ** 3)))
                watchdog = _MemoryWatchdog()
                watchdog.start()
            else:
                watchdog = None

            try:
                return f(self, maxsize)
            finally:
                if watchdog:
                    watchdog.stop()

        wrapper.size = size
        wrapper.memuse = memuse
        return wrapper
    return decorator

def bigaddrspacetest(f):
    """Decorator for tests that fill the address space."""
    def wrapper(self):
        if max_memuse < MAX_Py_ssize_t:
            if MAX_Py_ssize_t >= 2**63 - 1 and max_memuse >= 2**31:
                raise unittest.SkipTest(
                    "not enough memory: try a 32-bit build instead")
            else:
                raise unittest.SkipTest(
                    "not enough memory: %.1fG minimum needed"
                    % (MAX_Py_ssize_t / (1024 ** 3)))
        else:
            return f(self)
    return wrapper

#=======================================================================
# unittest integration.

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
    is_true = list(guards.values())[0]
    assert list(guards.values()) == [is_true] * len(guards)   # all True or all False
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
    return guards.get(sys.implementation.name, default)


def no_tracing(func):
    """Decorator to temporarily turn off tracing for the duration of a test."""
    trace_wrapper = func
    if hasattr(sys, 'gettrace'):
        @functools.wraps(func)
        def trace_wrapper(*args, **kwargs):
            original_trace = sys.gettrace()
            try:
                sys.settrace(None)
                return func(*args, **kwargs)
            finally:
                sys.settrace(original_trace)

    coverage_wrapper = trace_wrapper
    if 'test.cov' in sys.modules:  # -Xpresite=test.cov used
        cov = sys.monitoring.COVERAGE_ID
        @functools.wraps(func)
        def coverage_wrapper(*args, **kwargs):
            original_events = sys.monitoring.get_events(cov)
            try:
                sys.monitoring.set_events(cov, 0)
                return trace_wrapper(*args, **kwargs)
            finally:
                sys.monitoring.set_events(cov, original_events)

    return coverage_wrapper


def refcount_test(test):
    """Decorator for tests which involve reference counting.

    To start, the decorator does not run the test if is not run by CPython.
    After that, any trace function is unset during the test to prevent
    unexpected refcounts caused by the trace function.

    """
    return no_tracing(cpython_only(test))


def requires_limited_api(test):
    try:
        import _testcapi
        import _testlimitedcapi
    except ImportError:
        return unittest.skip('needs _testcapi and _testlimitedcapi modules')(test)
    return test

def requires_specialization(test):
    return unittest.skipUnless(
        _opcode.ENABLE_SPECIALIZATION, "requires specialization")(test)


#=======================================================================
# Check for the presence of docstrings.

# Rather than trying to enumerate all the cases where docstrings may be
# disabled, we just check for that directly

def _check_docstrings():
    """Just used to check if docstrings are enabled"""

MISSING_C_DOCSTRINGS = (check_impl_detail() and
                        sys.platform != 'win32' and
                        not sysconfig.get_config_var('WITH_DOC_STRINGS'))

HAVE_DOCSTRINGS = (_check_docstrings.__doc__ is not None and
                   not MISSING_C_DOCSTRINGS)

requires_docstrings = unittest.skipUnless(HAVE_DOCSTRINGS,
                                          "test requires docstrings")


#=======================================================================
# Support for saving and restoring the imported modules.

def flush_std_streams():
    if sys.stdout is not None:
        sys.stdout.flush()
    if sys.stderr is not None:
        sys.stderr.flush()


def print_warning(msg):
    # bpo-45410: Explicitly flush stdout to keep logs in order
    flush_std_streams()
    stream = print_warning.orig_stderr
    for line in msg.splitlines():
        print(f"Warning -- {line}", file=stream)
    stream.flush()

# bpo-39983: Store the original sys.stderr at Python startup to be able to
# log warnings even if sys.stderr is captured temporarily by a test.
print_warning.orig_stderr = sys.stderr


# Flag used by saved_test_environment of test.libregrtest.save_env,
# to check if a test modified the environment. The flag should be set to False
# before running a new test.
#
# For example, threading_helper.threading_cleanup() sets the flag is the function fails
# to cleanup threads.
environment_altered = False

def reap_children():
    """Use this function at the end of test_main() whenever sub-processes
    are started.  This will help ensure that no extra children (zombies)
    stick around to hog resources and create problems when looking
    for refleaks.
    """
    global environment_altered

    # Need os.waitpid(-1, os.WNOHANG): Windows is not supported
    if not (hasattr(os, 'waitpid') and hasattr(os, 'WNOHANG')):
        return
    elif not has_subprocess_support:
        return

    # Reap all our dead child processes so we don't leave zombies around.
    # These hog resources and might be causing some of the buildbots to die.
    while True:
        try:
            # Read the exit status of any child process which already completed
            pid, status = os.waitpid(-1, os.WNOHANG)
        except OSError:
            break

        if pid == 0:
            break

        print_warning(f"reap_children() reaped child process {pid}")
        environment_altered = True


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

        The old value (or None if it doesn't exist) will be assigned to the
        target of the "as" clause, if there is one.
    """
    if hasattr(obj, attr):
        real_val = getattr(obj, attr)
        setattr(obj, attr, new_val)
        try:
            yield real_val
        finally:
            setattr(obj, attr, real_val)
    else:
        setattr(obj, attr, new_val)
        try:
            yield
        finally:
            if hasattr(obj, attr):
                delattr(obj, attr)

@contextlib.contextmanager
def swap_item(obj, item, new_val):
    """Temporary swap out an item with a new object.

    Usage:
        with swap_item(obj, "item", 5):
            ...

        This will set obj["item"] to 5 for the duration of the with: block,
        restoring the old value at the end of the block. If `item` doesn't
        exist on `obj`, it will be created and then deleted at the end of the
        block.

        The old value (or None if it doesn't exist) will be assigned to the
        target of the "as" clause, if there is one.
    """
    if item in obj:
        real_val = obj[item]
        obj[item] = new_val
        try:
            yield real_val
        finally:
            obj[item] = real_val
    else:
        obj[item] = new_val
        try:
            yield
        finally:
            if item in obj:
                del obj[item]

def args_from_interpreter_flags():
    """Return a list of command-line arguments reproducing the current
    settings in sys.flags and sys.warnoptions."""
    import subprocess
    return subprocess._args_from_interpreter_flags()

def optim_args_from_interpreter_flags():
    """Return a list of command-line arguments reproducing the current
    optimization settings in sys.flags."""
    import subprocess
    return subprocess._optim_args_from_interpreter_flags()


class Matcher(object):

    _partial_matches = ('msg', 'message')

    def matches(self, d, **kwargs):
        """
        Try to match a single dict with the supplied arguments.

        Keys whose values are strings and which are in self._partial_matches
        will be checked for partial (i.e. substring) matches. You can extend
        this scheme to (for example) do regular expression matching, etc.
        """
        result = True
        for k in kwargs:
            v = kwargs[k]
            dv = d.get(k)
            if not self.match_value(k, dv, v):
                result = False
                break
        return result

    def match_value(self, k, dv, v):
        """
        Try to match a single stored value (dv) with a supplied value (v).
        """
        if type(v) != type(dv):
            result = False
        elif type(dv) is not str or k not in self._partial_matches:
            result = (v == dv)
        else:
            result = dv.find(v) >= 0
        return result


_buggy_ucrt = None
def skip_if_buggy_ucrt_strfptime(test):
    """
    Skip decorator for tests that use buggy strptime/strftime

    If the UCRT bugs are present time.localtime().tm_zone will be
    an empty string, otherwise we assume the UCRT bugs are fixed

    See bpo-37552 [Windows] strptime/strftime return invalid
    results with UCRT version 17763.615
    """
    import locale
    global _buggy_ucrt
    if _buggy_ucrt is None:
        if(sys.platform == 'win32' and
                locale.getencoding() == 'cp65001' and
                time.localtime().tm_zone == ''):
            _buggy_ucrt = True
        else:
            _buggy_ucrt = False
    return unittest.skip("buggy MSVC UCRT strptime/strftime")(test) if _buggy_ucrt else test

class PythonSymlink:
    """Creates a symlink for the current Python executable"""
    def __init__(self, link=None):
        from .os_helper import TESTFN

        self.link = link or os.path.abspath(TESTFN)
        self._linked = []
        self.real = os.path.realpath(sys.executable)
        self._also_link = []

        self._env = None

        self._platform_specific()

    if sys.platform == "win32":
        def _platform_specific(self):
            import glob
            import _winapi

            if os.path.lexists(self.real) and not os.path.exists(self.real):
                # App symlink appears to not exist, but we want the
                # real executable here anyway
                self.real = _winapi.GetModuleFileName(0)

            dll = _winapi.GetModuleFileName(sys.dllhandle)
            src_dir = os.path.dirname(dll)
            dest_dir = os.path.dirname(self.link)
            self._also_link.append((
                dll,
                os.path.join(dest_dir, os.path.basename(dll))
            ))
            for runtime in glob.glob(os.path.join(glob.escape(src_dir), "vcruntime*.dll")):
                self._also_link.append((
                    runtime,
                    os.path.join(dest_dir, os.path.basename(runtime))
                ))

            self._env = {k.upper(): os.getenv(k) for k in os.environ}
            self._env["PYTHONHOME"] = os.path.dirname(self.real)
            if sysconfig.is_python_build():
                self._env["PYTHONPATH"] = STDLIB_DIR
    else:
        def _platform_specific(self):
            pass

    def __enter__(self):
        os.symlink(self.real, self.link)
        self._linked.append(self.link)
        for real, link in self._also_link:
            os.symlink(real, link)
            self._linked.append(link)
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        for link in self._linked:
            try:
                os.remove(link)
            except IOError as ex:
                if verbose:
                    print("failed to clean up {}: {}".format(link, ex))

    def _call(self, python, args, env, returncode):
        import subprocess
        cmd = [python, *args]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, env=env)
        r = p.communicate()
        if p.returncode != returncode:
            if verbose:
                print(repr(r[0]))
                print(repr(r[1]), file=sys.stderr)
            raise RuntimeError(
                'unexpected return code: {0} (0x{0:08X})'.format(p.returncode))
        return r

    def call_real(self, *args, returncode=0):
        return self._call(self.real, args, None, returncode)

    def call_link(self, *args, returncode=0):
        return self._call(self.link, args, self._env, returncode)


def skip_if_pgo_task(test):
    """Skip decorator for tests not run in (non-extended) PGO task"""
    ok = not PGO or PGO_EXTENDED
    msg = "Not run for (non-extended) PGO task"
    return test if ok else unittest.skip(msg)(test)


def detect_api_mismatch(ref_api, other_api, *, ignore=()):
    """Returns the set of items in ref_api not in other_api, except for a
    defined list of items to be ignored in this check.

    By default this skips private attributes beginning with '_' but
    includes all magic methods, i.e. those starting and ending in '__'.
    """
    missing_items = set(dir(ref_api)) - set(dir(other_api))
    if ignore:
        missing_items -= set(ignore)
    missing_items = set(m for m in missing_items
                        if not m.startswith('_') or m.endswith('__'))
    return missing_items


def check__all__(test_case, module, name_of_module=None, extra=(),
                 not_exported=()):
    """Assert that the __all__ variable of 'module' contains all public names.

    The module's public names (its API) are detected automatically based on
    whether they match the public name convention and were defined in
    'module'.

    The 'name_of_module' argument can specify (as a string or tuple thereof)
    what module(s) an API could be defined in in order to be detected as a
    public API. One case for this is when 'module' imports part of its public
    API from other modules, possibly a C backend (like 'csv' and its '_csv').

    The 'extra' argument can be a set of names that wouldn't otherwise be
    automatically detected as "public", like objects without a proper
    '__module__' attribute. If provided, it will be added to the
    automatically detected ones.

    The 'not_exported' argument can be a set of names that must not be treated
    as part of the public API even though their names indicate otherwise.

    Usage:
        import bar
        import foo
        import unittest
        from test import support

        class MiscTestCase(unittest.TestCase):
            def test__all__(self):
                support.check__all__(self, foo)

        class OtherTestCase(unittest.TestCase):
            def test__all__(self):
                extra = {'BAR_CONST', 'FOO_CONST'}
                not_exported = {'baz'}  # Undocumented name.
                # bar imports part of its API from _bar.
                support.check__all__(self, bar, ('bar', '_bar'),
                                     extra=extra, not_exported=not_exported)

    """

    if name_of_module is None:
        name_of_module = (module.__name__, )
    elif isinstance(name_of_module, str):
        name_of_module = (name_of_module, )

    expected = set(extra)

    for name in dir(module):
        if name.startswith('_') or name in not_exported:
            continue
        obj = getattr(module, name)
        if (getattr(obj, '__module__', None) in name_of_module or
                (not hasattr(obj, '__module__') and
                 not isinstance(obj, types.ModuleType))):
            expected.add(name)
    test_case.assertCountEqual(module.__all__, expected)


def suppress_msvcrt_asserts(verbose=False):
    try:
        import msvcrt
    except ImportError:
        return

    msvcrt.SetErrorMode(msvcrt.SEM_FAILCRITICALERRORS
                        | msvcrt.SEM_NOALIGNMENTFAULTEXCEPT
                        | msvcrt.SEM_NOGPFAULTERRORBOX
                        | msvcrt.SEM_NOOPENFILEERRORBOX)

    # CrtSetReportMode() is only available in debug build
    if hasattr(msvcrt, 'CrtSetReportMode'):
        for m in [msvcrt.CRT_WARN, msvcrt.CRT_ERROR, msvcrt.CRT_ASSERT]:
            if verbose:
                msvcrt.CrtSetReportMode(m, msvcrt.CRTDBG_MODE_FILE)
                msvcrt.CrtSetReportFile(m, msvcrt.CRTDBG_FILE_STDERR)
            else:
                msvcrt.CrtSetReportMode(m, 0)


class SuppressCrashReport:
    """Try to prevent a crash report from popping up.

    On Windows, don't display the Windows Error Reporting dialog.  On UNIX,
    disable the creation of coredump file.
    """
    old_value = None
    old_modes = None

    def __enter__(self):
        """On Windows, disable Windows Error Reporting dialogs using
        SetErrorMode() and CrtSetReportMode().

        On UNIX, try to save the previous core file size limit, then set
        soft limit to 0.
        """
        if sys.platform.startswith('win'):
            # see http://msdn.microsoft.com/en-us/library/windows/desktop/ms680621.aspx
            try:
                import msvcrt
            except ImportError:
                return

            self.old_value = msvcrt.GetErrorMode()

            msvcrt.SetErrorMode(self.old_value | msvcrt.SEM_NOGPFAULTERRORBOX)

            # bpo-23314: Suppress assert dialogs in debug builds.
            # CrtSetReportMode() is only available in debug build.
            if hasattr(msvcrt, 'CrtSetReportMode'):
                self.old_modes = {}
                for report_type in [msvcrt.CRT_WARN,
                                    msvcrt.CRT_ERROR,
                                    msvcrt.CRT_ASSERT]:
                    old_mode = msvcrt.CrtSetReportMode(report_type,
                            msvcrt.CRTDBG_MODE_FILE)
                    old_file = msvcrt.CrtSetReportFile(report_type,
                            msvcrt.CRTDBG_FILE_STDERR)
                    self.old_modes[report_type] = old_mode, old_file

        else:
            try:
                import resource
                self.resource = resource
            except ImportError:
                self.resource = None
            if self.resource is not None:
                try:
                    self.old_value = self.resource.getrlimit(self.resource.RLIMIT_CORE)
                    self.resource.setrlimit(self.resource.RLIMIT_CORE,
                                            (0, self.old_value[1]))
                except (ValueError, OSError):
                    pass

            if sys.platform == 'darwin':
                import subprocess
                # Check if the 'Crash Reporter' on OSX was configured
                # in 'Developer' mode and warn that it will get triggered
                # when it is.
                #
                # This assumes that this context manager is used in tests
                # that might trigger the next manager.
                cmd = ['/usr/bin/defaults', 'read',
                       'com.apple.CrashReporter', 'DialogType']
                proc = subprocess.Popen(cmd,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
                with proc:
                    stdout = proc.communicate()[0]
                if stdout.strip() == b'developer':
                    print("this test triggers the Crash Reporter, "
                          "that is intentional", end='', flush=True)

        return self

    def __exit__(self, *ignore_exc):
        """Restore Windows ErrorMode or core file behavior to initial value."""
        if self.old_value is None:
            return

        if sys.platform.startswith('win'):
            import msvcrt
            msvcrt.SetErrorMode(self.old_value)

            if self.old_modes:
                for report_type, (old_mode, old_file) in self.old_modes.items():
                    msvcrt.CrtSetReportMode(report_type, old_mode)
                    msvcrt.CrtSetReportFile(report_type, old_file)
        else:
            if self.resource is not None:
                try:
                    self.resource.setrlimit(self.resource.RLIMIT_CORE, self.old_value)
                except (ValueError, OSError):
                    pass


def patch(test_instance, object_to_patch, attr_name, new_value):
    """Override 'object_to_patch'.'attr_name' with 'new_value'.

    Also, add a cleanup procedure to 'test_instance' to restore
    'object_to_patch' value for 'attr_name'.
    The 'attr_name' should be a valid attribute for 'object_to_patch'.

    """
    # check that 'attr_name' is a real attribute for 'object_to_patch'
    # will raise AttributeError if it does not exist
    getattr(object_to_patch, attr_name)

    # keep a copy of the old value
    attr_is_local = False
    try:
        old_value = object_to_patch.__dict__[attr_name]
    except (AttributeError, KeyError):
        old_value = getattr(object_to_patch, attr_name, None)
    else:
        attr_is_local = True

    # restore the value when the test is done
    def cleanup():
        if attr_is_local:
            setattr(object_to_patch, attr_name, old_value)
        else:
            delattr(object_to_patch, attr_name)

    test_instance.addCleanup(cleanup)

    # actually override the attribute
    setattr(object_to_patch, attr_name, new_value)


@contextlib.contextmanager
def patch_list(orig):
    """Like unittest.mock.patch.dict, but for lists."""
    try:
        saved = orig[:]
        yield
    finally:
        orig[:] = saved


def run_in_subinterp(code):
    """
    Run code in a subinterpreter. Raise unittest.SkipTest if the tracemalloc
    module is enabled.
    """
    _check_tracemalloc()
    import _testcapi
    return _testcapi.run_in_subinterp(code)


def run_in_subinterp_with_config(code, *, own_gil=None, **config):
    """
    Run code in a subinterpreter. Raise unittest.SkipTest if the tracemalloc
    module is enabled.
    """
    _check_tracemalloc()
    import _testinternalcapi
    if own_gil is not None:
        assert 'gil' not in config, (own_gil, config)
        config['gil'] = 2 if own_gil else 1
    return _testinternalcapi.run_in_subinterp_with_config(code, **config)


def _check_tracemalloc():
    # Issue #10915, #15751: PyGILState_*() functions don't work with
    # sub-interpreters, the tracemalloc module uses these functions internally
    try:
        import tracemalloc
    except ImportError:
        pass
    else:
        if tracemalloc.is_tracing():
            raise unittest.SkipTest("run_in_subinterp() cannot be used "
                                     "if tracemalloc module is tracing "
                                     "memory allocations")


def check_free_after_iterating(test, iter, cls, args=()):
    done = False
    def wrapper():
        class A(cls):
            def __del__(self):
                nonlocal done
                done = True
                try:
                    next(it)
                except StopIteration:
                    pass

        it = iter(A(*args))
        # Issue 26494: Shouldn't crash
        test.assertRaises(StopIteration, next, it)

    wrapper()
    # The sequence should be deallocated just after the end of iterating
    gc_collect()
    test.assertTrue(done)


def missing_compiler_executable(cmd_names=[]):
    """Check if the compiler components used to build the interpreter exist.

    Check for the existence of the compiler executables whose names are listed
    in 'cmd_names' or all the compiler executables when 'cmd_names' is empty
    and return the first missing executable or None when none is found
    missing.

    """
    from setuptools._distutils import ccompiler, sysconfig, spawn
    from setuptools import errors

    compiler = ccompiler.new_compiler()
    sysconfig.customize_compiler(compiler)
    if compiler.compiler_type == "msvc":
        # MSVC has no executables, so check whether initialization succeeds
        try:
            compiler.initialize()
        except errors.PlatformError:
            return "msvc"
    for name in compiler.executables:
        if cmd_names and name not in cmd_names:
            continue
        cmd = getattr(compiler, name)
        if cmd_names:
            assert cmd is not None, \
                    "the '%s' executable is not configured" % name
        elif not cmd:
            continue
        if spawn.find_executable(cmd[0]) is None:
            return cmd[0]


_is_android_emulator = None
def setswitchinterval(interval):
    # Setting a very low gil interval on the Android emulator causes python
    # to hang (issue #26939).
    minimum_interval = 1e-5
    if is_android and interval < minimum_interval:
        global _is_android_emulator
        if _is_android_emulator is None:
            import subprocess
            _is_android_emulator = (subprocess.check_output(
                               ['getprop', 'ro.kernel.qemu']).strip() == b'1')
        if _is_android_emulator:
            interval = minimum_interval
    return sys.setswitchinterval(interval)


def get_pagesize():
    """Get size of a page in bytes."""
    try:
        page_size = os.sysconf('SC_PAGESIZE')
    except (ValueError, AttributeError):
        try:
            page_size = os.sysconf('SC_PAGE_SIZE')
        except (ValueError, AttributeError):
            page_size = 4096
    return page_size


@contextlib.contextmanager
def disable_faulthandler():
    import faulthandler

    # use sys.__stderr__ instead of sys.stderr, since regrtest replaces
    # sys.stderr with a StringIO which has no file descriptor when a test
    # is run with -W/--verbose3.
    fd = sys.__stderr__.fileno()

    is_enabled = faulthandler.is_enabled()
    try:
        faulthandler.disable()
        yield
    finally:
        if is_enabled:
            faulthandler.enable(file=fd, all_threads=True)


class SaveSignals:
    """
    Save and restore signal handlers.

    This class is only able to save/restore signal handlers registered
    by the Python signal module: see bpo-13285 for "external" signal
    handlers.
    """

    def __init__(self):
        import signal
        self.signal = signal
        self.signals = signal.valid_signals()
        # SIGKILL and SIGSTOP signals cannot be ignored nor caught
        for signame in ('SIGKILL', 'SIGSTOP'):
            try:
                signum = getattr(signal, signame)
            except AttributeError:
                continue
            self.signals.remove(signum)
        self.handlers = {}

    def save(self):
        for signum in self.signals:
            handler = self.signal.getsignal(signum)
            if handler is None:
                # getsignal() returns None if a signal handler was not
                # registered by the Python signal module,
                # and the handler is not SIG_DFL nor SIG_IGN.
                #
                # Ignore the signal: we cannot restore the handler.
                continue
            self.handlers[signum] = handler

    def restore(self):
        for signum, handler in self.handlers.items():
            self.signal.signal(signum, handler)


def with_pymalloc():
    import _testcapi
    return _testcapi.WITH_PYMALLOC and not Py_GIL_DISABLED


def with_mimalloc():
    import _testcapi
    return _testcapi.WITH_MIMALLOC


class _ALWAYS_EQ:
    """
    Object that is equal to anything.
    """
    def __eq__(self, other):
        return True
    def __ne__(self, other):
        return False

ALWAYS_EQ = _ALWAYS_EQ()

class _NEVER_EQ:
    """
    Object that is not equal to anything.
    """
    def __eq__(self, other):
        return False
    def __ne__(self, other):
        return True
    def __hash__(self):
        return 1

NEVER_EQ = _NEVER_EQ()

@functools.total_ordering
class _LARGEST:
    """
    Object that is greater than anything (except itself).
    """
    def __eq__(self, other):
        return isinstance(other, _LARGEST)
    def __lt__(self, other):
        return False

LARGEST = _LARGEST()

@functools.total_ordering
class _SMALLEST:
    """
    Object that is less than anything (except itself).
    """
    def __eq__(self, other):
        return isinstance(other, _SMALLEST)
    def __gt__(self, other):
        return False

SMALLEST = _SMALLEST()

def maybe_get_event_loop_policy():
    """Return the global event loop policy if one is set, else return None."""
    import asyncio.events
    return asyncio.events._event_loop_policy

# Helpers for testing hashing.
NHASHBITS = sys.hash_info.width # number of bits in hash() result
assert NHASHBITS in (32, 64)

# Return mean and sdev of number of collisions when tossing nballs balls
# uniformly at random into nbins bins.  By definition, the number of
# collisions is the number of balls minus the number of occupied bins at
# the end.
def collision_stats(nbins, nballs):
    n, k = nbins, nballs
    # prob a bin empty after k trials = (1 - 1/n)**k
    # mean # empty is then n * (1 - 1/n)**k
    # so mean # occupied is n - n * (1 - 1/n)**k
    # so collisions = k - (n - n*(1 - 1/n)**k)
    #
    # For the variance:
    # n*(n-1)*(1-2/n)**k + meanempty - meanempty**2 =
    # n*(n-1)*(1-2/n)**k + meanempty * (1 - meanempty)
    #
    # Massive cancellation occurs, and, e.g., for a 64-bit hash code
    # 1-1/2**64 rounds uselessly to 1.0.  Rather than make heroic (and
    # error-prone) efforts to rework the naive formulas to avoid those,
    # we use the `decimal` module to get plenty of extra precision.
    #
    # Note:  the exact values are straightforward to compute with
    # rationals, but in context that's unbearably slow, requiring
    # multi-million bit arithmetic.
    import decimal
    with decimal.localcontext() as ctx:
        bits = n.bit_length() * 2  # bits in n**2
        # At least that many bits will likely cancel out.
        # Use that many decimal digits instead.
        ctx.prec = max(bits, 30)
        dn = decimal.Decimal(n)
        p1empty = ((dn - 1) / dn) ** k
        meanempty = n * p1empty
        occupied = n - meanempty
        collisions = k - occupied
        var = dn*(dn-1)*((dn-2)/dn)**k + meanempty * (1 - meanempty)
        return float(collisions), float(var.sqrt())


class catch_unraisable_exception:
    """
    Context manager catching unraisable exception using sys.unraisablehook.

    Storing the exception value (cm.unraisable.exc_value) creates a reference
    cycle. The reference cycle is broken explicitly when the context manager
    exits.

    Storing the object (cm.unraisable.object) can resurrect it if it is set to
    an object which is being finalized. Exiting the context manager clears the
    stored object.

    Usage:

        with support.catch_unraisable_exception() as cm:
            # code creating an "unraisable exception"
            ...

            # check the unraisable exception: use cm.unraisable
            ...

        # cm.unraisable attribute no longer exists at this point
        # (to break a reference cycle)
    """

    def __init__(self):
        self.unraisable = None
        self._old_hook = None

    def _hook(self, unraisable):
        # Storing unraisable.object can resurrect an object which is being
        # finalized. Storing unraisable.exc_value creates a reference cycle.
        self.unraisable = unraisable

    def __enter__(self):
        self._old_hook = sys.unraisablehook
        sys.unraisablehook = self._hook
        return self

    def __exit__(self, *exc_info):
        sys.unraisablehook = self._old_hook
        del self.unraisable


def wait_process(pid, *, exitcode, timeout=None):
    """
    Wait until process pid completes and check that the process exit code is
    exitcode.

    Raise an AssertionError if the process exit code is not equal to exitcode.

    If the process runs longer than timeout seconds (LONG_TIMEOUT by default),
    kill the process (if signal.SIGKILL is available) and raise an
    AssertionError. The timeout feature is not available on Windows.
    """
    if os.name != "nt":
        import signal

        if timeout is None:
            timeout = LONG_TIMEOUT

        start_time = time.monotonic()
        for _ in sleeping_retry(timeout, error=False):
            pid2, status = os.waitpid(pid, os.WNOHANG)
            if pid2 != 0:
                break
            # rety: the process is still running
        else:
            try:
                os.kill(pid, signal.SIGKILL)
                os.waitpid(pid, 0)
            except OSError:
                # Ignore errors like ChildProcessError or PermissionError
                pass

            dt = time.monotonic() - start_time
            raise AssertionError(f"process {pid} is still running "
                                 f"after {dt:.1f} seconds")
    else:
        # Windows implementation: don't support timeout :-(
        pid2, status = os.waitpid(pid, 0)

    exitcode2 = os.waitstatus_to_exitcode(status)
    if exitcode2 != exitcode:
        raise AssertionError(f"process {pid} exited with code {exitcode2}, "
                             f"but exit code {exitcode} is expected")

    # sanity check: it should not fail in practice
    if pid2 != pid:
        raise AssertionError(f"pid {pid2} != pid {pid}")

def skip_if_broken_multiprocessing_synchronize():
    """
    Skip tests if the multiprocessing.synchronize module is missing, if there
    is no available semaphore implementation, or if creating a lock raises an
    OSError (on Linux only).
    """
    from .import_helper import import_module

    # Skip tests if the _multiprocessing extension is missing.
    import_module('_multiprocessing')

    # Skip tests if there is no available semaphore implementation:
    # multiprocessing.synchronize requires _multiprocessing.SemLock.
    synchronize = import_module('multiprocessing.synchronize')

    if sys.platform == "linux":
        try:
            # bpo-38377: On Linux, creating a semaphore fails with OSError
            # if the current user does not have the permission to create
            # a file in /dev/shm/ directory.
            synchronize.Lock(ctx=None)
        except OSError as exc:
            raise unittest.SkipTest(f"broken multiprocessing SemLock: {exc!r}")


def check_disallow_instantiation(testcase, tp, *args, **kwds):
    """
    Check that given type cannot be instantiated using *args and **kwds.

    See bpo-43916: Add Py_TPFLAGS_DISALLOW_INSTANTIATION type flag.
    """
    mod = tp.__module__
    name = tp.__name__
    if mod != 'builtins':
        qualname = f"{mod}.{name}"
    else:
        qualname = f"{name}"
    msg = f"cannot create '{re.escape(qualname)}' instances"
    testcase.assertRaisesRegex(TypeError, msg, tp, *args, **kwds)

def get_recursion_depth():
    """Get the recursion depth of the caller function.

    In the __main__ module, at the module level, it should be 1.
    """
    try:
        import _testinternalcapi
        depth = _testinternalcapi.get_recursion_depth()
    except (ImportError, RecursionError) as exc:
        # sys._getframe() + frame.f_back implementation.
        try:
            depth = 0
            frame = sys._getframe()
            while frame is not None:
                depth += 1
                frame = frame.f_back
        finally:
            # Break any reference cycles.
            frame = None

    # Ignore get_recursion_depth() frame.
    return max(depth - 1, 1)

def get_recursion_available():
    """Get the number of available frames before RecursionError.

    It depends on the current recursion depth of the caller function and
    sys.getrecursionlimit().
    """
    limit = sys.getrecursionlimit()
    depth = get_recursion_depth()
    return limit - depth

@contextlib.contextmanager
def set_recursion_limit(limit):
    """Temporarily change the recursion limit."""
    original_limit = sys.getrecursionlimit()
    try:
        sys.setrecursionlimit(limit)
        yield
    finally:
        sys.setrecursionlimit(original_limit)

def infinite_recursion(max_depth=None):
    if max_depth is None:
        # Pick a number large enough to cause problems
        # but not take too long for code that can handle
        # very deep recursion.
        max_depth = 20_000
    elif max_depth < 3:
        raise ValueError("max_depth must be at least 3, got {max_depth}")
    depth = get_recursion_depth()
    depth = max(depth - 1, 1)  # Ignore infinite_recursion() frame.
    limit = depth + max_depth
    return set_recursion_limit(limit)

def ignore_deprecations_from(module: str, *, like: str) -> object:
    token = object()
    warnings.filterwarnings(
        "ignore",
        category=DeprecationWarning,
        module=module,
        message=like + fr"(?#support{id(token)})",
    )
    return token

def clear_ignored_deprecations(*tokens: object) -> None:
    if not tokens:
        raise ValueError("Provide token or tokens returned by ignore_deprecations_from")

    new_filters = []
    endswith = tuple(rf"(?#support{id(token)})" for token in tokens)
    for action, message, category, module, lineno in warnings.filters:
        if action == "ignore" and category is DeprecationWarning:
            if isinstance(message, re.Pattern):
                msg = message.pattern
            else:
                msg = message or ""
            if msg.endswith(endswith):
                continue
        new_filters.append((action, message, category, module, lineno))
    if warnings.filters != new_filters:
        warnings.filters[:] = new_filters
        warnings._filters_mutated()


# Skip a test if venv with pip is known to not work.
def requires_venv_with_pip():
    # ensurepip requires zlib to open ZIP archives (.whl binary wheel packages)
    try:
        import zlib
    except ImportError:
        return unittest.skipIf(True, "venv: ensurepip requires zlib")

    # bpo-26610: pip/pep425tags.py requires ctypes.
    # gh-92820: setuptools/windows_support.py uses ctypes (setuptools 58.1).
    try:
        import ctypes
    except ImportError:
        ctypes = None
    return unittest.skipUnless(ctypes, 'venv: pip requires ctypes')


@functools.cache
def _findwheel(pkgname):
    """Try to find a wheel with the package specified as pkgname.

    If set, the wheels are searched for in WHEEL_PKG_DIR (see ensurepip).
    Otherwise, they are searched for in the test directory.
    """
    wheel_dir = sysconfig.get_config_var('WHEEL_PKG_DIR') or os.path.join(
        TEST_HOME_DIR, 'wheeldata',
    )
    filenames = os.listdir(wheel_dir)
    filenames = sorted(filenames, reverse=True)  # approximate "newest" first
    for filename in filenames:
        # filename is like 'setuptools-67.6.1-py3-none-any.whl'
        if not filename.endswith(".whl"):
            continue
        prefix = pkgname + '-'
        if filename.startswith(prefix):
            return os.path.join(wheel_dir, filename)
    raise FileNotFoundError(f"No wheel for {pkgname} found in {wheel_dir}")


# Context manager that creates a virtual environment, install setuptools and wheel in it
# and returns the path to the venv directory and the path to the python executable
@contextlib.contextmanager
def setup_venv_with_pip_setuptools_wheel(venv_dir):
    import subprocess
    from .os_helper import temp_cwd

    with temp_cwd() as temp_dir:
        # Create virtual environment to get setuptools
        cmd = [sys.executable, '-X', 'dev', '-m', 'venv', venv_dir]
        if verbose:
            print()
            print('Run:', ' '.join(cmd))
        subprocess.run(cmd, check=True)

        venv = os.path.join(temp_dir, venv_dir)

        # Get the Python executable of the venv
        python_exe = os.path.basename(sys.executable)
        if sys.platform == 'win32':
            python = os.path.join(venv, 'Scripts', python_exe)
        else:
            python = os.path.join(venv, 'bin', python_exe)

        cmd = [python, '-X', 'dev',
               '-m', 'pip', 'install',
               _findwheel('setuptools'),
               _findwheel('wheel')]
        if verbose:
            print()
            print('Run:', ' '.join(cmd))
        subprocess.run(cmd, check=True)

        yield python


# True if Python is built with the Py_DEBUG macro defined: if
# Python is built in debug mode (./configure --with-pydebug).
Py_DEBUG = hasattr(sys, 'gettotalrefcount')


def late_deletion(obj):
    """
    Keep a Python alive as long as possible.

    Create a reference cycle and store the cycle in an object deleted late in
    Python finalization. Try to keep the object alive until the very last
    garbage collection.

    The function keeps a strong reference by design. It should be called in a
    subprocess to not mark a test as "leaking a reference".
    """

    # Late CPython finalization:
    # - finalize_interp_clear()
    # - _PyInterpreterState_Clear(): Clear PyInterpreterState members
    #   (ex: codec_search_path, before_forkers)
    # - clear os.register_at_fork() callbacks
    # - clear codecs.register() callbacks

    ref_cycle = [obj]
    ref_cycle.append(ref_cycle)

    # Store a reference in PyInterpreterState.codec_search_path
    import codecs
    def search_func(encoding):
        return None
    search_func.reference = ref_cycle
    codecs.register(search_func)

    if hasattr(os, 'register_at_fork'):
        # Store a reference in PyInterpreterState.before_forkers
        def atfork_func():
            pass
        atfork_func.reference = ref_cycle
        os.register_at_fork(before=atfork_func)


def busy_retry(timeout, err_msg=None, /, *, error=True):
    """
    Run the loop body until "break" stops the loop.

    After *timeout* seconds, raise an AssertionError if *error* is true,
    or just stop if *error is false.

    Example:

        for _ in support.busy_retry(support.SHORT_TIMEOUT):
            if check():
                break

    Example of error=False usage:

        for _ in support.busy_retry(support.SHORT_TIMEOUT, error=False):
            if check():
                break
        else:
            raise RuntimeError('my custom error')

    """
    if timeout <= 0:
        raise ValueError("timeout must be greater than zero")

    start_time = time.monotonic()
    deadline = start_time + timeout

    while True:
        yield

        if time.monotonic() >= deadline:
            break

    if error:
        dt = time.monotonic() - start_time
        msg = f"timeout ({dt:.1f} seconds)"
        if err_msg:
            msg = f"{msg}: {err_msg}"
        raise AssertionError(msg)


def sleeping_retry(timeout, err_msg=None, /,
                     *, init_delay=0.010, max_delay=1.0, error=True):
    """
    Wait strategy that applies exponential backoff.

    Run the loop body until "break" stops the loop. Sleep at each loop
    iteration, but not at the first iteration. The sleep delay is doubled at
    each iteration (up to *max_delay* seconds).

    See busy_retry() documentation for the parameters usage.

    Example raising an exception after SHORT_TIMEOUT seconds:

        for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
            if check():
                break

    Example of error=False usage:

        for _ in support.sleeping_retry(support.SHORT_TIMEOUT, error=False):
            if check():
                break
        else:
            raise RuntimeError('my custom error')
    """

    delay = init_delay
    for _ in busy_retry(timeout, err_msg, error=error):
        yield

        time.sleep(delay)
        delay = min(delay * 2, max_delay)


class CPUStopwatch:
    """Context manager to roughly time a CPU-bound operation.

    Disables GC. Uses CPU time if it can (i.e. excludes sleeps & time of
    other processes).

    N.B.:
    - This *includes* time spent in other threads.
    - Some systems only have a coarse resolution; check
      stopwatch.clock_info.rseolution if.

    Usage:

    with ProcessStopwatch() as stopwatch:
        ...
    elapsed = stopwatch.seconds
    resolution = stopwatch.clock_info.resolution
    """
    def __enter__(self):
        get_time = time.process_time
        clock_info = time.get_clock_info('process_time')
        if get_time() <= 0:  # some platforms like WASM lack process_time()
            get_time = time.monotonic
            clock_info = time.get_clock_info('monotonic')
        self.context = disable_gc()
        self.context.__enter__()
        self.get_time = get_time
        self.clock_info = clock_info
        self.start_time = get_time()
        return self

    def __exit__(self, *exc):
        try:
            end_time = self.get_time()
        finally:
            result = self.context.__exit__(*exc)
        self.seconds = end_time - self.start_time
        return result


@contextlib.contextmanager
def adjust_int_max_str_digits(max_digits):
    """Temporarily change the integer string conversion length limit."""
    current = sys.get_int_max_str_digits()
    try:
        sys.set_int_max_str_digits(max_digits)
        yield
    finally:
        sys.set_int_max_str_digits(current)


def _get_c_recursion_limit():
    try:
        import _testcapi
        return _testcapi.Py_C_RECURSION_LIMIT
    except (ImportError, AttributeError):
        # Originally taken from Include/cpython/pystate.h .
        if sys.platform == 'win32':
            return 4000
        else:
            return 10000

# The default C recursion limit.
Py_C_RECURSION_LIMIT = _get_c_recursion_limit()

#For recursion tests, easily exceeds default recursion limit
EXCEEDS_RECURSION_LIMIT = Py_C_RECURSION_LIMIT * 3

#Windows doesn't have os.uname() but it doesn't support s390x.
skip_on_s390x = unittest.skipIf(hasattr(os, 'uname') and os.uname().machine == 's390x',
                                'skipped on s390x')

Py_TRACE_REFS = hasattr(sys, 'getobjects')

# Decorator to disable optimizer while a function run
def without_optimizer(func):
    try:
        import _testinternalcapi
    except ImportError:
        return func
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        save_opt = _testinternalcapi.get_optimizer()
        try:
            _testinternalcapi.set_optimizer(None)
            return func(*args, **kwargs)
        finally:
            _testinternalcapi.set_optimizer(save_opt)
    return wrapper


_BASE_COPY_SRC_DIR_IGNORED_NAMES = frozenset({
    # SRC_DIR/.git
    '.git',
    # ignore all __pycache__/ sub-directories
    '__pycache__',
})

# Ignore function for shutil.copytree() to copy the Python source code.
def copy_python_src_ignore(path, names):
    ignored = _BASE_COPY_SRC_DIR_IGNORED_NAMES
    if os.path.basename(path) == 'Doc':
        ignored |= {
            # SRC_DIR/Doc/build/
            'build',
            # SRC_DIR/Doc/venv/
            'venv',
        }

    # check if we are at the root of the source code
    elif 'Modules' in names:
        ignored |= {
            # SRC_DIR/build/
            'build',
        }
    return ignored

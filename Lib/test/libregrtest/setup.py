import atexit
import faulthandler
import os
import signal
import sys
import unittest
from test import support
from test.support.os_helper import TESTFN_UNDECODABLE, FS_NONASCII
try:
    import gc
except ImportError:
    gc = None

from test.libregrtest.utils import (setup_unraisable_hook,
                                    setup_threading_excepthook)


UNICODE_GUARD_ENV = "PYTHONREGRTEST_UNICODE_GUARD"


def setup_tests(ns):
    try:
        stderr_fd = sys.__stderr__.fileno()
    except (ValueError, AttributeError):
        # Catch ValueError to catch io.UnsupportedOperation on TextIOBase
        # and ValueError on a closed stream.
        #
        # Catch AttributeError for stderr being None.
        stderr_fd = None
    else:
        # Display the Python traceback on fatal errors (e.g. segfault)
        faulthandler.enable(all_threads=True, file=stderr_fd)

        # Display the Python traceback on SIGALRM or SIGUSR1 signal
        signals = []
        if hasattr(signal, 'SIGALRM'):
            signals.append(signal.SIGALRM)
        if hasattr(signal, 'SIGUSR1'):
            signals.append(signal.SIGUSR1)
        for signum in signals:
            faulthandler.register(signum, chain=True, file=stderr_fd)

    _adjust_resource_limits()
    replace_stdout()
    support.record_original_stdout(sys.stdout)

    if ns.testdir:
        # Prepend test directory to sys.path, so runtest() will be able
        # to locate tests
        sys.path.insert(0, os.path.abspath(ns.testdir))

    # Some times __path__ and __file__ are not absolute (e.g. while running from
    # Lib/) and, if we change the CWD to run the tests in a temporary dir, some
    # imports might fail.  This affects only the modules imported before os.chdir().
    # These modules are searched first in sys.path[0] (so '' -- the CWD) and if
    # they are found in the CWD their __file__ and __path__ will be relative (this
    # happens before the chdir).  All the modules imported after the chdir, are
    # not found in the CWD, and since the other paths in sys.path[1:] are absolute
    # (site.py absolutize them), the __file__ and __path__ will be absolute too.
    # Therefore it is necessary to absolutize manually the __file__ and __path__ of
    # the packages to prevent later imports to fail when the CWD is different.
    for module in sys.modules.values():
        if hasattr(module, '__path__'):
            for index, path in enumerate(module.__path__):
                module.__path__[index] = os.path.abspath(path)
        if getattr(module, '__file__', None):
            module.__file__ = os.path.abspath(module.__file__)

    if ns.huntrleaks:
        unittest.BaseTestSuite._cleanup = False

    if ns.memlimit is not None:
        support.set_memlimit(ns.memlimit)

    if ns.threshold is not None:
        gc.set_threshold(ns.threshold)

    support.suppress_msvcrt_asserts(ns.verbose and ns.verbose >= 2)

    support.use_resources = ns.use_resources

    if hasattr(sys, 'addaudithook'):
        # Add an auditing hook for all tests to ensure PySys_Audit is tested
        def _test_audit_hook(name, args):
            pass
        sys.addaudithook(_test_audit_hook)

    setup_unraisable_hook()
    setup_threading_excepthook()

    if ns.timeout is not None:
        # For a slow buildbot worker, increase SHORT_TIMEOUT and LONG_TIMEOUT
        support.SHORT_TIMEOUT = max(support.SHORT_TIMEOUT, ns.timeout / 40)
        support.LONG_TIMEOUT = max(support.LONG_TIMEOUT, ns.timeout / 4)

        # If --timeout is short: reduce timeouts
        support.LOOPBACK_TIMEOUT = min(support.LOOPBACK_TIMEOUT, ns.timeout)
        support.INTERNET_TIMEOUT = min(support.INTERNET_TIMEOUT, ns.timeout)
        support.SHORT_TIMEOUT = min(support.SHORT_TIMEOUT, ns.timeout)
        support.LONG_TIMEOUT = min(support.LONG_TIMEOUT, ns.timeout)

    if ns.xmlpath:
        from test.support.testresult import RegressionTestResult
        RegressionTestResult.USE_XML = True

    # Ensure there's a non-ASCII character in env vars at all times to force
    # tests consider this case. See BPO-44647 for details.
    if TESTFN_UNDECODABLE and os.supports_bytes_environ:
        os.environb.setdefault(UNICODE_GUARD_ENV.encode(), TESTFN_UNDECODABLE)
    elif FS_NONASCII:
        os.environ.setdefault(UNICODE_GUARD_ENV, FS_NONASCII)


def replace_stdout():
    """Set stdout encoder error handler to backslashreplace (as stderr error
    handler) to avoid UnicodeEncodeError when printing a traceback"""
    stdout = sys.stdout
    try:
        fd = stdout.fileno()
    except ValueError:
        # On IDLE, sys.stdout has no file descriptor and is not a TextIOWrapper
        # object. Leaving sys.stdout unchanged.
        #
        # Catch ValueError to catch io.UnsupportedOperation on TextIOBase
        # and ValueError on a closed stream.
        return

    sys.stdout = open(fd, 'w',
        encoding=stdout.encoding,
        errors="backslashreplace",
        closefd=False,
        newline='\n')

    def restore_stdout():
        sys.stdout.close()
        sys.stdout = stdout
    atexit.register(restore_stdout)


def _adjust_resource_limits():
    """Adjust the system resource limits (ulimit) if needed."""
    try:
        import resource
        from resource import RLIMIT_NOFILE, RLIM_INFINITY
    except ImportError:
        return
    fd_limit, max_fds = resource.getrlimit(RLIMIT_NOFILE)
    # On macOS the default fd limit is sometimes too low (256) for our
    # test suite to succeed.  Raise it to something more reasonable.
    # 1024 is a common Linux default.
    desired_fds = 1024
    if fd_limit < desired_fds and fd_limit < max_fds:
        new_fd_limit = min(desired_fds, max_fds)
        try:
            resource.setrlimit(RLIMIT_NOFILE, (new_fd_limit, max_fds))
            print(f"Raised RLIMIT_NOFILE: {fd_limit} -> {new_fd_limit}")
        except (ValueError, OSError) as err:
            print(f"Unable to raise RLIMIT_NOFILE from {fd_limit} to "
                  f"{new_fd_limit}: {err}.")


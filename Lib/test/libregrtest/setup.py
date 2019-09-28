import atexit
import faulthandler
import os
import signal
import sys
import unittest
from test import support
try:
    import gc
except ImportError:
    gc = None


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

    suppress_msvcrt_asserts(ns.verbose and ns.verbose >= 2)

    support.use_resources = ns.use_resources

    if hasattr(sys, 'addaudithook'):
        # Add an auditing hook for all tests to ensure PySys_Audit is tested
        def _test_audit_hook(name, args):
            pass
        sys.addaudithook(_test_audit_hook)


def suppress_msvcrt_asserts(verbose):
    try:
        import msvcrt
    except ImportError:
        return

    msvcrt.SetErrorMode(msvcrt.SEM_FAILCRITICALERRORS|
                        msvcrt.SEM_NOALIGNMENTFAULTEXCEPT|
                        msvcrt.SEM_NOGPFAULTERRORBOX|
                        msvcrt.SEM_NOOPENFILEERRORBOX)
    try:
        msvcrt.CrtSetReportMode
    except AttributeError:
        # release build
        return

    for m in [msvcrt.CRT_WARN, msvcrt.CRT_ERROR, msvcrt.CRT_ASSERT]:
        if verbose:
            msvcrt.CrtSetReportMode(m, msvcrt.CRTDBG_MODE_FILE)
            msvcrt.CrtSetReportFile(m, msvcrt.CRTDBG_FILE_STDERR)
        else:
            msvcrt.CrtSetReportMode(m, 0)



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

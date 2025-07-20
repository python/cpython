import faulthandler
import gc
import io
import os
import random
import signal
import sys
import unittest
from test import support
from test.support.os_helper import TESTFN_UNDECODABLE, FS_NONASCII

from .filter import set_match_tests
from .runtests import RunTests
from .utils import (
    setup_unraisable_hook, setup_threading_excepthook,
    adjust_rlimit_nofile)


UNICODE_GUARD_ENV = "PYTHONREGRTEST_UNICODE_GUARD"


def setup_test_dir(testdir: str | None) -> None:
    if testdir:
        # Prepend test directory to sys.path, so runtest() will be able
        # to locate tests
        sys.path.insert(0, os.path.abspath(testdir))


def setup_process() -> None:
    assert sys.__stderr__ is not None, "sys.__stderr__ is None"
    try:
        stderr_fd = sys.__stderr__.fileno()
    except (ValueError, AttributeError):
        # Catch ValueError to catch io.UnsupportedOperation on TextIOBase
        # and ValueError on a closed stream.
        #
        # Catch AttributeError for stderr being None.
        pass
    else:
        # Display the Python traceback on fatal errors (e.g. segfault)
        faulthandler.enable(all_threads=True, file=stderr_fd)

        # Display the Python traceback on SIGALRM or SIGUSR1 signal
        signals: list[signal.Signals] = []
        if hasattr(signal, 'SIGALRM'):
            signals.append(signal.SIGALRM)
        if hasattr(signal, 'SIGUSR1'):
            signals.append(signal.SIGUSR1)
        for signum in signals:
            faulthandler.register(signum, chain=True, file=stderr_fd)

    adjust_rlimit_nofile()

    support.record_original_stdout(sys.stdout)

    # Set sys.stdout encoder error handler to backslashreplace,
    # similar to sys.stderr error handler, to avoid UnicodeEncodeError
    # when printing a traceback or any other non-encodable character.
    #
    # Use an assertion to fix mypy error.
    assert isinstance(sys.stdout, io.TextIOWrapper)
    sys.stdout.reconfigure(errors="backslashreplace")

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
            module.__file__ = os.path.abspath(module.__file__)  # type: ignore[type-var]

    if hasattr(sys, 'addaudithook'):
        # Add an auditing hook for all tests to ensure PySys_Audit is tested
        def _test_audit_hook(name, args):
            pass
        sys.addaudithook(_test_audit_hook)

    setup_unraisable_hook()
    setup_threading_excepthook()

    # Ensure there's a non-ASCII character in env vars at all times to force
    # tests consider this case. See BPO-44647 for details.
    if TESTFN_UNDECODABLE and os.supports_bytes_environ:
        os.environb.setdefault(UNICODE_GUARD_ENV.encode(), TESTFN_UNDECODABLE)
    elif FS_NONASCII:
        os.environ.setdefault(UNICODE_GUARD_ENV, FS_NONASCII)


def setup_tests(runtests: RunTests) -> None:
    support.verbose = runtests.verbose
    support.failfast = runtests.fail_fast
    support.PGO = runtests.pgo
    support.PGO_EXTENDED = runtests.pgo_extended

    set_match_tests(runtests.match_tests)

    if runtests.use_junit:
        support.junit_xml_list = []
        from .testresult import RegressionTestResult
        RegressionTestResult.USE_XML = True
    else:
        support.junit_xml_list = None

    if runtests.memory_limit is not None:
        support.set_memlimit(runtests.memory_limit)

    support.suppress_msvcrt_asserts(runtests.verbose >= 2)

    support.use_resources = runtests.use_resources

    timeout = runtests.timeout
    if timeout is not None:
        # For a slow buildbot worker, increase SHORT_TIMEOUT and LONG_TIMEOUT
        support.LOOPBACK_TIMEOUT = max(support.LOOPBACK_TIMEOUT, timeout / 120)
        # don't increase INTERNET_TIMEOUT
        support.SHORT_TIMEOUT = max(support.SHORT_TIMEOUT, timeout / 40)
        support.LONG_TIMEOUT = max(support.LONG_TIMEOUT, timeout / 4)

        # If --timeout is short: reduce timeouts
        support.LOOPBACK_TIMEOUT = min(support.LOOPBACK_TIMEOUT, timeout)
        support.INTERNET_TIMEOUT = min(support.INTERNET_TIMEOUT, timeout)
        support.SHORT_TIMEOUT = min(support.SHORT_TIMEOUT, timeout)
        support.LONG_TIMEOUT = min(support.LONG_TIMEOUT, timeout)

    if runtests.hunt_refleak:
        # private attribute that mypy doesn't know about:
        unittest.BaseTestSuite._cleanup = False  # type: ignore[attr-defined]

    if runtests.gc_threshold is not None:
        gc.set_threshold(runtests.gc_threshold)

    random.seed(runtests.random_seed)

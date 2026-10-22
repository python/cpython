import sys
import unittest
import threading

from test import support
from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

syslog = import_helper.import_module("syslog")

NTHREADS = 32

# Similar to Lib/test/test_syslog.py, this test's purpose is to verify that
# the code neither crashes nor leaks.


@threading_helper.requires_working_threading()
class TestSyslog(unittest.TestCase):
    def test_racing_syslog(self):
        def worker():
            """
            The syslog module provides the following functions:
            openlog(), syslog(), closelog(), and setlogmask().
            """
            thread_id = threading.get_ident()
            syslog.openlog(f"thread-id: {thread_id}")
            try:
                for _ in range(5):
                    syslog.syslog("logline")
                    syslog.setlogmask(syslog.LOG_MASK(syslog.LOG_INFO))
                    syslog.syslog(syslog.LOG_INFO, "logline LOG_INFO")
                    syslog.setlogmask(syslog.LOG_MASK(syslog.LOG_ERR))
                    syslog.syslog(syslog.LOG_ERR, "logline LOG_ERR")
                    syslog.setlogmask(syslog.LOG_UPTO(syslog.LOG_DEBUG))
            finally:
                syslog.closelog()

        # Run the worker concurrently to exercise all these syslog functions
        run_concurrently(
            worker_func=worker,
            nthreads=NTHREADS,
        )

    def test_racing_openlog_argv(self):
        # gh-159864: openlog() without an ident reads sys.argv[0] while
        # another thread replaces it.
        argv = ["prog"]

        def opener():
            for _ in range(1000):
                syslog.openlog()

        def writer():
            for i in range(1000):
                argv[0] = f"prog-{i}"

        with support.swap_attr(sys, "argv", argv):
            try:
                run_concurrently(
                    worker_func=[opener] * (NTHREADS - 1) + [writer],
                )
            finally:
                syslog.closelog()


if __name__ == "__main__":
    unittest.main()

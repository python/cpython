"""This test checks for correct fork() behavior.
"""

import _imp as imp
import os
import signal
import sys
import threading
import time
import unittest

from test.fork_wait import ForkWait
from test import support
from test.support import warnings_helper


# Skip test if fork does not exist.
if not support.has_fork_support:
    raise unittest.SkipTest("test module requires working os.fork")


class ForkTest(ForkWait):
    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_threaded_import_lock_fork(self):
        """Check fork() in main thread works while a subthread is doing an import"""
        import_started = threading.Event()
        fake_module_name = "fake test module"
        partial_module = "partial"
        complete_module = "complete"
        def importer():
            imp.acquire_lock()
            sys.modules[fake_module_name] = partial_module
            import_started.set()
            time.sleep(0.01) # Give the other thread time to try and acquire.
            sys.modules[fake_module_name] = complete_module
            imp.release_lock()
        t = threading.Thread(target=importer)
        t.start()
        import_started.wait()
        exitcode = 42
        pid = os.fork()
        try:
            # PyOS_BeforeFork should have waited for the import to complete
            # before forking, so the child can recreate the import lock
            # correctly, but also won't see a partially initialised module
            if not pid:
                m = __import__(fake_module_name)
                if m == complete_module:
                    os._exit(exitcode)
                else:
                    if support.verbose > 1:
                        print("Child encountered partial module")
                    os._exit(1)
            else:
                t.join()
                # Exitcode 1 means the child got a partial module (bad.) No
                # exitcode (but a hang, which manifests as 'got pid 0')
                # means the child deadlocked (also bad.)
                self.wait_impl(pid, exitcode=exitcode)
        finally:
            try:
                os.kill(pid, signal.SIGKILL)
            except OSError:
                pass

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_nested_import_lock_fork(self):
        """Check fork() in main thread works while the main thread is doing an import"""
        exitcode = 42
        # Issue 9573: this used to trigger RuntimeError in the child process
        def fork_with_import_lock(level):
            release = 0
            in_child = False
            try:
                try:
                    for i in range(level):
                        imp.acquire_lock()
                        release += 1
                    pid = os.fork()
                    in_child = not pid
                finally:
                    for i in range(release):
                        imp.release_lock()
            except RuntimeError:
                if in_child:
                    if support.verbose > 1:
                        print("RuntimeError in child")
                    os._exit(1)
                raise
            if in_child:
                os._exit(exitcode)
            self.wait_impl(pid, exitcode=exitcode)

        # Check this works with various levels of nested
        # import in the main thread
        for level in range(5):
            fork_with_import_lock(level)


def tearDownModule():
    support.reap_children()

if __name__ == "__main__":
    unittest.main()

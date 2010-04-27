"""This test checks for correct fork() behavior.
"""

import imp
import os
import signal
import sys
import time

from test.fork_wait import ForkWait
from test.test_support import run_unittest, reap_children, get_attribute, import_module
threading = import_module('threading')

#Skip test if fork does not exist.
get_attribute(os, 'fork')


class ForkTest(ForkWait):
    def wait_impl(self, cpid):
        for i in range(10):
            # waitpid() shouldn't hang, but some of the buildbots seem to hang
            # in the forking tests.  This is an attempt to fix the problem.
            spid, status = os.waitpid(cpid, os.WNOHANG)
            if spid == cpid:
                break
            time.sleep(1.0)

        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))

    def test_import_lock_fork(self):
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
        pid = os.fork()
        try:
            if not pid:
                m = __import__(fake_module_name)
                if m == complete_module:
                    os._exit(0)
                else:
                    os._exit(1)
            else:
                t.join()
                # Exitcode 1 means the child got a partial module (bad.) No
                # exitcode (but a hang, which manifests as 'got pid 0')
                # means the child deadlocked (also bad.)
                self.wait_impl(pid)
        finally:
            try:
                os.kill(pid, signal.SIGKILL)
            except OSError:
                pass

def test_main():
    run_unittest(ForkTest)
    reap_children()

if __name__ == "__main__":
    test_main()

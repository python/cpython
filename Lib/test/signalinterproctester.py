import gc
import os
import signal
import subprocess
import sys
import time
import unittest
from test import support


class SIGUSR1Exception(Exception):
    pass


class InterProcessSignalTests(unittest.TestCase):
    def setUp(self):
        self.got_signals = {'SIGHUP': 0, 'SIGUSR1': 0, 'SIGALRM': 0}

    def sighup_handler(self, signum, frame):
        self.got_signals['SIGHUP'] += 1

    def sigusr1_handler(self, signum, frame):
        self.got_signals['SIGUSR1'] += 1
        raise SIGUSR1Exception

    def wait_signal(self, child, signame):
        if child is not None:
            # This wait should be interrupted by exc_class
            # (if set)
            child.wait()

        start_time = time.monotonic()
        for _ in support.busy_retry(support.SHORT_TIMEOUT, error=False):
            if self.got_signals[signame]:
                return
            signal.pause()
        else:
            dt = time.monotonic() - start_time
            self.fail('signal %s not received after %.1f seconds'
                      % (signame, dt))

    def subprocess_send_signal(self, pid, signame):
        code = 'import os, signal; os.kill(%s, signal.%s)' % (pid, signame)
        args = [sys.executable, '-I', '-c', code]
        return subprocess.Popen(args)

    def test_interprocess_signal(self):
        # Install handlers. This function runs in a sub-process, so we
        # don't worry about re-setting the default handlers.
        signal.signal(signal.SIGHUP, self.sighup_handler)
        signal.signal(signal.SIGUSR1, self.sigusr1_handler)
        signal.signal(signal.SIGUSR2, signal.SIG_IGN)
        signal.signal(signal.SIGALRM, signal.default_int_handler)

        # Let the sub-processes know who to send signals to.
        pid = str(os.getpid())

        with self.subprocess_send_signal(pid, "SIGHUP") as child:
            self.wait_signal(child, 'SIGHUP')
        self.assertEqual(self.got_signals, {'SIGHUP': 1, 'SIGUSR1': 0,
                                            'SIGALRM': 0})

        # gh-110033: Make sure that the subprocess.Popen is deleted before
        # the next test which raises an exception. Otherwise, the exception
        # may be raised when Popen.__del__() is executed and so be logged
        # as "Exception ignored in: <function Popen.__del__ at ...>".
        child = None
        gc.collect()

        with self.assertRaises(SIGUSR1Exception):
            with self.subprocess_send_signal(pid, "SIGUSR1") as child:
                self.wait_signal(child, 'SIGUSR1')
        self.assertEqual(self.got_signals, {'SIGHUP': 1, 'SIGUSR1': 1,
                                            'SIGALRM': 0})

        with self.subprocess_send_signal(pid, "SIGUSR2") as child:
            # Nothing should happen: SIGUSR2 is ignored
            child.wait()

        try:
            with self.assertRaises(KeyboardInterrupt):
                signal.alarm(1)
                self.wait_signal(None, 'SIGALRM')
            self.assertEqual(self.got_signals, {'SIGHUP': 1, 'SIGUSR1': 1,
                                                'SIGALRM': 0})
        finally:
            signal.alarm(0)


if __name__ == "__main__":
    unittest.main()

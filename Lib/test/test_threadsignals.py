"""PyUnit testing that threads honor our signal semantics"""

import unittest
import signal
import os
import sys
from test.support import run_unittest, import_module
thread = import_module('_thread')
import time

if sys.platform[:3] in ('win', 'os2') or sys.platform=='riscos':
    raise unittest.SkipTest("Can't test signal on %s" % sys.platform)

process_pid = os.getpid()
signalled_all=thread.allocate_lock()


def registerSignals(for_usr1, for_usr2, for_alrm):
    usr1 = signal.signal(signal.SIGUSR1, for_usr1)
    usr2 = signal.signal(signal.SIGUSR2, for_usr2)
    alrm = signal.signal(signal.SIGALRM, for_alrm)
    return usr1, usr2, alrm


# The signal handler. Just note that the signal occurred and
# from who.
def handle_signals(sig,frame):
    signal_blackboard[sig]['tripped'] += 1
    signal_blackboard[sig]['tripped_by'] = thread.get_ident()

# a function that will be spawned as a separate thread.
def send_signals():
    os.kill(process_pid, signal.SIGUSR1)
    os.kill(process_pid, signal.SIGUSR2)
    signalled_all.release()

class ThreadSignals(unittest.TestCase):

    def test_signals(self):
        # Test signal handling semantics of threads.
        # We spawn a thread, have the thread send two signals, and
        # wait for it to finish. Check that we got both signals
        # and that they were run by the main thread.
        signalled_all.acquire()
        self.spawnSignallingThread()
        signalled_all.acquire()
        # the signals that we asked the kernel to send
        # will come back, but we don't know when.
        # (it might even be after the thread exits
        # and might be out of order.)  If we haven't seen
        # the signals yet, send yet another signal and
        # wait for it return.
        if signal_blackboard[signal.SIGUSR1]['tripped'] == 0 \
           or signal_blackboard[signal.SIGUSR2]['tripped'] == 0:
            signal.alarm(1)
            signal.pause()
            signal.alarm(0)

        self.assertEqual( signal_blackboard[signal.SIGUSR1]['tripped'], 1)
        self.assertEqual( signal_blackboard[signal.SIGUSR1]['tripped_by'],
                           thread.get_ident())
        self.assertEqual( signal_blackboard[signal.SIGUSR2]['tripped'], 1)
        self.assertEqual( signal_blackboard[signal.SIGUSR2]['tripped_by'],
                           thread.get_ident())
        signalled_all.release()

    def spawnSignallingThread(self):
        thread.start_new_thread(send_signals, ())

    def alarm_interrupt(self, sig, frame):
        raise KeyboardInterrupt

    def test_lock_acquire_interruption(self):
        # Mimic receiving a SIGINT (KeyboardInterrupt) with SIGALRM while stuck
        # in a deadlock.
        oldalrm = signal.signal(signal.SIGALRM, self.alarm_interrupt)
        try:
            lock = thread.allocate_lock()
            lock.acquire()
            signal.alarm(1)
            self.assertRaises(KeyboardInterrupt, lock.acquire)
        finally:
            signal.signal(signal.SIGALRM, oldalrm)

    def test_rlock_acquire_interruption(self):
        # Mimic receiving a SIGINT (KeyboardInterrupt) with SIGALRM while stuck
        # in a deadlock.
        oldalrm = signal.signal(signal.SIGALRM, self.alarm_interrupt)
        try:
            rlock = thread.RLock()
            # For reentrant locks, the initial acquisition must be in another
            # thread.
            def other_thread():
                rlock.acquire()
            thread.start_new_thread(other_thread, ())
            # Wait until we can't acquire it without blocking...
            while rlock.acquire(blocking=False):
                rlock.release()
                time.sleep(0.01)
            signal.alarm(1)
            self.assertRaises(KeyboardInterrupt, rlock.acquire)
        finally:
            signal.signal(signal.SIGALRM, oldalrm)

    def acquire_retries_on_intr(self, lock):
        self.sig_recvd = False
        def my_handler(signal, frame):
            self.sig_recvd = True
        old_handler = signal.signal(signal.SIGUSR1, my_handler)
        try:
            def other_thread():
                # Acquire the lock in a non-main thread, so this test works for
                # RLocks.
                lock.acquire()
                # Wait until the main thread is blocked in the lock acquire, and
                # then wake it up with this.
                time.sleep(0.5)
                os.kill(process_pid, signal.SIGUSR1)
                # Let the main thread take the interrupt, handle it, and retry
                # the lock acquisition.  Then we'll let it run.
                time.sleep(0.5)
                lock.release()
            thread.start_new_thread(other_thread, ())
            # Wait until we can't acquire it without blocking...
            while lock.acquire(blocking=False):
                lock.release()
                time.sleep(0.01)
            result = lock.acquire()  # Block while we receive a signal.
            self.assertTrue(self.sig_recvd)
            self.assertTrue(result)
        finally:
            signal.signal(signal.SIGUSR1, old_handler)

    def test_lock_acquire_retries_on_intr(self):
        self.acquire_retries_on_intr(thread.allocate_lock())

    def test_rlock_acquire_retries_on_intr(self):
        self.acquire_retries_on_intr(thread.RLock())

    def test_interrupted_timed_acquire(self):
        # Test to make sure we recompute lock acquisition timeouts when we
        # receive a signal.  Check this by repeatedly interrupting a lock
        # acquire in the main thread, and make sure that the lock acquire times
        # out after the right amount of time.
        # NOTE: this test only behaves as expected if C signals get delivered
        # to the main thread.  Otherwise lock.acquire() itself doesn't get
        # interrupted and the test trivially succeeds.
        self.start = None
        self.end = None
        self.sigs_recvd = 0
        done = thread.allocate_lock()
        done.acquire()
        lock = thread.allocate_lock()
        lock.acquire()
        def my_handler(signum, frame):
            self.sigs_recvd += 1
        old_handler = signal.signal(signal.SIGUSR1, my_handler)
        try:
            def timed_acquire():
                self.start = time.time()
                lock.acquire(timeout=0.5)
                self.end = time.time()
            def send_signals():
                for _ in range(40):
                    time.sleep(0.02)
                    os.kill(process_pid, signal.SIGUSR1)
                done.release()

            # Send the signals from the non-main thread, since the main thread
            # is the only one that can process signals.
            thread.start_new_thread(send_signals, ())
            timed_acquire()
            # Wait for thread to finish
            done.acquire()
            # This allows for some timing and scheduling imprecision
            self.assertLess(self.end - self.start, 2.0)
            self.assertGreater(self.end - self.start, 0.3)
            # If the signal is received several times before PyErr_CheckSignals()
            # is called, the handler will get called less than 40 times. Just
            # check it's been called at least once.
            self.assertGreater(self.sigs_recvd, 0)
        finally:
            signal.signal(signal.SIGUSR1, old_handler)


def test_main():
    global signal_blackboard

    signal_blackboard = { signal.SIGUSR1 : {'tripped': 0, 'tripped_by': 0 },
                          signal.SIGUSR2 : {'tripped': 0, 'tripped_by': 0 },
                          signal.SIGALRM : {'tripped': 0, 'tripped_by': 0 } }

    oldsigs = registerSignals(handle_signals, handle_signals, handle_signals)
    try:
        run_unittest(ThreadSignals)
    finally:
        registerSignals(*oldsigs)

if __name__ == '__main__':
    test_main()

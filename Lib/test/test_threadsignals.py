"""PyUnit testing that threads honor our signal semantics"""

import unittest
import thread
import signal
import os
import sys
from test.test_support import run_unittest, TestSkipped

if sys.platform[:3] in ('win', 'os2') or sys.platform=='riscos':
    raise TestSkipped, "Can't test signal on %s" % sys.platform

process_pid = os.getpid()
signalled_all=thread.allocate_lock()


def registerSignals((for_usr1, for_usr2, for_alrm)):
    usr1 = signal.signal(signal.SIGUSR1, for_usr1)
    usr2 = signal.signal(signal.SIGUSR2, for_usr2)
    alrm = signal.signal(signal.SIGALRM, for_alrm)
    return usr1, usr2, alrm


# The signal handler. Just note that the signal occured and
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
    """Test signal handling semantics of threads.
       We spawn a thread, have the thread send two signals, and
       wait for it to finish. Check that we got both signals
       and that they were run by the main thread.
    """
    def test_signals(self):
        signalled_all.acquire()
        self.spawnSignallingThread()
        signalled_all.acquire()
        # the signals that we asked the kernel to send
        # will come back, but we don't know when.
        # (it might even be after the thread exits
        # and might be out of order.)  If we haven't seen
        # the signals yet, send yet another signal and
        # wait for it return.
        if signal_blackboard[signal.SIGUSR2]['tripped'] == 0 \
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


def test_main():
    global signal_blackboard

    signal_blackboard = { signal.SIGUSR1 : {'tripped': 0, 'tripped_by': 0 },
                          signal.SIGUSR2 : {'tripped': 0, 'tripped_by': 0 },
                          signal.SIGALRM : {'tripped': 0, 'tripped_by': 0 } }

    oldsigs = registerSignals((handle_signals, handle_signals, handle_signals))
    try:
        run_unittest(ThreadSignals)
    finally:
        registerSignals(oldsigs)

if __name__ == '__main__':
    test_main()

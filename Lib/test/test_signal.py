import unittest
from test import test_support
from contextlib import closing, nested
import pickle
import select
import signal
import traceback
import sys, os, time, errno

if sys.platform[:3] in ('win', 'os2') or sys.platform == 'riscos':
    raise test_support.TestSkipped("Can't test signal on %s" % \
                                   sys.platform)


class HandlerBCalled(Exception):
    pass


def exit_subprocess():
    """Use os._exit(0) to exit the current subprocess.

    Otherwise, the test catches the SystemExit and continues executing
    in parallel with the original test, so you wind up with an
    exponential number of tests running concurrently.
    """
    os._exit(0)


class InterProcessSignalTests(unittest.TestCase):
    MAX_DURATION = 20   # Entire test should last at most 20 sec.

    def handlerA(self, *args):
        self.a_called = True
        if test_support.verbose:
            print "handlerA invoked", args

    def handlerB(self, *args):
        self.b_called = True
        if test_support.verbose:
            print "handlerB invoked", args
        raise HandlerBCalled(*args)

    def wait(self, child_pid):
        """Wait for child_pid to finish, ignoring EINTR."""
        while True:
            try:
                pid, status = os.waitpid(child_pid, 0)
                return status
            except OSError as e:
                if e.errno != errno.EINTR:
                    raise

    def run_test(self):
        # Install handlers. This function runs in a sub-process, so we
        # don't worry about re-setting the default handlers.
        signal.signal(signal.SIGHUP, self.handlerA)
        signal.signal(signal.SIGUSR1, self.handlerB)
        signal.signal(signal.SIGUSR2, signal.SIG_IGN)
        signal.signal(signal.SIGALRM, signal.default_int_handler)

        # Variables the signals will modify:
        self.a_called = False
        self.b_called = False

        # Let the sub-processes know who to send signals to.
        pid = os.getpid()
        if test_support.verbose:
            print "test runner's pid is", pid

        child = os.fork()
        if child == 0:
            os.kill(pid, signal.SIGHUP)
            exit_subprocess()
        self.wait(child)
        self.assertTrue(self.a_called)
        self.assertFalse(self.b_called)
        self.a_called = False

        try:
            child = os.fork()
            if child == 0:
                os.kill(pid, signal.SIGUSR1)
                exit_subprocess()
            # This wait should be interrupted by the signal's exception.
            self.wait(child)
            self.fail('HandlerBCalled exception not thrown')
        except HandlerBCalled:
            # So we call it again to reap the child's zombie.
            self.wait(child)
            self.assertTrue(self.b_called)
            self.assertFalse(self.a_called)
            if test_support.verbose:
                print "HandlerBCalled exception caught"

        child = os.fork()
        if child == 0:
            os.kill(pid, signal.SIGUSR2)
            exit_subprocess()
        self.wait(child)  # Nothing should happen.

        try:
            signal.alarm(1)
            # The race condition in pause doesn't matter in this case,
            # since alarm is going to raise a KeyboardException, which
            # will skip the call.
            signal.pause()
        except KeyboardInterrupt:
            if test_support.verbose:
                print "KeyboardInterrupt (the alarm() went off)"
        except:
            self.fail('Some other exception woke us from pause: %s' %
                      traceback.format_exc())
        else:
            self.fail('pause returned of its own accord')

    def test_main(self):
        # This function spawns a child process to insulate the main
        # test-running process from all the signals. It then
        # communicates with that child process over a pipe and
        # re-raises information about any exceptions the child
        # throws. The real work happens in self.run_test().
        os_done_r, os_done_w = os.pipe()
        with nested(closing(os.fdopen(os_done_r)),
                    closing(os.fdopen(os_done_w, 'w'))) as (done_r, done_w):
            child = os.fork()
            if child == 0:
                # In the child process; run the test and report results
                # through the pipe.
                try:
                    done_r.close()
                    # Have to close done_w again here because
                    # exit_subprocess() will skip the enclosing with block.
                    with closing(done_w):
                        try:
                            self.run_test()
                        except:
                            pickle.dump(traceback.format_exc(), done_w)
                        else:
                            pickle.dump(None, done_w)
                except:
                    print 'Uh oh, raised from pickle.'
                    traceback.print_exc()
                finally:
                    exit_subprocess()

            done_w.close()
            # Block for up to MAX_DURATION seconds for the test to finish.
            r, w, x = select.select([done_r], [], [], self.MAX_DURATION)
            if done_r in r:
                tb = pickle.load(done_r)
                if tb:
                    self.fail(tb)
            else:
                os.kill(child, signal.SIGKILL)
                self.fail('Test deadlocked after %d seconds.' %
                          self.MAX_DURATION)


class BasicSignalTests(unittest.TestCase):
    def trivial_signal_handler(self, *args):
        pass

    def test_out_of_range_signal_number_raises_error(self):
        self.assertRaises(ValueError, signal.getsignal, 4242)

        self.assertRaises(ValueError, signal.signal, 4242,
                          self.trivial_signal_handler)

    def test_setting_signal_handler_to_none_raises_error(self):
        self.assertRaises(TypeError, signal.signal,
                          signal.SIGUSR1, None)

    def test_getsignal(self):
        hup = signal.signal(signal.SIGHUP, self.trivial_signal_handler)
        self.assertEquals(signal.getsignal(signal.SIGHUP),
                          self.trivial_signal_handler)
        signal.signal(signal.SIGHUP, hup)
        self.assertEquals(signal.getsignal(signal.SIGHUP), hup)


class WakeupSignalTests(unittest.TestCase):
    TIMEOUT_FULL = 10
    TIMEOUT_HALF = 5

    def test_wakeup_fd_early(self):
        import select

        signal.alarm(1)
        before_time = time.time()
        # We attempt to get a signal during the sleep,
        # before select is called
        time.sleep(self.TIMEOUT_FULL)
        mid_time = time.time()
        self.assert_(mid_time - before_time < self.TIMEOUT_HALF)
        select.select([self.read], [], [], self.TIMEOUT_FULL)
        after_time = time.time()
        self.assert_(after_time - mid_time < self.TIMEOUT_HALF)

    def test_wakeup_fd_during(self):
        import select

        signal.alarm(1)
        before_time = time.time()
        # We attempt to get a signal during the select call
        self.assertRaises(select.error, select.select,
            [self.read], [], [], self.TIMEOUT_FULL)
        after_time = time.time()
        self.assert_(after_time - before_time < self.TIMEOUT_HALF)

    def setUp(self):
        import fcntl

        self.alrm = signal.signal(signal.SIGALRM, lambda x,y:None)
        self.read, self.write = os.pipe()
        flags = fcntl.fcntl(self.write, fcntl.F_GETFL, 0)
        flags = flags | os.O_NONBLOCK
        fcntl.fcntl(self.write, fcntl.F_SETFL, flags)
        self.old_wakeup = signal.set_wakeup_fd(self.write)

    def tearDown(self):
        signal.set_wakeup_fd(self.old_wakeup)
        os.close(self.read)
        os.close(self.write)
        signal.signal(signal.SIGALRM, self.alrm)

class SiginterruptTest(unittest.TestCase):
    signum = signal.SIGUSR1
    def readpipe_interrupted(self, cb):
        r, w = os.pipe()
        ppid = os.getpid()
        pid = os.fork()

        oldhandler = signal.signal(self.signum, lambda x,y: None)
        cb()
        if pid==0:
            # child code: sleep, kill, sleep. and then exit,
            # which closes the pipe from which the parent process reads
            try:
                time.sleep(0.2)
                os.kill(ppid, self.signum)
                time.sleep(0.2)
            finally:
                exit_subprocess()

        try:
            os.close(w)

            try:
                d=os.read(r, 1)
                return False
            except OSError, err:
                if err.errno != errno.EINTR:
                    raise
                return True
        finally:
            signal.signal(self.signum, oldhandler)
            os.waitpid(pid, 0)

    def test_without_siginterrupt(self):
        i=self.readpipe_interrupted(lambda: None)
        self.assertEquals(i, True)

    def test_siginterrupt_on(self):
        i=self.readpipe_interrupted(lambda: signal.siginterrupt(self.signum, 1))
        self.assertEquals(i, True)

    def test_siginterrupt_off(self):
        i=self.readpipe_interrupted(lambda: signal.siginterrupt(self.signum, 0))
        self.assertEquals(i, False)

def test_main():
    test_support.run_unittest(BasicSignalTests, InterProcessSignalTests,
        WakeupSignalTests, SiginterruptTest)


if __name__ == "__main__":
    test_main()

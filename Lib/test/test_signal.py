import errno
import gc
import os
import pickle
import select
import signal
import subprocess
import sys
import time
import traceback
import unittest
from test import support
from contextlib import closing
from test.script_helper import assert_python_ok, spawn_python

if sys.platform in ('os2', 'riscos'):
    raise unittest.SkipTest("Can't test signal on %s" % sys.platform)


class HandlerBCalled(Exception):
    pass


def exit_subprocess():
    """Use os._exit(0) to exit the current subprocess.

    Otherwise, the test catches the SystemExit and continues executing
    in parallel with the original test, so you wind up with an
    exponential number of tests running concurrently.
    """
    os._exit(0)


def ignoring_eintr(__func, *args, **kwargs):
    try:
        return __func(*args, **kwargs)
    except EnvironmentError as e:
        if e.errno != errno.EINTR:
            raise
        return None


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class InterProcessSignalTests(unittest.TestCase):
    MAX_DURATION = 20   # Entire test should last at most 20 sec.

    def setUp(self):
        self.using_gc = gc.isenabled()
        gc.disable()

    def tearDown(self):
        if self.using_gc:
            gc.enable()

    def format_frame(self, frame, limit=None):
        return ''.join(traceback.format_stack(frame, limit=limit))

    def handlerA(self, signum, frame):
        self.a_called = True
        if support.verbose:
            print("handlerA invoked from signal %s at:\n%s" % (
                signum, self.format_frame(frame, limit=1)))

    def handlerB(self, signum, frame):
        self.b_called = True
        if support.verbose:
            print ("handlerB invoked from signal %s at:\n%s" % (
                signum, self.format_frame(frame, limit=1)))
        raise HandlerBCalled(signum, self.format_frame(frame))

    def wait(self, child):
        """Wait for child to finish, ignoring EINTR."""
        while True:
            try:
                child.wait()
                return
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
        if support.verbose:
            print("test runner's pid is", pid)

        child = ignoring_eintr(subprocess.Popen, ['kill', '-HUP', str(pid)])
        if child:
            self.wait(child)
            if not self.a_called:
                time.sleep(1)  # Give the signal time to be delivered.
        self.assertTrue(self.a_called)
        self.assertFalse(self.b_called)
        self.a_called = False

        # Make sure the signal isn't delivered while the previous
        # Popen object is being destroyed, because __del__ swallows
        # exceptions.
        del child
        try:
            child = subprocess.Popen(['kill', '-USR1', str(pid)])
            # This wait should be interrupted by the signal's exception.
            self.wait(child)
            time.sleep(1)  # Give the signal time to be delivered.
            self.fail('HandlerBCalled exception not thrown')
        except HandlerBCalled:
            self.assertTrue(self.b_called)
            self.assertFalse(self.a_called)
            if support.verbose:
                print("HandlerBCalled exception caught")

        child = ignoring_eintr(subprocess.Popen, ['kill', '-USR2', str(pid)])
        if child:
            self.wait(child)  # Nothing should happen.

        try:
            signal.alarm(1)
            # The race condition in pause doesn't matter in this case,
            # since alarm is going to raise a KeyboardException, which
            # will skip the call.
            signal.pause()
            # But if another signal arrives before the alarm, pause
            # may return early.
            time.sleep(1)
        except KeyboardInterrupt:
            if support.verbose:
                print("KeyboardInterrupt (the alarm() went off)")
        except:
            self.fail("Some other exception woke us from pause: %s" %
                      traceback.format_exc())
        else:
            self.fail("pause returned of its own accord, and the signal"
                      " didn't arrive after another second.")

    # Issue 3864, unknown if this affects earlier versions of freebsd also
    @unittest.skipIf(sys.platform=='freebsd6',
        'inter process signals not reliable (do not mix well with threading) '
        'on freebsd6')
    def test_main(self):
        # This function spawns a child process to insulate the main
        # test-running process from all the signals. It then
        # communicates with that child process over a pipe and
        # re-raises information about any exceptions the child
        # throws. The real work happens in self.run_test().
        os_done_r, os_done_w = os.pipe()
        with closing(os.fdopen(os_done_r, 'rb')) as done_r, \
             closing(os.fdopen(os_done_w, 'wb')) as done_w:
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
                    print('Uh oh, raised from pickle.')
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


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
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
        self.assertEqual(signal.getsignal(signal.SIGHUP),
                         self.trivial_signal_handler)
        signal.signal(signal.SIGHUP, hup)
        self.assertEqual(signal.getsignal(signal.SIGHUP), hup)


@unittest.skipUnless(sys.platform == "win32", "Windows specific")
class WindowsSignalTests(unittest.TestCase):
    def test_issue9324(self):
        # Updated for issue #10003, adding SIGBREAK
        handler = lambda x, y: None
        for sig in (signal.SIGABRT, signal.SIGBREAK, signal.SIGFPE,
                    signal.SIGILL, signal.SIGINT, signal.SIGSEGV,
                    signal.SIGTERM):
            # Set and then reset a handler for signals that work on windows
            signal.signal(sig, signal.signal(sig, handler))

        with self.assertRaises(ValueError):
            signal.signal(-1, handler)

        with self.assertRaises(ValueError):
            signal.signal(7, handler)


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class WakeupSignalTests(unittest.TestCase):
    def check_wakeup(self, test_body):
        # use a subprocess to have only one thread and to not change signal
        # handling of the parent process
        code = """if 1:
        import fcntl
        import os
        import signal

        def handler(signum, frame):
            pass

        {}

        signal.signal(signal.SIGALRM, handler)
        read, write = os.pipe()
        flags = fcntl.fcntl(write, fcntl.F_GETFL, 0)
        flags = flags | os.O_NONBLOCK
        fcntl.fcntl(write, fcntl.F_SETFL, flags)
        signal.set_wakeup_fd(write)

        test()

        os.close(read)
        os.close(write)
        """.format(test_body)

        assert_python_ok('-c', code)

    def test_wakeup_fd_early(self):
        self.check_wakeup("""def test():
            import select
            import time

            TIMEOUT_FULL = 10
            TIMEOUT_HALF = 5

            signal.alarm(1)
            before_time = time.time()
            # We attempt to get a signal during the sleep,
            # before select is called
            time.sleep(TIMEOUT_FULL)
            mid_time = time.time()
            dt = mid_time - before_time
            if dt >= TIMEOUT_HALF:
                raise Exception("%s >= %s" % (dt, TIMEOUT_HALF))
            select.select([read], [], [], TIMEOUT_FULL)
            after_time = time.time()
            dt = after_time - mid_time
            if dt >= TIMEOUT_HALF:
                raise Exception("%s >= %s" % (dt, TIMEOUT_HALF))
        """)

    def test_wakeup_fd_during(self):
        self.check_wakeup("""def test():
            import select
            import time

            TIMEOUT_FULL = 10
            TIMEOUT_HALF = 5

            signal.alarm(1)
            before_time = time.time()
            # We attempt to get a signal during the select call
            try:
                select.select([read], [], [], TIMEOUT_FULL)
            except select.error:
                pass
            else:
                raise Exception("select.error not raised")
            after_time = time.time()
            dt = after_time - before_time
            if dt >= TIMEOUT_HALF:
                raise Exception("%s >= %s" % (dt, TIMEOUT_HALF))
        """)

@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class SiginterruptTest(unittest.TestCase):

    def readpipe_interrupted(self, interrupt):
        """Perform a read during which a signal will arrive.  Return True if the
        read is interrupted by the signal and raises an exception.  Return False
        if it returns normally.
        """
        class Timeout(Exception):
            pass

        # use a subprocess to have only one thread, to have a timeout on the
        # blocking read and to not touch signal handling in this process
        code = """if 1:
            import errno
            import os
            import signal
            import sys

            interrupt = %r
            r, w = os.pipe()

            def handler(signum, frame):
                pass

            signal.signal(signal.SIGALRM, handler)
            if interrupt is not None:
                signal.siginterrupt(signal.SIGALRM, interrupt)

            print("ready")
            sys.stdout.flush()

            # run the test twice
            for loop in range(2):
                # send a SIGALRM in a second (during the read)
                signal.alarm(1)
                try:
                    # blocking call: read from a pipe without data
                    os.read(r, 1)
                except OSError as err:
                    if err.errno != errno.EINTR:
                        raise
                else:
                    sys.exit(2)
            sys.exit(3)
        """ % (interrupt,)
        with spawn_python('-c', code) as process:
            try:
                # wait until the child process is loaded and has started
                first_line = process.stdout.readline()

                # Wait the process with a timeout of 5 seconds
                timeout = time.time() + 5.0
                while True:
                    if timeout < time.time():
                        raise Timeout()
                    status = process.poll()
                    if status is not None:
                        break
                    time.sleep(0.1)

                stdout, stderr = process.communicate()
            except Timeout:
                process.kill()
                return False
            else:
                stdout = first_line + stdout
                exitcode = process.wait()
                if exitcode not in (2, 3):
                    raise Exception("Child error (exit code %s): %s"
                                    % (exitcode, stdout))
                return (exitcode == 3)

    def test_without_siginterrupt(self):
        # If a signal handler is installed and siginterrupt is not called
        # at all, when that signal arrives, it interrupts a syscall that's in
        # progress.
        interrupted = self.readpipe_interrupted(None)
        self.assertTrue(interrupted)

    def test_siginterrupt_on(self):
        # If a signal handler is installed and siginterrupt is called with
        # a true value for the second argument, when that signal arrives, it
        # interrupts a syscall that's in progress.
        interrupted = self.readpipe_interrupted(True)
        self.assertTrue(interrupted)

    def test_siginterrupt_off(self):
        # If a signal handler is installed and siginterrupt is called with
        # a false value for the second argument, when that signal arrives, it
        # does not interrupt a syscall that's in progress.
        interrupted = self.readpipe_interrupted(False)
        self.assertFalse(interrupted)


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class ItimerTest(unittest.TestCase):
    def setUp(self):
        self.hndl_called = False
        self.hndl_count = 0
        self.itimer = None
        self.old_alarm = signal.signal(signal.SIGALRM, self.sig_alrm)

    def tearDown(self):
        signal.signal(signal.SIGALRM, self.old_alarm)
        if self.itimer is not None: # test_itimer_exc doesn't change this attr
            # just ensure that itimer is stopped
            signal.setitimer(self.itimer, 0)

    def sig_alrm(self, *args):
        self.hndl_called = True
        if support.verbose:
            print("SIGALRM handler invoked", args)

    def sig_vtalrm(self, *args):
        self.hndl_called = True

        if self.hndl_count > 3:
            # it shouldn't be here, because it should have been disabled.
            raise signal.ItimerError("setitimer didn't disable ITIMER_VIRTUAL "
                "timer.")
        elif self.hndl_count == 3:
            # disable ITIMER_VIRTUAL, this function shouldn't be called anymore
            signal.setitimer(signal.ITIMER_VIRTUAL, 0)
            if support.verbose:
                print("last SIGVTALRM handler call")

        self.hndl_count += 1

        if support.verbose:
            print("SIGVTALRM handler invoked", args)

    def sig_prof(self, *args):
        self.hndl_called = True
        signal.setitimer(signal.ITIMER_PROF, 0)

        if support.verbose:
            print("SIGPROF handler invoked", args)

    def test_itimer_exc(self):
        # XXX I'm assuming -1 is an invalid itimer, but maybe some platform
        # defines it ?
        self.assertRaises(signal.ItimerError, signal.setitimer, -1, 0)
        # Negative times are treated as zero on some platforms.
        if 0:
            self.assertRaises(signal.ItimerError,
                              signal.setitimer, signal.ITIMER_REAL, -1)

    def test_itimer_real(self):
        self.itimer = signal.ITIMER_REAL
        signal.setitimer(self.itimer, 1.0)
        if support.verbose:
            print("\ncall pause()...")
        signal.pause()

        self.assertEqual(self.hndl_called, True)

    # Issue 3864, unknown if this affects earlier versions of freebsd also
    @unittest.skipIf(sys.platform in ('freebsd6', 'netbsd5'),
        'itimer not reliable (does not mix well with threading) on some BSDs.')
    def test_itimer_virtual(self):
        self.itimer = signal.ITIMER_VIRTUAL
        signal.signal(signal.SIGVTALRM, self.sig_vtalrm)
        signal.setitimer(self.itimer, 0.3, 0.2)

        start_time = time.time()
        while time.time() - start_time < 60.0:
            # use up some virtual time by doing real work
            _ = pow(12345, 67890, 10000019)
            if signal.getitimer(self.itimer) == (0.0, 0.0):
                break # sig_vtalrm handler stopped this itimer
        else: # Issue 8424
            self.skipTest("timeout: likely cause: machine too slow or load too "
                          "high")

        # virtual itimer should be (0.0, 0.0) now
        self.assertEqual(signal.getitimer(self.itimer), (0.0, 0.0))
        # and the handler should have been called
        self.assertEqual(self.hndl_called, True)

    # Issue 3864, unknown if this affects earlier versions of freebsd also
    @unittest.skipIf(sys.platform=='freebsd6',
        'itimer not reliable (does not mix well with threading) on freebsd6')
    def test_itimer_prof(self):
        self.itimer = signal.ITIMER_PROF
        signal.signal(signal.SIGPROF, self.sig_prof)
        signal.setitimer(self.itimer, 0.2, 0.2)

        start_time = time.time()
        while time.time() - start_time < 60.0:
            # do some work
            _ = pow(12345, 67890, 10000019)
            if signal.getitimer(self.itimer) == (0.0, 0.0):
                break # sig_prof handler stopped this itimer
        else: # Issue 8424
            self.skipTest("timeout: likely cause: machine too slow or load too "
                          "high")

        # profiling itimer should be (0.0, 0.0) now
        self.assertEqual(signal.getitimer(self.itimer), (0.0, 0.0))
        # and the handler should have been called
        self.assertEqual(self.hndl_called, True)

def test_main():
    try:
        support.run_unittest(BasicSignalTests, InterProcessSignalTests,
                             WakeupSignalTests, SiginterruptTest,
                             ItimerTest, WindowsSignalTests)
    finally:
        support.reap_children()


if __name__ == "__main__":
    test_main()

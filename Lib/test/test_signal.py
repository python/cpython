import unittest
from test import test_support
import signal
import os, sys, time

class HandlerBCalled(Exception):
    pass

class InterProcessSignalTests(unittest.TestCase):
    MAX_DURATION = 20   # Entire test should last at most 20 sec.

    # Set up a child to send signals to us (the parent) after waiting
    # long enough to receive the alarm.  It seems we miss the alarm
    # for some reason.  This will hopefully stop the hangs on
    # Tru64/Alpha.  Alas, it doesn't.  Tru64 appears to miss all the
    # signals at times, or seemingly random subsets of them, and
    # nothing done in force_test_exit so far has actually helped.
    def spawn_force_test_exit_process(self, parent_pid):
        # Sigh, both imports seem necessary to avoid errors.
        import os
        fork_pid = os.fork()
        if fork_pid:
            # In parent.
            return fork_pid

        # In child.
        import os, time
        try:
            # Wait 5 seconds longer than the expected alarm to give enough
            # time for the normal sequence of events to occur.  This is
            # just a stop-gap to try to prevent the test from hanging.
            time.sleep(self.MAX_DURATION + 5)
            print("  child should not have to kill parent",
                  file=sys.__stdout__)
            for signame in "SIGHUP", "SIGUSR1", "SIGUSR2", "SIGALRM":
                os.kill(parent_pid, getattr(signal, signame))
                print("    child sent", signame, "to",
                      parent_pid, file=sys.__stdout__)
                time.sleep(1)
        finally:
            os._exit(0)

    def handlerA(self, *args):
        self.a_called = True
        if test_support.verbose:
            print("handlerA invoked", args)

    def handlerB(self, *args):
        self.b_called = True
        if test_support.verbose:
            print("handlerB invoked", args)
        raise HandlerBCalled(*args)

    def test_main(self):
        self.assertEquals(signal.getsignal(signal.SIGHUP), self.handlerA)
        self.assertEquals(signal.getsignal(signal.SIGUSR1), self.handlerB)
        self.assertEquals(signal.getsignal(signal.SIGUSR2), signal.SIG_IGN)
        self.assertEquals(signal.getsignal(signal.SIGALRM),
                          signal.default_int_handler)

        # Launch an external script to send us signals.
        # We expect the external script to:
        #    send HUP, which invokes handlerA to set a_called
        #    send USR1, which invokes handlerB to set b_called and raise
        #               HandlerBCalled
        #    send USR2, which is ignored
        #
        # Then we expect the alarm to go off, and its handler raises
        # KeyboardInterrupt, finally getting us out of the loop.

        if test_support.verbose:
            verboseflag = '-x'
        else:
            verboseflag = '+x'

        pid = self.pid
        if test_support.verbose:
            print("test runner's pid is", pid)

        # Shell script that will send us asynchronous signals
        script = """
         (
                set %(verboseflag)s
                sleep 2
                kill -HUP %(pid)d
                sleep 2
                kill -USR1 %(pid)d
                sleep 2
                kill -USR2 %(pid)d
         ) &
        """ % vars()

        signal.alarm(self.MAX_DURATION)

        handler_b_exception_raised = False

        os.system(script)
        try:
            if test_support.verbose:
                print("starting pause() loop...")
            while 1:
                try:
                    if test_support.verbose:
                        print("call pause()...")
                    signal.pause()
                    if test_support.verbose:
                        print("pause() returned")
                except HandlerBCalled:
                    handler_b_exception_raised = True
                    if test_support.verbose:
                        print("HandlerBCalled exception caught")

        except KeyboardInterrupt:
            if test_support.verbose:
                print("KeyboardInterrupt (the alarm() went off)")

        self.assert_(self.a_called)
        self.assert_(self.b_called)
        self.assert_(handler_b_exception_raised)

    def setUp(self):
        # Install handlers.
        self.hup = signal.signal(signal.SIGHUP, self.handlerA)
        self.usr1 = signal.signal(signal.SIGUSR1, self.handlerB)
        self.usr2 = signal.signal(signal.SIGUSR2, signal.SIG_IGN)
        self.alrm = signal.signal(signal.SIGALRM,
                                  signal.default_int_handler)
        self.a_called = False
        self.b_called = False
        self.pid = os.getpid()
        self.fork_pid = self.spawn_force_test_exit_process(self.pid)

    def tearDown(self):
        # Forcibly kill the child we created to ping us if there was a
        # test error.
        try:
            # Make sure we don't kill ourself if there was a fork
            # error.
            if self.fork_pid > 0:
                os.kill(self.fork_pid, signal.SIGKILL)
        except:
            # If the child killed us, it has probably exited.  Killing
            # a non-existent process will raise an error which we
            # don't care about.
            pass

        # Restore handlers.
        signal.alarm(0) # cancel alarm in case we died early
        signal.signal(signal.SIGHUP, self.hup)
        signal.signal(signal.SIGUSR1, self.usr1)
        signal.signal(signal.SIGUSR2, self.usr2)
        signal.signal(signal.SIGALRM, self.alrm)


class BasicSignalTests(unittest.TestCase):
    def test_out_of_range_signal_number_raises_error(self):
        self.assertRaises(ValueError, signal.getsignal, 4242)

        def trivial_signal_handler(*args):
            pass

        self.assertRaises(ValueError, signal.signal, 4242,
                          trivial_signal_handler)

    def test_setting_signal_handler_to_none_raises_error(self):
        self.assertRaises(TypeError, signal.signal,
                          signal.SIGUSR1, None)

def test_main():
    if sys.platform[:3] in ('win', 'os2'):
        raise test_support.TestSkipped("Can't test signal on %s" % \
                                       sys.platform)

    test_support.run_unittest(BasicSignalTests, InterProcessSignalTests)


if __name__ == "__main__":
    test_main()

import enum
import errno
import inspect
import os
import random
import signal
import socket
import statistics
import subprocess
import sys
import threading
import time
import unittest
from test import support
from test.support import os_helper
from test.support.script_helper import assert_python_ok, spawn_python
try:
    import _testcapi
except ImportError:
    _testcapi = None


class GenericTests(unittest.TestCase):

    def test_enums(self):
        for name in dir(signal):
            sig = getattr(signal, name)
            if name in {'SIG_DFL', 'SIG_IGN'}:
                self.assertIsInstance(sig, signal.Handlers)
            elif name in {'SIG_BLOCK', 'SIG_UNBLOCK', 'SIG_SETMASK'}:
                self.assertIsInstance(sig, signal.Sigmasks)
            elif name.startswith('SIG') and not name.startswith('SIG_'):
                self.assertIsInstance(sig, signal.Signals)
            elif name.startswith('CTRL_'):
                self.assertIsInstance(sig, signal.Signals)
                self.assertEqual(sys.platform, "win32")

        CheckedSignals = enum._old_convert_(
                enum.IntEnum, 'Signals', 'signal',
                lambda name:
                    name.isupper()
                    and (name.startswith('SIG') and not name.startswith('SIG_'))
                    or name.startswith('CTRL_'),
                source=signal,
                )
        enum._test_simple_enum(CheckedSignals, signal.Signals)

        CheckedHandlers = enum._old_convert_(
                enum.IntEnum, 'Handlers', 'signal',
                lambda name: name in ('SIG_DFL', 'SIG_IGN'),
                source=signal,
                )
        enum._test_simple_enum(CheckedHandlers, signal.Handlers)

        Sigmasks = getattr(signal, 'Sigmasks', None)
        if Sigmasks is not None:
            CheckedSigmasks = enum._old_convert_(
                    enum.IntEnum, 'Sigmasks', 'signal',
                    lambda name: name in ('SIG_BLOCK', 'SIG_UNBLOCK', 'SIG_SETMASK'),
                    source=signal,
                    )
            enum._test_simple_enum(CheckedSigmasks, Sigmasks)

    def test_functions_module_attr(self):
        # Issue #27718: If __all__ is not defined all non-builtin functions
        # should have correct __module__ to be displayed by pydoc.
        for name in dir(signal):
            value = getattr(signal, name)
            if inspect.isroutine(value) and not inspect.isbuiltin(value):
                self.assertEqual(value.__module__, 'signal')


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class PosixTests(unittest.TestCase):
    def trivial_signal_handler(self, *args):
        pass

    def test_out_of_range_signal_number_raises_error(self):
        self.assertRaises(ValueError, signal.getsignal, 4242)

        self.assertRaises(ValueError, signal.signal, 4242,
                          self.trivial_signal_handler)

        self.assertRaises(ValueError, signal.strsignal, 4242)

    def test_setting_signal_handler_to_none_raises_error(self):
        self.assertRaises(TypeError, signal.signal,
                          signal.SIGUSR1, None)

    def test_getsignal(self):
        hup = signal.signal(signal.SIGHUP, self.trivial_signal_handler)
        self.assertIsInstance(hup, signal.Handlers)
        self.assertEqual(signal.getsignal(signal.SIGHUP),
                         self.trivial_signal_handler)
        signal.signal(signal.SIGHUP, hup)
        self.assertEqual(signal.getsignal(signal.SIGHUP), hup)

    def test_strsignal(self):
        self.assertIn("Interrupt", signal.strsignal(signal.SIGINT))
        self.assertIn("Terminated", signal.strsignal(signal.SIGTERM))
        self.assertIn("Hangup", signal.strsignal(signal.SIGHUP))

    # Issue 3864, unknown if this affects earlier versions of freebsd also
    def test_interprocess_signal(self):
        dirname = os.path.dirname(__file__)
        script = os.path.join(dirname, 'signalinterproctester.py')
        assert_python_ok(script)

    def test_valid_signals(self):
        s = signal.valid_signals()
        self.assertIsInstance(s, set)
        self.assertIn(signal.Signals.SIGINT, s)
        self.assertIn(signal.Signals.SIGALRM, s)
        self.assertNotIn(0, s)
        self.assertNotIn(signal.NSIG, s)
        self.assertLess(len(s), signal.NSIG)

    @unittest.skipUnless(sys.executable, "sys.executable required.")
    @support.requires_subprocess()
    def test_keyboard_interrupt_exit_code(self):
        """KeyboardInterrupt triggers exit via SIGINT."""
        process = subprocess.run(
                [sys.executable, "-c",
                 "import os, signal, time\n"
                 "os.kill(os.getpid(), signal.SIGINT)\n"
                 "for _ in range(999): time.sleep(0.01)"],
                stderr=subprocess.PIPE)
        self.assertIn(b"KeyboardInterrupt", process.stderr)
        self.assertEqual(process.returncode, -signal.SIGINT)
        # Caveat: The exit code is insufficient to guarantee we actually died
        # via a signal.  POSIX shells do more than look at the 8 bit value.
        # Writing an automation friendly test of an interactive shell
        # to confirm that our process died via a SIGINT proved too complex.


@unittest.skipUnless(sys.platform == "win32", "Windows specific")
class WindowsSignalTests(unittest.TestCase):

    def test_valid_signals(self):
        s = signal.valid_signals()
        self.assertIsInstance(s, set)
        self.assertGreaterEqual(len(s), 6)
        self.assertIn(signal.Signals.SIGINT, s)
        self.assertNotIn(0, s)
        self.assertNotIn(signal.NSIG, s)
        self.assertLess(len(s), signal.NSIG)

    def test_issue9324(self):
        # Updated for issue #10003, adding SIGBREAK
        handler = lambda x, y: None
        checked = set()
        for sig in (signal.SIGABRT, signal.SIGBREAK, signal.SIGFPE,
                    signal.SIGILL, signal.SIGINT, signal.SIGSEGV,
                    signal.SIGTERM):
            # Set and then reset a handler for signals that work on windows.
            # Issue #18396, only for signals without a C-level handler.
            if signal.getsignal(sig) is not None:
                signal.signal(sig, signal.signal(sig, handler))
                checked.add(sig)
        # Issue #18396: Ensure the above loop at least tested *something*
        self.assertTrue(checked)

        with self.assertRaises(ValueError):
            signal.signal(-1, handler)

        with self.assertRaises(ValueError):
            signal.signal(7, handler)

    @unittest.skipUnless(sys.executable, "sys.executable required.")
    @support.requires_subprocess()
    def test_keyboard_interrupt_exit_code(self):
        """KeyboardInterrupt triggers an exit using STATUS_CONTROL_C_EXIT."""
        # We don't test via os.kill(os.getpid(), signal.CTRL_C_EVENT) here
        # as that requires setting up a console control handler in a child
        # in its own process group.  Doable, but quite complicated.  (see
        # @eryksun on https://github.com/python/cpython/pull/11862)
        process = subprocess.run(
                [sys.executable, "-c", "raise KeyboardInterrupt"],
                stderr=subprocess.PIPE)
        self.assertIn(b"KeyboardInterrupt", process.stderr)
        STATUS_CONTROL_C_EXIT = 0xC000013A
        self.assertEqual(process.returncode, STATUS_CONTROL_C_EXIT)


class WakeupFDTests(unittest.TestCase):

    def test_invalid_call(self):
        # First parameter is positional-only
        with self.assertRaises(TypeError):
            signal.set_wakeup_fd(signum=signal.SIGINT)

        # warn_on_full_buffer is a keyword-only parameter
        with self.assertRaises(TypeError):
            signal.set_wakeup_fd(signal.SIGINT, False)

    def test_invalid_fd(self):
        fd = os_helper.make_bad_fd()
        self.assertRaises((ValueError, OSError),
                          signal.set_wakeup_fd, fd)

    def test_invalid_socket(self):
        sock = socket.socket()
        fd = sock.fileno()
        sock.close()
        self.assertRaises((ValueError, OSError),
                          signal.set_wakeup_fd, fd)

    # Emscripten does not support fstat on pipes yet.
    # https://github.com/emscripten-core/emscripten/issues/16414
    @unittest.skipIf(support.is_emscripten, "Emscripten cannot fstat pipes.")
    def test_set_wakeup_fd_result(self):
        r1, w1 = os.pipe()
        self.addCleanup(os.close, r1)
        self.addCleanup(os.close, w1)
        r2, w2 = os.pipe()
        self.addCleanup(os.close, r2)
        self.addCleanup(os.close, w2)

        if hasattr(os, 'set_blocking'):
            os.set_blocking(w1, False)
            os.set_blocking(w2, False)

        signal.set_wakeup_fd(w1)
        self.assertEqual(signal.set_wakeup_fd(w2), w1)
        self.assertEqual(signal.set_wakeup_fd(-1), w2)
        self.assertEqual(signal.set_wakeup_fd(-1), -1)

    @unittest.skipIf(support.is_emscripten, "Emscripten cannot fstat pipes.")
    def test_set_wakeup_fd_socket_result(self):
        sock1 = socket.socket()
        self.addCleanup(sock1.close)
        sock1.setblocking(False)
        fd1 = sock1.fileno()

        sock2 = socket.socket()
        self.addCleanup(sock2.close)
        sock2.setblocking(False)
        fd2 = sock2.fileno()

        signal.set_wakeup_fd(fd1)
        self.assertEqual(signal.set_wakeup_fd(fd2), fd1)
        self.assertEqual(signal.set_wakeup_fd(-1), fd2)
        self.assertEqual(signal.set_wakeup_fd(-1), -1)

    # On Windows, files are always blocking and Windows does not provide a
    # function to test if a socket is in non-blocking mode.
    @unittest.skipIf(sys.platform == "win32", "tests specific to POSIX")
    @unittest.skipIf(support.is_emscripten, "Emscripten cannot fstat pipes.")
    def test_set_wakeup_fd_blocking(self):
        rfd, wfd = os.pipe()
        self.addCleanup(os.close, rfd)
        self.addCleanup(os.close, wfd)

        # fd must be non-blocking
        os.set_blocking(wfd, True)
        with self.assertRaises(ValueError) as cm:
            signal.set_wakeup_fd(wfd)
        self.assertEqual(str(cm.exception),
                         "the fd %s must be in non-blocking mode" % wfd)

        # non-blocking is ok
        os.set_blocking(wfd, False)
        signal.set_wakeup_fd(wfd)
        signal.set_wakeup_fd(-1)


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
class WakeupSignalTests(unittest.TestCase):
    @unittest.skipIf(_testcapi is None, 'need _testcapi')
    def check_wakeup(self, test_body, *signals, ordered=True):
        # use a subprocess to have only one thread
        code = """if 1:
        import _testcapi
        import os
        import signal
        import struct

        signals = {!r}

        def handler(signum, frame):
            pass

        def check_signum(signals):
            data = os.read(read, len(signals)+1)
            raised = struct.unpack('%uB' % len(data), data)
            if not {!r}:
                raised = set(raised)
                signals = set(signals)
            if raised != signals:
                raise Exception("%r != %r" % (raised, signals))

        {}

        signal.signal(signal.SIGALRM, handler)
        read, write = os.pipe()
        os.set_blocking(write, False)
        signal.set_wakeup_fd(write)

        test()
        check_signum(signals)

        os.close(read)
        os.close(write)
        """.format(tuple(map(int, signals)), ordered, test_body)

        assert_python_ok('-c', code)

    @unittest.skipIf(_testcapi is None, 'need _testcapi')
    def test_wakeup_write_error(self):
        # Issue #16105: write() errors in the C signal handler should not
        # pass silently.
        # Use a subprocess to have only one thread.
        code = """if 1:
        import _testcapi
        import errno
        import os
        import signal
        import sys
        from test.support import captured_stderr

        def handler(signum, frame):
            1/0

        signal.signal(signal.SIGALRM, handler)
        r, w = os.pipe()
        os.set_blocking(r, False)

        # Set wakeup_fd a read-only file descriptor to trigger the error
        signal.set_wakeup_fd(r)
        try:
            with captured_stderr() as err:
                signal.raise_signal(signal.SIGALRM)
        except ZeroDivisionError:
            # An ignored exception should have been printed out on stderr
            err = err.getvalue()
            if ('Exception ignored when trying to write to the signal wakeup fd'
                not in err):
                raise AssertionError(err)
            if ('OSError: [Errno %d]' % errno.EBADF) not in err:
                raise AssertionError(err)
        else:
            raise AssertionError("ZeroDivisionError not raised")

        os.close(r)
        os.close(w)
        """
        r, w = os.pipe()
        try:
            os.write(r, b'x')
        except OSError:
            pass
        else:
            self.skipTest("OS doesn't report write() error on the read end of a pipe")
        finally:
            os.close(r)
            os.close(w)

        assert_python_ok('-c', code)

    def test_wakeup_fd_early(self):
        self.check_wakeup("""def test():
            import select
            import time

            TIMEOUT_FULL = 10
            TIMEOUT_HALF = 5

            class InterruptSelect(Exception):
                pass

            def handler(signum, frame):
                raise InterruptSelect
            signal.signal(signal.SIGALRM, handler)

            signal.alarm(1)

            # We attempt to get a signal during the sleep,
            # before select is called
            try:
                select.select([], [], [], TIMEOUT_FULL)
            except InterruptSelect:
                pass
            else:
                raise Exception("select() was not interrupted")

            before_time = time.monotonic()
            select.select([read], [], [], TIMEOUT_FULL)
            after_time = time.monotonic()
            dt = after_time - before_time
            if dt >= TIMEOUT_HALF:
                raise Exception("%s >= %s" % (dt, TIMEOUT_HALF))
        """, signal.SIGALRM)

    def test_wakeup_fd_during(self):
        self.check_wakeup("""def test():
            import select
            import time

            TIMEOUT_FULL = 10
            TIMEOUT_HALF = 5

            class InterruptSelect(Exception):
                pass

            def handler(signum, frame):
                raise InterruptSelect
            signal.signal(signal.SIGALRM, handler)

            signal.alarm(1)
            before_time = time.monotonic()
            # We attempt to get a signal during the select call
            try:
                select.select([read], [], [], TIMEOUT_FULL)
            except InterruptSelect:
                pass
            else:
                raise Exception("select() was not interrupted")
            after_time = time.monotonic()
            dt = after_time - before_time
            if dt >= TIMEOUT_HALF:
                raise Exception("%s >= %s" % (dt, TIMEOUT_HALF))
        """, signal.SIGALRM)

    def test_signum(self):
        self.check_wakeup("""def test():
            signal.signal(signal.SIGUSR1, handler)
            signal.raise_signal(signal.SIGUSR1)
            signal.raise_signal(signal.SIGALRM)
        """, signal.SIGUSR1, signal.SIGALRM)

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_pending(self):
        self.check_wakeup("""def test():
            signum1 = signal.SIGUSR1
            signum2 = signal.SIGUSR2

            signal.signal(signum1, handler)
            signal.signal(signum2, handler)

            signal.pthread_sigmask(signal.SIG_BLOCK, (signum1, signum2))
            signal.raise_signal(signum1)
            signal.raise_signal(signum2)
            # Unblocking the 2 signals calls the C signal handler twice
            signal.pthread_sigmask(signal.SIG_UNBLOCK, (signum1, signum2))
        """,  signal.SIGUSR1, signal.SIGUSR2, ordered=False)


@unittest.skipUnless(hasattr(socket, 'socketpair'), 'need socket.socketpair')
class WakeupSocketSignalTests(unittest.TestCase):

    @unittest.skipIf(_testcapi is None, 'need _testcapi')
    def test_socket(self):
        # use a subprocess to have only one thread
        code = """if 1:
        import signal
        import socket
        import struct
        import _testcapi

        signum = signal.SIGINT
        signals = (signum,)

        def handler(signum, frame):
            pass

        signal.signal(signum, handler)

        read, write = socket.socketpair()
        write.setblocking(False)
        signal.set_wakeup_fd(write.fileno())

        signal.raise_signal(signum)

        data = read.recv(1)
        if not data:
            raise Exception("no signum written")
        raised = struct.unpack('B', data)
        if raised != signals:
            raise Exception("%r != %r" % (raised, signals))

        read.close()
        write.close()
        """

        assert_python_ok('-c', code)

    @unittest.skipIf(_testcapi is None, 'need _testcapi')
    def test_send_error(self):
        # Use a subprocess to have only one thread.
        if os.name == 'nt':
            action = 'send'
        else:
            action = 'write'
        code = """if 1:
        import errno
        import signal
        import socket
        import sys
        import time
        import _testcapi
        from test.support import captured_stderr

        signum = signal.SIGINT

        def handler(signum, frame):
            pass

        signal.signal(signum, handler)

        read, write = socket.socketpair()
        read.setblocking(False)
        write.setblocking(False)

        signal.set_wakeup_fd(write.fileno())

        # Close sockets: send() will fail
        read.close()
        write.close()

        with captured_stderr() as err:
            signal.raise_signal(signum)

        err = err.getvalue()
        if ('Exception ignored when trying to {action} to the signal wakeup fd'
            not in err):
            raise AssertionError(err)
        """.format(action=action)
        assert_python_ok('-c', code)

    @unittest.skipIf(_testcapi is None, 'need _testcapi')
    def test_warn_on_full_buffer(self):
        # Use a subprocess to have only one thread.
        if os.name == 'nt':
            action = 'send'
        else:
            action = 'write'
        code = """if 1:
        import errno
        import signal
        import socket
        import sys
        import time
        import _testcapi
        from test.support import captured_stderr

        signum = signal.SIGINT

        # This handler will be called, but we intentionally won't read from
        # the wakeup fd.
        def handler(signum, frame):
            pass

        signal.signal(signum, handler)

        read, write = socket.socketpair()

        # Fill the socketpair buffer
        if sys.platform == 'win32':
            # bpo-34130: On Windows, sometimes non-blocking send fails to fill
            # the full socketpair buffer, so use a timeout of 50 ms instead.
            write.settimeout(0.050)
        else:
            write.setblocking(False)

        written = 0
        if sys.platform == "vxworks":
            CHUNK_SIZES = (1,)
        else:
            # Start with large chunk size to reduce the
            # number of send needed to fill the buffer.
            CHUNK_SIZES = (2 ** 16, 2 ** 8, 1)
        for chunk_size in CHUNK_SIZES:
            chunk = b"x" * chunk_size
            try:
                while True:
                    write.send(chunk)
                    written += chunk_size
            except (BlockingIOError, TimeoutError):
                pass

        print(f"%s bytes written into the socketpair" % written, flush=True)

        write.setblocking(False)
        try:
            write.send(b"x")
        except BlockingIOError:
            # The socketpair buffer seems full
            pass
        else:
            raise AssertionError("%s bytes failed to fill the socketpair "
                                 "buffer" % written)

        # By default, we get a warning when a signal arrives
        msg = ('Exception ignored when trying to {action} '
               'to the signal wakeup fd')
        signal.set_wakeup_fd(write.fileno())

        with captured_stderr() as err:
            signal.raise_signal(signum)

        err = err.getvalue()
        if msg not in err:
            raise AssertionError("first set_wakeup_fd() test failed, "
                                 "stderr: %r" % err)

        # And also if warn_on_full_buffer=True
        signal.set_wakeup_fd(write.fileno(), warn_on_full_buffer=True)

        with captured_stderr() as err:
            signal.raise_signal(signum)

        err = err.getvalue()
        if msg not in err:
            raise AssertionError("set_wakeup_fd(warn_on_full_buffer=True) "
                                 "test failed, stderr: %r" % err)

        # But not if warn_on_full_buffer=False
        signal.set_wakeup_fd(write.fileno(), warn_on_full_buffer=False)

        with captured_stderr() as err:
            signal.raise_signal(signum)

        err = err.getvalue()
        if err != "":
            raise AssertionError("set_wakeup_fd(warn_on_full_buffer=False) "
                                 "test failed, stderr: %r" % err)

        # And then check the default again, to make sure warn_on_full_buffer
        # settings don't leak across calls.
        signal.set_wakeup_fd(write.fileno())

        with captured_stderr() as err:
            signal.raise_signal(signum)

        err = err.getvalue()
        if msg not in err:
            raise AssertionError("second set_wakeup_fd() test failed, "
                                 "stderr: %r" % err)

        """.format(action=action)
        assert_python_ok('-c', code)


@unittest.skipIf(sys.platform == "win32", "Not valid on Windows")
@unittest.skipUnless(hasattr(signal, 'siginterrupt'), "needs signal.siginterrupt()")
@support.requires_subprocess()
class SiginterruptTest(unittest.TestCase):

    def readpipe_interrupted(self, interrupt):
        """Perform a read during which a signal will arrive.  Return True if the
        read is interrupted by the signal and raises an exception.  Return False
        if it returns normally.
        """
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
                1 / 0

            signal.signal(signal.SIGALRM, handler)
            if interrupt is not None:
                signal.siginterrupt(signal.SIGALRM, interrupt)

            print("ready")
            sys.stdout.flush()

            # run the test twice
            try:
                for loop in range(2):
                    # send a SIGALRM in a second (during the read)
                    signal.alarm(1)
                    try:
                        # blocking call: read from a pipe without data
                        os.read(r, 1)
                    except ZeroDivisionError:
                        pass
                    else:
                        sys.exit(2)
                sys.exit(3)
            finally:
                os.close(r)
                os.close(w)
        """ % (interrupt,)
        with spawn_python('-c', code) as process:
            try:
                # wait until the child process is loaded and has started
                first_line = process.stdout.readline()

                stdout, stderr = process.communicate(timeout=support.SHORT_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                return False
            else:
                stdout = first_line + stdout
                exitcode = process.wait()
                if exitcode not in (2, 3):
                    raise Exception("Child error (exit code %s): %r"
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
@unittest.skipUnless(hasattr(signal, 'getitimer') and hasattr(signal, 'setitimer'),
                         "needs signal.getitimer() and signal.setitimer()")
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

    def sig_vtalrm(self, *args):
        self.hndl_called = True

        if self.hndl_count > 3:
            # it shouldn't be here, because it should have been disabled.
            raise signal.ItimerError("setitimer didn't disable ITIMER_VIRTUAL "
                "timer.")
        elif self.hndl_count == 3:
            # disable ITIMER_VIRTUAL, this function shouldn't be called anymore
            signal.setitimer(signal.ITIMER_VIRTUAL, 0)

        self.hndl_count += 1

    def sig_prof(self, *args):
        self.hndl_called = True
        signal.setitimer(signal.ITIMER_PROF, 0)

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
        signal.pause()
        self.assertEqual(self.hndl_called, True)

    # Issue 3864, unknown if this affects earlier versions of freebsd also
    @unittest.skipIf(sys.platform in ('netbsd5',),
        'itimer not reliable (does not mix well with threading) on some BSDs.')
    def test_itimer_virtual(self):
        self.itimer = signal.ITIMER_VIRTUAL
        signal.signal(signal.SIGVTALRM, self.sig_vtalrm)
        signal.setitimer(self.itimer, 0.3, 0.2)

        start_time = time.monotonic()
        while time.monotonic() - start_time < 60.0:
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

    def test_itimer_prof(self):
        self.itimer = signal.ITIMER_PROF
        signal.signal(signal.SIGPROF, self.sig_prof)
        signal.setitimer(self.itimer, 0.2, 0.2)

        start_time = time.monotonic()
        while time.monotonic() - start_time < 60.0:
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

    def test_setitimer_tiny(self):
        # bpo-30807: C setitimer() takes a microsecond-resolution interval.
        # Check that float -> timeval conversion doesn't round
        # the interval down to zero, which would disable the timer.
        self.itimer = signal.ITIMER_REAL
        signal.setitimer(self.itimer, 1e-6)
        time.sleep(1)
        self.assertEqual(self.hndl_called, True)


class PendingSignalsTests(unittest.TestCase):
    """
    Test pthread_sigmask(), pthread_kill(), sigpending() and sigwait()
    functions.
    """
    @unittest.skipUnless(hasattr(signal, 'sigpending'),
                         'need signal.sigpending()')
    def test_sigpending_empty(self):
        self.assertEqual(signal.sigpending(), set())

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    @unittest.skipUnless(hasattr(signal, 'sigpending'),
                         'need signal.sigpending()')
    def test_sigpending(self):
        code = """if 1:
            import os
            import signal

            def handler(signum, frame):
                1/0

            signum = signal.SIGUSR1
            signal.signal(signum, handler)

            signal.pthread_sigmask(signal.SIG_BLOCK, [signum])
            os.kill(os.getpid(), signum)
            pending = signal.sigpending()
            for sig in pending:
                assert isinstance(sig, signal.Signals), repr(pending)
            if pending != {signum}:
                raise Exception('%s != {%s}' % (pending, signum))
            try:
                signal.pthread_sigmask(signal.SIG_UNBLOCK, [signum])
            except ZeroDivisionError:
                pass
            else:
                raise Exception("ZeroDivisionError not raised")
        """
        assert_python_ok('-c', code)

    @unittest.skipUnless(hasattr(signal, 'pthread_kill'),
                         'need signal.pthread_kill()')
    def test_pthread_kill(self):
        code = """if 1:
            import signal
            import threading
            import sys

            signum = signal.SIGUSR1

            def handler(signum, frame):
                1/0

            signal.signal(signum, handler)

            tid = threading.get_ident()
            try:
                signal.pthread_kill(tid, signum)
            except ZeroDivisionError:
                pass
            else:
                raise Exception("ZeroDivisionError not raised")
        """
        assert_python_ok('-c', code)

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def wait_helper(self, blocked, test):
        """
        test: body of the "def test(signum):" function.
        blocked: number of the blocked signal
        """
        code = '''if 1:
        import signal
        import sys
        from signal import Signals

        def handler(signum, frame):
            1/0

        %s

        blocked = %s
        signum = signal.SIGALRM

        # child: block and wait the signal
        try:
            signal.signal(signum, handler)
            signal.pthread_sigmask(signal.SIG_BLOCK, [blocked])

            # Do the tests
            test(signum)

            # The handler must not be called on unblock
            try:
                signal.pthread_sigmask(signal.SIG_UNBLOCK, [blocked])
            except ZeroDivisionError:
                print("the signal handler has been called",
                      file=sys.stderr)
                sys.exit(1)
        except BaseException as err:
            print("error: {}".format(err), file=sys.stderr)
            sys.stderr.flush()
            sys.exit(1)
        ''' % (test.strip(), blocked)

        # sig*wait* must be called with the signal blocked: since the current
        # process might have several threads running, use a subprocess to have
        # a single thread.
        assert_python_ok('-c', code)

    @unittest.skipUnless(hasattr(signal, 'sigwait'),
                         'need signal.sigwait()')
    def test_sigwait(self):
        self.wait_helper(signal.SIGALRM, '''
        def test(signum):
            signal.alarm(1)
            received = signal.sigwait([signum])
            assert isinstance(received, signal.Signals), received
            if received != signum:
                raise Exception('received %s, not %s' % (received, signum))
        ''')

    @unittest.skipUnless(hasattr(signal, 'sigwaitinfo'),
                         'need signal.sigwaitinfo()')
    def test_sigwaitinfo(self):
        self.wait_helper(signal.SIGALRM, '''
        def test(signum):
            signal.alarm(1)
            info = signal.sigwaitinfo([signum])
            if info.si_signo != signum:
                raise Exception("info.si_signo != %s" % signum)
        ''')

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigtimedwait()')
    def test_sigtimedwait(self):
        self.wait_helper(signal.SIGALRM, '''
        def test(signum):
            signal.alarm(1)
            info = signal.sigtimedwait([signum], 10.1000)
            if info.si_signo != signum:
                raise Exception('info.si_signo != %s' % signum)
        ''')

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigtimedwait()')
    def test_sigtimedwait_poll(self):
        # check that polling with sigtimedwait works
        self.wait_helper(signal.SIGALRM, '''
        def test(signum):
            import os
            os.kill(os.getpid(), signum)
            info = signal.sigtimedwait([signum], 0)
            if info.si_signo != signum:
                raise Exception('info.si_signo != %s' % signum)
        ''')

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigtimedwait()')
    def test_sigtimedwait_timeout(self):
        self.wait_helper(signal.SIGALRM, '''
        def test(signum):
            received = signal.sigtimedwait([signum], 1.0)
            if received is not None:
                raise Exception("received=%r" % (received,))
        ''')

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigtimedwait()')
    def test_sigtimedwait_negative_timeout(self):
        signum = signal.SIGALRM
        self.assertRaises(ValueError, signal.sigtimedwait, [signum], -1.0)

    @unittest.skipUnless(hasattr(signal, 'sigwait'),
                         'need signal.sigwait()')
    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_sigwait_thread(self):
        # Check that calling sigwait() from a thread doesn't suspend the whole
        # process. A new interpreter is spawned to avoid problems when mixing
        # threads and fork(): only async-safe functions are allowed between
        # fork() and exec().
        assert_python_ok("-c", """if True:
            import os, threading, sys, time, signal

            # the default handler terminates the process
            signum = signal.SIGUSR1

            def kill_later():
                # wait until the main thread is waiting in sigwait()
                time.sleep(1)
                os.kill(os.getpid(), signum)

            # the signal must be blocked by all the threads
            signal.pthread_sigmask(signal.SIG_BLOCK, [signum])
            killer = threading.Thread(target=kill_later)
            killer.start()
            received = signal.sigwait([signum])
            if received != signum:
                print("sigwait() received %s, not %s" % (received, signum),
                      file=sys.stderr)
                sys.exit(1)
            killer.join()
            # unblock the signal, which should have been cleared by sigwait()
            signal.pthread_sigmask(signal.SIG_UNBLOCK, [signum])
        """)

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_pthread_sigmask_arguments(self):
        self.assertRaises(TypeError, signal.pthread_sigmask)
        self.assertRaises(TypeError, signal.pthread_sigmask, 1)
        self.assertRaises(TypeError, signal.pthread_sigmask, 1, 2, 3)
        self.assertRaises(OSError, signal.pthread_sigmask, 1700, [])
        with self.assertRaises(ValueError):
            signal.pthread_sigmask(signal.SIG_BLOCK, [signal.NSIG])
        with self.assertRaises(ValueError):
            signal.pthread_sigmask(signal.SIG_BLOCK, [0])
        with self.assertRaises(ValueError):
            signal.pthread_sigmask(signal.SIG_BLOCK, [1<<1000])

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_pthread_sigmask_valid_signals(self):
        s = signal.pthread_sigmask(signal.SIG_BLOCK, signal.valid_signals())
        self.addCleanup(signal.pthread_sigmask, signal.SIG_SETMASK, s)
        # Get current blocked set
        s = signal.pthread_sigmask(signal.SIG_UNBLOCK, signal.valid_signals())
        self.assertLessEqual(s, signal.valid_signals())

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_pthread_sigmask(self):
        code = """if 1:
        import signal
        import os; import threading

        def handler(signum, frame):
            1/0

        def kill(signum):
            os.kill(os.getpid(), signum)

        def check_mask(mask):
            for sig in mask:
                assert isinstance(sig, signal.Signals), repr(sig)

        def read_sigmask():
            sigmask = signal.pthread_sigmask(signal.SIG_BLOCK, [])
            check_mask(sigmask)
            return sigmask

        signum = signal.SIGUSR1

        # Install our signal handler
        old_handler = signal.signal(signum, handler)

        # Unblock SIGUSR1 (and copy the old mask) to test our signal handler
        old_mask = signal.pthread_sigmask(signal.SIG_UNBLOCK, [signum])
        check_mask(old_mask)
        try:
            kill(signum)
        except ZeroDivisionError:
            pass
        else:
            raise Exception("ZeroDivisionError not raised")

        # Block and then raise SIGUSR1. The signal is blocked: the signal
        # handler is not called, and the signal is now pending
        mask = signal.pthread_sigmask(signal.SIG_BLOCK, [signum])
        check_mask(mask)
        kill(signum)

        # Check the new mask
        blocked = read_sigmask()
        check_mask(blocked)
        if signum not in blocked:
            raise Exception("%s not in %s" % (signum, blocked))
        if old_mask ^ blocked != {signum}:
            raise Exception("%s ^ %s != {%s}" % (old_mask, blocked, signum))

        # Unblock SIGUSR1
        try:
            # unblock the pending signal calls immediately the signal handler
            signal.pthread_sigmask(signal.SIG_UNBLOCK, [signum])
        except ZeroDivisionError:
            pass
        else:
            raise Exception("ZeroDivisionError not raised")
        try:
            kill(signum)
        except ZeroDivisionError:
            pass
        else:
            raise Exception("ZeroDivisionError not raised")

        # Check the new mask
        unblocked = read_sigmask()
        if signum in unblocked:
            raise Exception("%s in %s" % (signum, unblocked))
        if blocked ^ unblocked != {signum}:
            raise Exception("%s ^ %s != {%s}" % (blocked, unblocked, signum))
        if old_mask != unblocked:
            raise Exception("%s != %s" % (old_mask, unblocked))
        """
        assert_python_ok('-c', code)

    @unittest.skipUnless(hasattr(signal, 'pthread_kill'),
                         'need signal.pthread_kill()')
    def test_pthread_kill_main_thread(self):
        # Test that a signal can be sent to the main thread with pthread_kill()
        # before any other thread has been created (see issue #12392).
        code = """if True:
            import threading
            import signal
            import sys

            def handler(signum, frame):
                sys.exit(3)

            signal.signal(signal.SIGUSR1, handler)
            signal.pthread_kill(threading.get_ident(), signal.SIGUSR1)
            sys.exit(2)
        """

        with spawn_python('-c', code) as process:
            stdout, stderr = process.communicate()
            exitcode = process.wait()
            if exitcode != 3:
                raise Exception("Child error (exit code %s): %s" %
                                (exitcode, stdout))


class StressTest(unittest.TestCase):
    """
    Stress signal delivery, especially when a signal arrives in
    the middle of recomputing the signal state or executing
    previously tripped signal handlers.
    """

    def setsig(self, signum, handler):
        old_handler = signal.signal(signum, handler)
        self.addCleanup(signal.signal, signum, old_handler)

    def measure_itimer_resolution(self):
        N = 20
        times = []

        def handler(signum=None, frame=None):
            if len(times) < N:
                times.append(time.perf_counter())
                # 1 µs is the smallest possible timer interval,
                # we want to measure what the concrete duration
                # will be on this platform
                signal.setitimer(signal.ITIMER_REAL, 1e-6)

        self.addCleanup(signal.setitimer, signal.ITIMER_REAL, 0)
        self.setsig(signal.SIGALRM, handler)
        handler()
        while len(times) < N:
            time.sleep(1e-3)

        durations = [times[i+1] - times[i] for i in range(len(times) - 1)]
        med = statistics.median(durations)
        if support.verbose:
            print("detected median itimer() resolution: %.6f s." % (med,))
        return med

    def decide_itimer_count(self):
        # Some systems have poor setitimer() resolution (for example
        # measured around 20 ms. on FreeBSD 9), so decide on a reasonable
        # number of sequential timers based on that.
        reso = self.measure_itimer_resolution()
        if reso <= 1e-4:
            return 10000
        elif reso <= 1e-2:
            return 100
        else:
            self.skipTest("detected itimer resolution (%.3f s.) too high "
                          "(> 10 ms.) on this platform (or system too busy)"
                          % (reso,))

    @unittest.skipUnless(hasattr(signal, "setitimer"),
                         "test needs setitimer()")
    def test_stress_delivery_dependent(self):
        """
        This test uses dependent signal handlers.
        """
        N = self.decide_itimer_count()
        sigs = []

        def first_handler(signum, frame):
            # 1e-6 is the minimum non-zero value for `setitimer()`.
            # Choose a random delay so as to improve chances of
            # triggering a race condition.  Ideally the signal is received
            # when inside critical signal-handling routines such as
            # Py_MakePendingCalls().
            signal.setitimer(signal.ITIMER_REAL, 1e-6 + random.random() * 1e-5)

        def second_handler(signum=None, frame=None):
            sigs.append(signum)

        # Here on Linux, SIGPROF > SIGALRM > SIGUSR1.  By using both
        # ascending and descending sequences (SIGUSR1 then SIGALRM,
        # SIGPROF then SIGALRM), we maximize chances of hitting a bug.
        self.setsig(signal.SIGPROF, first_handler)
        self.setsig(signal.SIGUSR1, first_handler)
        self.setsig(signal.SIGALRM, second_handler)  # for ITIMER_REAL

        expected_sigs = 0
        deadline = time.monotonic() + support.SHORT_TIMEOUT

        while expected_sigs < N:
            os.kill(os.getpid(), signal.SIGPROF)
            expected_sigs += 1
            # Wait for handlers to run to avoid signal coalescing
            while len(sigs) < expected_sigs and time.monotonic() < deadline:
                time.sleep(1e-5)

            os.kill(os.getpid(), signal.SIGUSR1)
            expected_sigs += 1
            while len(sigs) < expected_sigs and time.monotonic() < deadline:
                time.sleep(1e-5)

        # All ITIMER_REAL signals should have been delivered to the
        # Python handler
        self.assertEqual(len(sigs), N, "Some signals were lost")

    @unittest.skipUnless(hasattr(signal, "setitimer"),
                         "test needs setitimer()")
    def test_stress_delivery_simultaneous(self):
        """
        This test uses simultaneous signal handlers.
        """
        N = self.decide_itimer_count()
        sigs = []

        def handler(signum, frame):
            sigs.append(signum)

        self.setsig(signal.SIGUSR1, handler)
        self.setsig(signal.SIGALRM, handler)  # for ITIMER_REAL

        expected_sigs = 0
        deadline = time.monotonic() + support.SHORT_TIMEOUT

        while expected_sigs < N:
            # Hopefully the SIGALRM will be received somewhere during
            # initial processing of SIGUSR1.
            signal.setitimer(signal.ITIMER_REAL, 1e-6 + random.random() * 1e-5)
            os.kill(os.getpid(), signal.SIGUSR1)

            expected_sigs += 2
            # Wait for handlers to run to avoid signal coalescing
            while len(sigs) < expected_sigs and time.monotonic() < deadline:
                time.sleep(1e-5)

        # All ITIMER_REAL signals should have been delivered to the
        # Python handler
        self.assertEqual(len(sigs), N, "Some signals were lost")

    @unittest.skipUnless(hasattr(signal, "SIGUSR1"),
                         "test needs SIGUSR1")
    def test_stress_modifying_handlers(self):
        # bpo-43406: race condition between trip_signal() and signal.signal
        signum = signal.SIGUSR1
        num_sent_signals = 0
        num_received_signals = 0
        do_stop = False

        def custom_handler(signum, frame):
            nonlocal num_received_signals
            num_received_signals += 1

        def set_interrupts():
            nonlocal num_sent_signals
            while not do_stop:
                signal.raise_signal(signum)
                num_sent_signals += 1

        def cycle_handlers():
            while num_sent_signals < 100:
                for i in range(20000):
                    # Cycle between a Python-defined and a non-Python handler
                    for handler in [custom_handler, signal.SIG_IGN]:
                        signal.signal(signum, handler)

        old_handler = signal.signal(signum, custom_handler)
        self.addCleanup(signal.signal, signum, old_handler)

        t = threading.Thread(target=set_interrupts)
        try:
            ignored = False
            with support.catch_unraisable_exception() as cm:
                t.start()
                cycle_handlers()
                do_stop = True
                t.join()

                if cm.unraisable is not None:
                    # An unraisable exception may be printed out when
                    # a signal is ignored due to the aforementioned
                    # race condition, check it.
                    self.assertIsInstance(cm.unraisable.exc_value, OSError)
                    self.assertIn(
                        f"Signal {signum:d} ignored due to race condition",
                        str(cm.unraisable.exc_value))
                    ignored = True

            # bpo-43406: Even if it is unlikely, it's technically possible that
            # all signals were ignored because of race conditions.
            if not ignored:
                # Sanity check that some signals were received, but not all
                self.assertGreater(num_received_signals, 0)
            self.assertLess(num_received_signals, num_sent_signals)
        finally:
            do_stop = True
            t.join()


class RaiseSignalTest(unittest.TestCase):

    def test_sigint(self):
        with self.assertRaises(KeyboardInterrupt):
            signal.raise_signal(signal.SIGINT)

    @unittest.skipIf(sys.platform != "win32", "Windows specific test")
    def test_invalid_argument(self):
        try:
            SIGHUP = 1 # not supported on win32
            signal.raise_signal(SIGHUP)
            self.fail("OSError (Invalid argument) expected")
        except OSError as e:
            if e.errno == errno.EINVAL:
                pass
            else:
                raise

    def test_handler(self):
        is_ok = False
        def handler(a, b):
            nonlocal is_ok
            is_ok = True
        old_signal = signal.signal(signal.SIGINT, handler)
        self.addCleanup(signal.signal, signal.SIGINT, old_signal)

        signal.raise_signal(signal.SIGINT)
        self.assertTrue(is_ok)


class PidfdSignalTest(unittest.TestCase):

    @unittest.skipUnless(
        hasattr(signal, "pidfd_send_signal"),
        "pidfd support not built in",
    )
    def test_pidfd_send_signal(self):
        with self.assertRaises(OSError) as cm:
            signal.pidfd_send_signal(0, signal.SIGINT)
        if cm.exception.errno == errno.ENOSYS:
            self.skipTest("kernel does not support pidfds")
        elif cm.exception.errno == errno.EPERM:
            self.skipTest("Not enough privileges to use pidfs")
        self.assertEqual(cm.exception.errno, errno.EBADF)
        my_pidfd = os.open(f'/proc/{os.getpid()}', os.O_DIRECTORY)
        self.addCleanup(os.close, my_pidfd)
        with self.assertRaisesRegex(TypeError, "^siginfo must be None$"):
            signal.pidfd_send_signal(my_pidfd, signal.SIGINT, object(), 0)
        with self.assertRaises(KeyboardInterrupt):
            signal.pidfd_send_signal(my_pidfd, signal.SIGINT)

def tearDownModule():
    support.reap_children()

if __name__ == "__main__":
    unittest.main()

"""
This test suite exercises some system calls subject to interruption with EINTR,
to check that it is actually handled transparently.
It is intended to be run by the main test suite within a child process, to
ensure there is no background thread running (so that signals are delivered to
the correct thread).
Signals are generated in-process using setitimer(ITIMER_REAL), which allows
sub-second periodicity (contrarily to signal()).
"""

import contextlib
import faulthandler
import fcntl
import os
import platform
import select
import signal
import socket
import subprocess
import sys
import textwrap
import time
import unittest

from test import support
from test.support import os_helper
from test.support import socket_helper


# gh-109592: Tolerate a difference of 20 ms when comparing timings
# (clock resolution)
CLOCK_RES = 0.020


@contextlib.contextmanager
def kill_on_error(proc):
    """Context manager killing the subprocess if a Python exception is raised."""
    with proc:
        try:
            yield proc
        except:
            proc.kill()
            raise


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class EINTRBaseTest(unittest.TestCase):
    """ Base class for EINTR tests. """

    # delay for initial signal delivery
    signal_delay = 0.1
    # signal delivery periodicity
    signal_period = 0.1
    # default sleep time for tests - should obviously have:
    # sleep_time > signal_period
    sleep_time = 0.2

    def sighandler(self, signum, frame):
        self.signals += 1

    def setUp(self):
        self.signals = 0
        self.orig_handler = signal.signal(signal.SIGALRM, self.sighandler)
        signal.setitimer(signal.ITIMER_REAL, self.signal_delay,
                         self.signal_period)

        # Use faulthandler as watchdog to debug when a test hangs
        # (timeout of 10 minutes)
        faulthandler.dump_traceback_later(10 * 60, exit=True,
                                          file=sys.__stderr__)

    @staticmethod
    def stop_alarm():
        signal.setitimer(signal.ITIMER_REAL, 0, 0)

    def tearDown(self):
        self.stop_alarm()
        signal.signal(signal.SIGALRM, self.orig_handler)
        faulthandler.cancel_dump_traceback_later()

    def subprocess(self, *args, **kw):
        cmd_args = (sys.executable, '-c') + args
        return subprocess.Popen(cmd_args, **kw)

    def check_elapsed_time(self, elapsed):
        self.assertGreaterEqual(elapsed, self.sleep_time - CLOCK_RES)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class OSEINTRTest(EINTRBaseTest):
    """ EINTR tests for the os module. """

    def new_sleep_process(self):
        code = f'import time; time.sleep({self.sleep_time!r})'
        return self.subprocess(code)

    def _test_wait_multiple(self, wait_func):
        num = 3
        processes = [self.new_sleep_process() for _ in range(num)]
        for _ in range(num):
            wait_func()
        # Call the Popen method to avoid a ResourceWarning
        for proc in processes:
            proc.wait()

    def test_wait(self):
        self._test_wait_multiple(os.wait)

    @unittest.skipUnless(hasattr(os, 'wait3'), 'requires wait3()')
    def test_wait3(self):
        self._test_wait_multiple(lambda: os.wait3(0))

    def _test_wait_single(self, wait_func):
        proc = self.new_sleep_process()
        wait_func(proc.pid)
        # Call the Popen method to avoid a ResourceWarning
        proc.wait()

    def test_waitpid(self):
        self._test_wait_single(lambda pid: os.waitpid(pid, 0))

    @unittest.skipUnless(hasattr(os, 'wait4'), 'requires wait4()')
    def test_wait4(self):
        self._test_wait_single(lambda pid: os.wait4(pid, 0))

    def _interrupted_reads(self):
        """Make a fd which will force block on read of expected bytes."""
        rd, wr = os.pipe()
        self.addCleanup(os.close, rd)
        # wr closed explicitly by parent

        # the payload below are smaller than PIPE_BUF, hence the writes will be
        # atomic
        data = [b"hello", b"world", b"spam"]

        code = '\n'.join((
            'import os, sys, time',
            '',
            'wr = int(sys.argv[1])',
            f'data = {data!r}',
            f'sleep_time = {self.sleep_time!r}',
            '',
            'for item in data:',
            '    # let the parent block on read()',
            '    time.sleep(sleep_time)',
            '    os.write(wr, item)',
        ))

        proc = self.subprocess(code, str(wr), pass_fds=[wr])
        with kill_on_error(proc):
            os.close(wr)
            for datum in data:
                yield rd, datum
            self.assertEqual(proc.wait(), 0)

    def test_read(self):
        for fd, expected in self._interrupted_reads():
            self.assertEqual(expected, os.read(fd, len(expected)))

    def test_readinto(self):
        for fd, expected in self._interrupted_reads():
            buffer = bytearray(len(expected))
            self.assertEqual(os.readinto(fd, buffer), len(expected))
            self.assertEqual(buffer, expected)

    def test_write(self):
        rd, wr = os.pipe()
        self.addCleanup(os.close, wr)
        # rd closed explicitly by parent

        # we must write enough data for the write() to block
        data = b"x" * support.PIPE_MAX_SIZE

        code = '\n'.join((
            'import io, os, sys, time',
            '',
            'rd = int(sys.argv[1])',
            f'sleep_time = {self.sleep_time!r}',
            f'data = b"x" * {support.PIPE_MAX_SIZE}',
            'data_len = len(data)',
            '',
            '# let the parent block on write()',
            'time.sleep(sleep_time)',
            '',
            'read_data = io.BytesIO()',
            'while len(read_data.getvalue()) < data_len:',
            '    chunk = os.read(rd, 2 * data_len)',
            '    read_data.write(chunk)',
            '',
            'value = read_data.getvalue()',
            'if value != data:',
            '    raise Exception(f"read error: {len(value)}'
                                  ' vs {data_len} bytes")',
        ))

        proc = self.subprocess(code, str(rd), pass_fds=[rd])
        with kill_on_error(proc):
            os.close(rd)
            written = 0
            while written < len(data):
                written += os.write(wr, memoryview(data)[written:])
            self.assertEqual(proc.wait(), 0)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class SocketEINTRTest(EINTRBaseTest):
    """ EINTR tests for the socket module. """

    @unittest.skipUnless(hasattr(socket, 'socketpair'), 'needs socketpair()')
    def _test_recv(self, recv_func):
        rd, wr = socket.socketpair()
        self.addCleanup(rd.close)
        # wr closed explicitly by parent

        # single-byte payload guard us against partial recv
        data = [b"x", b"y", b"z"]

        code = '\n'.join((
            'import os, socket, sys, time',
            '',
            'fd = int(sys.argv[1])',
            f'family = {int(wr.family)}',
            f'sock_type = {int(wr.type)}',
            f'data = {data!r}',
            f'sleep_time = {self.sleep_time!r}',
            '',
            'wr = socket.fromfd(fd, family, sock_type)',
            'os.close(fd)',
            '',
            'with wr:',
            '    for item in data:',
            '        # let the parent block on recv()',
            '        time.sleep(sleep_time)',
            '        wr.sendall(item)',
        ))

        fd = wr.fileno()
        proc = self.subprocess(code, str(fd), pass_fds=[fd])
        with kill_on_error(proc):
            wr.close()
            for item in data:
                self.assertEqual(item, recv_func(rd, len(item)))
            self.assertEqual(proc.wait(), 0)

    def test_recv(self):
        self._test_recv(socket.socket.recv)

    @unittest.skipUnless(hasattr(socket.socket, 'recvmsg'), 'needs recvmsg()')
    def test_recvmsg(self):
        self._test_recv(lambda sock, data: sock.recvmsg(data)[0])

    def _test_send(self, send_func):
        rd, wr = socket.socketpair()
        self.addCleanup(wr.close)
        # rd closed explicitly by parent

        # we must send enough data for the send() to block
        data = b"xyz" * (support.SOCK_MAX_SIZE // 3)

        code = '\n'.join((
            'import os, socket, sys, time',
            '',
            'fd = int(sys.argv[1])',
            f'family = {int(rd.family)}',
            f'sock_type = {int(rd.type)}',
            f'sleep_time = {self.sleep_time!r}',
            f'data = b"xyz" * {support.SOCK_MAX_SIZE // 3}',
            'data_len = len(data)',
            '',
            'rd = socket.fromfd(fd, family, sock_type)',
            'os.close(fd)',
            '',
            'with rd:',
            '    # let the parent block on send()',
            '    time.sleep(sleep_time)',
            '',
            '    received_data = bytearray(data_len)',
            '    n = 0',
            '    while n < data_len:',
            '        n += rd.recv_into(memoryview(received_data)[n:])',
            '',
            'if received_data != data:',
            '    raise Exception(f"recv error: {len(received_data)}'
                                  ' vs {data_len} bytes")',
        ))

        fd = rd.fileno()
        proc = self.subprocess(code, str(fd), pass_fds=[fd])
        with kill_on_error(proc):
            rd.close()
            written = 0
            while written < len(data):
                sent = send_func(wr, memoryview(data)[written:])
                # sendall() returns None
                written += len(data) if sent is None else sent
            self.assertEqual(proc.wait(), 0)

    def test_send(self):
        self._test_send(socket.socket.send)

    def test_sendall(self):
        self._test_send(socket.socket.sendall)

    @unittest.skipUnless(hasattr(socket.socket, 'sendmsg'), 'needs sendmsg()')
    def test_sendmsg(self):
        self._test_send(lambda sock, data: sock.sendmsg([data]))

    def test_accept(self):
        sock = socket.create_server((socket_helper.HOST, 0))
        self.addCleanup(sock.close)
        port = sock.getsockname()[1]

        code = '\n'.join((
            'import socket, time',
            '',
            f'host = {socket_helper.HOST!r}',
            f'port = {port}',
            f'sleep_time = {self.sleep_time!r}',
            '',
            '# let parent block on accept()',
            'time.sleep(sleep_time)',
            'with socket.create_connection((host, port)):',
            '    time.sleep(sleep_time)',
        ))

        proc = self.subprocess(code)
        with kill_on_error(proc):
            client_sock, _ = sock.accept()
            client_sock.close()
            self.assertEqual(proc.wait(), 0)

    # Issue #25122: There is a race condition in the FreeBSD kernel on
    # handling signals in the FIFO device. Skip the test until the bug is
    # fixed in the kernel.
    # https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=203162
    @support.requires_freebsd_version(10, 3)
    @unittest.skipUnless(hasattr(os, 'mkfifo'), 'needs mkfifo()')
    def _test_open(self, do_open_close_reader, do_open_close_writer):
        filename = os_helper.TESTFN

        # Use a fifo: until the child opens it for reading, the parent will
        # block when trying to open it for writing.
        os_helper.unlink(filename)
        try:
            os.mkfifo(filename)
        except PermissionError as exc:
            self.skipTest(f'os.mkfifo(): {exc!r}')
        self.addCleanup(os_helper.unlink, filename)

        code = '\n'.join((
            'import os, time',
            '',
            f'path = {filename!a}',
            f'sleep_time = {self.sleep_time!r}',
            '',
            '# let the parent block',
            'time.sleep(sleep_time)',
            '',
            do_open_close_reader,
        ))

        proc = self.subprocess(code)
        with kill_on_error(proc):
            do_open_close_writer(filename)
            self.assertEqual(proc.wait(), 0)

    def python_open(self, path):
        fp = open(path, 'w')
        fp.close()

    @unittest.skipIf(sys.platform == "darwin",
                     "hangs under macOS; see bpo-25234, bpo-35363")
    def test_open(self):
        self._test_open("fp = open(path, 'r')\nfp.close()",
                        self.python_open)

    def os_open(self, path):
        fd = os.open(path, os.O_WRONLY)
        os.close(fd)

    @unittest.skipIf(sys.platform == "darwin",
                     "hangs under macOS; see bpo-25234, bpo-35363")
    def test_os_open(self):
        self._test_open("fd = os.open(path, os.O_RDONLY)\nos.close(fd)",
                        self.os_open)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class TimeEINTRTest(EINTRBaseTest):
    """ EINTR tests for the time module. """

    def test_sleep(self):
        t0 = time.monotonic()
        time.sleep(self.sleep_time)
        self.stop_alarm()
        dt = time.monotonic() - t0
        self.check_elapsed_time(dt)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
# bpo-30320: Need pthread_sigmask() to block the signal, otherwise the test
# is vulnerable to a race condition between the child and the parent processes.
@unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                     'need signal.pthread_sigmask()')
class SignalEINTRTest(EINTRBaseTest):
    """ EINTR tests for the signal module. """

    def check_sigwait(self, wait_func):
        signum = signal.SIGUSR1

        old_handler = signal.signal(signum, lambda *args: None)
        self.addCleanup(signal.signal, signum, old_handler)

        code = '\n'.join((
            'import os, time',
            f'pid = {os.getpid()}',
            f'signum = {int(signum)}',
            f'sleep_time = {self.sleep_time!r}',
            'time.sleep(sleep_time)',
            'os.kill(pid, signum)',
        ))

        signal.pthread_sigmask(signal.SIG_BLOCK, [signum])
        self.addCleanup(signal.pthread_sigmask, signal.SIG_UNBLOCK, [signum])

        proc = self.subprocess(code)
        with kill_on_error(proc):
            wait_func(signum)

        self.assertEqual(proc.wait(), 0)

    @unittest.skipUnless(hasattr(signal, 'sigwaitinfo'),
                         'need signal.sigwaitinfo()')
    def test_sigwaitinfo(self):
        def wait_func(signum):
            signal.sigwaitinfo([signum])

        self.check_sigwait(wait_func)

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigwaitinfo()')
    def test_sigtimedwait(self):
        def wait_func(signum):
            signal.sigtimedwait([signum], 120.0)

        self.check_sigwait(wait_func)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class SelectEINTRTest(EINTRBaseTest):
    """ EINTR tests for the select module. """

    def test_select(self):
        t0 = time.monotonic()
        select.select([], [], [], self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.check_elapsed_time(dt)

    @unittest.skipIf(sys.platform == "darwin",
                     "poll may fail on macOS; see issue #28087")
    @unittest.skipUnless(hasattr(select, 'poll'), 'need select.poll')
    def test_poll(self):
        poller = select.poll()

        t0 = time.monotonic()
        poller.poll(self.sleep_time * 1e3)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.check_elapsed_time(dt)

    @unittest.skipUnless(hasattr(select, 'epoll'), 'need select.epoll')
    def test_epoll(self):
        poller = select.epoll()
        self.addCleanup(poller.close)

        t0 = time.monotonic()
        poller.poll(self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.check_elapsed_time(dt)

    @unittest.skipUnless(hasattr(select, 'kqueue'), 'need select.kqueue')
    def test_kqueue(self):
        kqueue = select.kqueue()
        self.addCleanup(kqueue.close)

        t0 = time.monotonic()
        kqueue.control(None, 1, self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.check_elapsed_time(dt)

    @unittest.skipUnless(hasattr(select, 'devpoll'), 'need select.devpoll')
    def test_devpoll(self):
        poller = select.devpoll()
        self.addCleanup(poller.close)

        t0 = time.monotonic()
        poller.poll(self.sleep_time * 1e3)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.check_elapsed_time(dt)


class FCNTLEINTRTest(EINTRBaseTest):
    def _lock(self, lock_func, lock_name):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        rd1, wr1 = os.pipe()
        rd2, wr2 = os.pipe()
        for fd in (rd1, wr1, rd2, wr2):
            self.addCleanup(os.close, fd)
        code = textwrap.dedent(f"""
            import fcntl, os, time
            with open('{os_helper.TESTFN}', 'wb') as f:
                fcntl.{lock_name}(f, fcntl.LOCK_EX)
                os.write({wr1}, b"ok")
                _ = os.read({rd2}, 2)  # wait for parent process
                time.sleep({self.sleep_time})
        """)
        proc = self.subprocess(code, pass_fds=[wr1, rd2])
        with kill_on_error(proc):
            with open(os_helper.TESTFN, 'wb') as f:
                # synchronize the subprocess
                ok = os.read(rd1, 2)
                self.assertEqual(ok, b"ok")

                # notify the child that the parent is ready
                start_time = time.monotonic()
                os.write(wr2, b"go")

                # the child locked the file just a moment ago for 'sleep_time' seconds
                # that means that the lock below will block for 'sleep_time' minus some
                # potential context switch delay
                lock_func(f, fcntl.LOCK_EX)
                dt = time.monotonic() - start_time
                self.stop_alarm()
                self.check_elapsed_time(dt)
            proc.wait()

    # Issue 35633: See https://bugs.python.org/issue35633#msg333662
    # skip test rather than accept PermissionError from all platforms
    @unittest.skipIf(platform.system() == "AIX", "AIX returns PermissionError")
    def test_lockf(self):
        self._lock(fcntl.lockf, "lockf")

    def test_flock(self):
        self._lock(fcntl.flock, "flock")


if __name__ == "__main__":
    unittest.main()

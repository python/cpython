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
import os
import select
import signal
import socket
import subprocess
import sys
import time
import unittest

from test import support
android_not_root = support.android_not_root

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

    @classmethod
    def setUpClass(cls):
        cls.orig_handler = signal.signal(signal.SIGALRM, lambda *args: None)
        signal.setitimer(signal.ITIMER_REAL, cls.signal_delay,
                         cls.signal_period)

        # Issue #25277: Use faulthandler to try to debug a hang on FreeBSD
        if hasattr(faulthandler, 'dump_traceback_later'):
            faulthandler.dump_traceback_later(10 * 60, exit=True)

    @classmethod
    def stop_alarm(cls):
        signal.setitimer(signal.ITIMER_REAL, 0, 0)

    @classmethod
    def tearDownClass(cls):
        cls.stop_alarm()
        signal.signal(signal.SIGALRM, cls.orig_handler)
        if hasattr(faulthandler, 'cancel_dump_traceback_later'):
            faulthandler.cancel_dump_traceback_later()

    def subprocess(self, *args, **kw):
        cmd_args = (sys.executable, '-c') + args
        return subprocess.Popen(cmd_args, **kw)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class OSEINTRTest(EINTRBaseTest):
    """ EINTR tests for the os module. """

    def new_sleep_process(self):
        code = 'import time; time.sleep(%r)' % self.sleep_time
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

    def test_read(self):
        rd, wr = os.pipe()
        self.addCleanup(os.close, rd)
        # wr closed explicitly by parent

        # the payload below are smaller than PIPE_BUF, hence the writes will be
        # atomic
        datas = [b"hello", b"world", b"spam"]

        code = '\n'.join((
            'import os, sys, time',
            '',
            'wr = int(sys.argv[1])',
            'datas = %r' % datas,
            'sleep_time = %r' % self.sleep_time,
            '',
            'for data in datas:',
            '    # let the parent block on read()',
            '    time.sleep(sleep_time)',
            '    os.write(wr, data)',
        ))

        proc = self.subprocess(code, str(wr), pass_fds=[wr])
        with kill_on_error(proc):
            os.close(wr)
            for data in datas:
                self.assertEqual(data, os.read(rd, len(data)))
            self.assertEqual(proc.wait(), 0)

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
            'sleep_time = %r' % self.sleep_time,
            'data = b"x" * %s' % support.PIPE_MAX_SIZE,
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
            '    raise Exception("read error: %s vs %s bytes"',
            '                    % (len(value), data_len))',
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
        datas = [b"x", b"y", b"z"]

        code = '\n'.join((
            'import os, socket, sys, time',
            '',
            'fd = int(sys.argv[1])',
            'family = %s' % int(wr.family),
            'sock_type = %s' % int(wr.type),
            'datas = %r' % datas,
            'sleep_time = %r' % self.sleep_time,
            '',
            'wr = socket.fromfd(fd, family, sock_type)',
            'os.close(fd)',
            '',
            'with wr:',
            '    for data in datas:',
            '        # let the parent block on recv()',
            '        time.sleep(sleep_time)',
            '        wr.sendall(data)',
        ))

        fd = wr.fileno()
        proc = self.subprocess(code, str(fd), pass_fds=[fd])
        with kill_on_error(proc):
            wr.close()
            for data in datas:
                self.assertEqual(data, recv_func(rd, len(data)))
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
            'family = %s' % int(rd.family),
            'sock_type = %s' % int(rd.type),
            'sleep_time = %r' % self.sleep_time,
            'data = b"xyz" * %s' % (support.SOCK_MAX_SIZE // 3),
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
            '    raise Exception("recv error: %s vs %s bytes"',
            '                    % (len(received_data), data_len))',
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
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)

        sock.bind((support.HOST, 0))
        port = sock.getsockname()[1]
        sock.listen()

        code = '\n'.join((
            'import socket, time',
            '',
            'host = %r' % support.HOST,
            'port = %s' % port,
            'sleep_time = %r' % self.sleep_time,
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
    @unittest.skipIf(android_not_root, "mkfifo not allowed, non root user")
    def _test_open(self, do_open_close_reader, do_open_close_writer):
        filename = support.TESTFN

        # Use a fifo: until the child opens it for reading, the parent will
        # block when trying to open it for writing.
        support.unlink(filename)
        os.mkfifo(filename)
        self.addCleanup(support.unlink, filename)

        code = '\n'.join((
            'import os, time',
            '',
            'path = %a' % filename,
            'sleep_time = %r' % self.sleep_time,
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

    def test_open(self):
        self._test_open("fp = open(path, 'r')\nfp.close()",
                        self.python_open)

    @unittest.skipIf(sys.platform == 'darwin', "hangs under OS X; see issue #25234")
    def os_open(self, path):
        fd = os.open(path, os.O_WRONLY)
        os.close(fd)

    @unittest.skipIf(sys.platform == "darwin", "hangs under OS X; see issue #25234")
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
        self.assertGreaterEqual(dt, self.sleep_time)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
# bpo-30320: Need pthread_sigmask() to block the signal, otherwise the test
# is vulnerable to a race condition between the child and the parent processes.
@unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                     'need signal.pthread_sigmask()')
class SignalEINTRTest(EINTRBaseTest):
    """ EINTR tests for the signal module. """

    def check_sigwait(self, wait_func):
        signum = signal.SIGUSR1
        pid = os.getpid()

        old_handler = signal.signal(signum, lambda *args: None)
        self.addCleanup(signal.signal, signum, old_handler)

        code = '\n'.join((
            'import os, time',
            'pid = %s' % os.getpid(),
            'signum = %s' % int(signum),
            'sleep_time = %r' % self.sleep_time,
            'time.sleep(sleep_time)',
            'os.kill(pid, signum)',
        ))

        old_mask = signal.pthread_sigmask(signal.SIG_BLOCK, [signum])
        self.addCleanup(signal.pthread_sigmask, signal.SIG_UNBLOCK, [signum])

        t0 = time.monotonic()
        proc = self.subprocess(code)
        with kill_on_error(proc):
            wait_func(signum)
            dt = time.monotonic() - t0

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
        self.assertGreaterEqual(dt, self.sleep_time)

    @unittest.skipIf(sys.platform == "darwin",
                     "poll may fail on macOS; see issue #28087")
    @unittest.skipUnless(hasattr(select, 'poll'), 'need select.poll')
    def test_poll(self):
        poller = select.poll()

        t0 = time.monotonic()
        poller.poll(self.sleep_time * 1e3)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.assertGreaterEqual(dt, self.sleep_time)

    @unittest.skipUnless(hasattr(select, 'epoll'), 'need select.epoll')
    def test_epoll(self):
        poller = select.epoll()
        self.addCleanup(poller.close)

        t0 = time.monotonic()
        poller.poll(self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.assertGreaterEqual(dt, self.sleep_time)

    @unittest.skipUnless(hasattr(select, 'kqueue'), 'need select.kqueue')
    def test_kqueue(self):
        kqueue = select.kqueue()
        self.addCleanup(kqueue.close)

        t0 = time.monotonic()
        kqueue.control(None, 1, self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.assertGreaterEqual(dt, self.sleep_time)

    @unittest.skipUnless(hasattr(select, 'devpoll'), 'need select.devpoll')
    def test_devpoll(self):
        poller = select.devpoll()
        self.addCleanup(poller.close)

        t0 = time.monotonic()
        poller.poll(self.sleep_time * 1e3)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.assertGreaterEqual(dt, self.sleep_time)


def test_main():
    support.run_unittest(
        OSEINTRTest,
        SocketEINTRTest,
        TimeEINTRTest,
        SignalEINTRTest,
        SelectEINTRTest)


if __name__ == "__main__":
    test_main()

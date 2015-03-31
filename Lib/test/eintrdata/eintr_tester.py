"""
This test suite exercises some system calls subject to interruption with EINTR,
to check that it is actually handled transparently.
It is intended to be run by the main test suite within a child process, to
ensure there is no background thread running (so that signals are delivered to
the correct thread).
Signals are generated in-process using setitimer(ITIMER_REAL), which allows
sub-second periodicity (contrarily to signal()).
"""

import io
import os
import select
import signal
import socket
import time
import unittest

from test import support


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class EINTRBaseTest(unittest.TestCase):
    """ Base class for EINTR tests. """

    # delay for initial signal delivery
    signal_delay = 0.1
    # signal delivery periodicity
    signal_period = 0.1
    # default sleep time for tests - should obviously have:
    # sleep_time > signal_period
    sleep_time = 0.2

    @classmethod
    def setUpClass(cls):
        cls.orig_handler = signal.signal(signal.SIGALRM, lambda *args: None)
        signal.setitimer(signal.ITIMER_REAL, cls.signal_delay,
                         cls.signal_period)

    @classmethod
    def stop_alarm(cls):
        signal.setitimer(signal.ITIMER_REAL, 0, 0)

    @classmethod
    def tearDownClass(cls):
        cls.stop_alarm()
        signal.signal(signal.SIGALRM, cls.orig_handler)

    @classmethod
    def _sleep(cls):
        # default sleep time
        time.sleep(cls.sleep_time)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class OSEINTRTest(EINTRBaseTest):
    """ EINTR tests for the os module. """

    def _test_wait_multiple(self, wait_func):
        num = 3
        for _ in range(num):
            pid = os.fork()
            if pid == 0:
                self._sleep()
                os._exit(0)
        for _ in range(num):
            wait_func()

    def test_wait(self):
        self._test_wait_multiple(os.wait)

    @unittest.skipUnless(hasattr(os, 'wait3'), 'requires wait3()')
    def test_wait3(self):
        self._test_wait_multiple(lambda: os.wait3(0))

    def _test_wait_single(self, wait_func):
        pid = os.fork()
        if pid == 0:
            self._sleep()
            os._exit(0)
        else:
            wait_func(pid)

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

        pid = os.fork()
        if pid == 0:
            os.close(rd)
            for data in datas:
                # let the parent block on read()
                self._sleep()
                os.write(wr, data)
            os._exit(0)
        else:
            self.addCleanup(os.waitpid, pid, 0)
            os.close(wr)
            for data in datas:
                self.assertEqual(data, os.read(rd, len(data)))

    def test_write(self):
        rd, wr = os.pipe()
        self.addCleanup(os.close, wr)
        # rd closed explicitly by parent

        # we must write enough data for the write() to block
        data = b"xyz" * support.PIPE_MAX_SIZE

        pid = os.fork()
        if pid == 0:
            os.close(wr)
            read_data = io.BytesIO()
            # let the parent block on write()
            self._sleep()
            while len(read_data.getvalue()) < len(data):
                chunk = os.read(rd, 2 * len(data))
                read_data.write(chunk)
            self.assertEqual(read_data.getvalue(), data)
            os._exit(0)
        else:
            os.close(rd)
            written = 0
            while written < len(data):
                written += os.write(wr, memoryview(data)[written:])
            self.assertEqual(0, os.waitpid(pid, 0)[1])


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

        pid = os.fork()
        if pid == 0:
            rd.close()
            for data in datas:
                # let the parent block on recv()
                self._sleep()
                wr.sendall(data)
            os._exit(0)
        else:
            self.addCleanup(os.waitpid, pid, 0)
            wr.close()
            for data in datas:
                self.assertEqual(data, recv_func(rd, len(data)))

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

        pid = os.fork()
        if pid == 0:
            wr.close()
            # let the parent block on send()
            self._sleep()
            received_data = bytearray(len(data))
            n = 0
            while n < len(data):
                n += rd.recv_into(memoryview(received_data)[n:])
            self.assertEqual(received_data, data)
            os._exit(0)
        else:
            rd.close()
            written = 0
            while written < len(data):
                sent = send_func(wr, memoryview(data)[written:])
                # sendall() returns None
                written += len(data) if sent is None else sent
            self.assertEqual(0, os.waitpid(pid, 0)[1])

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
        _, port = sock.getsockname()
        sock.listen()

        pid = os.fork()
        if pid == 0:
            # let parent block on accept()
            self._sleep()
            with socket.create_connection((support.HOST, port)):
                self._sleep()
            os._exit(0)
        else:
            self.addCleanup(os.waitpid, pid, 0)
            client_sock, _ = sock.accept()
            client_sock.close()

    @unittest.skipUnless(hasattr(os, 'mkfifo'), 'needs mkfifo()')
    def _test_open(self, do_open_close_reader, do_open_close_writer):
        # Use a fifo: until the child opens it for reading, the parent will
        # block when trying to open it for writing.
        support.unlink(support.TESTFN)
        os.mkfifo(support.TESTFN)
        self.addCleanup(support.unlink, support.TESTFN)

        pid = os.fork()
        if pid == 0:
            # let the parent block
            self._sleep()
            do_open_close_reader(support.TESTFN)
            os._exit(0)
        else:
            self.addCleanup(os.waitpid, pid, 0)
            do_open_close_writer(support.TESTFN)

    def test_open(self):
        self._test_open(lambda path: open(path, 'r').close(),
                        lambda path: open(path, 'w').close())

    def test_os_open(self):
        self._test_open(lambda path: os.close(os.open(path, os.O_RDONLY)),
                        lambda path: os.close(os.open(path, os.O_WRONLY)))


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
class SignalEINTRTest(EINTRBaseTest):
    """ EINTR tests for the signal module. """

    @unittest.skipUnless(hasattr(signal, 'sigtimedwait'),
                         'need signal.sigtimedwait()')
    def test_sigtimedwait(self):
        t0 = time.monotonic()
        signal.sigtimedwait([signal.SIGUSR1], self.sleep_time)
        dt = time.monotonic() - t0
        self.assertGreaterEqual(dt, self.sleep_time)

    @unittest.skipUnless(hasattr(signal, 'sigwaitinfo'),
                         'need signal.sigwaitinfo()')
    def test_sigwaitinfo(self):
        signum = signal.SIGUSR1
        pid = os.getpid()

        old_handler = signal.signal(signum, lambda *args: None)
        self.addCleanup(signal.signal, signum, old_handler)

        t0 = time.monotonic()
        child_pid = os.fork()
        if child_pid == 0:
            # child
            try:
                self._sleep()
                os.kill(pid, signum)
            finally:
                os._exit(0)
        else:
            # parent
            signal.sigwaitinfo([signum])
            dt = time.monotonic() - t0
            os.waitpid(child_pid, 0)

        self.assertGreaterEqual(dt, self.sleep_time)


@unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
class SelectEINTRTest(EINTRBaseTest):
    """ EINTR tests for the select module. """

    def test_select(self):
        t0 = time.monotonic()
        select.select([], [], [], self.sleep_time)
        dt = time.monotonic() - t0
        self.stop_alarm()
        self.assertGreaterEqual(dt, self.sleep_time)

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

import errno
import os
import signal
import threading
import unittest
from test import support
from test.support import import_helper
from .utils import PyTestCase, CTestCase


requires_alarm = unittest.skipUnless(
    hasattr(signal, "alarm"), "test requires signal.alarm()"
)


@unittest.skipIf(os.name == 'nt', 'POSIX signals required for this test.')
class SignalsTest:

    def setUp(self):
        self.oldalrm = signal.signal(signal.SIGALRM, self.alarm_interrupt)

    def tearDown(self):
        signal.signal(signal.SIGALRM, self.oldalrm)

    def alarm_interrupt(self, sig, frame):
        1/0

    def check_interrupted_write(self, item, bytes, **fdopen_kwargs):
        """Check that a partial write, when it gets interrupted, properly
        invokes the signal handler, and bubbles up the exception raised
        in the latter."""

        # XXX This test has three flaws that appear when objects are
        # XXX not reference counted.

        # - if wio.write() happens to trigger a garbage collection,
        #   the signal exception may be raised when some __del__
        #   method is running; it will not reach the assertRaises()
        #   call.

        # - more subtle, if the wio object is not destroyed at once
        #   and survives this function, the next opened file is likely
        #   to have the same fileno (since the file descriptor was
        #   actively closed).  When wio.__del__ is finally called, it
        #   will close the other's test file...  To trigger this with
        #   CPython, try adding "global wio" in this function.

        # - This happens only for streams created by the _pyio module,
        #   because a wio.close() that fails still consider that the
        #   file needs to be closed again.  You can try adding an
        #   "assert wio.closed" at the end of the function.

        # Fortunately, a little gc.collect() seems to be enough to
        # work around all these issues.
        support.gc_collect()  # For PyPy or other GCs.

        read_results = []
        def _read():
            s = os.read(r, 1)
            read_results.append(s)

        t = threading.Thread(target=_read)
        t.daemon = True
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False
        large_data = item * (support.PIPE_MAX_SIZE // len(item) + 1)
        try:
            wio = self.io.open(w, **fdopen_kwargs)
            if hasattr(signal, 'pthread_sigmask'):
                # create the thread with SIGALRM signal blocked
                signal.pthread_sigmask(signal.SIG_BLOCK, [signal.SIGALRM])
                t.start()
                signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGALRM])
            else:
                t.start()

            # Fill the pipe enough that the write will be blocking.
            # It will be interrupted by the timer armed above.  Since the
            # other thread has read one byte, the low-level write will
            # return with a successful (partial) result rather than an EINTR.
            # The buffered IO layer must check for pending signal
            # handlers, which in this case will invoke alarm_interrupt().
            signal.alarm(1)
            try:
                self.assertRaises(ZeroDivisionError, wio.write, large_data)
            finally:
                signal.alarm(0)
                t.join()
            # We got one byte, get another one and check that it isn't a
            # repeat of the first one.
            read_results.append(os.read(r, 1))
            self.assertEqual(read_results, [bytes[0:1], bytes[1:2]])
        finally:
            os.close(w)
            os.close(r)
            # This is deliberate. If we didn't close the file descriptor
            # before closing wio, wio would try to flush its internal
            # buffer, and block again.
            try:
                wio.close()
            except OSError as e:
                if e.errno != errno.EBADF:
                    raise

    @requires_alarm
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_interrupted_write_unbuffered(self):
        self.check_interrupted_write(b"xy", b"xy", mode="wb", buffering=0)

    @requires_alarm
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_interrupted_write_buffered(self):
        self.check_interrupted_write(b"xy", b"xy", mode="wb")

    @requires_alarm
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_interrupted_write_text(self):
        self.check_interrupted_write("xy", b"xy", mode="w", encoding="ascii")

    @support.no_tracing
    def check_reentrant_write(self, data, **fdopen_kwargs):
        def on_alarm(*args):
            # Will be called reentrantly from the same thread
            wio.write(data)
            1/0
        signal.signal(signal.SIGALRM, on_alarm)
        r, w = os.pipe()
        wio = self.io.open(w, **fdopen_kwargs)
        try:
            signal.alarm(1)
            # Either the reentrant call to wio.write() fails with RuntimeError,
            # or the signal handler raises ZeroDivisionError.
            with self.assertRaises((ZeroDivisionError, RuntimeError)) as cm:
                while 1:
                    for i in range(100):
                        wio.write(data)
                        wio.flush()
                    # Make sure the buffer doesn't fill up and block further writes
                    os.read(r, len(data) * 100)
            exc = cm.exception
            if isinstance(exc, RuntimeError):
                self.assertStartsWith(str(exc), "reentrant call")
        finally:
            signal.alarm(0)
            wio.close()
            os.close(r)

    @requires_alarm
    def test_reentrant_write_buffered(self):
        self.check_reentrant_write(b"xy", mode="wb")

    @requires_alarm
    def test_reentrant_write_text(self):
        self.check_reentrant_write("xy", mode="w", encoding="ascii")

    def check_interrupted_read_retry(self, decode, **fdopen_kwargs):
        """Check that a buffered read, when it gets interrupted (either
        returning a partial result or EINTR), properly invokes the signal
        handler and retries if the latter returned successfully."""
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False
        def alarm_handler(sig, frame):
            os.write(w, b"bar")
        signal.signal(signal.SIGALRM, alarm_handler)
        try:
            rio = self.io.open(r, **fdopen_kwargs)
            os.write(w, b"foo")
            signal.alarm(1)
            # Expected behaviour:
            # - first raw read() returns partial b"foo"
            # - second raw read() returns EINTR
            # - third raw read() returns b"bar"
            self.assertEqual(decode(rio.read(6)), "foobar")
        finally:
            signal.alarm(0)
            rio.close()
            os.close(w)
            os.close(r)

    @requires_alarm
    @support.requires_resource('walltime')
    def test_interrupted_read_retry_buffered(self):
        self.check_interrupted_read_retry(lambda x: x.decode('latin1'),
                                          mode="rb")

    @requires_alarm
    @support.requires_resource('walltime')
    def test_interrupted_read_retry_text(self):
        self.check_interrupted_read_retry(lambda x: x,
                                          mode="r", encoding="latin1")

    def check_interrupted_write_retry(self, item, **fdopen_kwargs):
        """Check that a buffered write, when it gets interrupted (either
        returning a partial result or EINTR), properly invokes the signal
        handler and retries if the latter returned successfully."""
        select = import_helper.import_module("select")

        # A quantity that exceeds the buffer size of an anonymous pipe's
        # write end.
        N = support.PIPE_MAX_SIZE
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False

        # We need a separate thread to read from the pipe and allow the
        # write() to finish.  This thread is started after the SIGALRM is
        # received (forcing a first EINTR in write()).
        read_results = []
        write_finished = False
        error = None
        def _read():
            try:
                while not write_finished:
                    while r in select.select([r], [], [], 1.0)[0]:
                        s = os.read(r, 1024)
                        read_results.append(s)
            except BaseException as exc:
                nonlocal error
                error = exc
        t = threading.Thread(target=_read)
        t.daemon = True
        def alarm1(sig, frame):
            signal.signal(signal.SIGALRM, alarm2)
            signal.alarm(1)
        def alarm2(sig, frame):
            t.start()

        large_data = item * N
        signal.signal(signal.SIGALRM, alarm1)
        try:
            wio = self.io.open(w, **fdopen_kwargs)
            signal.alarm(1)
            # Expected behaviour:
            # - first raw write() is partial (because of the limited pipe buffer
            #   and the first alarm)
            # - second raw write() returns EINTR (because of the second alarm)
            # - subsequent write()s are successful (either partial or complete)
            written = wio.write(large_data)
            self.assertEqual(N, written)

            wio.flush()
            write_finished = True
            t.join()

            self.assertIsNone(error)
            self.assertEqual(N, sum(len(x) for x in read_results))
        finally:
            signal.alarm(0)
            write_finished = True
            os.close(w)
            os.close(r)
            # This is deliberate. If we didn't close the file descriptor
            # before closing wio, wio would try to flush its internal
            # buffer, and could block (in case of failure).
            try:
                wio.close()
            except OSError as e:
                if e.errno != errno.EBADF:
                    raise

    @requires_alarm
    @support.requires_resource('walltime')
    def test_interrupted_write_retry_buffered(self):
        self.check_interrupted_write_retry(b"x", mode="wb")

    @requires_alarm
    @support.requires_resource('walltime')
    def test_interrupted_write_retry_text(self):
        self.check_interrupted_write_retry("x", mode="w", encoding="latin1")


class CSignalsTest(SignalsTest, CTestCase):
    pass

class PySignalsTest(SignalsTest, PyTestCase):
    pass

    # Handling reentrancy issues would slow down _pyio even more, so the
    # tests are disabled.
    test_reentrant_write_buffered = None
    test_reentrant_write_text = None

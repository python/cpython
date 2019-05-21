# Test case for the os.poll() function

import os
import subprocess
import random
import select
import threading
import time
import unittest
from test.support import TESTFN, run_unittest, reap_threads, cpython_only

try:
    select.poll
except AttributeError:
    raise unittest.SkipTest("select.poll not defined")


def find_ready_matching(ready, flag):
    match = []
    for fd, mode in ready:
        if mode & flag:
            match.append(fd)
    return match

class PollTests(unittest.TestCase):

    def test_poll1(self):
        # Basic functional test of poll object
        # Create a bunch of pipe and test that poll works with them.

        p = select.poll()

        NUM_PIPES = 12
        MSG = b" This is a test."
        MSG_LEN = len(MSG)
        readers = []
        writers = []
        r2w = {}
        w2r = {}

        for i in range(NUM_PIPES):
            rd, wr = os.pipe()
            p.register(rd)
            p.modify(rd, select.POLLIN)
            p.register(wr, select.POLLOUT)
            readers.append(rd)
            writers.append(wr)
            r2w[rd] = wr
            w2r[wr] = rd

        bufs = []

        while writers:
            ready = p.poll()
            ready_writers = find_ready_matching(ready, select.POLLOUT)
            if not ready_writers:
                raise RuntimeError("no pipes ready for writing")
            wr = random.choice(ready_writers)
            os.write(wr, MSG)

            ready = p.poll()
            ready_readers = find_ready_matching(ready, select.POLLIN)
            if not ready_readers:
                raise RuntimeError("no pipes ready for reading")
            rd = random.choice(ready_readers)
            buf = os.read(rd, MSG_LEN)
            self.assertEqual(len(buf), MSG_LEN)
            bufs.append(buf)
            os.close(r2w[rd]) ; os.close( rd )
            p.unregister( r2w[rd] )
            p.unregister( rd )
            writers.remove(r2w[rd])

        self.assertEqual(bufs, [MSG] * NUM_PIPES)

    def test_poll_unit_tests(self):
        # returns NVAL for invalid file descriptor
        FD, w = os.pipe()
        os.close(FD)
        os.close(w)
        p = select.poll()
        p.register(FD)
        r = p.poll()
        self.assertEqual(r[0], (FD, select.POLLNVAL))

        with open(TESTFN, 'w') as f:
            fd = f.fileno()
            p = select.poll()
            p.register(f)
            r = p.poll()
            self.assertEqual(r[0][0], fd)
        r = p.poll()
        self.assertEqual(r[0], (fd, select.POLLNVAL))
        os.unlink(TESTFN)

        # type error for invalid arguments
        p = select.poll()
        self.assertRaises(TypeError, p.register, p)
        self.assertRaises(TypeError, p.unregister, p)

        # can't unregister non-existent object
        p = select.poll()
        self.assertRaises(KeyError, p.unregister, 3)

        # Test error cases
        pollster = select.poll()
        class Nope:
            pass

        class Almost:
            def fileno(self):
                return 'fileno'

        self.assertRaises(TypeError, pollster.register, Nope(), 0)
        self.assertRaises(TypeError, pollster.register, Almost(), 0)

    # Another test case for poll().  This is copied from the test case for
    # select(), modified to use poll() instead.

    def test_poll2(self):
        cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do echo testing...; sleep 1; done'
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                bufsize=0)
        proc.__enter__()
        self.addCleanup(proc.__exit__, None, None, None)
        p = proc.stdout
        pollster = select.poll()
        pollster.register( p, select.POLLIN )
        for tout in (0, 1000, 2000, 4000, 8000, 16000) + (-1,)*10:
            fdlist = pollster.poll(tout)
            if (fdlist == []):
                continue
            fd, flags = fdlist[0]
            if flags & select.POLLHUP:
                line = p.readline()
                if line != b"":
                    self.fail('error: pipe seems to be closed, but still returns data')
                continue

            elif flags & select.POLLIN:
                line = p.readline()
                if not line:
                    break
                self.assertEqual(line, b'testing...\n')
                continue
            else:
                self.fail('Unexpected return value from select.poll: %s' % fdlist)

    def test_poll3(self):
        # test int overflow
        pollster = select.poll()
        pollster.register(1)

        self.assertRaises(OverflowError, pollster.poll, 1 << 64)

        x = 2 + 3
        if x != 5:
            self.fail('Overflow must have occurred')

        # Issues #15989, #17919
        self.assertRaises(ValueError, pollster.register, 0, -1)
        self.assertRaises(OverflowError, pollster.register, 0, 1 << 64)
        self.assertRaises(ValueError, pollster.modify, 1, -1)
        self.assertRaises(OverflowError, pollster.modify, 1, 1 << 64)

    @cpython_only
    def test_poll_c_limits(self):
        from _testcapi import USHRT_MAX, INT_MAX, UINT_MAX
        pollster = select.poll()
        pollster.register(1)

        # Issues #15989, #17919
        self.assertRaises(OverflowError, pollster.register, 0, USHRT_MAX + 1)
        self.assertRaises(OverflowError, pollster.modify, 1, USHRT_MAX + 1)
        self.assertRaises(OverflowError, pollster.poll, INT_MAX + 1)
        self.assertRaises(OverflowError, pollster.poll, UINT_MAX + 1)

    @reap_threads
    def test_threaded_poll(self):
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        rfds = []
        for i in range(10):
            fd = os.dup(r)
            self.addCleanup(os.close, fd)
            rfds.append(fd)
        pollster = select.poll()
        for fd in rfds:
            pollster.register(fd, select.POLLIN)

        t = threading.Thread(target=pollster.poll)
        t.start()
        try:
            time.sleep(0.5)
            # trigger ufds array reallocation
            for fd in rfds:
                pollster.unregister(fd)
            pollster.register(w, select.POLLOUT)
            self.assertRaises(RuntimeError, pollster.poll)
        finally:
            # and make the call to poll() from the thread return
            os.write(w, b'spam')
            t.join()

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @reap_threads
    def test_poll_blocks_with_negative_ms(self):
        for timeout_ms in [None, -1000, -1, -1.0, -0.1, -1e-100]:
            # Create two file descriptors. This will be used to unlock
            # the blocking call to poll.poll inside the thread
            r, w = os.pipe()
            pollster = select.poll()
            pollster.register(r, select.POLLIN)

            poll_thread = threading.Thread(target=pollster.poll, args=(timeout_ms,))
            poll_thread.start()
            poll_thread.join(timeout=0.1)
            self.assertTrue(poll_thread.is_alive())

            # Write to the pipe so pollster.poll unblocks and the thread ends.
            os.write(w, b'spam')
            poll_thread.join()
            self.assertFalse(poll_thread.is_alive())
            os.close(r)
            os.close(w)


def test_main():
    run_unittest(PollTests)

if __name__ == '__main__':
    test_main()

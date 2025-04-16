import threading
from unittest import TestCase
from test.support import threading_helper
from random import randint
from io import BytesIO
from sys import getsizeof


class TestBytesIO(TestCase):
    # Test pretty much everything that can break under free-threading.
    # Non-deterministic, but at least one of these things will fail if
    # BytesIO object is not free-thread safe.

    def check(self, funcs, *args):
        barrier = threading.Barrier(len(funcs))
        threads = []

        for func in funcs:
            thread = threading.Thread(target=func, args=(barrier, *args))

            threads.append(thread)

        with threading_helper.start_threads(threads):
            pass

    @threading_helper.requires_working_threading()
    @threading_helper.reap_threads
    def test_free_threading(self):
        """Test for segfaults and aborts."""

        def write(barrier, b, *ignore):
            barrier.wait()
            try: b.write(b'0' * randint(100, 1000))
            except ValueError: pass  # ignore write fail to closed file

        def writelines(barrier, b, *ignore):
            barrier.wait()
            b.write(b'0\n' * randint(100, 1000))

        def truncate(barrier, b, *ignore):
            barrier.wait()
            try: b.truncate(0)
            except: BufferError  # ignore exported buffer

        def read(barrier, b, *ignore):
            barrier.wait()
            b.read()

        def read1(barrier, b, *ignore):
            barrier.wait()
            b.read1()

        def readline(barrier, b, *ignore):
            barrier.wait()
            b.readline()

        def readlines(barrier, b, *ignore):
            barrier.wait()
            b.readlines()

        def readinto(barrier, b, into, *ignore):
            barrier.wait()
            b.readinto(into)

        def close(barrier, b, *ignore):
            barrier.wait()
            b.close()

        def getvalue(barrier, b, *ignore):
            barrier.wait()
            b.getvalue()

        def getbuffer(barrier, b, *ignore):
            barrier.wait()
            b.getbuffer()

        def iter(barrier, b, *ignore):
            barrier.wait()
            list(b)

        def getstate(barrier, b, *ignore):
            barrier.wait()
            b.__getstate__()

        def setstate(barrier, b, st, *ignore):
            barrier.wait()
            b.__setstate__(st)

        def sizeof(barrier, b, *ignore):
            barrier.wait()
            getsizeof(b)

        self.check([write] * 10, BytesIO())
        self.check([writelines] * 10, BytesIO())
        self.check([write] * 10 + [truncate] * 10, BytesIO())
        self.check([truncate] + [read] * 10, BytesIO(b'0\n'*204800))
        self.check([truncate] + [read1] * 10, BytesIO(b'0\n'*204800))
        self.check([truncate] + [readline] * 10, BytesIO(b'0\n'*20480))
        self.check([truncate] + [readlines] * 10, BytesIO(b'0\n'*20480))
        self.check([truncate] + [readinto] * 10, BytesIO(b'0\n'*204800), bytearray(b'0\n'*204800))
        self.check([close] + [write] * 10, BytesIO())
        self.check([truncate] + [getvalue] * 10, BytesIO(b'0\n'*204800))
        self.check([truncate] + [getbuffer] * 10, BytesIO(b'0\n'*204800))
        self.check([truncate] + [iter] * 10, BytesIO(b'0\n'*20480))
        self.check([truncate] + [getstate] * 10, BytesIO(b'0\n'*204800))
        self.check([truncate] + [setstate] * 10, BytesIO(b'0\n'*204800), (b'123', 0, None))
        self.check([truncate] + [sizeof] * 10, BytesIO(b'0\n'*204800))

        # no tests for seek or tell because they don't break anything

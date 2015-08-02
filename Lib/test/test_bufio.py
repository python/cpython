import unittest
from test import test_support as support

import io # C implementation.
import _pyio as pyio # Python implementation.

# Simple test to ensure that optimizations in the IO library deliver the
# expected results.  For best testing, run this under a debug-build Python too
# (to exercise asserts in the C code).

lengths = list(range(1, 257)) + [512, 1000, 1024, 2048, 4096, 8192, 10000,
                                 16384, 32768, 65536, 1000000]

class BufferSizeTest(unittest.TestCase):
    def try_one(self, s):
        # Write s + "\n" + s to file, then open it and ensure that successive
        # .readline()s deliver what we wrote.

        # Ensure we can open TESTFN for writing.
        support.unlink(support.TESTFN)

        # Since C doesn't guarantee we can write/read arbitrary bytes in text
        # files, use binary mode.
        f = self.open(support.TESTFN, "wb")
        try:
            # write once with \n and once without
            f.write(s)
            f.write(b"\n")
            f.write(s)
            f.close()
            f = open(support.TESTFN, "rb")
            line = f.readline()
            self.assertEqual(line, s + b"\n")
            line = f.readline()
            self.assertEqual(line, s)
            line = f.readline()
            self.assertFalse(line) # Must be at EOF
            f.close()
        finally:
            support.unlink(support.TESTFN)

    def drive_one(self, pattern):
        for length in lengths:
            # Repeat string 'pattern' as often as needed to reach total length
            # 'length'.  Then call try_one with that string, a string one larger
            # than that, and a string one smaller than that.  Try this with all
            # small sizes and various powers of 2, so we exercise all likely
            # stdio buffer sizes, and "off by one" errors on both sides.
            q, r = divmod(length, len(pattern))
            teststring = pattern * q + pattern[:r]
            self.assertEqual(len(teststring), length)
            self.try_one(teststring)
            self.try_one(teststring + b"x")
            self.try_one(teststring[:-1])

    def test_primepat(self):
        # A pattern with prime length, to avoid simple relationships with
        # stdio buffer sizes.
        self.drive_one(b"1234567890\00\01\02\03\04\05\06")

    def test_nullpat(self):
        self.drive_one(bytes(1000))


class CBufferSizeTest(BufferSizeTest):
    open = io.open

class PyBufferSizeTest(BufferSizeTest):
    open = staticmethod(pyio.open)

class BuiltinBufferSizeTest(BufferSizeTest):
    open = open


def test_main():
    support.run_unittest(CBufferSizeTest, PyBufferSizeTest, BuiltinBufferSizeTest)

if __name__ == "__main__":
    test_main()

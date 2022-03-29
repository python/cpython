import unittest
from test import support
from test.support import os_helper

import io # C implementation.
import _pyio as pyio # Python implementation.

# Simple test to ensure that optimizations in the IO library deliver the
# expected results.  For best testing, run this under a debug-build Python too
# (to exercise asserts in the C code).

lengths = list(range(1, 257)) + [512, 1000, 1024, 2048, 4096, 8192, 10000,
                                 16384, 32768, 65536, 1000000]

class BufferSizeTest:
    def try_one(self, s):
        # Write s + "\n" + s to file, then open it and ensure that successive
        # .readline()s deliver what we wrote.

        # Ensure we can open TESTFN for writing.
        os_helper.unlink(os_helper.TESTFN)

        # Since C doesn't guarantee we can write/read arbitrary bytes in text
        # files, use binary mode.
        f = self.open(os_helper.TESTFN, "wb")
        try:
            # write once with \n and once without
            f.write(s)
            f.write(b"\n")
            f.write(s)
            f.close()
            f = open(os_helper.TESTFN, "rb")
            line = f.readline()
            self.assertEqual(line, s + b"\n")
            line = f.readline()
            self.assertEqual(line, s)
            line = f.readline()
            self.assertFalse(line) # Must be at EOF
            f.close()
        finally:
            os_helper.unlink(os_helper.TESTFN)

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
        self.drive_one(b'\0' * 1000)


class CBufferSizeTest(BufferSizeTest, unittest.TestCase):
    open = io.open

class PyBufferSizeTest(BufferSizeTest, unittest.TestCase):
    open = staticmethod(pyio.open)


if __name__ == "__main__":
    unittest.main()

"""Unit tests for io.py."""

import unittest
from test import test_support

import io


class MockReadIO(io.RawIOBase):
    def __init__(self, readStack):
        self._readStack = list(readStack)

    def read(self, n=None):
        try:
            return self._readStack.pop(0)
        except:
            return io.EOF

    def fileno(self):
        return 42

    def readable(self):
        return True


class MockWriteIO(io.RawIOBase):
    def __init__(self):
        self._writeStack = []

    def write(self, b):
        self._writeStack.append(b)
        return len(b)

    def writeable(self):
        return True

    def fileno(self):
        return 42


class IOTest(unittest.TestCase):

    def write_ops(self, f):
        f.write(b"blah.")
        f.seek(0)
        f.write(b"Hello.")
        self.assertEqual(f.tell(), 6)
        f.seek(-1, 1)
        self.assertEqual(f.tell(), 5)
        f.write(" world\n\n\n")
        f.seek(0)
        f.write("h")
        f.seek(-2, 2)
        f.truncate()

    def read_ops(self, f):
        data = f.read(5)
        self.assertEqual(data, b"hello")
        f.readinto(data)
        self.assertEqual(data, b" worl")
        f.readinto(data)
        self.assertEqual(data, b"d\n")
        f.seek(0)
        self.assertEqual(f.read(20), b"hello world\n")
        f.seek(-6, 2)
        self.assertEqual(f.read(5), b"world")
        f.seek(-6, 1)
        self.assertEqual(f.read(5), b" worl")
        self.assertEqual(f.tell(), 10)

    def test_raw_file_io(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        self.assertEqual(f.readable(), False)
        self.assertEqual(f.writable(), True)
        self.assertEqual(f.seekable(), True)
        self.write_ops(f)
        f.close()
        f = io.open(test_support.TESTFN, "rb", buffering=0)
        self.assertEqual(f.readable(), True)
        self.assertEqual(f.writable(), False)
        self.assertEqual(f.seekable(), True)
        self.read_ops(f)
        f.close()

    def test_raw_bytes_io(self):
        f = io.BytesIO()
        self.write_ops(f)
        data = f.getvalue()
        self.assertEqual(data, b"hello world\n")
        f = io.BytesIO(data)
        self.read_ops(f)


class BytesIOTest(unittest.TestCase):

    def testInit(self):
        buf = b"1234567890"
        bytesIo = io.BytesIO(buf)

    def testRead(self):
        buf = b"1234567890"
        bytesIo = io.BytesIO(buf)

        self.assertEquals(buf[:1], bytesIo.read(1))
        self.assertEquals(buf[1:5], bytesIo.read(4))
        self.assertEquals(buf[5:], bytesIo.read(900))
        self.assertEquals(io.EOF, bytesIo.read())

    def testReadNoArgs(self):
        buf = b"1234567890"
        bytesIo = io.BytesIO(buf)

        self.assertEquals(buf, bytesIo.read())
        self.assertEquals(io.EOF, bytesIo.read())

    def testSeek(self):
        buf = b"1234567890"
        bytesIo = io.BytesIO(buf)

        bytesIo.read(5)
        bytesIo.seek(0)
        self.assertEquals(buf, bytesIo.read())

        bytesIo.seek(3)
        self.assertEquals(buf[3:], bytesIo.read())

    def testTell(self):
        buf = b"1234567890"
        bytesIo = io.BytesIO(buf)

        self.assertEquals(0, bytesIo.tell())
        bytesIo.seek(5)
        self.assertEquals(5, bytesIo.tell())
        bytesIo.seek(10000)
        self.assertEquals(10000, bytesIo.tell())


class BufferedReaderTest(unittest.TestCase):

    def testRead(self):
        rawIo = MockReadIO((b"abc", b"d", b"efg"))
        bufIo = io.BufferedReader(rawIo)

        self.assertEquals(b"abcdef", bufIo.read(6))

    def testReadToEof(self):
        rawIo = MockReadIO((b"abc", b"d", b"efg"))
        bufIo = io.BufferedReader(rawIo)

        self.assertEquals(b"abcdefg", bufIo.read(9000))

    def testReadNoArgs(self):
        rawIo = MockReadIO((b"abc", b"d", b"efg"))
        bufIo = io.BufferedReader(rawIo)

        self.assertEquals(b"abcdefg", bufIo.read())

    def testFileno(self):
        rawIo = MockReadIO((b"abc", b"d", b"efg"))
        bufIo = io.BufferedReader(rawIo)

        self.assertEquals(42, bufIo.fileno())

    def testFilenoNoFileno(self):
        # TODO will we always have fileno() function? If so, kill
        # this test. Else, write it.
        pass


class BufferedWriterTest(unittest.TestCase):

    def testWrite(self):
        # Write to the buffered IO but don't overflow the buffer.
        writer = MockWriteIO()
        bufIo = io.BufferedWriter(writer, 8)

        bufIo.write(b"abc")

        self.assertFalse(writer._writeStack)

    def testWriteOverflow(self):
        writer = MockWriteIO()
        bufIo = io.BufferedWriter(writer, 8)

        bufIo.write(b"abc")
        bufIo.write(b"defghijkl")

        self.assertEquals(b"abcdefghijkl", writer._writeStack[0])

    def testFlush(self):
        writer = MockWriteIO()
        bufIo = io.BufferedWriter(writer, 8)

        bufIo.write(b"abc")
        bufIo.flush()

        self.assertEquals(b"abc", writer._writeStack[0])

# TODO. Tests for open()

def test_main():
    test_support.run_unittest(IOTest, BytesIOTest, BufferedReaderTest,
                              BufferedWriterTest)

if __name__ == "__main__":
    test_main()

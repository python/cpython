"""Unit tests for memory-based file-like objects.
StringIO -- for unicode strings
BytesIO -- for bytes
"""

import unittest
from test import support

import io
import _pyio as pyio
import sys

class MemorySeekTestMixin:

    def testInit(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

    def testRead(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEqual(buf[:1], bytesIo.read(1))
        self.assertEqual(buf[1:5], bytesIo.read(4))
        self.assertEqual(buf[5:], bytesIo.read(900))
        self.assertEqual(self.EOF, bytesIo.read())

    def testReadNoArgs(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEqual(buf, bytesIo.read())
        self.assertEqual(self.EOF, bytesIo.read())

    def testSeek(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        bytesIo.read(5)
        bytesIo.seek(0)
        self.assertEqual(buf, bytesIo.read())

        bytesIo.seek(3)
        self.assertEqual(buf[3:], bytesIo.read())
        self.assertRaises(TypeError, bytesIo.seek, 0.0)

    def testTell(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEqual(0, bytesIo.tell())
        bytesIo.seek(5)
        self.assertEqual(5, bytesIo.tell())
        bytesIo.seek(10000)
        self.assertEqual(10000, bytesIo.tell())


class MemoryTestMixin:

    def test_detach(self):
        buf = self.ioclass()
        self.assertRaises(self.UnsupportedOperation, buf.detach)

    def write_ops(self, f, t):
        self.assertEqual(f.write(t("blah.")), 5)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(t("Hello.")), 6)
        self.assertEqual(f.tell(), 6)
        self.assertEqual(f.seek(5), 5)
        self.assertEqual(f.tell(), 5)
        self.assertEqual(f.write(t(" world\n\n\n")), 9)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(t("h")), 1)
        self.assertEqual(f.truncate(12), 12)
        self.assertEqual(f.tell(), 1)

    def test_write(self):
        buf = self.buftype("hello world\n")
        memio = self.ioclass(buf)

        self.write_ops(memio, self.buftype)
        self.assertEqual(memio.getvalue(), buf)
        memio = self.ioclass()
        self.write_ops(memio, self.buftype)
        self.assertEqual(memio.getvalue(), buf)
        self.assertRaises(TypeError, memio.write, None)
        memio.close()
        self.assertRaises(ValueError, memio.write, self.buftype(""))

    def test_writelines(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass()

        self.assertEqual(memio.writelines([buf] * 100), None)
        self.assertEqual(memio.getvalue(), buf * 100)
        memio.writelines([])
        self.assertEqual(memio.getvalue(), buf * 100)
        memio = self.ioclass()
        self.assertRaises(TypeError, memio.writelines, [buf] + [1])
        self.assertEqual(memio.getvalue(), buf)
        self.assertRaises(TypeError, memio.writelines, None)
        memio.close()
        self.assertRaises(ValueError, memio.writelines, [])

    def test_writelines_error(self):
        memio = self.ioclass()
        def error_gen():
            yield self.buftype('spam')
            raise KeyboardInterrupt

        self.assertRaises(KeyboardInterrupt, memio.writelines, error_gen())

    def test_truncate(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertRaises(ValueError, memio.truncate, -1)
        memio.seek(6)
        self.assertEqual(memio.truncate(), 6)
        self.assertEqual(memio.getvalue(), buf[:6])
        self.assertEqual(memio.truncate(4), 4)
        self.assertEqual(memio.getvalue(), buf[:4])
        self.assertEqual(memio.tell(), 6)
        memio.seek(0, 2)
        memio.write(buf)
        self.assertEqual(memio.getvalue(), buf[:4] + buf)
        pos = memio.tell()
        self.assertEqual(memio.truncate(None), pos)
        self.assertEqual(memio.tell(), pos)
        self.assertRaises(TypeError, memio.truncate, '0')
        memio.close()
        self.assertRaises(ValueError, memio.truncate, 0)

    def test_init(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)
        self.assertEqual(memio.getvalue(), buf)
        memio = self.ioclass(None)
        self.assertEqual(memio.getvalue(), self.EOF)
        memio.__init__(buf * 2)
        self.assertEqual(memio.getvalue(), buf * 2)
        memio.__init__(buf)
        self.assertEqual(memio.getvalue(), buf)
        self.assertRaises(TypeError, memio.__init__, [])

    def test_read(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.read(0), self.EOF)
        self.assertEqual(memio.read(1), buf[:1])
        self.assertEqual(memio.read(4), buf[1:5])
        self.assertEqual(memio.read(900), buf[5:])
        self.assertEqual(memio.read(), self.EOF)
        memio.seek(0)
        self.assertEqual(memio.read(), buf)
        self.assertEqual(memio.read(), self.EOF)
        self.assertEqual(memio.tell(), 10)
        memio.seek(0)
        self.assertEqual(memio.read(-1), buf)
        memio.seek(0)
        self.assertEqual(type(memio.read()), type(buf))
        memio.seek(100)
        self.assertEqual(type(memio.read()), type(buf))
        memio.seek(0)
        self.assertEqual(memio.read(None), buf)
        self.assertRaises(TypeError, memio.read, '')
        memio.close()
        self.assertRaises(ValueError, memio.read)

    def test_readline(self):
        buf = self.buftype("1234567890\n")
        memio = self.ioclass(buf * 2)

        self.assertEqual(memio.readline(0), self.EOF)
        self.assertEqual(memio.readline(), buf)
        self.assertEqual(memio.readline(), buf)
        self.assertEqual(memio.readline(), self.EOF)
        memio.seek(0)
        self.assertEqual(memio.readline(5), buf[:5])
        self.assertEqual(memio.readline(5), buf[5:10])
        self.assertEqual(memio.readline(5), buf[10:15])
        memio.seek(0)
        self.assertEqual(memio.readline(-1), buf)
        memio.seek(0)
        self.assertEqual(memio.readline(0), self.EOF)

        buf = self.buftype("1234567890\n")
        memio = self.ioclass((buf * 3)[:-1])
        self.assertEqual(memio.readline(), buf)
        self.assertEqual(memio.readline(), buf)
        self.assertEqual(memio.readline(), buf[:-1])
        self.assertEqual(memio.readline(), self.EOF)
        memio.seek(0)
        self.assertEqual(type(memio.readline()), type(buf))
        self.assertEqual(memio.readline(), buf)
        self.assertRaises(TypeError, memio.readline, '')
        memio.close()
        self.assertRaises(ValueError,  memio.readline)

    def test_readlines(self):
        buf = self.buftype("1234567890\n")
        memio = self.ioclass(buf * 10)

        self.assertEqual(memio.readlines(), [buf] * 10)
        memio.seek(5)
        self.assertEqual(memio.readlines(), [buf[5:]] + [buf] * 9)
        memio.seek(0)
        self.assertEqual(memio.readlines(15), [buf] * 2)
        memio.seek(0)
        self.assertEqual(memio.readlines(-1), [buf] * 10)
        memio.seek(0)
        self.assertEqual(memio.readlines(0), [buf] * 10)
        memio.seek(0)
        self.assertEqual(type(memio.readlines()[0]), type(buf))
        memio.seek(0)
        self.assertEqual(memio.readlines(None), [buf] * 10)
        self.assertRaises(TypeError, memio.readlines, '')
        memio.close()
        self.assertRaises(ValueError, memio.readlines)

    def test_iterator(self):
        buf = self.buftype("1234567890\n")
        memio = self.ioclass(buf * 10)

        self.assertEqual(iter(memio), memio)
        self.assertTrue(hasattr(memio, '__iter__'))
        self.assertTrue(hasattr(memio, '__next__'))
        i = 0
        for line in memio:
            self.assertEqual(line, buf)
            i += 1
        self.assertEqual(i, 10)
        memio.seek(0)
        i = 0
        for line in memio:
            self.assertEqual(line, buf)
            i += 1
        self.assertEqual(i, 10)
        memio = self.ioclass(buf * 2)
        memio.close()
        self.assertRaises(ValueError, memio.__next__)

    def test_getvalue(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.getvalue(), buf)
        memio.read()
        self.assertEqual(memio.getvalue(), buf)
        self.assertEqual(type(memio.getvalue()), type(buf))
        memio = self.ioclass(buf * 1000)
        self.assertEqual(memio.getvalue()[-3:], self.buftype("890"))
        memio = self.ioclass(buf)
        memio.close()
        self.assertRaises(ValueError, memio.getvalue)

    def test_seek(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        memio.read(5)
        self.assertRaises(ValueError, memio.seek, -1)
        self.assertRaises(ValueError, memio.seek, 1, -1)
        self.assertRaises(ValueError, memio.seek, 1, 3)
        self.assertEqual(memio.seek(0), 0)
        self.assertEqual(memio.seek(0, 0), 0)
        self.assertEqual(memio.read(), buf)
        self.assertEqual(memio.seek(3), 3)
        self.assertEqual(memio.seek(0, 1), 3)
        self.assertEqual(memio.read(), buf[3:])
        self.assertEqual(memio.seek(len(buf)), len(buf))
        self.assertEqual(memio.read(), self.EOF)
        memio.seek(len(buf) + 1)
        self.assertEqual(memio.read(), self.EOF)
        self.assertEqual(memio.seek(0, 2), len(buf))
        self.assertEqual(memio.read(), self.EOF)
        memio.close()
        self.assertRaises(ValueError, memio.seek, 0)

    def test_overseek(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.seek(len(buf) + 1), 11)
        self.assertEqual(memio.read(), self.EOF)
        self.assertEqual(memio.tell(), 11)
        self.assertEqual(memio.getvalue(), buf)
        memio.write(self.EOF)
        self.assertEqual(memio.getvalue(), buf)
        memio.write(buf)
        self.assertEqual(memio.getvalue(), buf + self.buftype('\0') + buf)

    def test_tell(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.tell(), 0)
        memio.seek(5)
        self.assertEqual(memio.tell(), 5)
        memio.seek(10000)
        self.assertEqual(memio.tell(), 10000)
        memio.close()
        self.assertRaises(ValueError, memio.tell)

    def test_flush(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.flush(), None)

    def test_flags(self):
        memio = self.ioclass()

        self.assertEqual(memio.writable(), True)
        self.assertEqual(memio.readable(), True)
        self.assertEqual(memio.seekable(), True)
        self.assertEqual(memio.isatty(), False)
        self.assertEqual(memio.closed, False)
        memio.close()
        self.assertEqual(memio.writable(), True)
        self.assertEqual(memio.readable(), True)
        self.assertEqual(memio.seekable(), True)
        self.assertRaises(ValueError, memio.isatty)
        self.assertEqual(memio.closed, True)

    def test_subclassing(self):
        buf = self.buftype("1234567890")
        def test1():
            class MemIO(self.ioclass):
                pass
            m = MemIO(buf)
            return m.getvalue()
        def test2():
            class MemIO(self.ioclass):
                def __init__(me, a, b):
                    self.ioclass.__init__(me, a)
            m = MemIO(buf, None)
            return m.getvalue()
        self.assertEqual(test1(), buf)
        self.assertEqual(test2(), buf)

    def test_instance_dict_leak(self):
        # Test case for issue #6242.
        # This will be caught by regrtest.py -R if this leak.
        for _ in range(100):
            memio = self.ioclass()
            memio.foo = 1


class PyBytesIOTest(MemoryTestMixin, MemorySeekTestMixin, unittest.TestCase):

    UnsupportedOperation = pyio.UnsupportedOperation

    @staticmethod
    def buftype(s):
        return s.encode("ascii")
    ioclass = pyio.BytesIO
    EOF = b""

    def test_read1(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertRaises(TypeError, memio.read1)
        self.assertEqual(memio.read(), buf)

    def test_readinto(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        b = bytearray(b"hello")
        self.assertEqual(memio.readinto(b), 5)
        self.assertEqual(b, b"12345")
        self.assertEqual(memio.readinto(b), 5)
        self.assertEqual(b, b"67890")
        self.assertEqual(memio.readinto(b), 0)
        self.assertEqual(b, b"67890")
        b = bytearray(b"hello world")
        memio.seek(0)
        self.assertEqual(memio.readinto(b), 10)
        self.assertEqual(b, b"1234567890d")
        b = bytearray(b"")
        memio.seek(0)
        self.assertEqual(memio.readinto(b), 0)
        self.assertEqual(b, b"")
        self.assertRaises(TypeError, memio.readinto, '')
        import array
        a = array.array('b', b"hello world")
        memio = self.ioclass(buf)
        memio.readinto(a)
        self.assertEqual(a.tostring(), b"1234567890d")
        memio.close()
        self.assertRaises(ValueError, memio.readinto, b)
        memio = self.ioclass(b"123")
        b = bytearray()
        memio.seek(42)
        memio.readinto(b)
        self.assertEqual(b, b"")

    def test_relative_seek(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.seek(-1, 1), 0)
        self.assertEqual(memio.seek(3, 1), 3)
        self.assertEqual(memio.seek(-4, 1), 0)
        self.assertEqual(memio.seek(-1, 2), 9)
        self.assertEqual(memio.seek(1, 1), 10)
        self.assertEqual(memio.seek(1, 2), 11)
        memio.seek(-3, 2)
        self.assertEqual(memio.read(), buf[-3:])
        memio.seek(0)
        memio.seek(1, 1)
        self.assertEqual(memio.read(), buf[1:])

    def test_unicode(self):
        memio = self.ioclass()

        self.assertRaises(TypeError, self.ioclass, "1234567890")
        self.assertRaises(TypeError, memio.write, "1234567890")
        self.assertRaises(TypeError, memio.writelines, ["1234567890"])

    def test_bytes_array(self):
        buf = b"1234567890"
        import array
        a = array.array('b', list(buf))
        memio = self.ioclass(a)
        self.assertEqual(memio.getvalue(), buf)
        self.assertEqual(memio.write(a), 10)
        self.assertEqual(memio.getvalue(), buf)

    def test_issue5449(self):
        buf = self.buftype("1234567890")
        self.ioclass(initial_bytes=buf)
        self.assertRaises(TypeError, self.ioclass, buf, foo=None)

class PyStringIOTest(MemoryTestMixin, MemorySeekTestMixin, unittest.TestCase):
    buftype = str
    ioclass = pyio.StringIO
    UnsupportedOperation = pyio.UnsupportedOperation
    EOF = ""

    # TextIO-specific behaviour.

    def test_newlines_property(self):
        memio = self.ioclass(newline=None)
        # The C StringIO decodes newlines in write() calls, but the Python
        # implementation only does when reading.  This function forces them to
        # be decoded for testing.
        def force_decode():
            memio.seek(0)
            memio.read()
        self.assertEqual(memio.newlines, None)
        memio.write("a\n")
        force_decode()
        self.assertEqual(memio.newlines, "\n")
        memio.write("b\r\n")
        force_decode()
        self.assertEqual(memio.newlines, ("\n", "\r\n"))
        memio.write("c\rd")
        force_decode()
        self.assertEqual(memio.newlines, ("\r", "\n", "\r\n"))

    def test_relative_seek(self):
        memio = self.ioclass()

        self.assertRaises(IOError, memio.seek, -1, 1)
        self.assertRaises(IOError, memio.seek, 3, 1)
        self.assertRaises(IOError, memio.seek, -3, 1)
        self.assertRaises(IOError, memio.seek, -1, 2)
        self.assertRaises(IOError, memio.seek, 1, 1)
        self.assertRaises(IOError, memio.seek, 1, 2)

    def test_textio_properties(self):
        memio = self.ioclass()

        # These are just dummy values but we nevertheless check them for fear
        # of unexpected breakage.
        self.assertIsNone(memio.encoding)
        self.assertIsNone(memio.errors)
        self.assertFalse(memio.line_buffering)

    def test_newline_none(self):
        # newline=None
        memio = self.ioclass("a\nb\r\nc\rd", newline=None)
        self.assertEqual(list(memio), ["a\n", "b\n", "c\n", "d"])
        memio.seek(0)
        self.assertEqual(memio.read(1), "a")
        self.assertEqual(memio.read(2), "\nb")
        self.assertEqual(memio.read(2), "\nc")
        self.assertEqual(memio.read(1), "\n")
        memio = self.ioclass(newline=None)
        self.assertEqual(2, memio.write("a\n"))
        self.assertEqual(3, memio.write("b\r\n"))
        self.assertEqual(3, memio.write("c\rd"))
        memio.seek(0)
        self.assertEqual(memio.read(), "a\nb\nc\nd")
        memio = self.ioclass("a\r\nb", newline=None)
        self.assertEqual(memio.read(3), "a\nb")

    def test_newline_empty(self):
        # newline=""
        memio = self.ioclass("a\nb\r\nc\rd", newline="")
        self.assertEqual(list(memio), ["a\n", "b\r\n", "c\r", "d"])
        memio.seek(0)
        self.assertEqual(memio.read(4), "a\nb\r")
        self.assertEqual(memio.read(2), "\nc")
        self.assertEqual(memio.read(1), "\r")
        memio = self.ioclass(newline="")
        self.assertEqual(2, memio.write("a\n"))
        self.assertEqual(2, memio.write("b\r"))
        self.assertEqual(2, memio.write("\nc"))
        self.assertEqual(2, memio.write("\rd"))
        memio.seek(0)
        self.assertEqual(list(memio), ["a\n", "b\r\n", "c\r", "d"])

    def test_newline_lf(self):
        # newline="\n"
        memio = self.ioclass("a\nb\r\nc\rd")
        self.assertEqual(list(memio), ["a\n", "b\r\n", "c\rd"])

    def test_newline_cr(self):
        # newline="\r"
        memio = self.ioclass("a\nb\r\nc\rd", newline="\r")
        memio.seek(0)
        self.assertEqual(memio.read(), "a\rb\r\rc\rd")
        memio.seek(0)
        self.assertEqual(list(memio), ["a\r", "b\r", "\r", "c\r", "d"])

    def test_newline_crlf(self):
        # newline="\r\n"
        memio = self.ioclass("a\nb\r\nc\rd", newline="\r\n")
        memio.seek(0)
        self.assertEqual(memio.read(), "a\r\nb\r\r\nc\rd")
        memio.seek(0)
        self.assertEqual(list(memio), ["a\r\n", "b\r\r\n", "c\rd"])

    def test_issue5265(self):
        # StringIO can duplicate newlines in universal newlines mode
        memio = self.ioclass("a\r\nb\r\n", newline=None)
        self.assertEqual(memio.read(5), "a\nb\n")

    def test_newline_argument(self):
        self.assertRaises(TypeError, self.ioclass, newline=b"\n")
        self.assertRaises(ValueError, self.ioclass, newline="error")
        # These should not raise an error
        for newline in (None, "", "\n", "\r", "\r\n"):
            self.ioclass(newline=newline)


class CBytesIOTest(PyBytesIOTest):
    ioclass = io.BytesIO
    UnsupportedOperation = io.UnsupportedOperation

class CStringIOTest(PyStringIOTest):
    ioclass = io.StringIO
    UnsupportedOperation = io.UnsupportedOperation

    # XXX: For the Python version of io.StringIO, this is highly
    # dependent on the encoding used for the underlying buffer.
    def test_widechar(self):
        buf = self.buftype("\U0002030a\U00020347")
        memio = self.ioclass(buf)

        self.assertEqual(memio.getvalue(), buf)
        self.assertEqual(memio.write(buf), len(buf))
        self.assertEqual(memio.tell(), len(buf))
        self.assertEqual(memio.getvalue(), buf)
        self.assertEqual(memio.write(buf), len(buf))
        self.assertEqual(memio.tell(), len(buf) * 2)
        self.assertEqual(memio.getvalue(), buf + buf)


def test_main():
    tests = [PyBytesIOTest, PyStringIOTest, CBytesIOTest, CStringIOTest]
    support.run_unittest(*tests)

if __name__ == '__main__':
    test_main()

"""Unit tests for memory-based file-like objects.
StringIO -- for unicode strings
BytesIO -- for bytes
"""

from __future__ import unicode_literals

import unittest
from test import test_support

import io
import sys
import array

try:
    import _bytesio
    has_c_implementation = True
except ImportError:
    has_c_implementation = False


class MemoryTestMixin:

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
        # truncate() accepts long objects
        self.assertEqual(memio.truncate(4L), 4)
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

    def test_read(self):
        buf = self.buftype("1234567890")
        memio = self.ioclass(buf)

        self.assertEqual(memio.read(0), self.EOF)
        self.assertEqual(memio.read(1), buf[:1])
        # read() accepts long objects
        self.assertEqual(memio.read(4L), buf[1:5])
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
        # readline() accepts long objects
        self.assertEqual(memio.readline(5L), buf[5:10])
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
        self.assertEqual(memio.readline(None), buf)
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
        # readlines() accepts long objects
        self.assertEqual(memio.readlines(15L), [buf] * 2)
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
        self.failUnless(hasattr(memio, '__iter__'))
        self.failUnless(hasattr(memio, 'next'))
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
        self.assertRaises(ValueError, memio.next)

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
        # seek() accepts long objects
        self.assertEqual(memio.seek(3L), 3)
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


class PyBytesIOTest(MemoryTestMixin, unittest.TestCase):
    @staticmethod
    def buftype(s):
        return s.encode("ascii")
    ioclass = io._BytesIO
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
        a = array.array(b'b', map(ord, b"hello world"))
        memio = self.ioclass(buf)
        memio.readinto(a)
        self.assertEqual(a.tostring(), b"1234567890d")
        memio.close()
        self.assertRaises(ValueError, memio.readinto, b)

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

        a = array.array(b'b', map(ord, buf))
        memio = self.ioclass(a)
        self.assertEqual(memio.getvalue(), buf)
        self.assertEqual(memio.write(a), 10)
        self.assertEqual(memio.getvalue(), buf)


class PyStringIOTest(MemoryTestMixin, unittest.TestCase):
    buftype = unicode
    ioclass = io.StringIO
    EOF = ""

    def test_relative_seek(self):
        memio = self.ioclass()

        self.assertRaises(IOError, memio.seek, -1, 1)
        self.assertRaises(IOError, memio.seek, 3, 1)
        self.assertRaises(IOError, memio.seek, -3, 1)
        self.assertRaises(IOError, memio.seek, -1, 2)
        self.assertRaises(IOError, memio.seek, 1, 1)
        self.assertRaises(IOError, memio.seek, 1, 2)

    # XXX: For the Python version of io.StringIO, this is highly
    # dependent on the encoding used for the underlying buffer.
    # def test_widechar(self):
    #     buf = self.buftype("\U0002030a\U00020347")
    #     memio = self.ioclass(buf)
    #
    #     self.assertEqual(memio.getvalue(), buf)
    #     self.assertEqual(memio.write(buf), len(buf))
    #     self.assertEqual(memio.tell(), len(buf))
    #     self.assertEqual(memio.getvalue(), buf)
    #     self.assertEqual(memio.write(buf), len(buf))
    #     self.assertEqual(memio.tell(), len(buf) * 2)
    #     self.assertEqual(memio.getvalue(), buf + buf)

if has_c_implementation:
    class CBytesIOTest(PyBytesIOTest):
        ioclass = io.BytesIO

def test_main():
    tests = [PyBytesIOTest, PyStringIOTest]
    if has_c_implementation:
        tests.extend([CBytesIOTest])
    test_support.run_unittest(*tests)

if __name__ == '__main__':
    test_main()

import os
import unittest
from test import support


class CAPITest(unittest.TestCase):

    # Test PyBytes_FromFormat()
    def test_from_format(self):
        ctypes = support.import_module('ctypes')
        _testcapi = support.import_module('_testcapi')
        from ctypes import pythonapi, py_object
        from ctypes import (
            c_int, c_uint,
            c_long, c_ulong,
            c_size_t, c_ssize_t,
            c_char_p)

        PyBytes_FromFormat = pythonapi.PyBytes_FromFormat
        PyBytes_FromFormat.restype = py_object

        # basic tests
        self.assertEqual(PyBytes_FromFormat(b'format'),
                         b'format')
        self.assertEqual(PyBytes_FromFormat(b'Hello %s !', b'world'),
                         b'Hello world !')

        # test formatters
        self.assertEqual(PyBytes_FromFormat(b'c=%c', c_int(0)),
                         b'c=\0')
        self.assertEqual(PyBytes_FromFormat(b'c=%c', c_int(ord('@'))),
                         b'c=@')
        self.assertEqual(PyBytes_FromFormat(b'c=%c', c_int(255)),
                         b'c=\xff')
        self.assertEqual(PyBytes_FromFormat(b'd=%d ld=%ld zd=%zd',
                                            c_int(1), c_long(2),
                                            c_size_t(3)),
                         b'd=1 ld=2 zd=3')
        self.assertEqual(PyBytes_FromFormat(b'd=%d ld=%ld zd=%zd',
                                            c_int(-1), c_long(-2),
                                            c_size_t(-3)),
                         b'd=-1 ld=-2 zd=-3')
        self.assertEqual(PyBytes_FromFormat(b'u=%u lu=%lu zu=%zu',
                                            c_uint(123), c_ulong(456),
                                            c_size_t(789)),
                         b'u=123 lu=456 zu=789')
        self.assertEqual(PyBytes_FromFormat(b'i=%i', c_int(123)),
                         b'i=123')
        self.assertEqual(PyBytes_FromFormat(b'i=%i', c_int(-123)),
                         b'i=-123')
        self.assertEqual(PyBytes_FromFormat(b'x=%x', c_int(0xabc)),
                         b'x=abc')

        sizeof_ptr = ctypes.sizeof(c_char_p)

        if os.name == 'nt':
            # Windows (MSCRT)
            ptr_format = '0x%0{}X'.format(2 * sizeof_ptr)
            def ptr_formatter(ptr):
                return (ptr_format % ptr)
        else:
            # UNIX (glibc)
            def ptr_formatter(ptr):
                return '%#x' % ptr

        ptr = 0xabcdef
        self.assertEqual(PyBytes_FromFormat(b'ptr=%p', c_char_p(ptr)),
                         ('ptr=' + ptr_formatter(ptr)).encode('ascii'))
        self.assertEqual(PyBytes_FromFormat(b's=%s', c_char_p(b'cstr')),
                         b's=cstr')

        # test minimum and maximum integer values
        size_max = c_size_t(-1).value
        for formatstr, ctypes_type, value, py_formatter in (
            (b'%d', c_int, _testcapi.INT_MIN, str),
            (b'%d', c_int, _testcapi.INT_MAX, str),
            (b'%ld', c_long, _testcapi.LONG_MIN, str),
            (b'%ld', c_long, _testcapi.LONG_MAX, str),
            (b'%lu', c_ulong, _testcapi.ULONG_MAX, str),
            (b'%zd', c_ssize_t, _testcapi.PY_SSIZE_T_MIN, str),
            (b'%zd', c_ssize_t, _testcapi.PY_SSIZE_T_MAX, str),
            (b'%zu', c_size_t, size_max, str),
            (b'%p', c_char_p, size_max, ptr_formatter),
        ):
            self.assertEqual(PyBytes_FromFormat(formatstr, ctypes_type(value)),
                             py_formatter(value).encode('ascii')),

        # width and precision (width is currently ignored)
        self.assertEqual(PyBytes_FromFormat(b'%5s', b'a'),
                         b'a')
        self.assertEqual(PyBytes_FromFormat(b'%.3s', b'abcdef'),
                         b'abc')

        # '%%' formatter
        self.assertEqual(PyBytes_FromFormat(b'%%'),
                         b'%')
        self.assertEqual(PyBytes_FromFormat(b'[%%]'),
                         b'[%]')
        self.assertEqual(PyBytes_FromFormat(b'%%%c', c_int(ord('_'))),
                         b'%_')
        self.assertEqual(PyBytes_FromFormat(b'%%s'),
                         b'%s')

        # Invalid formats and partial formatting
        self.assertEqual(PyBytes_FromFormat(b'%'), b'%')
        self.assertEqual(PyBytes_FromFormat(b'x=%i y=%', c_int(2), c_int(3)),
                         b'x=2 y=%')

        # Issue #19969: %c must raise OverflowError for values
        # not in the range [0; 255]
        self.assertRaises(OverflowError,
                          PyBytes_FromFormat, b'%c', c_int(-1))
        self.assertRaises(OverflowError,
                          PyBytes_FromFormat, b'%c', c_int(256))


if __name__ == "__main__":
    unittest.main()

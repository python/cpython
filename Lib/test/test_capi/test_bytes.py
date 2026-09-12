import sys
import unittest
from test import support
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testcapi = import_helper.import_module('_testcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None

class BytesSubclass(bytes):
    pass

class BytesLike:
    def __init__(self, value):
        self.value = value
    def __bytes__(self):
        return self.value


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyBytes_Check()
        check = _testlimitedcapi.bytes_check
        self.assertTrue(check(b'abc'))
        self.assertTrue(check(b''))
        self.assertFalse(check('abc'))
        self.assertFalse(check(bytearray(b'abc')))
        self.assertTrue(check(BytesSubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # Test PyBytes_CheckExact()
        check = _testlimitedcapi.bytes_checkexact
        self.assertTrue(check(b'abc'))
        self.assertTrue(check(b''))
        self.assertFalse(check('abc'))
        self.assertFalse(check(bytearray(b'abc')))
        self.assertFalse(check(BytesSubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_fromstringandsize(self):
        # Test PyBytes_FromStringAndSize()
        fromstringandsize = _testlimitedcapi.bytes_fromstringandsize

        self.assertEqual(fromstringandsize(b'abc'), b'abc')
        self.assertEqual(fromstringandsize(b'abc', 2), b'ab')
        self.assertEqual(fromstringandsize(b'abc\0def'), b'abc\0def')
        self.assertEqual(fromstringandsize(b'a'), b'a')
        self.assertEqual(fromstringandsize(b'a', 1), b'a')
        self.assertEqual(fromstringandsize(b'', 0), b'')
        self.assertEqual(fromstringandsize(NULL, 0), b'')
        self.assertEqual(len(fromstringandsize(NULL, 3)), 3)
        self.assertRaises((MemoryError, OverflowError),
                          fromstringandsize, NULL, PY_SSIZE_T_MAX)

        self.assertRaises(SystemError, fromstringandsize, b'abc', -1)
        self.assertRaises(SystemError, fromstringandsize, b'abc', PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, fromstringandsize, NULL, -1)
        self.assertRaises(SystemError, fromstringandsize, NULL, PY_SSIZE_T_MIN)

    def test_fromstring(self):
        # Test PyBytes_FromString()
        fromstring = _testlimitedcapi.bytes_fromstring

        self.assertEqual(fromstring(b'abc\0def'), b'abc')
        self.assertEqual(fromstring(b''), b'')

        # CRASHES fromstring(NULL)

    def test_fromobject(self):
        # Test PyBytes_FromObject()
        fromobject = _testlimitedcapi.bytes_fromobject

        self.assertEqual(fromobject(b''), b'')
        self.assertEqual(fromobject(b'abc'), b'abc')
        self.assertEqual(fromobject(bytearray(b'abc')), b'abc')
        self.assertEqual(fromobject(BytesSubclass(b'abc')), b'abc')
        self.assertEqual(fromobject([97, 98, 99]), b'abc')
        self.assertRaises(TypeError, fromobject, 3)
        self.assertRaises(TypeError, fromobject, BytesLike(b'abc'))
        self.assertRaises(TypeError, fromobject, 'abc')
        self.assertRaises(TypeError, fromobject, object())
        self.assertRaises(SystemError, fromobject, NULL)

    def test_size(self):
        # Test PyBytes_Size()
        size = _testlimitedcapi.bytes_size

        self.assertEqual(size(b''), 0)
        self.assertEqual(size(b'abc'), 3)
        self.assertEqual(size(BytesSubclass(b'abc')), 3)
        self.assertRaises(TypeError, size, bytearray(b'abc'))
        self.assertRaises(TypeError, size, 'abc')
        self.assertRaises(TypeError, size, object())

        # CRASHES size(NULL)

    def test_asstring(self):
        """Test PyBytes_AsString()"""
        asstring = _testlimitedcapi.bytes_asstring

        self.assertEqual(asstring(b'abc', 4), b'abc\0')
        self.assertEqual(asstring(b'abc\0def', 8), b'abc\0def\0')
        self.assertEqual(asstring(b'', 1), b'\0')
        self.assertRaises(TypeError, asstring, 'abc', 0)
        self.assertRaises(TypeError, asstring, object(), 0)

        # CRASHES asstring(NULL, 0)

    def test_asstringandsize(self):
        """Test PyBytes_AsStringAndSize()"""
        asstringandsize = _testlimitedcapi.bytes_asstringandsize
        asstringandsize_null = _testlimitedcapi.bytes_asstringandsize_null

        self.assertEqual(asstringandsize(b'abc', 4), (b'abc\0', 3))
        self.assertEqual(asstringandsize(b'abc\0def', 8), (b'abc\0def\0', 7))
        self.assertEqual(asstringandsize(b'', 1), (b'\0', 0))
        self.assertEqual(asstringandsize_null(b'abc', 4), b'abc\0')
        self.assertRaises(ValueError, asstringandsize_null, b'abc\0def', 8)
        self.assertRaises(TypeError, asstringandsize, 'abc', 0)
        self.assertRaises(TypeError, asstringandsize_null, 'abc', 0)
        self.assertRaises(TypeError, asstringandsize, object(), 0)
        self.assertRaises(TypeError, asstringandsize_null, object(), 0)

        # CRASHES asstringandsize(NULL, 0)
        # CRASHES asstringandsize_null(NULL, 0)

    def test_repr(self):
        # Test PyBytes_Repr()
        bytes_repr = _testlimitedcapi.bytes_repr

        self.assertEqual(bytes_repr(b'', 0), r"""b''""")
        self.assertEqual(bytes_repr(b'''abc''', 0), r"""b'abc'""")
        self.assertEqual(bytes_repr(b'''abc''', 1), r"""b'abc'""")
        self.assertEqual(bytes_repr(b'''a'b"c"d''', 0), r"""b'a\'b"c"d'""")
        self.assertEqual(bytes_repr(b'''a'b"c"d''', 1), r"""b'a\'b"c"d'""")
        self.assertEqual(bytes_repr(b'''a'b"c''', 0), r"""b'a\'b"c'""")
        self.assertEqual(bytes_repr(b'''a'b"c''', 1), r"""b'a\'b"c'""")
        self.assertEqual(bytes_repr(b'''a'b'c"d''', 0), r"""b'a\'b\'c"d'""")
        self.assertEqual(bytes_repr(b'''a'b'c"d''', 1), r"""b'a\'b\'c"d'""")
        self.assertEqual(bytes_repr(b'''a'b'c'd''', 0), r"""b'a\'b\'c\'d'""")
        self.assertEqual(bytes_repr(b'''a'b'c'd''', 1), r'''b"a'b'c'd"''')

        self.assertEqual(bytes_repr(BytesSubclass(b'abc'), 0), r"""b'abc'""")

        # UDEFINED bytes_repr(object(), 0)
        # CRASHES bytes_repr(NULL, 0)

    def test_concat(self, concat=None):
        """Test PyBytes_Concat()"""
        if concat is None:
            concat = _testlimitedcapi.bytes_concat

        self.assertEqual(concat(b'abc', b'def'), b'abcdef')
        self.assertEqual(concat(b'a\0b', b'c\0d'), b'a\0bc\0d')
        self.assertEqual(concat(bytearray(b'abc'), b'def'), b'abcdef')
        self.assertEqual(concat(b'abc', bytearray(b'def')), b'abcdef')
        self.assertEqual(concat(bytearray(b'abc'), b''), b'abc')
        self.assertEqual(concat(b'', bytearray(b'def')), b'def')
        self.assertEqual(concat(memoryview(b'xabcy')[1:4], b'def'), b'abcdef')
        self.assertEqual(concat(b'abc', memoryview(b'xdefy')[1:4]), b'abcdef')
        self.assertEqual(concat(b'', b''), b'')

        self.assertEqual(concat(b'abc', b'def', True), b'abcdef')
        self.assertEqual(concat(b'abc', bytearray(b'def'), True), b'abcdef')
        # Check that it does not change the singleton
        self.assertEqual(concat(bytes(), b'def', True), b'def')
        self.assertEqual(len(bytes()), 0)

        self.assertRaises(TypeError, concat, memoryview(b'axbycz')[::2], b'def')
        self.assertRaises(TypeError, concat, b'abc', memoryview(b'dxeyfz')[::2])
        self.assertRaises(TypeError, concat, b'abc', 'def')
        self.assertRaises(TypeError, concat, 'abc', b'def')
        self.assertRaises(TypeError, concat, 'abc', 'def')
        self.assertRaises(TypeError, concat, [], b'def')
        self.assertRaises(TypeError, concat, b'abc', [])
        self.assertRaises(TypeError, concat, [], [])

        self.assertEqual(concat(NULL, b'def'), NULL)
        self.assertEqual(concat(b'abc', NULL), NULL)
        self.assertEqual(concat(NULL, object()), NULL)
        self.assertEqual(concat(object(), NULL), NULL)

    def test_concatanddel(self):
        """Test PyBytes_ConcatAndDel()"""
        self.test_concat(_testlimitedcapi.bytes_concatanddel)

    def test_decodeescape(self):
        """Test PyBytes_DecodeEscape()"""
        decodeescape = _testlimitedcapi.bytes_decodeescape

        self.assertEqual(decodeescape(b''), b'')
        self.assertEqual(decodeescape(b'abc'), b'abc')
        self.assertEqual(decodeescape(br'\t\n\r\x0b\x0c\x00\\\'\"'),
                         b'''\t\n\r\v\f\0\\'"''')
        self.assertEqual(decodeescape(b'\t\n\r\x0b\x0c\x00'), b'\t\n\r\v\f\0')
        self.assertEqual(decodeescape(br'\xa1\xa2'), b'\xa1\xa2')
        self.assertEqual(decodeescape(br'\2\24\241'), b'\x02\x14\xa1')
        self.assertEqual(decodeescape(b'\xa1\xa2'), b'\xa1\xa2')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\u4f60'), br'\u4f60')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\z'), br'\z')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\541'), b'a')

        for b in b'\\', br'\x', br'\xa', br'\xz', br'\xaz':
            self.assertRaises(ValueError, decodeescape, b)
            self.assertRaises(ValueError, decodeescape, b, 'strict')
        self.assertEqual(decodeescape(br'x\xa', 'replace'), b'x?')
        self.assertEqual(decodeescape(br'x\xay', 'replace'), b'x?y')
        self.assertEqual(decodeescape(br'x\xa\xy', 'replace'), b'x??y')
        self.assertEqual(decodeescape(br'x\xa\xy', 'ignore'), b'xy')
        self.assertRaises(ValueError, decodeescape, b'\\', 'spam')
        self.assertRaises(OverflowError, decodeescape, b'abc', NULL, PY_SSIZE_T_MAX)
        self.assertRaises(OverflowError, decodeescape, NULL, NULL, PY_SSIZE_T_MAX)

        # INVALID decodeescape(NULL)
        # CRASHES decodeescape(b'abc', NULL, -1)
        # CRASHES decodeescape(NULL, NULL, 1)

    def test_resize(self):
        """Test _PyBytes_Resize()"""
        _resize = _testcapi.bytes_resize

        def resize(obj, size, new):
            result = _resize(obj, size, new)
            if 1 <= len(result):
                if new or size != len(obj):
                    # gh-156995: Make sure that the result is a fresh object.
                    # Previously, _PyBytes_Resize(&obj, 1) returned a singleton
                    # if _PyObject_IsUniquelyReferenced() is false.
                    self.assertEqual(sys.getrefcount(result), 1)
                    self.assertFalse(sys._is_immortal(result))
            else:
                # check that the result is the empty bytes string singleton
                self.assertTrue(sys._is_immortal(result))
            return result

        for new in True, False:
            with self.subTest(new=new):
                self.assertEqual(resize(b'abc', 0, new), b'')
                self.assertEqual(resize(b'abc', 1, new), b'a')
                self.assertEqual(resize(b'abc', 2, new), b'ab')
                self.assertEqual(resize(b'abc', 3, new), b'abc')
                b = resize(b'abc', 4, new)
                self.assertEqual(len(b), 4)
                self.assertEqual(b[:3], b'abc')

                self.assertEqual(resize(b'a', 0, new), b'')
                self.assertEqual(resize(b'a', 1, new), b'a')
                b = resize(b'a', 2, new)
                self.assertEqual(len(b), 2)
                self.assertEqual(b[:1], b'a')

                self.assertEqual(resize(b'', 0, new), b'')
                self.assertEqual(len(resize(b'', 1, new)), 1)
                self.assertEqual(len(resize(b'', 2, new)), 2)

        self.assertRaises(SystemError, resize, b'abc', -1, False)
        self.assertRaises(SystemError, resize, bytearray(b'abc'), 3, False)

        # CRASHES resize(NULL, 0, False)
        # CRASHES resize(NULL, 3, False)

    def test_join(self):
        """Test PyBytes_Join()"""
        bytes_join = _testcapi.bytes_join

        self.assertEqual(bytes_join(b'', []), b'')
        self.assertEqual(bytes_join(b'sep', []), b'')

        self.assertEqual(bytes_join(b'', [b'a', b'b', b'c']), b'abc')
        self.assertEqual(bytes_join(b'-', [b'a', b'b', b'c']), b'a-b-c')
        self.assertEqual(bytes_join(b' - ', [b'a', b'b', b'c']), b'a - b - c')
        self.assertEqual(bytes_join(b'-', [bytearray(b'abc'),
                                           memoryview(b'def')]),
                         b'abc-def')

        self.assertEqual(bytes_join(b'-', iter([b'a', b'b', b'c'])), b'a-b-c')

        # invalid 'sep' argument
        with self.assertRaises(TypeError):
            bytes_join(bytearray(b'sep'), [])
        with self.assertRaises(TypeError):
            bytes_join(memoryview(b'sep'), [])
        with self.assertRaises(TypeError):
            bytes_join('', [])  # empty Unicode string
        with self.assertRaises(TypeError):
            bytes_join('unicode', [])
        with self.assertRaises(TypeError):
            bytes_join(123, [])
        with self.assertRaises(SystemError):
            self.assertEqual(bytes_join(NULL, [b'a', b'b', b'c']), b'abc')

        # invalid 'iterable' argument
        with self.assertRaises(TypeError):
            bytes_join(b'', [b'bytes', 'unicode'])
        with self.assertRaises(TypeError):
            bytes_join(b'', [b'bytes', 123])
        with self.assertRaises(TypeError):
            bytes_join(b'', 123)
        with self.assertRaises(SystemError):
            bytes_join(b'', NULL)


class BaseWriterTest:
    RESULT_TYPE = NotImplementedError
    SMALL_BUFFER = 11  # bytes
    assert SMALL_BUFFER < _testcapi.PyBytesWriter_small_buffer
    LARGE_BUFFER = _testcapi.PyBytesWriter_small_buffer + 17  # bytes
    NEW_BYTE = b'\xff'

    def create_writer(self, alloc=0, string=b''):
        raise NotImplementedError

    def test_create(self):
        # Test PyBytesWriter_Create()
        writer = self.create_writer()
        self.assertEqual(writer.get_size(), 0)
        self.assertEqual(writer.finish(), b'')

        writer = self.create_writer(3)
        writer.write(0, b'abc')
        self.assertEqual(writer.get_size(), 3)
        result = writer.finish()
        self.assertEqual(result, b'abc')
        self.assertEqual(type(result), self.RESULT_TYPE)

    @unittest.skipUnless(support.Py_DEBUG, 'need Py_DEBUG')
    def test_get_data(self):
        # Test PyBytesWriter_GetData()
        writer = self.create_writer(6)
        NEW_BYTE = self.NEW_BYTE
        self.assertEqual(writer.get_data(), NEW_BYTE * 6)
        writer.write(0, b'abc')
        self.assertEqual(writer.get_data(), b'abc' + NEW_BYTE * 3)
        writer.write(3, b'123')
        self.assertEqual(writer.get_data(), b'abc123')

        # Switch from small buffer to large buffer
        small, large = self.SMALL_BUFFER, self.LARGE_BUFFER
        writer = self.create_writer(small)
        self.assertEqual(writer.get_data(), NEW_BYTE * small)
        writer.write(0, b's' * small)
        self.assertEqual(writer.get_data(), b's' * small)
        writer.resize(large)
        self.assertEqual(writer.get_data(), b's' * small + NEW_BYTE * (large - small))
        writer.write(small, b'L' * (large - small))
        self.assertEqual(writer.get_data(), b's' * small + b'L' * (large - small))

        # Resize large buffer
        small, large = self.SMALL_BUFFER, self.LARGE_BUFFER
        writer = self.create_writer(large)
        self.assertEqual(writer.get_data(), NEW_BYTE * large)
        writer.write(0, b'L' * large)
        self.assertEqual(writer.get_data(), b'L' * large)
        writer.resize(large + 10)
        self.assertEqual(writer.get_data(), b'L' * large + NEW_BYTE * 10)
        writer.write(large, b'#' * 10)
        self.assertEqual(writer.get_data(), b'L' * large + b'#' * 10)

    def test_finish_with_size(self):
        # Test PyBytesWriter_FinishWithSize()
        writer = self.create_writer(10)
        writer.write(0, b'abc123')
        self.assertEqual(writer.get_size(), 10)
        result = writer.finish_with_size(3)
        self.assertEqual(result, b'abc')
        self.assertEqual(type(result), self.RESULT_TYPE)

        # Error if the size is negative
        writer = self.create_writer(3, )
        writer.write(0, b'abc')
        with self.assertRaises(ValueError):
            writer.finish_with_size(-3)

        # Error if the requested size is larger than the allocated size
        writer = self.create_writer(3)
        writer.write(0, b'abc')
        with self.assertRaises(ValueError):
            writer.finish_with_size(4)

    def test_write_bytes(self):
        # Test PyBytesWriter_WriteBytes()
        writer = self.create_writer()
        writer.write_bytes(b'Hello World!', -1)
        self.assertEqual(writer.finish(), b'Hello World!')

        writer = self.create_writer()
        writer.write_bytes(b'Hello ', -1)
        writer.write_bytes(b'World! <truncated>', 6)
        self.assertEqual(writer.finish(), b'Hello World!')

    def test_resize(self):
        # Test PyBytesWriter_Resize()
        writer = self.create_writer()
        writer.resize(len(b'hello'))
        writer.write(0, b'hello')
        self.assertEqual(writer.finish(), b'hello')

        writer = self.create_writer()
        writer.resize(0)  # noop
        writer.resize(len(b'number'))
        writer.write(0, b'number')
        writer.resize(len(b'number='))
        writer.write(len(b'number'), b'=')
        writer.resize(len(b'number=123'), )
        writer.write(len(b'number='), b'123')
        writer.resize(len(b'number=123'))  # noop
        self.assertEqual(writer.finish(), b'number=123')

        # Switch from small buffer to large buffer
        writer = self.create_writer()
        small, large = self.SMALL_BUFFER, self.LARGE_BUFFER
        writer.resize(small)
        writer.write(0, b's' * small)
        writer.resize(large)
        writer.write(small, b'L' * (large - small))
        self.assertEqual(writer.finish(),
                         b's' * small + b'L' * (large - small))

        # invalid size
        for size in (self.SMALL_BUFFER, self.LARGE_BUFFER):
            with self.subTest(size=size):
                writer = self.create_writer()
                writer.write_bytes(b'x' * size, -1)
                with self.assertRaisesRegex(ValueError, 'size must be >= 0'):
                    writer.resize(-1)
                with self.assertRaises((MemoryError, OverflowError)):
                    writer.resize(_testcapi.PY_SSIZE_T_MAX)
                self.assertEqual(writer.finish(), b'x' * size)

    def test_grow(self):
        # Test PyBytesWriter_Grow()
        writer = self.create_writer(0)
        writer.grow(len(b'number=123'))
        writer.write(0, b'number=123')
        self.assertEqual(writer.finish(), b'number=123')

        writer = self.create_writer()
        writer.grow(0)  # noop
        writer.grow(len(b'number'))
        writer.write(0, b'number')
        writer.grow(len(b'='))
        writer.write(len(b'number'), b'=')
        writer.grow(len(b'123'), )
        writer.write(len(b'number='), b'123')
        writer.grow(0)  # noop
        self.assertEqual(writer.finish(), b'number=123')

        for size in (self.SMALL_BUFFER, self.LARGE_BUFFER):
            with self.subTest(size=size):
                # Truncate the last byte
                data = b'x' * size
                writer = self.create_writer(size)
                writer.write(0, data)
                self.assertEqual(writer.get_data(), data)
                writer.grow(-1)
                self.assertEqual(writer.get_data(), data[:-1])
                self.assertEqual(writer.finish(),  data[:-1])

                # Make the buffer empty
                writer = self.create_writer(size)
                writer.write(0, data)
                writer.grow(-size)
                self.assertEqual(writer.get_data(), b'')
                self.assertEqual(writer.finish(),  b'')

        # Switch from small buffer to large buffer
        writer = self.create_writer()
        small, large = self.SMALL_BUFFER, self.LARGE_BUFFER
        writer.grow(small)
        writer.write(0, b's' * small)
        writer.grow(large - small)
        writer.write(small, b'L' * (large - small))
        self.assertEqual(writer.finish(),
                         b's' * small + b'L' * (large - small))

        # invalid size
        for size in (self.SMALL_BUFFER, self.LARGE_BUFFER):
            with self.subTest(size=size):
                writer = self.create_writer()
                writer.write_bytes(b'x' * size, -1)
                with self.assertRaisesRegex(ValueError, 'invalid size'):
                    writer.grow(-size - 1)
                with self.assertRaises(MemoryError):
                    writer.grow(_testcapi.PY_SSIZE_T_MAX)
                self.assertEqual(writer.finish(), b'x' * size)

    @support.nomemtest
    def test_resize_error(self):
        # Test PyBytesWriter_Resize() error
        init = b'x' * self.LARGE_BUFFER
        writer = self.create_writer(len(init))
        writer.write(0, init)
        size = len(init) + 100
        try:
            with self.assertRaises(MemoryError):
                _testcapi.set_nomemory(0)
                writer.resize(size)
        finally:
            _testcapi.remove_mem_hooks()
        suffix = b'still working'
        writer.write_bytes(suffix, -1)
        self.assertEqual(writer.finish(), init + suffix)

        # Note: PyBytesWriter_Resize() leaves the buffer unchanged (no resize)
        # if the new size is smaller than the allocated size

    def test_format_i(self):
        # Test PyBytesWriter_Format()
        writer = self.create_writer()
        writer.format_i(b'x=%i', 123456)
        self.assertEqual(writer.finish(), b'x=123456')

        writer = self.create_writer()
        writer.format_i(b'x=%i, ', 123)
        writer.format_i(b'y=%i', 456)
        self.assertEqual(writer.finish(), b'x=123, y=456')


class BytesWriterTest(BaseWriterTest, unittest.TestCase):
    RESULT_TYPE = bytes

    def create_writer(self, size=0):
        # Test PyBytesWriter_Create()
        return _testcapi.PyBytesWriter(size, 0)

    # Only PyBytesWriter_Create() returns singletons
    def test_singletons(self):
        empty = b''
        singletons = {ch: bytes((ch,)) for ch in range(256)}

        writer = self.create_writer()
        self.assertIs(writer.finish(), empty)

        # Large buffer
        writer = self.create_writer()
        unused_text = b'x' * self.LARGE_BUFFER
        writer.write_bytes(unused_text, len(unused_text))
        self.assertIs(writer.finish_with_size(0), empty)

        # Large buffer with resize
        writer = self.create_writer()
        unused_text = b'x' * self.LARGE_BUFFER
        writer.write_bytes(unused_text, len(unused_text))
        writer.resize(0)
        self.assertIs(writer.finish(), empty)

        for ch in range(256):
            byte = bytes((ch,))

            writer = self.create_writer()
            writer.write_bytes(byte, len(byte))
            self.assertIs(writer.finish(), singletons[ch])

            # Large buffer
            writer = self.create_writer()
            unused_text = b'x' * self.LARGE_BUFFER
            writer.write_bytes(byte + unused_text,
                               len(byte) + len(unused_text))
            self.assertIs(writer.finish_with_size(1), singletons[ch])

    def test_example_abc(self):
        self.assertEqual(_testcapi.byteswriter_abc(), b'abc')

    def test_example_resize(self):
        self.assertEqual(_testcapi.byteswriter_resize(), b'Hello World')

    def test_example_highlevel(self):
        self.assertEqual(_testcapi.byteswriter_highlevel(), b'Hello World!')


class ByteArrayWriterTest(BaseWriterTest, unittest.TestCase):
    RESULT_TYPE = bytearray

    def create_writer(self, size=0):
        # Test private _PyBytesWriter_CreateByteArray()
        return _testcapi.PyBytesWriter(size, 1)


if __name__ == "__main__":
    unittest.main()

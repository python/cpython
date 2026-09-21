# Test PyMarshal C API

import marshal
import os.path
import struct
import unittest

from test import support
from test.support import import_helper
from test.support import os_helper


# Skip this test if _testcapi is are not available.
_testcapi = import_helper.import_module('_testcapi')


def noop_func():
    pass

NULL = None
SIMPLE_OBJECT = 123
# Only test a few objects: see test_marshal for more exhaustive tests
TEST_OBJECTS = (
    '\u20ac',
    b'abc',
    True,
    123,
    45.6,
    7+8j,
    'long line '*1000,
    # Check that serializing code object is allowed (allow_code = 1)
    noop_func.__code__,
)
UNMARSHALLABLE = object()

# Invalid marshal data
JUNK_BYTES = b'\xff' * 32


def read_file(filename):
    with open(filename, 'rb') as fp:
        return fp.read()


def write_file(filename, data):
    with open(filename, 'wb') as fp:
        fp.write(data)


@support.cpython_only
class CAPI_TestCase(unittest.TestCase):

    def test_read_from_file_error(self):
        # A read error is reported as OSError, not EOFError.
        # A directory cannot be read (on some platforms it cannot even
        # be opened, which is reported as OSError as well).
        os.mkdir(os_helper.TESTFN)
        self.addCleanup(os_helper.rmdir, os_helper.TESTFN)
        for func in (_testcapi.pymarshal_read_short_from_file,
                     _testcapi.pymarshal_read_long_from_file,
                     _testcapi.pymarshal_read_object_from_file,
                     _testcapi.pymarshal_read_last_object_from_file):
            with self.subTest(func=func.__name__):
                self.assertRaises(OSError, func, os_helper.TESTFN)

    @unittest.skipUnless(os.path.exists('/dev/full'), 'requires /dev/full')
    def test_write_to_file_error(self):
        # A write error is reported as OSError.
        # The data is large enough to not fit in the stdio buffer, so that
        # the error is detected before the file is closed.
        obj = b'x' * 100000
        with self.assertRaises(OSError):
            _testcapi.pymarshal_write_object_to_file(obj, '/dev/full',
                                                     marshal.version)

    def check_object(self, obj2, obj):
        self.assertEqual(obj2, obj)
        self.assertEqual(type(obj2), type(obj))

    def test_write_long_to_file(self):
        # Test PyMarshal_WriteLongToFile()
        write_long_to_file = _testcapi.pymarshal_write_long_to_file
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        def mask32(value):
            res = value & (2 ** 32  - 1)
            if res >= 2147483648:
                return res - 4294967296
            else:
                return res

        limit = 2 ** 31
        values = [
            _testcapi.LONG_MIN, _testcapi.LONG_MAX,
            -limit, -limit + 2, limit - 2, limit - 1,
            0, 123, -123,
        ]
        # Test values larger than 32-bit on platforms with 64-bit C long
        if _testcapi.LONG_MAX > (2**31-1):
            values.extend((-limit - 2, limit, limit + 2))

        for version in range(marshal.version + 1):
            for value in values:
                with self.subTest(value=value, version=version):
                    write_long_to_file(value, filename, version)
                    data = read_file(filename)
                    self.assertEqual(len(data), 4)
                    value2 = struct.unpack('<i', data)[0]
                    self.assertEqual(value2, mask32(value))

    def test_write_object_to_file(self):
        # Test PyMarshal_WriteObjectToFile()
        write_object_to_file = _testcapi.pymarshal_write_object_to_file
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for version in range(marshal.version + 1):
            for obj in TEST_OBJECTS:
                with self.subTest(obj=obj, version=version):
                    write_object_to_file(obj, filename, version)
                    data = read_file(filename)
                    self.assertEqual(marshal.loads(data), obj)

            with self.assertRaises(SystemError):
                write_object_to_file(NULL, filename, version)

            with self.assertRaisesRegex(ValueError, 'cannot marshal object objects'):
                write_object_to_file(UNMARSHALLABLE, filename, version)

    def test_read_short_from_file(self):
        # Test PyMarshal_ReadShortFromFile()
        read_short_from_file = _testcapi.pymarshal_read_short_from_file
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for value in (-2**15, 2**15-1, 0, 123, -123):
            with self.subTest(value=value):
                data = struct.pack('<h', value) + b'xxxx'
                write_file(filename, data)
                value2 = read_short_from_file(filename)
                self.assertEqual(value2, value)

        write_file(filename, b'\x12')  # less than 2 bytes
        with self.assertRaises(EOFError):
            read_short_from_file(filename)

    def test_read_long_from_file(self):
        # Test PyMarshal_ReadLongFromFile()
        read_long_from_file = _testcapi.pymarshal_read_long_from_file
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for value in (_testcapi.INT_MIN, _testcapi.INT_MAX, 0, 123, -123):
            with self.subTest(value=value):
                data = struct.pack('<i', value)
                write_file(filename, data)
                value2 = read_long_from_file(filename)
                self.assertEqual(value2, value)

        write_file(filename, b'\x56\x34\x12')  # less than 4 bytes
        with self.assertRaises(EOFError):
            read_long_from_file(filename)

    def check_read_object(self, read_object_func, check_pos=True):
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        version = marshal.version
        for obj in TEST_OBJECTS:
            with self.subTest(obj=obj):
                data = marshal.dumps(obj, version)
                data += b'abc'  # following data is ignored
                write_file(filename, data)
                obj2, pos = read_object_func(filename)
                self.check_object(obj2, obj)
                if check_pos:
                    self.assertEqual(pos, len(data))

        data = marshal.dumps(SIMPLE_OBJECT, version)
        data = data[:-1]  # truncate last byte
        write_file(filename, data)
        with self.assertRaises(EOFError):
            read_object_func(filename)

        write_file(filename, JUNK_BYTES)
        with self.assertRaisesRegex(ValueError, 'bad marshal data'):
            read_object_func(filename)

    def test_read_last_object_from_file(self):
        # Test PyMarshal_ReadLastObjectFromFile()
        read_last_object_from_file = _testcapi.pymarshal_read_last_object_from_file
        self.check_read_object(read_last_object_from_file)

    def test_read_object_from_file(self):
        # Test PyMarshal_ReadObjectFromFile()
        read_object_from_file = _testcapi.pymarshal_read_object_from_file
        self.check_read_object(read_object_from_file, check_pos=False)

    def test_pymarshal_readobjectfromstring(self):
        # Test PyMarshal_ReadObjectFromString()
        readobjectfromstring = _testcapi.pymarshal_readobjectfromstring
        for obj in TEST_OBJECTS:
            for version in range(marshal.version + 1):
                with self.subTest(obj=obj, version=version):
                    data = marshal.dumps(obj, version)
                    obj2 = readobjectfromstring(data)
                    self.check_object(obj2, obj)

        data = marshal.dumps(SIMPLE_OBJECT, marshal.version)
        data = data[:-1]  # truncate last byte
        with self.assertRaises(EOFError):
            readobjectfromstring(data)

        with self.assertRaisesRegex(ValueError, 'bad marshal data'):
            readobjectfromstring(JUNK_BYTES)

    def test_pymarshal_writeobjecttostring(self):
        # Test PyMarshal_WriteObjectToString()
        writeobjecttostring = _testcapi.pymarshal_writeobjecttostring
        for version in range(marshal.version + 1):
            for obj in TEST_OBJECTS:
                with self.subTest(obj=obj, version=version):
                    data = writeobjecttostring(obj, version)
                    obj2 = marshal.loads(data)
                    self.check_object(obj2, obj)

            with self.assertRaisesRegex(ValueError, 'cannot marshal object objects'):
                writeobjecttostring(UNMARSHALLABLE, version)

            with self.assertRaises(SystemError):
                writeobjecttostring(NULL, version)


if __name__ == "__main__":
    unittest.main()

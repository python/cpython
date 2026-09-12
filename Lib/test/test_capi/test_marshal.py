# Test PyMarshal C API

import marshal
import struct
import unittest
from test.support import import_helper
from test.support import os_helper

_testcapi = import_helper.import_module('_testcapi')

NULL = None
Py_MARSHAL_VERSION = _testcapi.Py_MARSHAL_VERSION

def noop_func():
    pass

SIMPLE_OBJECT = 123
# Only test a few objects: see test_marshal for more exhaustive tests
TEST_OBJECTS = (
    '\u20ac',
    b'abc',
    True,
    45.6,
    7+8j,
    SIMPLE_OBJECT,
    # Check that serializing code object is allowed (allow_code = 1)
    noop_func.__code__,
)

# Invalid marshal data
JUNK_BYTES = b'\xff' * 32


def read_file(filename):
    with open(filename, 'rb') as fp:
        return fp.read()


def write_file(filename, data):
    with open(filename, 'wb') as fp:
        fp.write(data)


class CAPIUnicodeTest(unittest.TestCase):
    def check_object(self, obj2, obj):
        self.assertEqual(obj2, obj)
        self.assertEqual(type(obj2), type(obj))

    def test_pymarshal_readobjectfromstring(self):
        # Test PyMarshal_ReadObjectFromString()
        readobjectfromstring = _testcapi.pymarshal_readobjectfromstring
        for obj in TEST_OBJECTS:
            for version in range(Py_MARSHAL_VERSION + 1):
                with self.subTest(obj=obj, version=version):
                    data = marshal.dumps(obj, version)
                    obj2 = readobjectfromstring(data)
                    self.check_object(obj2, obj)

        data = marshal.dumps(SIMPLE_OBJECT, Py_MARSHAL_VERSION)
        data = data[:-1]  # truncate
        with self.assertRaises(EOFError):
            readobjectfromstring(data)

        with self.assertRaisesRegex(ValueError, 'bad marshal data'):
            readobjectfromstring(JUNK_BYTES)

    def test_pymarshal_writeobjecttostring(self):
        # Test PyMarshal_WriteObjectToString()
        writeobjecttostring = _testcapi.pymarshal_writeobjecttostring
        for version in range(Py_MARSHAL_VERSION + 1):
            for obj in TEST_OBJECTS:
                with self.subTest(obj=obj, version=version):
                    data = writeobjecttostring(obj, version)
                    obj2 = marshal.loads(data)
                    self.check_object(obj2, obj)

            with self.assertRaises(SystemError):
                writeobjecttostring(NULL, version)

    def test_pymarshal_writeobjecttofile(self):
        # Test PyMarshal_WriteObjectToFile()
        writeobjecttofile = _testcapi.pymarshal_writeobjecttofile

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for version in range(Py_MARSHAL_VERSION + 1):
            for obj in TEST_OBJECTS:
                with self.subTest(obj=obj, version=version):
                    writeobjecttofile(obj, filename, version)
                    data = read_file(filename)
                    obj2 = marshal.loads(data)
                    self.check_object(obj2, obj)

            with self.assertRaises(SystemError):
                writeobjecttofile(NULL, filename, version)

    def test_pymarshal_writelongtofile(self):
        # Test PyMarshal_WriteLongToFile()
        writelongtofile = _testcapi.pymarshal_writelongtofile

        def mask32(value):
            res = value & (2 ** 32  - 1)
            if res >= 2147483648:
                return res - 4294967296
            else:
                return res

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        limit = 2 ** 31
        for version in range(Py_MARSHAL_VERSION + 1):
            for value in (
                _testcapi.LONG_MIN,
                _testcapi.LONG_MAX,
                -limit - 2,
                -limit,
                -limit + 2,
                limit - 2,
                limit,
                limit + 2,
                0,
                123,
                -123,
            ):
                with self.subTest(value=value, version=version):
                    writelongtofile(value, filename, version)
                    data = read_file(filename)
                    self.assertEqual(len(data), 4)
                    value2 = struct.unpack('<i', data)[0]
                    self.assertEqual(value2, mask32(value))

    def test_pymarshal_readshortfromfile(self):
        # Test PyMarshal_ReadShortFromFile()
        readshortfromfile = _testcapi.pymarshal_readshortfromfile

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for value in (
            -2**15,
            2**15-1,
            0,
            123,
            -123,
        ):
            with self.subTest(value=value):
                data = struct.pack('<h', value)
                write_file(filename, data)
                value2 = readshortfromfile(filename)
                self.assertEqual(value2, value)

        write_file(filename, b'\x00')  # less than 2 bytes
        with self.assertRaises(EOFError):
            readshortfromfile(filename)

    def test_pymarshal_readlongfromfile(self):
        # Test PyMarshal_ReadLongFromFile()
        readlongfromfile = _testcapi.pymarshal_readlongfromfile

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        for value in (
            -2**31,
            2**31-1,
            0,
            123,
            -123,
        ):
            with self.subTest(value=value):
                data = struct.pack('<i', value)
                write_file(filename, data)
                value2 = readlongfromfile(filename)
                self.assertEqual(value2, value)

        write_file(filename, b'\x00\x01\x02')  # less than 4 bytes
        with self.assertRaises(EOFError):
            readlongfromfile(filename)

    def check_read_object(self, read_object_func):
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        version = Py_MARSHAL_VERSION
        for obj in TEST_OBJECTS:
            with self.subTest(obj=obj):
                data = marshal.dumps(obj, version)
                data += b'abc'  # following data is ignored
                write_file(filename, data)
                obj2 = read_object_func(filename)
                self.check_object(obj2, obj)

        data = marshal.dumps(SIMPLE_OBJECT, version)
        data = data[:-1]  # truncate
        write_file(filename, data)
        with self.assertRaises(EOFError):
            read_object_func(filename)

        write_file(filename, JUNK_BYTES)
        with self.assertRaisesRegex(ValueError, 'bad marshal data'):
            read_object_func(filename)

    def test_pymarshal_readobjetfromfile(self):
        # Test PyMarshal_ReadObjectFromFile()
        readobjectfromfile = _testcapi.pymarshal_readobjectfromfile
        self.check_read_object(readobjectfromfile)

    def test_pymarshal_readlastobjetfromfile(self):
        # Test PyMarshal_ReadLastObjectFromFile()
        readlastobjectfromfile = _testcapi.pymarshal_readlastobjectfromfile
        self.check_read_object(readlastobjectfromfile)


if __name__ == "__main__":
    unittest.main()

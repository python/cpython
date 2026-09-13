import marshal
import os.path
import unittest

from test import support
from test.support import import_helper
from test.support import os_helper
from test.test_marshal import HelperMixin, omit_last_byte


# Skip this test if _testcapi is are not available.
_testcapi = import_helper.import_module('_testcapi')


@support.cpython_only
class CAPI_TestCase(unittest.TestCase, HelperMixin):

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

    def test_write_unmarshallable_to_file(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with self.assertRaisesRegex(ValueError, 'unmarshallable object'):
            _testcapi.pymarshal_write_object_to_file(object(), os_helper.TESTFN,
                                                     marshal.version)

    def test_write_long_to_file(self):
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_long_to_file(0x12345678, os_helper.TESTFN, v)
            with open(os_helper.TESTFN, 'rb') as f:
                data = f.read()
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(data, b'\x78\x56\x34\x12')

    def test_write_object_to_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j, 'long line '*1000)
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_object_to_file(obj, os_helper.TESTFN, v)
            with open(os_helper.TESTFN, 'rb') as f:
                data = f.read()
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(marshal.loads(data), obj)

    def test_read_short_from_file(self):
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_short_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)
        self.assertEqual(r, 0x1234)
        self.assertEqual(p, 2)

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_short_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)

    def test_read_long_from_file(self):
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x78\x56\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_long_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)
        self.assertEqual(r, 0x12345678)
        self.assertEqual(p, 4)

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'\x56\x34\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_long_from_file(os_helper.TESTFN)
        os_helper.unlink(os_helper.TESTFN)

    def test_read_last_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(os_helper.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_last_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(r, obj)

            with open(os_helper.TESTFN, 'wb') as f:
                f.write(omit_last_byte(data))
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_last_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)

    def test_read_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(os_helper.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)
            self.assertEqual(r, obj)
            self.assertEqual(p, len(data))

            with open(os_helper.TESTFN, 'wb') as f:
                f.write(omit_last_byte(data))
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_object_from_file(os_helper.TESTFN)
            os_helper.unlink(os_helper.TESTFN)


if __name__ == "__main__":
    unittest.main()

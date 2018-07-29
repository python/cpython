from test import support
import marshal
import unittest

try:
    import _testcapi
except ImportError:
    _testcapi = None

@support.cpython_only
@unittest.skipUnless(_testcapi, 'requires _testcapi')
class CAPI_TestCase(unittest.TestCase):

    def test_write_long_to_file(self):
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_long_to_file(0x12345678, support.TESTFN, v)
            with open(support.TESTFN, 'rb') as f:
                data = f.read()
            support.unlink(support.TESTFN)
            self.assertEqual(data, b'\x78\x56\x34\x12')

    def test_write_object_to_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j, 'long line '*1000)
        for v in range(marshal.version + 1):
            _testcapi.pymarshal_write_object_to_file(obj, support.TESTFN, v)
            with open(support.TESTFN, 'rb') as f:
                data = f.read()
            support.unlink(support.TESTFN)
            self.assertEqual(marshal.loads(data), obj)

    def test_read_short_from_file(self):
        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_short_from_file(support.TESTFN)
        support.unlink(support.TESTFN)
        self.assertEqual(r, 0x1234)
        self.assertEqual(p, 2)

        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_short_from_file(support.TESTFN)
        support.unlink(support.TESTFN)

    def test_read_long_from_file(self):
        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x78\x56\x34\x12xxxx')
        r, p = _testcapi.pymarshal_read_long_from_file(support.TESTFN)
        support.unlink(support.TESTFN)
        self.assertEqual(r, 0x12345678)
        self.assertEqual(p, 4)

        with open(support.TESTFN, 'wb') as f:
            f.write(b'\x56\x34\x12')
        with self.assertRaises(EOFError):
            _testcapi.pymarshal_read_long_from_file(support.TESTFN)
        support.unlink(support.TESTFN)

    def test_read_last_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(support.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_last_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)
            self.assertEqual(r, obj)

            with open(support.TESTFN, 'wb') as f:
                f.write(data[:1])
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_last_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)

    def test_read_object_from_file(self):
        obj = ('\u20ac', b'abc', 123, 45.6, 7+8j)
        for v in range(marshal.version + 1):
            data = marshal.dumps(obj, v)
            with open(support.TESTFN, 'wb') as f:
                f.write(data + b'xxxx')
            r, p = _testcapi.pymarshal_read_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)
            self.assertEqual(r, obj)
            self.assertEqual(p, len(data))

            with open(support.TESTFN, 'wb') as f:
                f.write(data[:1])
            with self.assertRaises(EOFError):
                _testcapi.pymarshal_read_object_from_file(support.TESTFN)
            support.unlink(support.TESTFN)


if __name__ == "__main__":
    unittest.main()

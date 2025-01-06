import os
import unittest
from test import support
from test.support import import_helper, os_helper

_testcapi = import_helper.import_module('_testcapi')


class CAPIFileTest(unittest.TestCase):
    def test_py_fopen(self):
        # Test Py_fopen() and Py_fclose()

        with open(__file__, "rb") as fp:
            source = fp.read()

        for filename in (__file__, os.fsencode(__file__)):
            with self.subTest(filename=filename):
                data = _testcapi.py_fopen(filename, "rb")
                self.assertEqual(data, source[:256])

                data = _testcapi.py_fopen(os_helper.FakePath(filename), "rb")
                self.assertEqual(data, source[:256])

        filenames = [
            os_helper.TESTFN,
            os.fsencode(os_helper.TESTFN),
        ]
        # TESTFN_UNDECODABLE cannot be used to create a file on macOS/WASI.
        if os_helper.TESTFN_UNENCODABLE is not None:
            filenames.append(os_helper.TESTFN_UNENCODABLE)
        for filename in filenames:
            with self.subTest(filename=filename):
                try:
                    with open(filename, "wb") as fp:
                        fp.write(source)

                    data = _testcapi.py_fopen(filename, "rb")
                    self.assertEqual(data, source[:256])
                finally:
                    os_helper.unlink(filename)

        # embedded null character/byte in the filename
        with self.assertRaises(ValueError):
            _testcapi.py_fopen("a\x00b", "rb")
        with self.assertRaises(ValueError):
            _testcapi.py_fopen(b"a\x00b", "rb")

        # non-ASCII mode failing with "Invalid argument"
        with self.assertRaises(OSError):
            _testcapi.py_fopen(__file__, "\xe9")

        # invalid filename type
        for invalid_type in (123, object()):
            with self.subTest(filename=invalid_type):
                with self.assertRaises(TypeError):
                    _testcapi.py_fopen(invalid_type, "rb")

        if support.MS_WINDOWS:
            with self.assertRaises(OSError):
                # On Windows, the file mode is limited to 10 characters
                _testcapi.py_fopen(__file__, "rt+, ccs=UTF-8")

        # CRASHES py_fopen(__file__, None)


if __name__ == "__main__":
    unittest.main()

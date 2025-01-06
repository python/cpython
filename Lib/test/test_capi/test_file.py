import os
import unittest
from test import support
from test.support import import_helper, os_helper

_testcapi = import_helper.import_module('_testcapi')


class CAPIFileTest(unittest.TestCase):
    def test_py_fopen(self):
        # Test Py_fopen() and Py_fclose()
        for filename in (__file__, os.fsencode(__file__)):
            with self.subTest(filename=filename):
                content = _testcapi.py_fopen(filename, "rb")
                with open(filename, "rb") as fp:
                    self.assertEqual(fp.read(256), content)

        with open(__file__, "rb") as fp:
            content = fp.read()
        for filename in (
            os_helper.TESTFN,
            os.fsencode(os_helper.TESTFN),
            os_helper.TESTFN_UNDECODABLE,
            os_helper.TESTFN_UNENCODABLE,
        ):
            with self.subTest(filename=filename):
                try:
                    with open(filename, "wb") as fp:
                        fp.write(content)

                    content = _testcapi.py_fopen(filename, "rb")
                    with open(filename, "rb") as fp:
                        self.assertEqual(fp.read(256), content[:256])
                finally:
                    os_helper.unlink(filename)

        # embedded null character/byte in the filename
        with self.assertRaises(ValueError):
            _testcapi.py_fopen("a\x00b", "rb")
        with self.assertRaises(ValueError):
            _testcapi.py_fopen(b"a\x00b", "rb")

        for invalid_type in (123, object()):
            with self.subTest(filename=invalid_type):
                with self.assertRaises(TypeError):
                    _testcapi.py_fopen(invalid_type, "r")

        if support.MS_WINDOWS:
            with self.assertRaises(OSError):
                # On Windows, the file mode is limited to 10 characters
                _testcapi.py_fopen(__file__, "rt+, ccs=UTF-8")


if __name__ == "__main__":
    unittest.main()

import os
import unittest
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')


class CAPIFileTest(unittest.TestCase):
    def test_py_fopen(self):
        for filename in (__file__, os.fsencode(__file__)):
            with self.subTest(filename=filename):
                content = _testcapi.py_fopen(filename, "rb")
                with open(filename, "rb") as fp:
                    self.assertEqual(fp.read(256), content)

        for invalid_type in (123, object()):
            with self.subTest(filename=invalid_type):
                with self.assertRaises(TypeError):
                    _testcapi.py_fopen(invalid_type, "r")


if __name__ == "__main__":
    unittest.main()

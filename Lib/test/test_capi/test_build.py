import sys
import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


class CAPITest(unittest.TestCase):
    # Test Py_GetVersion()
    def test_getversion(self):
        self.assertEqual(_testlimitedcapi.py_getversion(), sys.version)

    # Test Py_GetPlatform()
    def test_getplatform(self):
        self.assertEqual(_testlimitedcapi.py_getplatform(), sys.platform)

    # Test Py_GetCopyright()
    def test_getcopyright(self):
        self.assertEqual(_testlimitedcapi.py_getcopyright(), sys.copyright)

    # Test Py_GetCompiler()
    def test_getcompiler(self):
        compiler = _testlimitedcapi.py_getcompiler()
        self.assertIsInstance(compiler, str)
        self.assertNotEqual(compiler, '')

    # Test Py_GetBuildInfo()
    def test_getbuildinfo(self):
        buildinfo = _testlimitedcapi.py_getbuildinfo()
        self.assertIsInstance(buildinfo, str)
        self.assertNotEqual(buildinfo, '')

    # Test internal _Py_GetBuiltWithAssert()
    def test__getbuiltwithassert(self):
        self.assertIn(_testlimitedcapi._py_getbuiltwithassert(), (0, 1))


if __name__ == "__main__":
    unittest.main()

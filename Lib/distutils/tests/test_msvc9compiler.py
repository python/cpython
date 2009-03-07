"""Tests for distutils.msvc9compiler."""
import sys
import unittest

from distutils.errors import DistutilsPlatformError

class msvc9compilerTestCase(unittest.TestCase):

    def test_no_compiler(self):
        # makes sure query_vcvarsall throws
        # a DistutilsPlatformError if the compiler
        # is not found
        if sys.platform != 'win32':
            # this test is only for win32
            return
        from distutils.msvccompiler import get_build_version
        if get_build_version() < 8.0:
            # this test is only for MSVC8.0 or above
            return
        from distutils.msvc9compiler import query_vcvarsall
        def _find_vcvarsall(version):
            return None

        from distutils import msvc9compiler
        old_find_vcvarsall = msvc9compiler.find_vcvarsall
        msvc9compiler.find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(DistutilsPlatformError, query_vcvarsall,
                             'wont find this version')
        finally:
            msvc9compiler.find_vcvarsall = old_find_vcvarsall

    def test_reg_class(self):
        if sys.platform != 'win32':
            # this test is only for win32
            return

        from distutils.msvc9compiler import Reg
        self.assertRaises(KeyError, Reg.get_value, 'xxx', 'xxx')

        # looking for values that should exist on all
        # windows registeries versions.
        path = r'Software\Microsoft\Notepad'
        v = Reg.get_value(path, "lfitalic")
        self.assert_(v in (0, 1))

        import _winreg
        HKCU = _winreg.HKEY_CURRENT_USER
        keys = Reg.read_keys(HKCU, 'xxxx')
        self.assertEquals(keys, None)

        keys = Reg.read_keys(HKCU, r'Software\Microsoft')
        self.assert_('Notepad' in keys)

def test_suite():
    return unittest.makeSuite(msvc9compilerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")

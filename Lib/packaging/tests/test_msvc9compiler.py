"""Tests for packaging.compiler.msvc9compiler."""
import os
import sys

from packaging.errors import PackagingPlatformError

from packaging.tests import unittest, support

_MANIFEST = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1"
          manifestVersion="1.0">
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false">
        </requestedExecutionLevel>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.VC90.CRT"
         version="9.0.21022.8" processorArchitecture="x86"
         publicKeyToken="XXXX">
      </assemblyIdentity>
    </dependentAssembly>
  </dependency>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.VC90.MFC"
        version="9.0.21022.8" processorArchitecture="x86"
        publicKeyToken="XXXX"></assemblyIdentity>
    </dependentAssembly>
  </dependency>
</assembly>
"""

_CLEANED_MANIFEST = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1"
          manifestVersion="1.0">
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false">
        </requestedExecutionLevel>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <dependency>

  </dependency>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.VC90.MFC"
        version="9.0.21022.8" processorArchitecture="x86"
        publicKeyToken="XXXX"></assemblyIdentity>
    </dependentAssembly>
  </dependency>
</assembly>"""


class msvc9compilerTestCase(support.TempdirManager,
                            unittest.TestCase):

    @unittest.skipUnless(sys.platform == "win32", "runs only on win32")
    def test_no_compiler(self):
        # make sure query_vcvarsall raises a PackagingPlatformError if
        # the compiler is not found
        from packaging.compiler.msvccompiler import get_build_version
        if get_build_version() < 8.0:
            raise unittest.SkipTest('only for MSVC8.0 or above')

        from packaging.compiler import msvc9compiler
        from packaging.compiler.msvc9compiler import query_vcvarsall

        def _find_vcvarsall(version):
            return None

        old_find_vcvarsall = msvc9compiler.find_vcvarsall
        msvc9compiler.find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(PackagingPlatformError, query_vcvarsall,
                             'wont find this version')
        finally:
            msvc9compiler.find_vcvarsall = old_find_vcvarsall

    @unittest.skipUnless(sys.platform == "win32", "runs only on win32")
    def test_reg_class(self):
        from packaging.compiler.msvccompiler import get_build_version
        if get_build_version() < 8.0:
            raise unittest.SkipTest("requires MSVC 8.0 or later")

        from packaging.compiler.msvc9compiler import Reg
        self.assertRaises(KeyError, Reg.get_value, 'xxx', 'xxx')

        # looking for values that should exist on all
        # windows registeries versions.
        path = r'Control Panel\Desktop'
        v = Reg.get_value(path, 'dragfullwindows')
        self.assertIn(v, ('0', '1', '2'))

        import winreg
        HKCU = winreg.HKEY_CURRENT_USER
        keys = Reg.read_keys(HKCU, 'xxxx')
        self.assertEqual(keys, None)

        keys = Reg.read_keys(HKCU, r'Control Panel')
        self.assertIn('Desktop', keys)

    @unittest.skipUnless(sys.platform == "win32", "runs only on win32")
    def test_remove_visual_c_ref(self):
        from packaging.compiler.msvccompiler import get_build_version
        if get_build_version() < 8.0:
            raise unittest.SkipTest("requires MSVC 8.0 or later")

        from packaging.compiler.msvc9compiler import MSVCCompiler
        tempdir = self.mkdtemp()
        manifest = os.path.join(tempdir, 'manifest')
        with open(manifest, 'w') as f:
            f.write(_MANIFEST)

        compiler = MSVCCompiler()
        compiler._remove_visual_c_ref(manifest)

        # see what we got
        with open(manifest) as f:
            # removing trailing spaces
            content = '\n'.join(line.rstrip() for line in f.readlines())

        # makes sure the manifest was properly cleaned
        self.assertEqual(content, _CLEANED_MANIFEST)


def test_suite():
    return unittest.makeSuite(msvc9compilerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")

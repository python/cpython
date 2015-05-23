"""Tests for distutils._msvccompiler."""
import sys
import unittest
import os

from distutils.errors import DistutilsPlatformError
from distutils.tests import support
from test.support import run_unittest

# A manifest with the only assembly reference being the msvcrt assembly, so
# should have the assembly completely stripped.  Note that although the
# assembly has a <security> reference the assembly is removed - that is
# currently a "feature", not a bug :)
_MANIFEST_WITH_ONLY_MSVC_REFERENCE = """\
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
</assembly>
"""

# A manifest with references to assemblies other than msvcrt.  When processed,
# this assembly should be returned with just the msvcrt part removed.
_MANIFEST_WITH_MULTIPLE_REFERENCES = """\
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

SKIP_MESSAGE = (None if sys.platform == "win32" else
                "These tests are only for win32")

@unittest.skipUnless(SKIP_MESSAGE is None, SKIP_MESSAGE)
class msvccompilerTestCase(support.TempdirManager,
                            unittest.TestCase):

    def test_no_compiler(self):
        # makes sure query_vcvarsall raises
        # a DistutilsPlatformError if the compiler
        # is not found
        from distutils._msvccompiler import _get_vc_env
        def _find_vcvarsall():
            return None

        import distutils._msvccompiler as _msvccompiler
        old_find_vcvarsall = _msvccompiler._find_vcvarsall
        _msvccompiler._find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(DistutilsPlatformError, _get_vc_env,
                             'wont find this version')
        finally:
            _msvccompiler._find_vcvarsall = old_find_vcvarsall

    def test_remove_visual_c_ref(self):
        from distutils._msvccompiler import MSVCCompiler
        tempdir = self.mkdtemp()
        manifest = os.path.join(tempdir, 'manifest')
        f = open(manifest, 'w')
        try:
            f.write(_MANIFEST_WITH_MULTIPLE_REFERENCES)
        finally:
            f.close()

        compiler = MSVCCompiler()
        compiler._remove_visual_c_ref(manifest)

        # see what we got
        f = open(manifest)
        try:
            # removing trailing spaces
            content = '\n'.join([line.rstrip() for line in f.readlines()])
        finally:
            f.close()

        # makes sure the manifest was properly cleaned
        self.assertEqual(content, _CLEANED_MANIFEST)

    def test_remove_entire_manifest(self):
        from distutils._msvccompiler import MSVCCompiler
        tempdir = self.mkdtemp()
        manifest = os.path.join(tempdir, 'manifest')
        f = open(manifest, 'w')
        try:
            f.write(_MANIFEST_WITH_ONLY_MSVC_REFERENCE)
        finally:
            f.close()

        compiler = MSVCCompiler()
        got = compiler._remove_visual_c_ref(manifest)
        self.assertIsNone(got)


def test_suite():
    return unittest.makeSuite(msvccompilerTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())

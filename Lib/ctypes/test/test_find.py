import unittest
import os.path
import sys
import test.support
from ctypes import *
from ctypes.util import find_library

# On some systems, loading the OpenGL libraries needs the RTLD_GLOBAL mode.
class Test_OpenGL_libs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lib_gl = lib_glu = lib_gle = None
        if sys.platform == "win32":
            lib_gl = find_library("OpenGL32")
            lib_glu = find_library("Glu32")
        elif sys.platform == "darwin":
            lib_gl = lib_glu = find_library("OpenGL")
        else:
            lib_gl = find_library("GL")
            lib_glu = find_library("GLU")
            lib_gle = find_library("gle")

        ## print, for debugging
        if test.support.verbose:
            print("OpenGL libraries:")
            for item in (("GL", lib_gl),
                         ("GLU", lib_glu),
                         ("gle", lib_gle)):
                print("\t", item)

        cls.gl = cls.glu = cls.gle = None
        if lib_gl:
            try:
                cls.gl = CDLL(lib_gl, mode=RTLD_GLOBAL)
            except OSError:
                pass
        if lib_glu:
            try:
                cls.glu = CDLL(lib_glu, RTLD_GLOBAL)
            except OSError:
                pass
        if lib_gle:
            try:
                cls.gle = CDLL(lib_gle)
            except OSError:
                pass

    @classmethod
    def tearDownClass(cls):
        cls.gl = cls.glu = cls.gle = None

    def test_gl(self):
        if self.gl is None:
            self.skipTest('lib_gl not available')
        self.gl.glClearIndex

    def test_glu(self):
        if self.glu is None:
            self.skipTest('lib_glu not available')
        self.glu.gluBeginCurve

    def test_gle(self):
        if self.gle is None:
            self.skipTest('lib_gle not available')
        self.gle.gleGetJoinStyle

    def test_shell_injection(self):
        result = find_library('; echo Hello shell > ' + test.support.TESTFN)
        self.assertFalse(os.path.lexists(test.support.TESTFN))
        self.assertIsNone(result)


@unittest.skipUnless(sys.platform.startswith('linux'),
                     'Test only valid for Linux')
class LibPathFindTest(unittest.TestCase):
    def test_find_on_libpath(self):
        import subprocess
        import tempfile

        try:
            p = subprocess.Popen(['gcc', '--version'], stdout=subprocess.PIPE,
                                 stderr=subprocess.DEVNULL)
            out, _ = p.communicate()
        except OSError:
            raise unittest.SkipTest('gcc, needed for test, not available')
        with tempfile.TemporaryDirectory() as d:
            # create an empty temporary file
            srcname = os.path.join(d, 'dummy.c')
            libname = 'py_ctypes_test_dummy'
            dstname = os.path.join(d, 'lib%s.so' % libname)
            with open(srcname, 'w') as f:
                pass
            self.assertTrue(os.path.exists(srcname))
            # compile the file to a shared library
            cmd = ['gcc', '-o', dstname, '--shared',
                   '-Wl,-soname,lib%s.so' % libname, srcname]
            out = subprocess.check_output(cmd)
            self.assertTrue(os.path.exists(dstname))
            # now check that the .so can't be found (since not in
            # LD_LIBRARY_PATH)
            self.assertIsNone(find_library(libname))
            # now add the location to LD_LIBRARY_PATH
            with test.support.EnvironmentVarGuard() as env:
                KEY = 'LD_LIBRARY_PATH'
                if KEY not in env:
                    v = d
                else:
                    v = '%s:%s' % (env[KEY], d)
                env.set(KEY, v)
                # now check that the .so can be found (since in
                # LD_LIBRARY_PATH)
                self.assertEqual(find_library(libname), 'lib%s.so' % libname)


if __name__ == "__main__":
    unittest.main()

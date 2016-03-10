import unittest
import os
import sys
import test.support
from ctypes import *
from ctypes.util import find_library

# On some systems, loading the OpenGL libraries needs the RTLD_GLOBAL mode.
class Test_OpenGL_libs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib_gl = cls.lib_glu = cls.lib_gle = None
        if sys.platform == "win32":
            cls.lib_gl = find_library("OpenGL32")
            cls.lib_glu = find_library("Glu32")
        elif sys.platform == "darwin":
            cls.lib_gl = cls.lib_glu = find_library("OpenGL")
        else:
            cls.lib_gl = find_library("GL")
            cls.lib_glu = find_library("GLU")
            cls.lib_gle = find_library("gle")

        ## print, for debugging
        if test.support.verbose:
            print("OpenGL libraries:")
            for item in (("GL", cls.lib_gl),
                         ("GLU", cls.lib_glu),
                         ("gle", cls.lib_gle)):
                print("\t", item)

        cls.gl = cls.glu = cls.gle = None
        if cls.lib_gl:
            try:
                cls.gl = CDLL(cls.lib_gl, mode=RTLD_GLOBAL)
            except OSError:
                pass
        if cls.lib_glu:
            try:
                cls.glu = CDLL(cls.lib_glu, RTLD_GLOBAL)
            except OSError:
                pass
        if cls.lib_gle:
            try:
                cls.gle = CDLL(cls.lib_gle)
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

    def test_abspath(self):
        if self.lib_gl:
            self.assertTrue(os.path.isabs(self.lib_gl))
        if self.lib_glu:
            self.assertTrue(os.path.isabs(self.lib_glu))
        if self.lib_gle:
            self.assertTrue(os.path.isabs(self.lib_gle))

# On platforms where the default shared library suffix is '.so',
# at least some libraries can be loaded as attributes of the cdll
# object, since ctypes now tries loading the lib again
# with '.so' appended of the first try fails.
#
# Won't work for libc, unfortunately.  OTOH, it isn't
# needed for libc since this is already mapped into the current
# process (?)
#
# On MAC OSX, it won't work either, because dlopen() needs a full path,
# and the default suffix is either none or '.dylib'.
@unittest.skip('test disabled')
@unittest.skipUnless(os.name=="posix" and sys.platform != "darwin",
                     'test not suitable for this platform')
class LoadLibs(unittest.TestCase):
    def test_libm(self):
        import math
        libm = cdll.libm
        sqrt = libm.sqrt
        sqrt.argtypes = (c_double,)
        sqrt.restype = c_double
        self.assertEqual(sqrt(2), math.sqrt(2))

if __name__ == "__main__":
    unittest.main()

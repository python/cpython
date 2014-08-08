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
            cls.gl = CDLL(lib_gl, mode=RTLD_GLOBAL)
        if lib_glu:
            cls.glu = CDLL(lib_glu, RTLD_GLOBAL)
        if lib_gle:
            try:
                cls.gle = CDLL(lib_gle)
            except OSError:
                pass

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

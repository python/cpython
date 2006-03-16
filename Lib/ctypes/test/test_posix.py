import unittest, os, sys
from ctypes import *

if os.name == "posix" and sys.platform == "linux2":
    # I don't really know on which platforms this works,
    # later it should use the find_library stuff to avoid
    # hardcoding the names.

    class TestRTLD_GLOBAL(unittest.TestCase):
        def test_GL(self):
            if os.path.exists('/usr/lib/libGL.so'):
                cdll.load('libGL.so', mode=RTLD_GLOBAL)
            if os.path.exists('/usr/lib/libGLU.so'):
                cdll.load('libGLU.so')

##if os.name == "posix" and sys.platform != "darwin":

##    # On platforms where the default shared library suffix is '.so',
##    # at least some libraries can be loaded as attributes of the cdll
##    # object, since ctypes now tries loading the lib again
##    # with '.so' appended of the first try fails.
##    #
##    # Won't work for libc, unfortunately.  OTOH, it isn't
##    # needed for libc since this is already mapped into the current
##    # process (?)
##    #
##    # On MAC OSX, it won't work either, because dlopen() needs a full path,
##    # and the default suffix is either none or '.dylib'.

##    class LoadLibs(unittest.TestCase):
##        def test_libm(self):
##            import math
##            libm = cdll.libm
##            sqrt = libm.sqrt
##            sqrt.argtypes = (c_double,)
##            sqrt.restype = c_double
##            self.failUnlessEqual(sqrt(2), math.sqrt(2))

if __name__ == "__main__":
    unittest.main()

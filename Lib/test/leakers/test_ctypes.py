
# Taken from Lib/ctypes/test/test_keeprefs.py, PointerToStructure.test().
# When this leak is fixed, remember to remove from Misc/build.sh LEAKY_TESTS.

from ctypes import Structure, c_int, POINTER
import gc

def leak_inner():
    class POINT(Structure):
        _fields_ = [("x", c_int)]
    class RECT(Structure):
        _fields_ = [("a", POINTER(POINT))]

def leak():
    leak_inner()
    gc.collect()

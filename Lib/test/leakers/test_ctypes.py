
# Taken from Lib/test/test_ctypes/test_keeprefs.py, PointerToStructure.test().

import gc
from ctypes import POINTER, Structure, c_int


def leak_inner():
    class POINT(Structure):
        _fields_ = [("x", c_int)]
    class RECT(Structure):
        _fields_ = [("a", POINTER(POINT))]

def leak():
    leak_inner()
    gc.collect()

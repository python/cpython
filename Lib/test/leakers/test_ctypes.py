
# Taken from Lib/ctypes/test/test_keeprefs.py
# When this leak is fixed, remember to remove from Misc/build.sh LEAKY_TESTS.

from ctypes import Structure, c_int

def leak():
    class POINT(Structure):
        _fields_ = [("x", c_int)]
    class RECT(Structure):
        _fields_ = [("ul", POINT)]

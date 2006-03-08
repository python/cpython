from ctypes import *
import unittest
import sys

class Test(unittest.TestCase):

    def test_array2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        # casting an array to a pointer works.
        ptr = cast(array, POINTER(c_int))
        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])

        if 2*sizeof(c_short) == sizeof(c_int):
            ptr = cast(array, POINTER(c_short))
            if sys.byteorder == "little":
                self.failUnlessEqual([ptr[i] for i in range(6)],
                                     [42, 0, 17, 0, 2, 0])
            else:
                self.failUnlessEqual([ptr[i] for i in range(6)],
                                     [0, 42, 0, 17, 0, 2])

    def test_address2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        # on AMD64, sizeof(int) == 4 and sizeof(void *) == 8.
        # By default, cast would convert a Python int (or long) into
        # a C int, which would be too short to represent a pointer
        # on this platform.

        # So we have to wrap the address into a c_void_p for this to work.
        #
        # XXX Better would be to hide the differences in the cast function.
        address = addressof(array)
        ptr = cast(c_void_p(address), POINTER(c_int))
        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])


    def test_ptr2array(self):
        array = (c_int * 3)(42, 17, 2)

##        # Hm, already tested above.
##        ptr = cast(array, POINTER(c_int))
##        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])

#        print cast(addressof(array), c_int * 3)[:]
##        ptr = cast(addressof(ptr)

##        print ptr[0], ptr[1], ptr[2]
##        ptr = POINTER(c_int).from_address(addressof(array))
##        # XXX this crashes:
##        print ptr[0], ptr[1], ptr[2]

if __name__ == "__main__":
    unittest.main()

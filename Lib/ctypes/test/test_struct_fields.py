import unittest
from ctypes import *

class StructFieldsTestCase(unittest.TestCase):
    # Structure/Union classes must get 'finalized' sooner or
    # later, when one of these things happen:
    #
    # 1. _fields_ is set.
    # 2. An instance is created.
    # 3. The type is used as field of another Structure/Union.
    # 4. The type is subclassed
    #
    # When they are finalized, assigning _fields_ is no longer allowed.

    def test_1_A(self):
        class X(Structure):
            pass
        self.failUnlessEqual(sizeof(X), 0) # not finalized
        X._fields_ = [] # finalized
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_1_B(self):
        class X(Structure):
            _fields_ = [] # finalized
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_2(self):
        class X(Structure):
            pass
        X()
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_3(self):
        class X(Structure):
            pass
        class Y(Structure):
            _fields_ = [("x", X)] # finalizes X
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_4(self):
        class X(Structure):
            pass
        class Y(X):
            pass
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])
        Y._fields_ = []
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

if __name__ == "__main__":
    unittest.main()

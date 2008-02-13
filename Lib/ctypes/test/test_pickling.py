import unittest
import pickle
from ctypes import *
import _ctypes_test
dll = CDLL(_ctypes_test.__file__)

class X(Structure):
    _fields_ = [("a", c_int), ("b", c_double)]
    init_called = 0
    def __init__(self, *args, **kw):
        X.init_called += 1
        self.x = 42

class Y(X):
    _fields_ = [("str", c_char_p)]

class PickleTest(unittest.TestCase):
    def dumps(self, item):
        return pickle.dumps(item)

    def loads(self, item):
        return pickle.loads(item)

    def test_simple(self):
        for src in [
            c_int(42),
            c_double(3.14),
            ]:
            dst = self.loads(self.dumps(src))
            self.failUnlessEqual(src.__dict__, dst.__dict__)
            self.failUnlessEqual(memoryview(src).tobytes(),
                                 memoryview(dst).tobytes())

    def test_struct(self):
        X.init_called = 0

        x = X()
        x.a = 42
        self.failUnlessEqual(X.init_called, 1)

        y = self.loads(self.dumps(x))

        # loads must NOT call __init__
        self.failUnlessEqual(X.init_called, 1)

        # ctypes instances are identical when the instance __dict__
        # and the memory buffer are identical
        self.failUnlessEqual(y.__dict__, x.__dict__)
        self.failUnlessEqual(memoryview(y).tobytes(),
                             memoryview(x).tobytes())

    def test_unpickable(self):
        # ctypes objects that are pointers or contain pointers are
        # unpickable.
        self.assertRaises(ValueError, lambda: self.dumps(Y()))

        prototype = CFUNCTYPE(c_int)

        for item in [
            c_char_p(),
            c_wchar_p(),
            c_void_p(),
            pointer(c_int(42)),
            dll._testfunc_p_p,
            prototype(lambda: 42),
            ]:
            self.assertRaises(ValueError, lambda: self.dumps(item))

class PickleTest_1(PickleTest):
    def dumps(self, item):
        return pickle.dumps(item, 1)

class PickleTest_2(PickleTest):
    def dumps(self, item):
        return pickle.dumps(item, 2)

if __name__ == "__main__":
    unittest.main()

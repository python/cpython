import unittest
from ctypes import *

class MyInt(c_int):
    def __cmp__(self, other):
        if type(other) != MyInt:
            return -1
        return cmp(self.value, other.value)
    def __hash__(self): # Silence Py3k warning
        return hash(self.value)

class Test(unittest.TestCase):

    def test_compare(self):
        self.assertEqual(MyInt(3), MyInt(3))
        self.assertNotEqual(MyInt(42), MyInt(43))

    def test_ignore_retval(self):
        # Test if the return value of a callback is ignored
        # if restype is None
        proto = CFUNCTYPE(None)
        def func():
            return (1, "abc", None)

        cb = proto(func)
        self.assertEqual(None, cb())


    def test_int_callback(self):
        args = []
        def func(arg):
            args.append(arg)
            return arg

        cb = CFUNCTYPE(None, MyInt)(func)

        self.assertEqual(None, cb(42))
        self.assertEqual(type(args[-1]), MyInt)

        cb = CFUNCTYPE(c_int, c_int)(func)

        self.assertEqual(42, cb(42))
        self.assertEqual(type(args[-1]), int)

    def test_int_struct(self):
        class X(Structure):
            _fields_ = [("x", MyInt)]

        self.assertEqual(X().x, MyInt())

        s = X()
        s.x = MyInt(42)

        self.assertEqual(s.x, MyInt(42))

if __name__ == "__main__":
    unittest.main()

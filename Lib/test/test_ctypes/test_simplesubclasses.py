import unittest
from ctypes import Structure, CFUNCTYPE, c_int, _SimpleCData
from .support import _CData, PyCSimpleType


class MyInt(c_int):
    def __eq__(self, other):
        if type(other) != MyInt:
            return NotImplementedError
        return self.value == other.value


class Test(unittest.TestCase):
    def test_inheritance_hierarchy(self):
        self.assertEqual(_SimpleCData.mro(), [_SimpleCData, _CData, object])

        self.assertEqual(PyCSimpleType.__name__, "PyCSimpleType")
        self.assertEqual(type(PyCSimpleType), type)

        self.assertEqual(c_int.mro(), [c_int, _SimpleCData, _CData, object])

    def test_abstract_class(self):
        with self.assertRaisesRegex(TypeError, "abstract class"):
            _SimpleCData()

        PyCSimpleType("Foo", (_SimpleCData,), {"_type_": "i"})

    def test_immutability(self):
        for t in [_SimpleCData, PyCSimpleType]:
            msg = "cannot set 'foo' attribute of immutable type"
            with self.assertRaisesRegex(TypeError, msg):
                t.foo = "bar"


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

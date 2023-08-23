import sys
import unittest
from ctypes import (Structure, Union, pointer, POINTER, sizeof, addressof,
                    c_void_p, c_char_p, c_wchar_p, cast,
                    c_byte, c_short, c_int, c_int16)


class Test(unittest.TestCase):
    def test_array2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        # casting an array to a pointer works.
        ptr = cast(array, POINTER(c_int))
        self.assertEqual([ptr[i] for i in range(3)], [42, 17, 2])

        if 2 * sizeof(c_short) == sizeof(c_int):
            ptr = cast(array, POINTER(c_short))
            if sys.byteorder == "little":
                self.assertEqual([ptr[i] for i in range(6)],
                                     [42, 0, 17, 0, 2, 0])
            else:
                self.assertEqual([ptr[i] for i in range(6)],
                                     [0, 42, 0, 17, 0, 2])

    def test_address2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        address = addressof(array)
        ptr = cast(c_void_p(address), POINTER(c_int))
        self.assertEqual([ptr[i] for i in range(3)], [42, 17, 2])

        ptr = cast(address, POINTER(c_int))
        self.assertEqual([ptr[i] for i in range(3)], [42, 17, 2])

    def test_p2a_objects(self):
        array = (c_char_p * 5)()
        self.assertEqual(array._objects, None)
        array[0] = b"foo bar"
        self.assertEqual(array._objects, {'0': b"foo bar"})

        p = cast(array, POINTER(c_char_p))
        # array and p share a common _objects attribute
        self.assertIs(p._objects, array._objects)
        self.assertEqual(array._objects, {'0': b"foo bar", id(array): array})
        p[0] = b"spam spam"
        self.assertEqual(p._objects, {'0': b"spam spam", id(array): array})
        self.assertIs(array._objects, p._objects)
        p[1] = b"foo bar"
        self.assertEqual(p._objects, {'1': b'foo bar', '0': b"spam spam", id(array): array})
        self.assertIs(array._objects, p._objects)

    def test_other(self):
        p = cast((c_int * 4)(1, 2, 3, 4), POINTER(c_int))
        self.assertEqual(p[:4], [1,2, 3, 4])
        self.assertEqual(p[:4:], [1, 2, 3, 4])
        self.assertEqual(p[3:-1:-1], [4, 3, 2, 1])
        self.assertEqual(p[:4:3], [1, 4])
        c_int()
        self.assertEqual(p[:4], [1, 2, 3, 4])
        self.assertEqual(p[:4:], [1, 2, 3, 4])
        self.assertEqual(p[3:-1:-1], [4, 3, 2, 1])
        self.assertEqual(p[:4:3], [1, 4])
        p[2] = 96
        self.assertEqual(p[:4], [1, 2, 96, 4])
        self.assertEqual(p[:4:], [1, 2, 96, 4])
        self.assertEqual(p[3:-1:-1], [4, 96, 2, 1])
        self.assertEqual(p[:4:3], [1, 4])
        c_int()
        self.assertEqual(p[:4], [1, 2, 96, 4])
        self.assertEqual(p[:4:], [1, 2, 96, 4])
        self.assertEqual(p[3:-1:-1], [4, 96, 2, 1])
        self.assertEqual(p[:4:3], [1, 4])

    def test_char_p(self):
        # This didn't work: bad argument to internal function
        s = c_char_p(b"hiho")
        self.assertEqual(cast(cast(s, c_void_p), c_char_p).value,
                             b"hiho")

    def test_wchar_p(self):
        s = c_wchar_p("hiho")
        self.assertEqual(cast(cast(s, c_void_p), c_wchar_p).value,
                         "hiho")

    def test_bad_type_arg(self):
        # The type argument must be a ctypes pointer type.
        array_type = c_byte * sizeof(c_int)
        array = array_type()
        self.assertRaises(TypeError, cast, array, None)
        self.assertRaises(TypeError, cast, array, array_type)
        class Struct(Structure):
            _fields_ = [("a", c_int)]
        self.assertRaises(TypeError, cast, array, Struct)
        class MyUnion(Union):
            _fields_ = [("a", c_int)]
        self.assertRaises(TypeError, cast, array, MyUnion)

    def test_pointer_identity(self):
        class Struct(Structure):
            _fields_ = [('a', c_int16)]
        Struct3 = 3 * Struct
        c_array = (2 * Struct3)(
            Struct3(Struct(a=1), Struct(a=2), Struct(a=3)),
            Struct3(Struct(a=4), Struct(a=5), Struct(a=6))
        )
        self.assertEqual(c_array[0][0].a, 1)
        self.assertEqual(c_array[0][1].a, 2)
        self.assertEqual(c_array[0][2].a, 3)
        self.assertEqual(c_array[1][0].a, 4)
        self.assertEqual(c_array[1][1].a, 5)
        self.assertEqual(c_array[1][2].a, 6)
        p_obj = cast(pointer(c_array), POINTER(pointer(c_array)._type_))
        obj = p_obj.contents
        self.assertEqual(obj[0][0].a, 1)
        self.assertEqual(obj[0][1].a, 2)
        self.assertEqual(obj[0][2].a, 3)
        self.assertEqual(obj[1][0].a, 4)
        self.assertEqual(obj[1][1].a, 5)
        self.assertEqual(obj[1][2].a, 6)
        p_obj = cast(pointer(c_array[0]), POINTER(pointer(c_array)._type_))
        obj = p_obj.contents
        self.assertEqual(obj[0][0].a, 1)
        self.assertEqual(obj[0][1].a, 2)
        self.assertEqual(obj[0][2].a, 3)
        self.assertEqual(obj[1][0].a, 4)
        self.assertEqual(obj[1][1].a, 5)
        self.assertEqual(obj[1][2].a, 6)
        StructPointer = POINTER(Struct)
        s1 = Struct(a=10)
        s2 = Struct(a=20)
        s3 = Struct(a=30)
        pointer_array = (3 * StructPointer)(pointer(s1), pointer(s2), pointer(s3))
        self.assertEqual(pointer_array[0][0].a, 10)
        self.assertEqual(pointer_array[1][0].a, 20)
        self.assertEqual(pointer_array[2][0].a, 30)
        self.assertEqual(pointer_array[0].contents.a, 10)
        self.assertEqual(pointer_array[1].contents.a, 20)
        self.assertEqual(pointer_array[2].contents.a, 30)
        p_obj = cast(pointer(pointer_array[0]), POINTER(pointer(pointer_array)._type_))
        obj = p_obj.contents
        self.assertEqual(obj[0][0].a, 10)
        self.assertEqual(obj[1][0].a, 20)
        self.assertEqual(obj[2][0].a, 30)
        self.assertEqual(obj[0].contents.a, 10)
        self.assertEqual(obj[1].contents.a, 20)
        self.assertEqual(obj[2].contents.a, 30)
        class StructWithPointers(Structure):
            _fields_ = [("s1", POINTER(Struct)), ("s2", POINTER(Struct))]
        struct = StructWithPointers(s1=pointer(s1), s2=pointer(s2))
        p_obj = pointer(struct)
        obj = p_obj.contents
        self.assertEqual(obj.s1[0].a, 10)
        self.assertEqual(obj.s2[0].a, 20)
        self.assertEqual(obj.s1.contents.a, 10)
        self.assertEqual(obj.s2.contents.a, 20)
        p_obj = cast(pointer(struct), POINTER(pointer(pointer_array)._type_))
        obj = p_obj.contents
        self.assertEqual(obj[0][0].a, 10)
        self.assertEqual(obj[1][0].a, 20)
        self.assertEqual(obj[0].contents.a, 10)
        self.assertEqual(obj[1].contents.a, 20)

    def test_pointer_set_contents(self):
        class Struct(Structure):
            _fields_ = [('a', c_int16)]
        p = pointer(Struct(a=23))
        self.assertEqual(p.contents.a, 23)
        self.assertIs(p._type_, Struct)
        cp = cast(p, POINTER(c_int16))
        self.assertEqual(cp.contents._type_, 'h')
        cp.contents = c_int16(24)
        self.assertEqual(cp.contents.value, 24)
        self.assertEqual(p.contents.a, 24)

        pp = pointer(p)
        self.assertIs(pp._type_, POINTER(Struct))

        from code import interact; interact(local=locals())

        cast(pp, POINTER(POINTER(c_int16))).contents.contents = c_int16(32)

        # self.assertIs(p.contents, pp.contents.contents)

        self.assertEqual(cast(p, POINTER(c_int16)).contents.value, 32)
        self.assertEqual(p[0].a, 32)  # works
        self.assertEqual(pp[0].contents.a, 32)  # works
        self.assertEqual(pp.contents[0].a, 32)  # works

        self.assertEqual(p.contents.a, 32)  # fails, wat, holds 23
        self.assertEqual(pp.contents.contents.a, 32)  # fails, wat, holds 23


if __name__ == "__main__":
    unittest.main()

import gc
import sys
import unittest
from ctypes import (Structure, Union, pointer, POINTER, sizeof, addressof,
                    c_void_p, c_char_p, c_wchar_p, cast,
                    c_byte, c_short, c_int, c_int16,
                    _pointer_type_cache)
import weakref


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

    def test_cast_detached_structure(self):
        w = weakref.WeakValueDictionary()

        class Struct(Structure):
            _fields_ = [("a", POINTER(c_int))]

        x = Struct(a=pointer(w.setdefault(23,c_int(23))))
        self.assertEqual(x.a.contents.value, 23)

        p = cast(x.a, POINTER(c_int))  # intentinally, cast to same
        p[0] = w.setdefault(32, c_int(32))
        p.contents = w.setdefault(42, c_int(42))
        gc.collect()
        self.assertEqual(p[0], 42)

        del p
        gc.collect()

        self.assertEqual(x.a.contents.value, 32)

    @unittest.expectedFailure  # gh-46376, gh-107131, gh-107940, gh-108222
    def test_casted_structure(self):
        w = weakref.WeakValueDictionary()

        class Struct(Structure):
            _fields_ = [("a", POINTER(c_int))]

        x = Struct(a=pointer(w.setdefault(23, c_int(23))))
        self.assertEqual(x.a.contents.value, 23)

        p = cast(pointer(x), POINTER(POINTER(c_int)))
        p[0] = pointer(w.setdefault(32, c_int(32)))  # legit write to x.a

        # change where pointer is pointing to:
        p.contents = pointer(w.setdefault(42, c_int(42)))
        # p is now pointing to a new object

        gc.collect()
        self.assertEqual(p[0][0], 42)  # c_int(42) should be kept by p._objects

        del p
        gc.collect()

        # c_int(32) should be kept by x._objects, but
        # if c_int(32) was destroyed both of these two asserts should fail
        self.assertIn(32, w)   # but fail here first, so we don't use-after-fee
        self.assertEqual(x.a.contents.value, 32)
        # hint to fix if it fails, basically we need
        # x._objects["bla"] = p._objects

        # to avoid leaking when tests are run several times
        # clean up the types left in the cache.
        del _pointer_type_cache[Struct]

    @unittest.expectedFailure  # gh-46376, gh-107131, gh-107940, gh-108222
    def test_pointer_array(self):
        w = weakref.WeakValueDictionary()
        arr = 3 * POINTER(c_short)

        vals = arr(
            w.setdefault("10", pointer(c_short(10))),
            w.setdefault("11", pointer(c_short(11))),
            None,
        )

        self.assertEqual(vals[0].contents.value, 10)
        self.assertEqual(vals[1].contents.value, 11)

        class Struct(Structure):
            _fields_ = [('a', POINTER(c_short))]

        v = cast(pointer(vals), POINTER(3 * Struct)).contents
        self.assertEqual(addressof(v), addressof(vals))
        self.assertEqual(addressof(v[1]), addressof(vals[1]))

        v[2] = v[0]

        self.assertTrue(v[2])
        self.assertEqual(addressof(v), addressof(vals))
        self.assertEqual(addressof(v[0]), addressof(vals[0]))
        self.assertEqual(addressof(v[1]), addressof(vals[1]))

        self.assertTrue(vals[0])
        self.assertTrue(vals[1])

        self.assertTrue(vals[2])
        self.assertEqual(vals[2].contents.value, 10)

        vals[2] = vals[0]
        vals[2][0] = w.setdefault("12", c_short(12))
        # change where vals[1] is pointing to
        vals[1].contents = w.setdefault("13", c_short(13))
        self.assertEqual(addressof(v[1]), addressof(vals[1]))
        self.assertEqual(addressof(v[0].a.contents), addressof(vals[0].contents))

        self.assertEqual(addressof(v[1]), addressof(vals[1]))

        gc.collect()

        self.assertEqual(vals[2].contents.value, 12)
        self.assertEqual(vals[0].contents.value, 12)

        # again, change where vals[1] is pointing to, because Struct is castable to ptr
        v[1] = Struct(a=pointer(w.setdefault("14", c_short(14))))
        del v

        gc.collect()
        self.assertNotIn("10", w)

        # Struct(a=pointer(c_short(14))) should be kept by somewhere in vals._objects
        # but if it was destroyed both of these two asserts should fail
        self.assertIn("14", w)  # but fail here first, so we don't use-after-fee
        self.assertEqual(vals[1].contents.value, 14)
        # hint to fix if it fails, basically we need
        # vals._objects["bla"] = v._b_base_._objects

    def test_pointer_identity1(self):
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

    def test_cast_structure_array(self):
        w = weakref.WeakValueDictionary()

        class Struct(Structure):
            _fields_ = [('a', c_short)]

        Struct3 = 3 * Struct
        c_array = (2 * Struct3)(
            Struct3(Struct(a=1), Struct(a=2), Struct(a=3)),
            Struct3(Struct(a=4), Struct(a=5), Struct(a=6))
        )
        obj = cast(pointer(c_array), POINTER(pointer(c_array)._type_)).contents

        c_array[0][0] = w.setdefault(100, Struct(a=100))
        gc.collect()

        self.assertEqual(c_array[0][0].a, 100)
        self.assertEqual(obj[0][0].a, 100)

        obj[0][0] = w.setdefault(200, Struct(a=200))
        del obj
        gc.collect()
        self.assertEqual(c_array[0][0].a, 200)

        obj = cast(pointer(c_array), POINTER(pointer(c_array)._type_)).contents
        cast(pointer(c_array), POINTER(c_short))[0] = w.setdefault(
            300, c_short(300),
        )
        gc.collect()

        self.assertEqual(c_array[0][0].a, 300)

        del c_array
        gc.collect()

        self.assertEqual(obj[0][0].a, 300)

    def test_pointer_identity2(self):
        class Struct(Structure):
            _fields_ = [('a', c_int16)]

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

    @unittest.expectedFailure  # gh-46376, gh-107131, gh-107940, gh-108222
    def test_cast_structure_pointer_array(self):
        w = weakref.WeakValueDictionary()

        class Struct(Structure):
            _fields_ = [('a', c_short)]

        StructPointer = POINTER(Struct)
        s1 = w.setdefault(10, Struct(a=10))
        s2 = w.setdefault(20, Struct(a=20))
        s3 = w.setdefault(30, Struct(a=30))
        pointer_array = (3 * StructPointer)(pointer(s1), pointer(s2), pointer(s3))
        del s1
        del s2
        del s3
        gc.collect()

        obj = cast(pointer(pointer_array[0]),
                   POINTER(pointer(pointer_array)._type_)).contents

        pointer_array[0][0].a = 50
        self.assertEqual(obj[0][0].a, 50)

        c = cast(pointer(pointer_array[0]), POINTER(POINTER(c_short)))
        self.assertEqual(c[0][0], 50)
        self.assertEqual(c[0].contents.value, 50)

        # pointer_array[0] = new pointer to 100, castable to Struct(a=100)
        c[0] = pointer(w.setdefault(100, c_short(100)))

        del c
        gc.collect()

        # if c_short(100) was destroyed all three of these two asserts should fail
        self.assertIn(100, w)   # but fail here first, so we don't use-after-fee
        self.assertEqual(pointer_array[0][0].a, 100)
        self.assertEqual(obj[0][0].a, 100)
        # hint to fix if it fails, basically we need
        # pointer_array._objects["bla"] = c._objects

        del obj
        gc.collect()

        r = pointer_array[0]
        del pointer_array
        gc.collect()

        self.assertEqual(r[0].a, 100)

    def test_pointer_identity3(self):
        w = weakref.WeakValueDictionary()

        class Struct(Structure):
            _fields_ = [('a', c_int16)]

        class StructWithPointers(Structure):
            _fields_ = [("s1", POINTER(Struct)), ("s2", POINTER(Struct))]

        s1 = w.setdefault(10, Struct(a=10))
        s2 = w.setdefault(20, Struct(a=20))
        s3 = w.setdefault(30, Struct(a=30))

        StructPointer = POINTER(Struct)

        pointer_array = (3 * StructPointer)(pointer(s1), pointer(s2), pointer(s3))

        struct = StructWithPointers(s1=pointer(s1), s2=pointer(s2))
        del s1
        del s2
        del s3
        gc.collect()

        p_obj = pointer(struct)
        obj = p_obj.contents
        self.assertEqual(addressof(obj), addressof(struct))

        self.assertEqual(obj.s1[0].a, 10)
        self.assertEqual(obj.s2[0].a, 20)
        self.assertEqual(obj.s1.contents.a, 10)
        self.assertEqual(obj.s2.contents.a, 20)

        p_obj = cast(pointer(struct), POINTER(pointer(pointer_array)._type_))
        obj = p_obj.contents
        gc.collect()

        self.assertEqual(addressof(obj), addressof(struct))
        self.assertEqual(addressof(obj[0]), addressof(struct))
        self.assertEqual(addressof(obj[0]), addressof(struct.s1))
        self.assertEqual(addressof(obj[1]), addressof(struct.s2))
        self.assertEqual(addressof(obj[0].contents), addressof(struct.s1.contents))
        del struct
        gc.collect()

        self.assertEqual(addressof(obj[0][0]), addressof(pointer_array[0].contents))
        self.assertEqual(addressof(obj[1][0]), addressof(pointer_array[1].contents))

        self.assertEqual(obj[0][0].a, 10)
        self.assertEqual(obj[1][0].a, 20)
        self.assertEqual(obj[0].contents.a, 10)
        self.assertEqual(obj[1].contents.a, 20)

        obj[0][0].a = 23
        self.assertEqual(pointer_array[0].contents.a, 23)

        obj[0].contents.a = 32
        self.assertEqual(pointer_array[0].contents.a, 32)

        obj[1][0] = w.setdefault(42, Struct(a=42))
        r = pointer_array[1]
        del obj
        del p_obj
        del pointer_array
        gc.collect()

        self.assertEqual(r.contents.a, 42)

    @unittest.expectedFailure  # gh-46376, gh-107131, gh-107940, gh-108222
    def test_pointer_set_contents(self):
        class Struct(Structure):
            _fields_ = [('a', c_int16)]

        w = weakref.WeakValueDictionary()
        p = pointer(w.setdefault("k23", Struct(a=23)))
        self.assertIs(p._type_, Struct)
        self.assertEqual(cast(p, POINTER(c_int16)).contents._type_, 'h')

        pp = pointer(p)
        self.assertIs(pp._type_, POINTER(Struct))
        address = addressof(p.contents)

        cast(pp, POINTER(POINTER(c_int16))).contents.contents = w.setdefault(
            "k32",
            c_int16(32)
        )
        # cast(...).contents returns a new temporary pointer object;
        # setting contents of this temporary pointer doesn't change the original pointer
        # see https://docs.python.org/3/library/ctypes.html#pointers

        self.assertEqual(addressof(p.contents), address)
        self.assertEqual(pp[0].contents.a, 23)
        self.assertEqual(pp.contents[0].a, 23)
        self.assertEqual(pp.contents.contents.a, 23)

        # no ownership changes:
        del pp
        gc.collect()

        # if c_int16(23) was destroyed all of these two asserts should fail
        self.assertIn("k23", w)   # but fail here first, so we don't use-after-fee
        self.assertEqual(p[0].a, 23)
        self.assertEqual(p.contents.a, 23)
        self.assertEqual(cast(p, POINTER(c_int16)).contents.value, 23)

    @unittest.expectedFailure  # gh-46376, gh-107131, gh-107940, gh-108222
    def test_p2p_set_contents(self):
        class StructA(Structure):
            _fields_ = [('a', POINTER(c_int))]
        class StructB(Structure):
            _fields_ = [('b', POINTER(c_int))]

        p = pointer(StructA(a=pointer(c_int(23))))
        self.assertEqual(p.contents.a.contents.value, 23)

        p.contents.a.contents.value = 32
        self.assertEqual(p.contents.a.contents.value, 32)

        w = weakref.WeakValueDictionary()
        p[0].a = pointer(w.setdefault("1", c_int(100)))

        self.assertEqual(p.contents.a.contents.value, 100)
        self.assertEqual(p[0].a.contents.value, 100)

        pp = cast(p, POINTER(POINTER(c_int)))
        pp.contents.contents = w.setdefault("2", c_int(200))
        # pp.contents is a temporary pointer object
        # setting its contents (the address of where it points to) doesn't the original
        # pointer p, see https://docs.python.org/3/library/ctypes.html#pointers

        gc.collect()

        self.assertEqual(p.contents.a.contents.value, 100)
        self.assertEqual(cast(pp, POINTER(StructA)).contents.a.contents.value, 100)
        self.assertEqual(cast(pp, POINTER(StructA))[0].a.contents.value, 100)

        del pp
        gc.collect()

        # if c_int(100) was destroyed all three of these two asserts should fail
        self.assertIn("1", w)   # but fail here first, so we don't use-after-fee
        self.assertEqual(p.contents.a.contents.value, 100)
        self.assertEqual(p[0].a.contents.value, 100)

        x = StructA(a=pointer(w.setdefault("23", c_int(23))))
        r = cast(pointer(x), POINTER(POINTER(c_int)))
        r.contents.contents = w.setdefault("3", c_int(300))
        # no change to x expected

        del r
        gc.collect()

        # if c_int(23) was destroyed both of these two asserts should fail
        self.assertIn("23", w)   # but fail here first, so we don't use-after-fee
        self.assertEqual(x.a[0], 23)

        # this time, we actually legit change x.a[0]
        cast(pointer(x), POINTER(StructB)).contents.b = pointer(
            w.setdefault("4", c_int(400))
        )
        gc.collect()

        self.assertIn("4", w)  # prevent user-after-free if c_int(400) was destroyed
        self.assertEqual(x.a[0], 400)
        # hint to fix if it fails, basically we need
        # tmp = cast(...)...
        # tmp.contents.b = pointer(...)
        # x._objects["bla"] = tmp._objects
        # del tmp

        # to avoid leaking when tests are run several times
        # clean up the types left in the cache.
        del _pointer_type_cache[StructA]
        del _pointer_type_cache[StructB]


if __name__ == "__main__":
    unittest.main()

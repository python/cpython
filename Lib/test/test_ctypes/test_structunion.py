"""Common tests for ctypes.Structure and ctypes.Union"""

import unittest
from ctypes import (Structure, Union, POINTER, sizeof, alignment,
                    c_char, c_byte, c_ubyte,
                    c_short, c_ushort, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_ulonglong, c_float, c_double)
from ._support import (_CData, PyCStructType, UnionType,
                       Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)
from struct import calcsize


class StructUnionTestBase:
    formats = {"c": c_char,
               "b": c_byte,
               "B": c_ubyte,
               "h": c_short,
               "H": c_ushort,
               "i": c_int,
               "I": c_uint,
               "l": c_long,
               "L": c_ulong,
               "q": c_longlong,
               "Q": c_ulonglong,
               "f": c_float,
               "d": c_double,
               }

    def test_subclass(self):
        class X(self.cls):
            _fields_ = [("a", c_int)]

        class Y(X):
            _fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.assertEqual(sizeof(X), sizeof(c_int))
        self.check_sizeof(Y,
                          struct_size=sizeof(c_int)*2,
                          union_size=sizeof(c_int))
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])

    def test_subclass_delayed(self):
        class X(self.cls):
            pass
        self.assertEqual(sizeof(X), 0)
        X._fields_ = [("a", c_int)]

        class Y(X):
            pass
        self.assertEqual(sizeof(Y), sizeof(X))
        Y._fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.assertEqual(sizeof(X), sizeof(c_int))
        self.check_sizeof(Y,
                          struct_size=sizeof(c_int)*2,
                          union_size=sizeof(c_int))
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])

    def test_inheritance_hierarchy(self):
        self.assertEqual(self.cls.mro(), [self.cls, _CData, object])
        self.assertEqual(type(self.metacls), type)

    def test_type_flags(self):
        for cls in self.cls, self.metacls:
            with self.subTest(cls=cls):
                self.assertTrue(cls.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
                self.assertFalse(cls.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

    def test_metaclass_details(self):
        # Abstract classes (whose metaclass __init__ was not called) can't be
        # instantiated directly
        NewClass = self.metacls.__new__(self.metacls, 'NewClass',
                                        (self.cls,), {})
        for cls in self.cls, NewClass:
            with self.subTest(cls=cls):
                with self.assertRaisesRegex(TypeError, "abstract class"):
                    obj = cls()

        # Cannot call the metaclass __init__ more than once
        class T(self.cls):
            _fields_ = [("x", c_char),
                        ("y", c_char)]
        with self.assertRaisesRegex(SystemError, "already initialized"):
            self.metacls.__init__(T, 'ptr', (), {})

    def test_alignment(self):
        class X(self.cls):
            _fields_ = [("x", c_char * 3)]
        self.assertEqual(alignment(X), calcsize("s"))
        self.assertEqual(sizeof(X), calcsize("3s"))

        class Y(self.cls):
            _fields_ = [("x", c_char * 3),
                        ("y", c_int)]
        self.assertEqual(alignment(Y), alignment(c_int))
        self.check_sizeof(Y,
                          struct_size=calcsize("3s i"),
                          union_size=max(calcsize("3s"), calcsize("i")))

        class SI(self.cls):
            _fields_ = [("a", X),
                        ("b", Y)]
        self.assertEqual(alignment(SI), max(alignment(Y), alignment(X)))
        self.check_sizeof(SI,
                          struct_size=calcsize("3s0i 3si 0i"),
                          union_size=max(calcsize("3s"), calcsize("i")))

        class IS(self.cls):
            _fields_ = [("b", Y),
                        ("a", X)]

        self.assertEqual(alignment(SI), max(alignment(X), alignment(Y)))
        self.check_sizeof(IS,
                          struct_size=calcsize("3si 3s 0i"),
                          union_size=max(calcsize("3s"), calcsize("i")))

        class XX(self.cls):
            _fields_ = [("a", X),
                        ("b", X)]
        self.assertEqual(alignment(XX), alignment(X))
        self.check_sizeof(XX,
                          struct_size=calcsize("3s 3s 0s"),
                          union_size=calcsize("3s"))

    def test_empty(self):
        # I had problems with these
        #
        # Although these are pathological cases: Empty Structures!
        class X(self.cls):
            _fields_ = []

        # Is this really the correct alignment, or should it be 0?
        self.assertTrue(alignment(X) == 1)
        self.assertTrue(sizeof(X) == 0)

        class XX(self.cls):
            _fields_ = [("a", X),
                        ("b", X)]

        self.assertEqual(alignment(XX), 1)
        self.assertEqual(sizeof(XX), 0)

    def test_fields(self):
        # test the offset and size attributes of Structure/Union fields.
        class X(self.cls):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.assertEqual(X.x.offset, 0)
        self.assertEqual(X.x.size, sizeof(c_int))

        if self.cls == Structure:
            self.assertEqual(X.y.offset, sizeof(c_int))
        else:
            self.assertEqual(X.y.offset, 0)
        self.assertEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        # XXX Should we check nested data types also?
        # offset is always relative to the class...

    def test_invalid_field_types(self):
        class POINT(self.cls):
            pass
        self.assertRaises(TypeError, setattr, POINT, "_fields_", [("x", 1), ("y", 2)])

    def test_invalid_name(self):
        # field name must be string
        def declare_with_name(name):
            class S(self.cls):
                _fields_ = [(name, c_int)]

        self.assertRaises(TypeError, declare_with_name, b"x")

    def test_intarray_fields(self):
        class SomeInts(self.cls):
            _fields_ = [("a", c_int * 4)]

        # can use tuple to initialize array (but not list!)
        self.assertEqual(SomeInts((1, 2)).a[:], [1, 2, 0, 0])
        self.assertEqual(SomeInts((1, 2)).a[::], [1, 2, 0, 0])
        self.assertEqual(SomeInts((1, 2)).a[::-1], [0, 0, 2, 1])
        self.assertEqual(SomeInts((1, 2)).a[::2], [1, 0])
        self.assertEqual(SomeInts((1, 2)).a[1:5:6], [2])
        self.assertEqual(SomeInts((1, 2)).a[6:4:-1], [])
        self.assertEqual(SomeInts((1, 2, 3, 4)).a[:], [1, 2, 3, 4])
        self.assertEqual(SomeInts((1, 2, 3, 4)).a[::], [1, 2, 3, 4])
        # too long
        # XXX Should raise ValueError?, not RuntimeError
        self.assertRaises(RuntimeError, SomeInts, (1, 2, 3, 4, 5))

    def test_huge_field_name(self):
        # issue12881: segfault with large structure field names
        def create_class(length):
            class S(self.cls):
                _fields_ = [('x' * length, c_int)]

        for length in [10 ** i for i in range(0, 8)]:
            try:
                create_class(length)
            except MemoryError:
                # MemoryErrors are OK, we just don't want to segfault
                pass

    def test_abstract_class(self):
        class X(self.cls):
            _abstract_ = "something"
        with self.assertRaisesRegex(TypeError, r"^abstract class$"):
            X()

    def test_methods(self):
        self.assertIn("in_dll", dir(type(self.cls)))
        self.assertIn("from_address", dir(type(self.cls)))
        self.assertIn("in_dll", dir(type(self.cls)))


class StructureTestCase(unittest.TestCase, StructUnionTestBase):
    cls = Structure
    metacls = PyCStructType

    def test_metaclass_name(self):
        self.assertEqual(self.metacls.__name__, "PyCStructType")

    def check_sizeof(self, cls, *, struct_size, union_size):
        self.assertEqual(sizeof(cls), struct_size)

    def test_simple_structs(self):
        for code, tp in self.formats.items():
            class X(Structure):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                                 (calcsize("c%c0%c" % (code, code)), code))


class UnionTestCase(unittest.TestCase, StructUnionTestBase):
    cls = Union
    metacls = UnionType

    def test_metaclass_name(self):
        self.assertEqual(self.metacls.__name__, "UnionType")

    def check_sizeof(self, cls, *, struct_size, union_size):
        self.assertEqual(sizeof(cls), union_size)

    def test_simple_unions(self):
        for code, tp in self.formats.items():
            class X(Union):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                             (calcsize("%c" % (code)), code))


class PointerMemberTestBase:
    def test(self):
        # a Structure/Union with a POINTER field
        class S(self.cls):
            _fields_ = [("array", POINTER(c_int))]

        s = S()
        # We can assign arrays of the correct type
        s.array = (c_int * 3)(1, 2, 3)
        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

        s.array[0] = 42

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [42, 2, 3])

        s.array[0] = 1

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

class PointerMemberTestCase_Struct(unittest.TestCase, PointerMemberTestBase):
    cls = Structure

    def test_none_to_pointer_fields(self):
        class S(self.cls):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.assertEqual(s.x, 12345678)

class PointerMemberTestCase_Union(unittest.TestCase, PointerMemberTestBase):
    cls = Union

    def test_none_to_pointer_fields(self):
        class S(self.cls):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.assertFalse(s.p)  # NULL pointers are falsy


class TestRecursiveBase:
    def test_contains_itself(self):
        class Recursive(self.cls):
            pass

        try:
            Recursive._fields_ = [("next", Recursive)]
        except AttributeError as details:
            self.assertIn("Structure or union cannot contain itself",
                          str(details))
        else:
            self.fail("Structure or union cannot contain itself")


    def test_vice_versa(self):
        class First(self.cls):
            pass
        class Second(self.cls):
            pass

        First._fields_ = [("second", Second)]

        try:
            Second._fields_ = [("first", First)]
        except AttributeError as details:
            self.assertIn("_fields_ is final", str(details))
        else:
            self.fail("AttributeError not raised")

class TestRecursiveStructure(unittest.TestCase, TestRecursiveBase):
    cls = Structure

class TestRecursiveUnion(unittest.TestCase, TestRecursiveBase):
    cls = Union

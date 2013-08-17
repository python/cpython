import unittest
from ctypes import *
from struct import calcsize
import _testcapi

class SubclassesTest(unittest.TestCase):
    def test_subclass(self):
        class X(Structure):
            _fields_ = [("a", c_int)]

        class Y(X):
            _fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.assertEqual(sizeof(X), sizeof(c_int))
        self.assertEqual(sizeof(Y), sizeof(c_int)*2)
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])

    def test_subclass_delayed(self):
        class X(Structure):
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
        self.assertEqual(sizeof(Y), sizeof(c_int)*2)
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])

class StructureTestCase(unittest.TestCase):
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

    def test_simple_structs(self):
        for code, tp in self.formats.items():
            class X(Structure):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                                 (calcsize("c%c0%c" % (code, code)), code))

    def test_unions(self):
        for code, tp in self.formats.items():
            class X(Union):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                                 (calcsize("%c" % (code)), code))

    def test_struct_alignment(self):
        class X(Structure):
            _fields_ = [("x", c_char * 3)]
        self.assertEqual(alignment(X), calcsize("s"))
        self.assertEqual(sizeof(X), calcsize("3s"))

        class Y(Structure):
            _fields_ = [("x", c_char * 3),
                        ("y", c_int)]
        self.assertEqual(alignment(Y), calcsize("i"))
        self.assertEqual(sizeof(Y), calcsize("3si"))

        class SI(Structure):
            _fields_ = [("a", X),
                        ("b", Y)]
        self.assertEqual(alignment(SI), max(alignment(Y), alignment(X)))
        self.assertEqual(sizeof(SI), calcsize("3s0i 3si 0i"))

        class IS(Structure):
            _fields_ = [("b", Y),
                        ("a", X)]

        self.assertEqual(alignment(SI), max(alignment(X), alignment(Y)))
        self.assertEqual(sizeof(IS), calcsize("3si 3s 0i"))

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]
        self.assertEqual(alignment(XX), alignment(X))
        self.assertEqual(sizeof(XX), calcsize("3s 3s 0s"))

    def test_emtpy(self):
        # I had problems with these
        #
        # Although these are pathological cases: Empty Structures!
        class X(Structure):
            _fields_ = []

        class Y(Union):
            _fields_ = []

        # Is this really the correct alignment, or should it be 0?
        self.assertTrue(alignment(X) == alignment(Y) == 1)
        self.assertTrue(sizeof(X) == sizeof(Y) == 0)

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]

        self.assertEqual(alignment(XX), 1)
        self.assertEqual(sizeof(XX), 0)

    def test_fields(self):
        # test the offset and size attributes of Structure/Unoin fields.
        class X(Structure):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.assertEqual(X.x.offset, 0)
        self.assertEqual(X.x.size, sizeof(c_int))

        self.assertEqual(X.y.offset, sizeof(c_int))
        self.assertEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        class X(Union):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.assertEqual(X.x.offset, 0)
        self.assertEqual(X.x.size, sizeof(c_int))

        self.assertEqual(X.y.offset, 0)
        self.assertEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        # XXX Should we check nested data types also?
        # offset is always relative to the class...

    def test_packed(self):
        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 1

        self.assertEqual(sizeof(X), 9)
        self.assertEqual(X.b.offset, 1)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 2
        self.assertEqual(sizeof(X), 10)
        self.assertEqual(X.b.offset, 2)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 4
        self.assertEqual(sizeof(X), 12)
        self.assertEqual(X.b.offset, 4)

        import struct
        longlong_size = struct.calcsize("q")
        longlong_align = struct.calcsize("bq") - longlong_size

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 8

        self.assertEqual(sizeof(X), longlong_align + longlong_size)
        self.assertEqual(X.b.offset, min(8, longlong_align))


        d = {"_fields_": [("a", "b"),
                          ("b", "q")],
             "_pack_": -1}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)

        # Issue 15989
        d = {"_fields_": [("a", c_byte)],
             "_pack_": _testcapi.INT_MAX + 1}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)
        d = {"_fields_": [("a", c_byte)],
             "_pack_": _testcapi.UINT_MAX + 2}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)

    def test_initializers(self):
        class Person(Structure):
            _fields_ = [("name", c_char*6),
                        ("age", c_int)]

        self.assertRaises(TypeError, Person, 42)
        self.assertRaises(ValueError, Person, "asldkjaslkdjaslkdj")
        self.assertRaises(TypeError, Person, "Name", "HI")

        # short enough
        self.assertEqual(Person("12345", 5).name, "12345")
        # exact fit
        self.assertEqual(Person("123456", 5).name, "123456")
        # too long
        self.assertRaises(ValueError, Person, "1234567", 5)

    def test_conflicting_initializers(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        # conflicting positional and keyword args
        self.assertRaises(TypeError, POINT, 2, 3, x=4)
        self.assertRaises(TypeError, POINT, 2, 3, y=4)

        # too many initializers
        self.assertRaises(TypeError, POINT, 2, 3, 4)

    def test_keyword_initializers(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        pt = POINT(1, 2)
        self.assertEqual((pt.x, pt.y), (1, 2))

        pt = POINT(y=2, x=1)
        self.assertEqual((pt.x, pt.y), (1, 2))

    def test_invalid_field_types(self):
        class POINT(Structure):
            pass
        self.assertRaises(TypeError, setattr, POINT, "_fields_", [("x", 1), ("y", 2)])

    def test_invalid_name(self):
        # field name must be string
        def declare_with_name(name):
            class S(Structure):
                _fields_ = [(name, c_int)]

        self.assertRaises(TypeError, declare_with_name, u"x\xe9")

    def test_intarray_fields(self):
        class SomeInts(Structure):
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

    def test_nested_initializers(self):
        # test initializing nested structures
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        p = Person("Someone", ("1234", "5678"), 5)

        self.assertEqual(p.name, "Someone")
        self.assertEqual(p.phone.areacode, "1234")
        self.assertEqual(p.phone.number, "5678")
        self.assertEqual(p.age, 5)

    def test_structures_with_wchar(self):
        try:
            c_wchar
        except NameError:
            return # no unicode

        class PersonW(Structure):
            _fields_ = [("name", c_wchar * 12),
                        ("age", c_int)]

        p = PersonW(u"Someone")
        self.assertEqual(p.name, "Someone")

        self.assertEqual(PersonW(u"1234567890").name, u"1234567890")
        self.assertEqual(PersonW(u"12345678901").name, u"12345678901")
        # exact fit
        self.assertEqual(PersonW(u"123456789012").name, u"123456789012")
        #too long
        self.assertRaises(ValueError, PersonW, u"1234567890123")

    def test_init_errors(self):
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        cls, msg = self.get_except(Person, "Someone", (1, 2))
        self.assertEqual(cls, RuntimeError)
        # In Python 2.5, Exception is a new-style class, and the repr changed
        if issubclass(Exception, object):
            self.assertEqual(msg,
                                 "(Phone) <type 'exceptions.TypeError'>: "
                                 "expected string or Unicode object, int found")
        else:
            self.assertEqual(msg,
                                 "(Phone) exceptions.TypeError: "
                                 "expected string or Unicode object, int found")

        cls, msg = self.get_except(Person, "Someone", ("a", "b", "c"))
        self.assertEqual(cls, RuntimeError)
        if issubclass(Exception, object):
            self.assertEqual(msg,
                                 "(Phone) <type 'exceptions.TypeError'>: too many initializers")
        else:
            self.assertEqual(msg, "(Phone) exceptions.TypeError: too many initializers")

    def test_huge_field_name(self):
        # issue12881: segfault with large structure field names
        def create_class(length):
            class S(Structure):
                _fields_ = [('x' * length, c_int)]

        for length in [10 ** i for i in range(0, 8)]:
            try:
                create_class(length)
            except MemoryError:
                # MemoryErrors are OK, we just don't want to segfault
                pass

    def get_except(self, func, *args):
        try:
            func(*args)
        except Exception, detail:
            return detail.__class__, str(detail)


##    def test_subclass_creation(self):
##        meta = type(Structure)
##        # same as 'class X(Structure): pass'
##        # fails, since we need either a _fields_ or a _abstract_ attribute
##        cls, msg = self.get_except(meta, "X", (Structure,), {})
##        self.assertEqual((cls, msg),
##                             (AttributeError, "class must define a '_fields_' attribute"))

    def test_abstract_class(self):
        class X(Structure):
            _abstract_ = "something"
        # try 'X()'
        cls, msg = self.get_except(eval, "X()", locals())
        self.assertEqual((cls, msg), (TypeError, "abstract class"))

    def test_methods(self):
##        class X(Structure):
##            _fields_ = []

        self.assertTrue("in_dll" in dir(type(Structure)))
        self.assertTrue("from_address" in dir(type(Structure)))
        self.assertTrue("in_dll" in dir(type(Structure)))

    def test_positional_args(self):
        # see also http://bugs.python.org/issue5042
        class W(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]
        class X(W):
            _fields_ = [("c", c_int)]
        class Y(X):
            pass
        class Z(Y):
            _fields_ = [("d", c_int), ("e", c_int), ("f", c_int)]

        z = Z(1, 2, 3, 4, 5, 6)
        self.assertEqual((z.a, z.b, z.c, z.d, z.e, z.f),
                         (1, 2, 3, 4, 5, 6))
        z = Z(1)
        self.assertEqual((z.a, z.b, z.c, z.d, z.e, z.f),
                         (1, 0, 0, 0, 0, 0))
        self.assertRaises(TypeError, lambda: Z(1, 2, 3, 4, 5, 6, 7))

class PointerMemberTestCase(unittest.TestCase):

    def test(self):
        # a Structure with a POINTER field
        class S(Structure):
            _fields_ = [("array", POINTER(c_int))]

        s = S()
        # We can assign arrays of the correct type
        s.array = (c_int * 3)(1, 2, 3)
        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

        # The following are bugs, but are included here because the unittests
        # also describe the current behaviour.
        #
        # This fails with SystemError: bad arg to internal function
        # or with IndexError (with a patch I have)

        s.array[0] = 42

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [42, 2, 3])

        s.array[0] = 1

##        s.array[1] = 42

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

    def test_none_to_pointer_fields(self):
        class S(Structure):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.assertEqual(s.x, 12345678)

class TestRecursiveStructure(unittest.TestCase):
    def test_contains_itself(self):
        class Recursive(Structure):
            pass

        try:
            Recursive._fields_ = [("next", Recursive)]
        except AttributeError, details:
            self.assertTrue("Structure or union cannot contain itself" in
                            str(details))
        else:
            self.fail("Structure or union cannot contain itself")


    def test_vice_versa(self):
        class First(Structure):
            pass
        class Second(Structure):
            pass

        First._fields_ = [("second", Second)]

        try:
            Second._fields_ = [("first", First)]
        except AttributeError, details:
            self.assertTrue("_fields_ is final" in
                            str(details))
        else:
            self.fail("AttributeError not raised")

if __name__ == '__main__':
    unittest.main()

import unittest
from ctypes import *
from struct import calcsize

class SubclassesTest(unittest.TestCase):
    def test_subclass(self):
        class X(Structure):
            _fields_ = [("a", c_int)]

        class Y(X):
            _fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.failUnlessEqual(sizeof(X), sizeof(c_int))
        self.failUnlessEqual(sizeof(Y), sizeof(c_int)*2)
        self.failUnlessEqual(sizeof(Z), sizeof(c_int))
        self.failUnlessEqual(X._fields_, [("a", c_int)])
        self.failUnlessEqual(Y._fields_, [("b", c_int)])
        self.failUnlessEqual(Z._fields_, [("a", c_int)])

    def test_subclass_delayed(self):
        class X(Structure):
            pass
        self.failUnlessEqual(sizeof(X), 0)
        X._fields_ = [("a", c_int)]

        class Y(X):
            pass
        self.failUnlessEqual(sizeof(Y), sizeof(X))
        Y._fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.failUnlessEqual(sizeof(X), sizeof(c_int))
        self.failUnlessEqual(sizeof(Y), sizeof(c_int)*2)
        self.failUnlessEqual(sizeof(Z), sizeof(c_int))
        self.failUnlessEqual(X._fields_, [("a", c_int)])
        self.failUnlessEqual(Y._fields_, [("b", c_int)])
        self.failUnlessEqual(Z._fields_, [("a", c_int)])

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
            self.failUnlessEqual((sizeof(X), code),
                                 (calcsize("c%c0%c" % (code, code)), code))

    def test_unions(self):
        for code, tp in self.formats.items():
            class X(Union):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.failUnlessEqual((sizeof(X), code),
                                 (calcsize("%c" % (code)), code))

    def test_struct_alignment(self):
        class X(Structure):
            _fields_ = [("x", c_char * 3)]
        self.failUnlessEqual(alignment(X), calcsize("s"))
        self.failUnlessEqual(sizeof(X), calcsize("3s"))

        class Y(Structure):
            _fields_ = [("x", c_char * 3),
                        ("y", c_int)]
        self.failUnlessEqual(alignment(Y), calcsize("i"))
        self.failUnlessEqual(sizeof(Y), calcsize("3si"))

        class SI(Structure):
            _fields_ = [("a", X),
                        ("b", Y)]
        self.failUnlessEqual(alignment(SI), max(alignment(Y), alignment(X)))
        self.failUnlessEqual(sizeof(SI), calcsize("3s0i 3si 0i"))

        class IS(Structure):
            _fields_ = [("b", Y),
                        ("a", X)]

        self.failUnlessEqual(alignment(SI), max(alignment(X), alignment(Y)))
        self.failUnlessEqual(sizeof(IS), calcsize("3si 3s 0i"))

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]
        self.failUnlessEqual(alignment(XX), alignment(X))
        self.failUnlessEqual(sizeof(XX), calcsize("3s 3s 0s"))

    def test_emtpy(self):
        # I had problems with these
        #
        # Although these are patological cases: Empty Structures!
        class X(Structure):
            _fields_ = []

        class Y(Union):
            _fields_ = []

        # Is this really the correct alignment, or should it be 0?
        self.failUnless(alignment(X) == alignment(Y) == 1)
        self.failUnless(sizeof(X) == sizeof(Y) == 0)

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]

        self.failUnlessEqual(alignment(XX), 1)
        self.failUnlessEqual(sizeof(XX), 0)

    def test_fields(self):
        # test the offset and size attributes of Structure/Unoin fields.
        class X(Structure):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.failUnlessEqual(X.x.offset, 0)
        self.failUnlessEqual(X.x.size, sizeof(c_int))

        self.failUnlessEqual(X.y.offset, sizeof(c_int))
        self.failUnlessEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        class X(Union):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.failUnlessEqual(X.x.offset, 0)
        self.failUnlessEqual(X.x.size, sizeof(c_int))

        self.failUnlessEqual(X.y.offset, 0)
        self.failUnlessEqual(X.y.size, sizeof(c_char))

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

        self.failUnlessEqual(sizeof(X), 9)
        self.failUnlessEqual(X.b.offset, 1)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 2
        self.failUnlessEqual(sizeof(X), 10)
        self.failUnlessEqual(X.b.offset, 2)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 4
        self.failUnlessEqual(sizeof(X), 12)
        self.failUnlessEqual(X.b.offset, 4)

        import struct
        longlong_size = struct.calcsize("q")
        longlong_align = struct.calcsize("bq") - longlong_size

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 8

        self.failUnlessEqual(sizeof(X), longlong_align + longlong_size)
        self.failUnlessEqual(X.b.offset, min(8, longlong_align))


        d = {"_fields_": [("a", "b"),
                          ("b", "q")],
             "_pack_": -1}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)

    def test_initializers(self):
        class Person(Structure):
            _fields_ = [("name", c_char*6),
                        ("age", c_int)]

        self.assertRaises(TypeError, Person, 42)
        self.assertRaises(ValueError, Person, "asldkjaslkdjaslkdj")
        self.assertRaises(TypeError, Person, "Name", "HI")

        # short enough
        self.failUnlessEqual(Person("12345", 5).name, "12345")
        # exact fit
        self.failUnlessEqual(Person("123456", 5).name, "123456")
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
        self.failUnlessEqual((pt.x, pt.y), (1, 2))

        pt = POINT(y=2, x=1)
        self.failUnlessEqual((pt.x, pt.y), (1, 2))

    def test_invalid_field_types(self):
        class POINT(Structure):
            pass
        self.assertRaises(TypeError, setattr, POINT, "_fields_", [("x", 1), ("y", 2)])

    def test_intarray_fields(self):
        class SomeInts(Structure):
            _fields_ = [("a", c_int * 4)]

        # can use tuple to initialize array (but not list!)
        self.failUnlessEqual(SomeInts((1, 2)).a[:], [1, 2, 0, 0])
        self.failUnlessEqual(SomeInts((1, 2)).a[::], [1, 2, 0, 0])
        self.failUnlessEqual(SomeInts((1, 2)).a[::-1], [0, 0, 2, 1])
        self.failUnlessEqual(SomeInts((1, 2)).a[::2], [1, 0])
        self.failUnlessEqual(SomeInts((1, 2)).a[1:5:6], [2])
        self.failUnlessEqual(SomeInts((1, 2)).a[6:4:-1], [])
        self.failUnlessEqual(SomeInts((1, 2, 3, 4)).a[:], [1, 2, 3, 4])
        self.failUnlessEqual(SomeInts((1, 2, 3, 4)).a[::], [1, 2, 3, 4])
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

        self.failUnlessEqual(p.name, "Someone")
        self.failUnlessEqual(p.phone.areacode, "1234")
        self.failUnlessEqual(p.phone.number, "5678")
        self.failUnlessEqual(p.age, 5)

    def test_structures_with_wchar(self):
        try:
            c_wchar
        except NameError:
            return # no unicode

        class PersonW(Structure):
            _fields_ = [("name", c_wchar * 12),
                        ("age", c_int)]

        p = PersonW("Someone")
        self.failUnlessEqual(p.name, "Someone")

        self.failUnlessEqual(PersonW("1234567890").name, "1234567890")
        self.failUnlessEqual(PersonW("12345678901").name, "12345678901")
        # exact fit
        self.failUnlessEqual(PersonW("123456789012").name, "123456789012")
        #too long
        self.assertRaises(ValueError, PersonW, "1234567890123")

    def test_init_errors(self):
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        cls, msg = self.get_except(Person, "Someone", (1, 2))
        self.failUnlessEqual(cls, RuntimeError)
        self.failUnlessEqual(msg,
                             "(Phone) <class 'TypeError'>: "
                             "expected string, int found")

        cls, msg = self.get_except(Person, "Someone", ("a", "b", "c"))
        self.failUnlessEqual(cls, RuntimeError)
        if issubclass(Exception, object):
            self.failUnlessEqual(msg,
                                 "(Phone) <class 'TypeError'>: too many initializers")
        else:
            self.failUnlessEqual(msg, "(Phone) TypeError: too many initializers")


    def get_except(self, func, *args):
        try:
            func(*args)
        except Exception as detail:
            return detail.__class__, str(detail)


##    def test_subclass_creation(self):
##        meta = type(Structure)
##        # same as 'class X(Structure): pass'
##        # fails, since we need either a _fields_ or a _abstract_ attribute
##        cls, msg = self.get_except(meta, "X", (Structure,), {})
##        self.failUnlessEqual((cls, msg),
##                             (AttributeError, "class must define a '_fields_' attribute"))

    def test_abstract_class(self):
        class X(Structure):
            _abstract_ = "something"
        # try 'X()'
        cls, msg = self.get_except(eval, "X()", locals())
        self.failUnlessEqual((cls, msg), (TypeError, "abstract class"))

    def test_methods(self):
##        class X(Structure):
##            _fields_ = []

        self.failUnless("in_dll" in dir(type(Structure)))
        self.failUnless("from_address" in dir(type(Structure)))
        self.failUnless("in_dll" in dir(type(Structure)))

class PointerMemberTestCase(unittest.TestCase):

    def test(self):
        # a Structure with a POINTER field
        class S(Structure):
            _fields_ = [("array", POINTER(c_int))]

        s = S()
        # We can assign arrays of the correct type
        s.array = (c_int * 3)(1, 2, 3)
        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [1, 2, 3])

        # The following are bugs, but are included here because the unittests
        # also describe the current behaviour.
        #
        # This fails with SystemError: bad arg to internal function
        # or with IndexError (with a patch I have)

        s.array[0] = 42

        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [42, 2, 3])

        s.array[0] = 1

##        s.array[1] = 42

        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [1, 2, 3])

    def test_none_to_pointer_fields(self):
        class S(Structure):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.failUnlessEqual(s.x, 12345678)

class TestRecursiveStructure(unittest.TestCase):
    def test_contains_itself(self):
        class Recursive(Structure):
            pass

        try:
            Recursive._fields_ = [("next", Recursive)]
        except AttributeError as details:
            self.failUnless("Structure or union cannot contain itself" in
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
        except AttributeError as details:
            self.failUnless("_fields_ is final" in
                            str(details))
        else:
            self.fail("AttributeError not raised")

if __name__ == '__main__':
    unittest.main()

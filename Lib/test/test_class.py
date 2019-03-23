"Test the functionality of Python classes implementing operators."

import unittest


testmeths = [

# Binary operations
    "add",
    "radd",
    "sub",
    "rsub",
    "mul",
    "rmul",
    "matmul",
    "rmatmul",
    "truediv",
    "rtruediv",
    "floordiv",
    "rfloordiv",
    "mod",
    "rmod",
    "divmod",
    "rdivmod",
    "pow",
    "rpow",
    "rshift",
    "rrshift",
    "lshift",
    "rlshift",
    "and",
    "rand",
    "or",
    "ror",
    "xor",
    "rxor",

# List/dict operations
    "contains",
    "getitem",
    "setitem",
    "delitem",

# Unary operations
    "neg",
    "pos",
    "abs",

# generic operations
    "init",
    ]

# These need to return something other than None
#    "hash",
#    "str",
#    "repr",
#    "int",
#    "float",

# These are separate because they can influence the test of other methods.
#    "getattr",
#    "setattr",
#    "delattr",

callLst = []
def trackCall(f):
    def track(*args, **kwargs):
        callLst.append((f.__name__, args))
        return f(*args, **kwargs)
    return track

statictests = """
@trackCall
def __hash__(self, *args):
    return hash(id(self))

@trackCall
def __str__(self, *args):
    return "AllTests"

@trackCall
def __repr__(self, *args):
    return "AllTests"

@trackCall
def __int__(self, *args):
    return 1

@trackCall
def __index__(self, *args):
    return 1

@trackCall
def __float__(self, *args):
    return 1.0

@trackCall
def __eq__(self, *args):
    return True

@trackCall
def __ne__(self, *args):
    return False

@trackCall
def __lt__(self, *args):
    return False

@trackCall
def __le__(self, *args):
    return True

@trackCall
def __gt__(self, *args):
    return False

@trackCall
def __ge__(self, *args):
    return True
"""

# Synthesize all the other AllTests methods from the names in testmeths.

method_template = """\
@trackCall
def __%s__(self, *args):
    pass
"""

d = {}
exec(statictests, globals(), d)
for method in testmeths:
    exec(method_template % method, globals(), d)
AllTests = type("AllTests", (object,), d)
del d, statictests, method, method_template

class ClassTests(unittest.TestCase):
    def setUp(self):
        callLst[:] = []

    def assertCallStack(self, expected_calls):
        actualCallList = callLst[:]  # need to copy because the comparison below will add
                                     # additional calls to callLst
        if expected_calls != actualCallList:
            self.fail("Expected call list:\n  %s\ndoes not match actual call list\n  %s" %
                      (expected_calls, actualCallList))

    def testInit(self):
        foo = AllTests()
        self.assertCallStack([("__init__", (foo,))])

    def testBinaryOps(self):
        testme = AllTests()
        # Binary operations

        callLst[:] = []
        testme + 1
        self.assertCallStack([("__add__", (testme, 1))])

        callLst[:] = []
        1 + testme
        self.assertCallStack([("__radd__", (testme, 1))])

        callLst[:] = []
        testme - 1
        self.assertCallStack([("__sub__", (testme, 1))])

        callLst[:] = []
        1 - testme
        self.assertCallStack([("__rsub__", (testme, 1))])

        callLst[:] = []
        testme * 1
        self.assertCallStack([("__mul__", (testme, 1))])

        callLst[:] = []
        1 * testme
        self.assertCallStack([("__rmul__", (testme, 1))])

        callLst[:] = []
        testme @ 1
        self.assertCallStack([("__matmul__", (testme, 1))])

        callLst[:] = []
        1 @ testme
        self.assertCallStack([("__rmatmul__", (testme, 1))])

        callLst[:] = []
        testme / 1
        self.assertCallStack([("__truediv__", (testme, 1))])


        callLst[:] = []
        1 / testme
        self.assertCallStack([("__rtruediv__", (testme, 1))])

        callLst[:] = []
        testme // 1
        self.assertCallStack([("__floordiv__", (testme, 1))])


        callLst[:] = []
        1 // testme
        self.assertCallStack([("__rfloordiv__", (testme, 1))])

        callLst[:] = []
        testme % 1
        self.assertCallStack([("__mod__", (testme, 1))])

        callLst[:] = []
        1 % testme
        self.assertCallStack([("__rmod__", (testme, 1))])


        callLst[:] = []
        divmod(testme,1)
        self.assertCallStack([("__divmod__", (testme, 1))])

        callLst[:] = []
        divmod(1, testme)
        self.assertCallStack([("__rdivmod__", (testme, 1))])

        callLst[:] = []
        testme ** 1
        self.assertCallStack([("__pow__", (testme, 1))])

        callLst[:] = []
        1 ** testme
        self.assertCallStack([("__rpow__", (testme, 1))])

        callLst[:] = []
        testme >> 1
        self.assertCallStack([("__rshift__", (testme, 1))])

        callLst[:] = []
        1 >> testme
        self.assertCallStack([("__rrshift__", (testme, 1))])

        callLst[:] = []
        testme << 1
        self.assertCallStack([("__lshift__", (testme, 1))])

        callLst[:] = []
        1 << testme
        self.assertCallStack([("__rlshift__", (testme, 1))])

        callLst[:] = []
        testme & 1
        self.assertCallStack([("__and__", (testme, 1))])

        callLst[:] = []
        1 & testme
        self.assertCallStack([("__rand__", (testme, 1))])

        callLst[:] = []
        testme | 1
        self.assertCallStack([("__or__", (testme, 1))])

        callLst[:] = []
        1 | testme
        self.assertCallStack([("__ror__", (testme, 1))])

        callLst[:] = []
        testme ^ 1
        self.assertCallStack([("__xor__", (testme, 1))])

        callLst[:] = []
        1 ^ testme
        self.assertCallStack([("__rxor__", (testme, 1))])

    def testListAndDictOps(self):
        testme = AllTests()

        # List/dict operations

        class Empty: pass

        try:
            1 in Empty()
            self.fail('failed, should have raised TypeError')
        except TypeError:
            pass

        callLst[:] = []
        1 in testme
        self.assertCallStack([('__contains__', (testme, 1))])

        callLst[:] = []
        testme[1]
        self.assertCallStack([('__getitem__', (testme, 1))])

        callLst[:] = []
        testme[1] = 1
        self.assertCallStack([('__setitem__', (testme, 1, 1))])

        callLst[:] = []
        del testme[1]
        self.assertCallStack([('__delitem__', (testme, 1))])

        callLst[:] = []
        testme[:42]
        self.assertCallStack([('__getitem__', (testme, slice(None, 42)))])

        callLst[:] = []
        testme[:42] = "The Answer"
        self.assertCallStack([('__setitem__', (testme, slice(None, 42),
                                               "The Answer"))])

        callLst[:] = []
        del testme[:42]
        self.assertCallStack([('__delitem__', (testme, slice(None, 42)))])

        callLst[:] = []
        testme[2:1024:10]
        self.assertCallStack([('__getitem__', (testme, slice(2, 1024, 10)))])

        callLst[:] = []
        testme[2:1024:10] = "A lot"
        self.assertCallStack([('__setitem__', (testme, slice(2, 1024, 10),
                                                                    "A lot"))])
        callLst[:] = []
        del testme[2:1024:10]
        self.assertCallStack([('__delitem__', (testme, slice(2, 1024, 10)))])

        callLst[:] = []
        testme[:42, ..., :24:, 24, 100]
        self.assertCallStack([('__getitem__', (testme, (slice(None, 42, None),
                                                        Ellipsis,
                                                        slice(None, 24, None),
                                                        24, 100)))])
        callLst[:] = []
        testme[:42, ..., :24:, 24, 100] = "Strange"
        self.assertCallStack([('__setitem__', (testme, (slice(None, 42, None),
                                                        Ellipsis,
                                                        slice(None, 24, None),
                                                        24, 100), "Strange"))])
        callLst[:] = []
        del testme[:42, ..., :24:, 24, 100]
        self.assertCallStack([('__delitem__', (testme, (slice(None, 42, None),
                                                        Ellipsis,
                                                        slice(None, 24, None),
                                                        24, 100)))])

    def testUnaryOps(self):
        testme = AllTests()

        callLst[:] = []
        -testme
        self.assertCallStack([('__neg__', (testme,))])
        callLst[:] = []
        +testme
        self.assertCallStack([('__pos__', (testme,))])
        callLst[:] = []
        abs(testme)
        self.assertCallStack([('__abs__', (testme,))])
        callLst[:] = []
        int(testme)
        self.assertCallStack([('__int__', (testme,))])
        callLst[:] = []
        float(testme)
        self.assertCallStack([('__float__', (testme,))])
        callLst[:] = []
        oct(testme)
        self.assertCallStack([('__index__', (testme,))])
        callLst[:] = []
        hex(testme)
        self.assertCallStack([('__index__', (testme,))])


    def testMisc(self):
        testme = AllTests()

        callLst[:] = []
        hash(testme)
        self.assertCallStack([('__hash__', (testme,))])

        callLst[:] = []
        repr(testme)
        self.assertCallStack([('__repr__', (testme,))])

        callLst[:] = []
        str(testme)
        self.assertCallStack([('__str__', (testme,))])

        callLst[:] = []
        testme == 1
        self.assertCallStack([('__eq__', (testme, 1))])

        callLst[:] = []
        testme < 1
        self.assertCallStack([('__lt__', (testme, 1))])

        callLst[:] = []
        testme > 1
        self.assertCallStack([('__gt__', (testme, 1))])

        callLst[:] = []
        testme != 1
        self.assertCallStack([('__ne__', (testme, 1))])

        callLst[:] = []
        1 == testme
        self.assertCallStack([('__eq__', (1, testme))])

        callLst[:] = []
        1 < testme
        self.assertCallStack([('__gt__', (1, testme))])

        callLst[:] = []
        1 > testme
        self.assertCallStack([('__lt__', (1, testme))])

        callLst[:] = []
        1 != testme
        self.assertCallStack([('__ne__', (1, testme))])


    def testGetSetAndDel(self):
        # Interfering tests
        class ExtraTests(AllTests):
            @trackCall
            def __getattr__(self, *args):
                return "SomeVal"

            @trackCall
            def __setattr__(self, *args):
                pass

            @trackCall
            def __delattr__(self, *args):
                pass

        testme = ExtraTests()

        callLst[:] = []
        testme.spam
        self.assertCallStack([('__getattr__', (testme, "spam"))])

        callLst[:] = []
        testme.eggs = "spam, spam, spam and ham"
        self.assertCallStack([('__setattr__', (testme, "eggs",
                                               "spam, spam, spam and ham"))])

        callLst[:] = []
        del testme.cardinal
        self.assertCallStack([('__delattr__', (testme, "cardinal"))])

    def testDel(self):
        x = []

        class DelTest:
            def __del__(self):
                x.append("crab people, crab people")
        testme = DelTest()
        del testme
        import gc
        gc.collect()
        self.assertEqual(["crab people, crab people"], x)

    def testBadTypeReturned(self):
        # return values of some method are type-checked
        class BadTypeClass:
            def __int__(self):
                return None
            __float__ = __int__
            __complex__ = __int__
            __str__ = __int__
            __repr__ = __int__
            __bytes__ = __int__
            __bool__ = __int__
            __index__ = __int__
        def index(x):
            return [][x]

        for f in [float, complex, str, repr, bytes, bin, oct, hex, bool, index]:
            self.assertRaises(TypeError, f, BadTypeClass())

    def testHashStuff(self):
        # Test correct errors from hash() on objects with comparisons but
        #  no __hash__

        class C0:
            pass

        hash(C0()) # This should work; the next two should raise TypeError

        class C2:
            def __eq__(self, other): return 1

        self.assertRaises(TypeError, hash, C2())


    def testSFBug532646(self):
        # Test for SF bug 532646

        class A:
            pass
        A.__call__ = A()
        a = A()

        try:
            a() # This should not segfault
        except RecursionError:
            pass
        else:
            self.fail("Failed to raise RecursionError")

    def testForExceptionsRaisedInInstanceGetattr2(self):
        # Tests for exceptions raised in instance_getattr2().

        def booh(self):
            raise AttributeError("booh")

        class A:
            a = property(booh)
        try:
            A().a # Raised AttributeError: A instance has no attribute 'a'
        except AttributeError as x:
            if str(x) != "booh":
                self.fail("attribute error for A().a got masked: %s" % x)

        class E:
            __eq__ = property(booh)
        E() == E() # In debug mode, caused a C-level assert() to fail

        class I:
            __init__ = property(booh)
        try:
            # In debug mode, printed XXX undetected error and
            #  raises AttributeError
            I()
        except AttributeError as x:
            pass
        else:
            self.fail("attribute error for I.__init__ got masked")

    def assertNotOrderable(self, a, b):
        with self.assertRaises(TypeError):
            a < b
        with self.assertRaises(TypeError):
            a > b
        with self.assertRaises(TypeError):
            a <= b
        with self.assertRaises(TypeError):
            a >= b

    def testHashComparisonOfMethods(self):
        # Test comparison and hash of methods
        class A:
            def __init__(self, x):
                self.x = x
            def f(self):
                pass
            def g(self):
                pass
            def __eq__(self, other):
                return True
            def __hash__(self):
                raise TypeError
        class B(A):
            pass

        a1 = A(1)
        a2 = A(1)
        self.assertTrue(a1.f == a1.f)
        self.assertFalse(a1.f != a1.f)
        self.assertFalse(a1.f == a2.f)
        self.assertTrue(a1.f != a2.f)
        self.assertFalse(a1.f == a1.g)
        self.assertTrue(a1.f != a1.g)
        self.assertNotOrderable(a1.f, a1.f)
        self.assertEqual(hash(a1.f), hash(a1.f))

        self.assertFalse(A.f == a1.f)
        self.assertTrue(A.f != a1.f)
        self.assertFalse(A.f == A.g)
        self.assertTrue(A.f != A.g)
        self.assertTrue(B.f == A.f)
        self.assertFalse(B.f != A.f)
        self.assertNotOrderable(A.f, A.f)
        self.assertEqual(hash(B.f), hash(A.f))

        # the following triggers a SystemError in 2.4
        a = A(hash(A.f)^(-1))
        hash(a.f)

    def testSetattrWrapperNameIntern(self):
        # Issue #25794: __setattr__ should intern the attribute name
        class A:
            pass

        def add(self, other):
            return 'summa'

        name = str(b'__add__', 'ascii')  # shouldn't be optimized
        self.assertIsNot(name, '__add__')  # not interned
        type.__setattr__(A, name, add)
        self.assertEqual(A() + 1, 'summa')

        name2 = str(b'__add__', 'ascii')
        self.assertIsNot(name2, '__add__')
        self.assertIsNot(name2, name)
        type.__delattr__(A, name2)
        with self.assertRaises(TypeError):
            A() + 1

    def testSetattrNonStringName(self):
        class A:
            pass

        with self.assertRaises(TypeError):
            type.__setattr__(A, b'x', None)

    def testConstructorErrorMessages(self):
        # bpo-31506: Improves the error message logic for object_new & object_init

        # Class without any method overrides
        class C:
            pass

        error_msg = r'C.__init__\(\) takes exactly one argument \(the instance to initialize\)'

        with self.assertRaisesRegex(TypeError, r'C\(\) takes no arguments'):
            C(42)

        with self.assertRaisesRegex(TypeError, r'C\(\) takes no arguments'):
            C.__new__(C, 42)

        with self.assertRaisesRegex(TypeError, error_msg):
            C().__init__(42)

        with self.assertRaisesRegex(TypeError, r'C\(\) takes no arguments'):
            object.__new__(C, 42)

        with self.assertRaisesRegex(TypeError, error_msg):
            object.__init__(C(), 42)

        # Class with both `__init__` & `__new__` method overridden
        class D:
            def __new__(cls, *args, **kwargs):
                super().__new__(cls, *args, **kwargs)
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

        error_msg =  r'object.__new__\(\) takes exactly one argument \(the type to instantiate\)'

        with self.assertRaisesRegex(TypeError, error_msg):
            D(42)

        with self.assertRaisesRegex(TypeError, error_msg):
            D.__new__(D, 42)

        with self.assertRaisesRegex(TypeError, error_msg):
            object.__new__(D, 42)

        # Class that only overrides __init__
        class E:
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

        error_msg = r'object.__init__\(\) takes exactly one argument \(the instance to initialize\)'

        with self.assertRaisesRegex(TypeError, error_msg):
            E().__init__(42)

        with self.assertRaisesRegex(TypeError, error_msg):
            object.__init__(E(), 42)

if __name__ == '__main__':
    unittest.main()

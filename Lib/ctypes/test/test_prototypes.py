from ctypes import *
import unittest

# IMPORTANT INFO:
#
# Consider this call:
#    func.restype = c_char_p
#    func(c_char_p("123"))
# It returns
#    "123"
#
# WHY IS THIS SO?
#
# argument tuple (c_char_p("123"), ) is destroyed after the function
# func is called, but NOT before the result is actually built.
#
# If the arglist would be destroyed BEFORE the result has been built,
# the c_char_p("123") object would already have a zero refcount,
# and the pointer passed to (and returned by) the function would
# probably point to deallocated space.
#
# In this case, there would have to be an additional reference to the argument...

import _ctypes_test
testdll = CDLL(_ctypes_test.__file__)

# Return machine address `a` as a (possibly long) non-negative integer.
# Starting with Python 2.5, id(anything) is always non-negative, and
# the ctypes addressof() inherits that via PyLong_FromVoidPtr().
def positive_address(a):
    if a >= 0:
        return a
    # View the bits in `a` as unsigned instead.
    import struct
    num_bits = struct.calcsize("P") * 8 # num bits in native machine address
    a += 1 << num_bits
    assert a >= 0
    return a

def c_wbuffer(init):
    n = len(init) + 1
    return (c_wchar * n)(*init)

class CharPointersTestCase(unittest.TestCase):

    def setUp(self):
        func = testdll._testfunc_p_p
        func.restype = c_long
        func.argtypes = None

    def test_paramflags(self):
        # function returns c_void_p result,
        # and has a required parameter named 'input'
        prototype = CFUNCTYPE(c_void_p, c_void_p)
        func = prototype(("_testfunc_p_p", testdll),
                         ((1, "input"),))

        try:
            func()
        except TypeError as details:
            self.failUnlessEqual(str(details), "required argument 'input' missing")
        else:
            self.fail("TypeError not raised")

        self.failUnlessEqual(func(None), None)
        self.failUnlessEqual(func(input=None), None)


    def test_int_pointer_arg(self):
        func = testdll._testfunc_p_p
        func.restype = c_long
        self.failUnlessEqual(0, func(0))

        ci = c_int(0)

        func.argtypes = POINTER(c_int),
        self.failUnlessEqual(positive_address(addressof(ci)),
                             positive_address(func(byref(ci))))

        func.argtypes = c_char_p,
        self.assertRaises(ArgumentError, func, byref(ci))

        func.argtypes = POINTER(c_short),
        self.assertRaises(ArgumentError, func, byref(ci))

        func.argtypes = POINTER(c_double),
        self.assertRaises(ArgumentError, func, byref(ci))

    def test_POINTER_c_char_arg(self):
        func = testdll._testfunc_p_p
        func.restype = c_char_p
        func.argtypes = POINTER(c_char),

        self.failUnlessEqual(None, func(None))
        self.failUnlessEqual("123", func("123"))
        self.failUnlessEqual(None, func(c_char_p(None)))
        self.failUnlessEqual("123", func(c_char_p("123")))

        self.failUnlessEqual("123", func(c_buffer("123")))
        ca = c_char("a")
        self.failUnlessEqual("a", func(pointer(ca))[0])
        self.failUnlessEqual("a", func(byref(ca))[0])

    def test_c_char_p_arg(self):
        func = testdll._testfunc_p_p
        func.restype = c_char_p
        func.argtypes = c_char_p,

        self.failUnlessEqual(None, func(None))
        self.failUnlessEqual("123", func("123"))
        self.failUnlessEqual(None, func(c_char_p(None)))
        self.failUnlessEqual("123", func(c_char_p("123")))

        self.failUnlessEqual("123", func(c_buffer("123")))
        ca = c_char("a")
        self.failUnlessEqual("a", func(pointer(ca))[0])
        self.failUnlessEqual("a", func(byref(ca))[0])

    def test_c_void_p_arg(self):
        func = testdll._testfunc_p_p
        func.restype = c_char_p
        func.argtypes = c_void_p,

        self.failUnlessEqual(None, func(None))
        self.failUnlessEqual("123", func(b"123"))
        self.failUnlessEqual("123", func(c_char_p("123")))
        self.failUnlessEqual(None, func(c_char_p(None)))

        self.failUnlessEqual("123", func(c_buffer("123")))
        ca = c_char("a")
        self.failUnlessEqual("a", func(pointer(ca))[0])
        self.failUnlessEqual("a", func(byref(ca))[0])

        func(byref(c_int()))
        func(pointer(c_int()))
        func((c_int * 3)())

        try:
            func.restype = c_wchar_p
        except NameError:
            pass
        else:
            self.failUnlessEqual(None, func(c_wchar_p(None)))
            self.failUnlessEqual("123", func(c_wchar_p("123")))

    def test_instance(self):
        func = testdll._testfunc_p_p
        func.restype = c_void_p

        class X:
            _as_parameter_ = None

        func.argtypes = c_void_p,
        self.failUnlessEqual(None, func(X()))

        func.argtypes = None
        self.failUnlessEqual(None, func(X()))

try:
    c_wchar
except NameError:
    pass
else:
    class WCharPointersTestCase(unittest.TestCase):

        def setUp(self):
            func = testdll._testfunc_p_p
            func.restype = c_int
            func.argtypes = None


        def test_POINTER_c_wchar_arg(self):
            func = testdll._testfunc_p_p
            func.restype = c_wchar_p
            func.argtypes = POINTER(c_wchar),

            self.failUnlessEqual(None, func(None))
            self.failUnlessEqual("123", func("123"))
            self.failUnlessEqual(None, func(c_wchar_p(None)))
            self.failUnlessEqual("123", func(c_wchar_p("123")))

            self.failUnlessEqual("123", func(c_wbuffer("123")))
            ca = c_wchar("a")
            self.failUnlessEqual("a", func(pointer(ca))[0])
            self.failUnlessEqual("a", func(byref(ca))[0])

        def test_c_wchar_p_arg(self):
            func = testdll._testfunc_p_p
            func.restype = c_wchar_p
            func.argtypes = c_wchar_p,

            c_wchar_p.from_param("123")

            self.failUnlessEqual(None, func(None))
            self.failUnlessEqual("123", func("123"))
            self.failUnlessEqual(None, func(c_wchar_p(None)))
            self.failUnlessEqual("123", func(c_wchar_p("123")))

            # XXX Currently, these raise TypeErrors, although they shouldn't:
            self.failUnlessEqual("123", func(c_wbuffer("123")))
            ca = c_wchar("a")
            self.failUnlessEqual("a", func(pointer(ca))[0])
            self.failUnlessEqual("a", func(byref(ca))[0])

class ArrayTest(unittest.TestCase):
    def test(self):
        func = testdll._testfunc_ai8
        func.restype = POINTER(c_int)
        func.argtypes = c_int * 8,

        func((c_int * 8)(1, 2, 3, 4, 5, 6, 7, 8))

        # This did crash before:

        def func(): pass
        CFUNCTYPE(None, c_int * 3)(func)

################################################################

if __name__ == '__main__':
    unittest.main()

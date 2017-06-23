"""This module includes tests of the code object representation.

>>> def f(x):
...     def g(y):
...         return x + y
...     return g
...

>>> dump(f.__code__)
name: f
argcount: 1
kwonlyargcount: 0
names: ()
varnames: ('x', 'g')
cellvars: ('x',)
freevars: ()
nlocals: 2
flags: 3
consts: ('None', '<code object g>', "'f.<locals>.g'")

>>> dump(f(4).__code__)
name: g
argcount: 1
kwonlyargcount: 0
names: ()
varnames: ('y',)
cellvars: ()
freevars: ('x',)
nlocals: 1
flags: 19
consts: ('None',)

>>> def h(x, y):
...     a = x + y
...     b = x - y
...     c = a * b
...     return c
...

>>> dump(h.__code__)
name: h
argcount: 2
kwonlyargcount: 0
names: ()
varnames: ('x', 'y', 'a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 5
flags: 67
consts: ('None',)

>>> def attrs(obj):
...     print(obj.attr1)
...     print(obj.attr2)
...     print(obj.attr3)

>>> dump(attrs.__code__)
name: attrs
argcount: 1
kwonlyargcount: 0
names: ('print', 'attr1', 'attr2', 'attr3')
varnames: ('obj',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 67
consts: ('None',)

>>> def optimize_away():
...     'doc string'
...     'not a docstring'
...     53
...     0x53

>>> dump(optimize_away.__code__)
name: optimize_away
argcount: 0
kwonlyargcount: 0
names: ()
varnames: ()
cellvars: ()
freevars: ()
nlocals: 0
flags: 67
consts: ("'doc string'", 'None')

>>> def keywordonly_args(a,b,*,k1):
...     return a,b,k1
...

>>> dump(keywordonly_args.__code__)
name: keywordonly_args
argcount: 2
kwonlyargcount: 1
names: ()
varnames: ('a', 'b', 'k1')
cellvars: ()
freevars: ()
nlocals: 3
flags: 67
consts: ('None',)

"""

import sys
import threading
import unittest
import weakref
try:
    import ctypes
except ImportError:
    ctypes = None
from test.support import (run_doctest, run_unittest, cpython_only,
                          check_impl_detail)


def consts(t):
    """Yield a doctest-safe sequence of object reprs."""
    for elt in t:
        r = repr(elt)
        if r.startswith("<code object"):
            yield "<code object %s>" % elt.co_name
        else:
            yield r

def dump(co):
    """Print out a text representation of a code object."""
    for attr in ["name", "argcount", "kwonlyargcount", "names", "varnames",
                 "cellvars", "freevars", "nlocals", "flags"]:
        print("%s: %s" % (attr, getattr(co, "co_" + attr)))
    print("consts:", tuple(consts(co.co_consts)))


class CodeTest(unittest.TestCase):

    @cpython_only
    def test_newempty(self):
        import _testcapi
        co = _testcapi.code_newempty("filename", "funcname", 15)
        self.assertEqual(co.co_filename, "filename")
        self.assertEqual(co.co_name, "funcname")
        self.assertEqual(co.co_firstlineno, 15)


def isinterned(s):
    return s is sys.intern(('_' + s + '_')[1:-1])

class CodeConstsTest(unittest.TestCase):

    def find_const(self, consts, value):
        for v in consts:
            if v == value:
                return v
        self.assertIn(value, consts)  # raises an exception
        self.fail('Should never be reached')

    def assertIsInterned(self, s):
        if not isinterned(s):
            self.fail('String %r is not interned' % (s,))

    def assertIsNotInterned(self, s):
        if isinterned(s):
            self.fail('String %r is interned' % (s,))

    @cpython_only
    def test_interned_string(self):
        co = compile('res = "str_value"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str_value')
        self.assertIsInterned(v)

    @cpython_only
    def test_interned_string_in_tuple(self):
        co = compile('res = ("str_value",)', '?', 'exec')
        v = self.find_const(co.co_consts, ('str_value',))
        self.assertIsInterned(v[0])

    @cpython_only
    def test_interned_string_in_frozenset(self):
        co = compile('res = a in {"str_value"}', '?', 'exec')
        v = self.find_const(co.co_consts, frozenset(('str_value',)))
        self.assertIsInterned(tuple(v)[0])

    @cpython_only
    def test_interned_string_default(self):
        def f(a='str_value'):
            return a
        self.assertIsInterned(f())

    @cpython_only
    def test_interned_string_with_null(self):
        co = compile(r'res = "str\0value!"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str\0value!')
        self.assertIsNotInterned(v)


class CodeWeakRefTest(unittest.TestCase):

    def test_basic(self):
        # Create a code object in a clean environment so that we know we have
        # the only reference to it left.
        namespace = {}
        exec("def f(): pass", globals(), namespace)
        f = namespace["f"]
        del namespace

        self.called = False
        def callback(code):
            self.called = True

        # f is now the last reference to the function, and through it, the code
        # object.  While we hold it, check that we can create a weakref and
        # deref it.  Then delete it, and check that the callback gets called and
        # the reference dies.
        coderef = weakref.ref(f.__code__, callback)
        self.assertTrue(bool(coderef()))
        del f
        self.assertFalse(bool(coderef()))
        self.assertTrue(self.called)


if check_impl_detail(cpython=True) and ctypes is not None:
    py = ctypes.pythonapi
    freefunc = ctypes.CFUNCTYPE(None,ctypes.c_voidp)

    RequestCodeExtraIndex = py._PyEval_RequestCodeExtraIndex
    RequestCodeExtraIndex.argtypes = (freefunc,)
    RequestCodeExtraIndex.restype = ctypes.c_ssize_t

    SetExtra = py._PyCode_SetExtra
    SetExtra.argtypes = (ctypes.py_object, ctypes.c_ssize_t, ctypes.c_voidp)
    SetExtra.restype = ctypes.c_int

    GetExtra = py._PyCode_GetExtra
    GetExtra.argtypes = (ctypes.py_object, ctypes.c_ssize_t, 
                         ctypes.POINTER(ctypes.c_voidp))
    GetExtra.restype = ctypes.c_int

    LAST_FREED = None
    def myfree(ptr):
        global LAST_FREED
        LAST_FREED = ptr

    FREE_FUNC = freefunc(myfree)
    FREE_INDEX = RequestCodeExtraIndex(FREE_FUNC)

    class CoExtra(unittest.TestCase):
        def get_func(self):
            # Defining a function causes the containing function to have a
            # reference to the code object.  We need the code objects to go
            # away, so we eval a lambda.
            return eval('lambda:42')

        def test_get_non_code(self):
            f = self.get_func()

            self.assertRaises(SystemError, SetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))
            self.assertRaises(SystemError, GetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))

        def test_bad_index(self):
            f = self.get_func()
            self.assertRaises(SystemError, SetExtra, f.__code__,
                              FREE_INDEX+100, ctypes.c_voidp(100))
            self.assertEqual(GetExtra(f.__code__, FREE_INDEX+100,
                              ctypes.c_voidp(100)), 0)

        def test_free_called(self):
            # Verify that the provided free function gets invoked
            # when the code object is cleaned up.
            f = self.get_func()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(100))
            del f
            self.assertEqual(LAST_FREED, 100)

        def test_get_set(self):
            # Test basic get/set round tripping.
            f = self.get_func()

            extra = ctypes.c_voidp()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(200))
            # reset should free...
            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(300))
            self.assertEqual(LAST_FREED, 200)

            extra = ctypes.c_voidp()
            GetExtra(f.__code__, FREE_INDEX, extra)
            self.assertEqual(extra.value, 300)
            del f

        def test_free_different_thread(self):
            # Freeing a code object on a different thread then
            # where the co_extra was set should be safe.
            f = self.get_func()
            class ThreadTest(threading.Thread):
                def __init__(self, f, test):
                    super().__init__()
                    self.f = f
                    self.test = test
                def run(self):
                    del self.f
                    self.test.assertEqual(LAST_FREED, 500)

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(500))
            tt = ThreadTest(f, self)
            del f
            tt.start()
            tt.join()
            self.assertEqual(LAST_FREED, 500)

def test_main(verbose=None):
    from test import test_code
    run_doctest(test_code, verbose)
    tests = [CodeTest, CodeConstsTest, CodeWeakRefTest]
    if check_impl_detail(cpython=True) and ctypes is not None:
        tests.append(CoExtra)
    run_unittest(*tests)

if __name__ == "__main__":
    test_main()

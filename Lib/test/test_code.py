"""This module includes tests of the code object representation.

>>> def f(x):
...     def g(y):
...         return x + y
...     return g
...

>>> dump(f.func_code)
name: f
argcount: 1
names: ()
varnames: ('x', 'g')
cellvars: ('x',)
freevars: ()
nlocals: 2
flags: 3
consts: ('None', '<code object g>')

>>> dump(f(4).func_code)
name: g
argcount: 1
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
>>> dump(h.func_code)
name: h
argcount: 2
names: ()
varnames: ('x', 'y', 'a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 5
flags: 67
consts: ('None',)

>>> def attrs(obj):
...     print obj.attr1
...     print obj.attr2
...     print obj.attr3

>>> dump(attrs.func_code)
name: attrs
argcount: 1
names: ('attr1', 'attr2', 'attr3')
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
...     53L

>>> dump(optimize_away.func_code)
name: optimize_away
argcount: 0
names: ()
varnames: ()
cellvars: ()
freevars: ()
nlocals: 0
flags: 67
consts: ("'doc string'", 'None')

"""

import unittest
import weakref
from test.test_support import run_doctest, run_unittest, cpython_only


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
    for attr in ["name", "argcount", "names", "varnames", "cellvars",
                 "freevars", "nlocals", "flags"]:
        print "%s: %s" % (attr, getattr(co, "co_" + attr))
    print "consts:", tuple(consts(co.co_consts))


class CodeTest(unittest.TestCase):

    @cpython_only
    def test_newempty(self):
        import _testcapi
        co = _testcapi.code_newempty("filename", "funcname", 15)
        self.assertEqual(co.co_filename, "filename")
        self.assertEqual(co.co_name, "funcname")
        self.assertEqual(co.co_firstlineno, 15)


def isinterned(s):
    return s is intern(('_' + s + '_')[1:-1])

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
        exec "def f(): pass" in globals(), namespace
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


def test_main(verbose=None):
    from test import test_code
    run_doctest(test_code, verbose)
    run_unittest(CodeTest, CodeConstsTest, CodeWeakRefTest)


if __name__ == "__main__":
    test_main()

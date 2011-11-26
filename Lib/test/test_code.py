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

import unittest
import weakref
import _testcapi


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

    def test_newempty(self):
        co = _testcapi.code_newempty("filename", "funcname", 15)
        self.assertEqual(co.co_filename, "filename")
        self.assertEqual(co.co_name, "funcname")
        self.assertEqual(co.co_firstlineno, 15)


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


def test_main(verbose=None):
    from test.support import run_doctest, run_unittest
    from test import test_code
    run_doctest(test_code, verbose)
    run_unittest(CodeTest, CodeWeakRefTest)


if __name__ == "__main__":
    test_main()

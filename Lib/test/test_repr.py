"""
  Test cases for the repr module
  Nick Mathewson
"""

import sys
import os
import unittest

from test_support import run_unittest
from repr import repr as r # Don't shadow builtin repr


def nestedTuple(nesting):
    t = ()
    for i in range(nesting):
        t = (t,)
    return t

class ReprTests(unittest.TestCase):

    def test_string(self):
        eq = self.assertEquals
        eq(r("abc"), "'abc'")
        eq(r("abcdefghijklmnop"),"'abcdefghijklmnop'")

        s = "a"*30+"b"*30
        expected = `s`[:13] + "..." + `s`[-14:]
        eq(r(s), expected)

        eq(r("\"'"), repr("\"'"))
        s = "\""*30+"'"*100
        expected = `s`[:13] + "..." + `s`[-14:]
        eq(r(s), expected)

    def test_container(self):
        eq = self.assertEquals
        # Tuples give up after 6 elements
        eq(r(()), "()")
        eq(r((1,)), "(1,)")
        eq(r((1, 2, 3)), "(1, 2, 3)")
        eq(r((1, 2, 3, 4, 5, 6)), "(1, 2, 3, 4, 5, 6)")
        eq(r((1, 2, 3, 4, 5, 6, 7)), "(1, 2, 3, 4, 5, 6, ...)")

        # Lists give up after 6 as well
        eq(r([]), "[]")
        eq(r([1]), "[1]")
        eq(r([1, 2, 3]), "[1, 2, 3]")
        eq(r([1, 2, 3, 4, 5, 6]), "[1, 2, 3, 4, 5, 6]")
        eq(r([1, 2, 3, 4, 5, 6, 7]), "[1, 2, 3, 4, 5, 6, ...]")

        # Dictionaries give up after 4.
        eq(r({}), "{}")
        d = {'alice': 1, 'bob': 2, 'charles': 3, 'dave': 4}
        eq(r(d), "{'alice': 1, 'bob': 2, 'charles': 3, 'dave': 4}")
        d['arthur'] = 1
        eq(r(d), "{'alice': 1, 'arthur': 1, 'bob': 2, 'charles': 3, ...}")

    def test_numbers(self):
        eq = self.assertEquals
        eq(r(123), repr(123))
        eq(r(123L), repr(123L))
        eq(r(1.0/3), repr(1.0/3))

        n = 10L**100
        expected = `n`[:18] + "..." + `n`[-19:]
        eq(r(n), expected)

    def test_instance(self):
        eq = self.assertEquals
        i1 = ClassWithRepr("a")
        eq(r(i1), repr(i1))

        i2 = ClassWithRepr("x"*1000)
        expected = `i2`[:13] + "..." + `i2`[-14:]
        eq(r(i2), expected)

        i3 = ClassWithFailingRepr()
        eq(r(i3), ("<ClassWithFailingRepr instance at %x>"%id(i3)))

        s = r(ClassWithFailingRepr)
        self.failUnless(s.startswith("<class "))
        self.failUnless(s.endswith(">"))
        self.failUnless(s.find("...") == 8)

    def test_file(self):
        fp = open(unittest.__file__)
        self.failUnless(repr(fp).startswith(
            "<open file '%s', mode 'r' at 0x" % unittest.__file__))
        fp.close()
        self.failUnless(repr(fp).startswith(
            "<closed file '%s', mode 'r' at 0x" % unittest.__file__))

    def test_lambda(self):
        self.failUnless(repr(lambda x: x).startswith(
            "<function <lambda"))
        # XXX anonymous functions?  see func_repr

    def test_builtin_function(self):
        eq = self.assertEquals
        # Functions
        eq(repr(hash), '<built-in function hash>')
        # Methods
        self.failUnless(repr(''.split).startswith(
            '<built-in method split of str object at 0x'))

    def test_xrange(self):
        eq = self.assertEquals
        eq(repr(xrange(1)), 'xrange(1)')
        eq(repr(xrange(1, 2)), 'xrange(1, 2)')
        eq(repr(xrange(1, 2, 3)), 'xrange(1, 4, 3)')
        # Turn off warnings for deprecated multiplication
        import warnings
        warnings.filterwarnings('ignore', category=DeprecationWarning,
                                module=ReprTests.__module__)
        eq(repr(xrange(1) * 3), '(xrange(1) * 3)')

    def test_nesting(self):
        eq = self.assertEquals
        # everything is meant to give up after 6 levels.
        eq(r([[[[[[[]]]]]]]), "[[[[[[[]]]]]]]")
        eq(r([[[[[[[[]]]]]]]]), "[[[[[[[...]]]]]]]")

        eq(r(nestedTuple(6)), "(((((((),),),),),),)")
        eq(r(nestedTuple(7)), "(((((((...),),),),),),)")

        eq(r({ nestedTuple(5) : nestedTuple(5) }),
           "{((((((),),),),),): ((((((),),),),),)}")
        eq(r({ nestedTuple(6) : nestedTuple(6) }),
           "{((((((...),),),),),): ((((((...),),),),),)}")

        eq(r([[[[[[{}]]]]]]), "[[[[[[{}]]]]]]")
        eq(r([[[[[[[{}]]]]]]]), "[[[[[[[...]]]]]]]")

    def test_buffer(self):
        # XXX doesn't test buffers with no b_base or read-write buffers (see
        # bufferobject.c).  The test is fairly incomplete too.  Sigh.
        x = buffer('foo')
        self.failUnless(repr(x).startswith('<read-only buffer for 0x'))

    def test_cell(self):
        # XXX Hmm? How to get at a cell object?
        pass

    def test_descriptors(self):
        eq = self.assertEquals
        # method descriptors
        eq(repr(dict.items), "<method 'items' of 'dict' objects>")
        # XXX member descriptors
        # XXX attribute descriptors
        # XXX slot descriptors
        # static and class methods
        class C:
            def foo(cls): pass
        x = staticmethod(C.foo)
        self.failUnless(repr(x).startswith('<staticmethod object at 0x'))
        x = classmethod(C.foo)
        self.failUnless(repr(x).startswith('<classmethod object at 0x'))

def touch(path, text=''):
    fp = open(path, 'w')
    fp.write(text)
    fp.close()

def zap(actions, dirname, names):
    for name in names:
        actions.append(os.path.join(dirname, name))

class LongReprTest(unittest.TestCase):
    def setUp(self):
        longname = 'areallylongpackageandmodulenametotestreprtruncation'
        self.pkgname = os.path.join(longname)
        self.subpkgname = os.path.join(longname, longname)
        # Make the package and subpackage
        os.mkdir(self.pkgname)
        touch(os.path.join(self.pkgname, '__init__'+os.extsep+'py'))
        os.mkdir(self.subpkgname)
        touch(os.path.join(self.subpkgname, '__init__'+os.extsep+'py'))
        # Remember where we are
        self.here = os.getcwd()
        sys.path.insert(0, self.here)

    def tearDown(self):
        actions = []
        os.path.walk(self.pkgname, zap, actions)
        actions.append(self.pkgname)
        actions.sort()
        actions.reverse()
        for p in actions:
            if os.path.isdir(p):
                os.rmdir(p)
            else:
                os.remove(p)
        del sys.path[0]

    def test_module(self):
        eq = self.assertEquals
        touch(os.path.join(self.subpkgname, self.pkgname + os.extsep + 'py'))
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import areallylongpackageandmodulenametotestreprtruncation
        eq(repr(areallylongpackageandmodulenametotestreprtruncation),
           "<module 'areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation' from '%s'>" % areallylongpackageandmodulenametotestreprtruncation.__file__)
        eq(repr(sys), "<module 'sys' (built-in)>")

    def test_type(self):
        eq = self.assertEquals
        touch(os.path.join(self.subpkgname, 'foo'+os.extsep+'py'), '''\
class foo(object):
    pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import foo
        eq(repr(foo.foo),
               "<class 'areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation.foo.foo'>")

    def test_object(self):
        # XXX Test the repr of a type with a really long tp_name but with no
        # tp_repr.  WIBNI we had ::Inline? :)
        pass

    def test_class(self):
        touch(os.path.join(self.subpkgname, 'bar'+os.extsep+'py'), '''\
class bar:
    pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import bar
        self.failUnless(repr(bar.bar).startswith(
            "<class areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation.bar.bar at 0x"))

    def test_instance(self):
        touch(os.path.join(self.subpkgname, 'baz'+os.extsep+'py'), '''\
class baz:
    pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import baz
        ibaz = baz.baz()
        self.failUnless(repr(ibaz).startswith(
            "<areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation.baz.baz instance at 0x"))

    def test_method(self):
        eq = self.assertEquals
        touch(os.path.join(self.subpkgname, 'qux'+os.extsep+'py'), '''\
class aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:
    def amethod(self): pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import qux
        # Unbound methods first
        eq(repr(qux.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod),
        '<unbound method aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod>')
        # Bound method next
        iqux = qux.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa()
        self.failUnless(repr(iqux.amethod).startswith(
            '<bound method aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod of <areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation.qux.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa instance at 0x'))

    def test_builtin_function(self):
        # XXX test built-in functions and methods with really long names
        pass

class ClassWithRepr:
    def __init__(self, s):
        self.s = s
    def __repr__(self):
        return "ClassWithLongRepr(%r)" % self.s


class ClassWithFailingRepr:
    def __repr__(self):
        raise Exception("This should be caught by Repr.repr_instance")


def test_main():
    run_unittest(ReprTests)
    if os.name != 'mac':
        run_unittest(LongReprTest)


if __name__ == "__main__":
    test_main()

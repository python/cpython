"""
  Test cases for the repr module
  Nick Mathewson
"""

import sys
import os
import shutil
import unittest

from test.test_support import run_unittest, check_py3k_warnings
from repr import repr as r # Don't shadow builtin repr
from repr import Repr


def nestedTuple(nesting):
    t = ()
    for i in range(nesting):
        t = (t,)
    return t

class ReprTests(unittest.TestCase):

    def test_string(self):
        eq = self.assertEqual
        eq(r("abc"), "'abc'")
        eq(r("abcdefghijklmnop"),"'abcdefghijklmnop'")

        s = "a"*30+"b"*30
        expected = repr(s)[:13] + "..." + repr(s)[-14:]
        eq(r(s), expected)

        eq(r("\"'"), repr("\"'"))
        s = "\""*30+"'"*100
        expected = repr(s)[:13] + "..." + repr(s)[-14:]
        eq(r(s), expected)

    def test_tuple(self):
        eq = self.assertEqual
        eq(r((1,)), "(1,)")

        t3 = (1, 2, 3)
        eq(r(t3), "(1, 2, 3)")

        r2 = Repr()
        r2.maxtuple = 2
        expected = repr(t3)[:-2] + "...)"
        eq(r2.repr(t3), expected)

    def test_container(self):
        from array import array
        from collections import deque

        eq = self.assertEqual
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

        # Sets give up after 6 as well
        eq(r(set([])), "set([])")
        eq(r(set([1])), "set([1])")
        eq(r(set([1, 2, 3])), "set([1, 2, 3])")
        eq(r(set([1, 2, 3, 4, 5, 6])), "set([1, 2, 3, 4, 5, 6])")
        eq(r(set([1, 2, 3, 4, 5, 6, 7])), "set([1, 2, 3, 4, 5, 6, ...])")

        # Frozensets give up after 6 as well
        eq(r(frozenset([])), "frozenset([])")
        eq(r(frozenset([1])), "frozenset([1])")
        eq(r(frozenset([1, 2, 3])), "frozenset([1, 2, 3])")
        eq(r(frozenset([1, 2, 3, 4, 5, 6])), "frozenset([1, 2, 3, 4, 5, 6])")
        eq(r(frozenset([1, 2, 3, 4, 5, 6, 7])), "frozenset([1, 2, 3, 4, 5, 6, ...])")

        # collections.deque after 6
        eq(r(deque([1, 2, 3, 4, 5, 6, 7])), "deque([1, 2, 3, 4, 5, 6, ...])")

        # Dictionaries give up after 4.
        eq(r({}), "{}")
        d = {'alice': 1, 'bob': 2, 'charles': 3, 'dave': 4}
        eq(r(d), "{'alice': 1, 'bob': 2, 'charles': 3, 'dave': 4}")
        d['arthur'] = 1
        eq(r(d), "{'alice': 1, 'arthur': 1, 'bob': 2, 'charles': 3, ...}")

        # array.array after 5.
        eq(r(array('i')), "array('i', [])")
        eq(r(array('i', [1])), "array('i', [1])")
        eq(r(array('i', [1, 2])), "array('i', [1, 2])")
        eq(r(array('i', [1, 2, 3])), "array('i', [1, 2, 3])")
        eq(r(array('i', [1, 2, 3, 4])), "array('i', [1, 2, 3, 4])")
        eq(r(array('i', [1, 2, 3, 4, 5])), "array('i', [1, 2, 3, 4, 5])")
        eq(r(array('i', [1, 2, 3, 4, 5, 6])),
                   "array('i', [1, 2, 3, 4, 5, ...])")

    def test_numbers(self):
        eq = self.assertEqual
        eq(r(123), repr(123))
        eq(r(123L), repr(123L))
        eq(r(1.0/3), repr(1.0/3))

        n = 10L**100
        expected = repr(n)[:18] + "..." + repr(n)[-19:]
        eq(r(n), expected)

    def test_instance(self):
        eq = self.assertEqual
        i1 = ClassWithRepr("a")
        eq(r(i1), repr(i1))

        i2 = ClassWithRepr("x"*1000)
        expected = repr(i2)[:13] + "..." + repr(i2)[-14:]
        eq(r(i2), expected)

        i3 = ClassWithFailingRepr()
        eq(r(i3), ("<ClassWithFailingRepr instance at %x>"%id(i3)))

        s = r(ClassWithFailingRepr)
        self.assertTrue(s.startswith("<class "))
        self.assertTrue(s.endswith(">"))
        self.assertTrue(s.find("...") == 8)

    def test_file(self):
        fp = open(unittest.__file__)
        self.assertTrue(repr(fp).startswith(
            "<open file '%s', mode 'r' at 0x" % unittest.__file__))
        fp.close()
        self.assertTrue(repr(fp).startswith(
            "<closed file '%s', mode 'r' at 0x" % unittest.__file__))

    def test_lambda(self):
        self.assertTrue(repr(lambda x: x).startswith(
            "<function <lambda"))
        # XXX anonymous functions?  see func_repr

    def test_builtin_function(self):
        eq = self.assertEqual
        # Functions
        eq(repr(hash), '<built-in function hash>')
        # Methods
        self.assertTrue(repr(''.split).startswith(
            '<built-in method split of str object at 0x'))

    def test_xrange(self):
        eq = self.assertEqual
        eq(repr(xrange(1)), 'xrange(1)')
        eq(repr(xrange(1, 2)), 'xrange(1, 2)')
        eq(repr(xrange(1, 2, 3)), 'xrange(1, 4, 3)')

    def test_nesting(self):
        eq = self.assertEqual
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
        with check_py3k_warnings():
            x = buffer('foo')
        self.assertTrue(repr(x).startswith('<read-only buffer for 0x'))

    def test_cell(self):
        # XXX Hmm? How to get at a cell object?
        pass

    def test_descriptors(self):
        eq = self.assertEqual
        # method descriptors
        eq(repr(dict.items), "<method 'items' of 'dict' objects>")
        # XXX member descriptors
        # XXX attribute descriptors
        # XXX slot descriptors
        # static and class methods
        class C:
            def foo(cls): pass
        x = staticmethod(C.foo)
        self.assertTrue(repr(x).startswith('<staticmethod object at 0x'))
        x = classmethod(C.foo)
        self.assertTrue(repr(x).startswith('<classmethod object at 0x'))

    def test_unsortable(self):
        # Repr.repr() used to call sorted() on sets, frozensets and dicts
        # without taking into account that not all objects are comparable
        x = set([1j, 2j, 3j])
        y = frozenset(x)
        z = {1j: 1, 2j: 2}
        r(x)
        r(y)
        r(z)

def touch(path, text=''):
    fp = open(path, 'w')
    fp.write(text)
    fp.close()

class LongReprTest(unittest.TestCase):
    def setUp(self):
        longname = 'areallylongpackageandmodulenametotestreprtruncation'
        self.pkgname = os.path.join(longname)
        self.subpkgname = os.path.join(longname, longname)
        # Make the package and subpackage
        shutil.rmtree(self.pkgname, ignore_errors=True)
        os.mkdir(self.pkgname)
        touch(os.path.join(self.pkgname, '__init__'+os.extsep+'py'))
        shutil.rmtree(self.subpkgname, ignore_errors=True)
        os.mkdir(self.subpkgname)
        touch(os.path.join(self.subpkgname, '__init__'+os.extsep+'py'))
        # Remember where we are
        self.here = os.getcwd()
        sys.path.insert(0, self.here)

    def tearDown(self):
        actions = []
        for dirpath, dirnames, filenames in os.walk(self.pkgname):
            for name in dirnames + filenames:
                actions.append(os.path.join(dirpath, name))
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
        eq = self.assertEqual
        touch(os.path.join(self.subpkgname, self.pkgname + os.extsep + 'py'))
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import areallylongpackageandmodulenametotestreprtruncation
        eq(repr(areallylongpackageandmodulenametotestreprtruncation),
           "<module '%s' from '%s'>" % (areallylongpackageandmodulenametotestreprtruncation.__name__, areallylongpackageandmodulenametotestreprtruncation.__file__))
        eq(repr(sys), "<module 'sys' (built-in)>")

    def test_type(self):
        eq = self.assertEqual
        touch(os.path.join(self.subpkgname, 'foo'+os.extsep+'py'), '''\
class foo(object):
    pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import foo
        eq(repr(foo.foo),
               "<class '%s.foo'>" % foo.__name__)

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
        # Module name may be prefixed with "test.", depending on how run.
        self.assertTrue(repr(bar.bar).startswith(
            "<class %s.bar at 0x" % bar.__name__))

    def test_instance(self):
        touch(os.path.join(self.subpkgname, 'baz'+os.extsep+'py'), '''\
class baz:
    pass
''')
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import baz
        ibaz = baz.baz()
        self.assertTrue(repr(ibaz).startswith(
            "<%s.baz instance at 0x" % baz.__name__))

    def test_method(self):
        eq = self.assertEqual
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
        self.assertTrue(repr(iqux.amethod).startswith(
            '<bound method aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod of <%s.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa instance at 0x' \
            % (qux.__name__,) ))

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
    run_unittest(LongReprTest)


if __name__ == "__main__":
    test_main()

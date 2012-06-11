"""
  Test cases for the repr module
  Nick Mathewson
"""

import imp
import sys
import os
import shutil
import importlib
import unittest

from test.support import run_unittest, create_empty_file
from reprlib import repr as r # Don't shadow builtin repr
from reprlib import Repr
from reprlib import recursive_repr


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
        eq(r(123), repr(123))
        eq(r(1.0/3), repr(1.0/3))

        n = 10**100
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
        self.assertIn(s.find("..."), [12, 13])

    def test_lambda(self):
        r = repr(lambda x: x)
        self.assertTrue(r.startswith("<function ReprTests.test_lambda.<locals>.<lambda"), r)
        # XXX anonymous functions?  see func_repr

    def test_builtin_function(self):
        eq = self.assertEqual
        # Functions
        eq(repr(hash), '<built-in function hash>')
        # Methods
        self.assertTrue(repr(''.split).startswith(
            '<built-in method split of str object at 0x'))

    def test_range(self):
        eq = self.assertEqual
        eq(repr(range(1)), 'range(0, 1)')
        eq(repr(range(1, 2)), 'range(1, 2)')
        eq(repr(range(1, 4, 3)), 'range(1, 4, 3)')

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

def write_file(path, text):
    with open(path, 'w', encoding='ASCII') as fp:
        fp.write(text)

class LongReprTest(unittest.TestCase):
    longname = 'areallylongpackageandmodulenametotestreprtruncation'

    def setUp(self):
        self.pkgname = os.path.join(self.longname)
        self.subpkgname = os.path.join(self.longname, self.longname)
        # Make the package and subpackage
        shutil.rmtree(self.pkgname, ignore_errors=True)
        os.mkdir(self.pkgname)
        create_empty_file(os.path.join(self.pkgname, '__init__.py'))
        shutil.rmtree(self.subpkgname, ignore_errors=True)
        os.mkdir(self.subpkgname)
        create_empty_file(os.path.join(self.subpkgname, '__init__.py'))
        # Remember where we are
        self.here = os.getcwd()
        sys.path.insert(0, self.here)
        # When regrtest is run with its -j option, this command alone is not
        # enough.
        importlib.invalidate_caches()

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

    def _check_path_limitations(self, module_name):
        # base directory
        source_path_len = len(self.here)
        # a path separator + `longname` (twice)
        source_path_len += 2 * (len(self.longname) + 1)
        # a path separator + `module_name` + ".py"
        source_path_len += len(module_name) + 1 + len(".py")
        cached_path_len = source_path_len + len(imp.cache_from_source("x.py")) - len("x.py")
        if os.name == 'nt' and cached_path_len >= 259:
            # Under Windows, the max path len is 260 including C's terminating
            # NUL character.
            # (see http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx#maxpath)
            self.skipTest("test paths too long (%d characters) for Windows' 260 character limit"
                          % cached_path_len)

    def test_module(self):
        self._check_path_limitations(self.pkgname)
        create_empty_file(os.path.join(self.subpkgname, self.pkgname + '.py'))
        importlib.invalidate_caches()
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import areallylongpackageandmodulenametotestreprtruncation
        module = areallylongpackageandmodulenametotestreprtruncation
        self.assertEqual(repr(module), "<module %r from %r>" % (module.__name__, module.__file__))
        self.assertEqual(repr(sys), "<module 'sys' (built-in)>")

    def test_type(self):
        self._check_path_limitations('foo')
        eq = self.assertEqual
        write_file(os.path.join(self.subpkgname, 'foo.py'), '''\
class foo(object):
    pass
''')
        importlib.invalidate_caches()
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import foo
        eq(repr(foo.foo),
               "<class '%s.foo'>" % foo.__name__)

    def test_object(self):
        # XXX Test the repr of a type with a really long tp_name but with no
        # tp_repr.  WIBNI we had ::Inline? :)
        pass

    def test_class(self):
        self._check_path_limitations('bar')
        write_file(os.path.join(self.subpkgname, 'bar.py'), '''\
class bar:
    pass
''')
        importlib.invalidate_caches()
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import bar
        # Module name may be prefixed with "test.", depending on how run.
        self.assertEqual(repr(bar.bar), "<class '%s.bar'>" % bar.__name__)

    def test_instance(self):
        self._check_path_limitations('baz')
        write_file(os.path.join(self.subpkgname, 'baz.py'), '''\
class baz:
    pass
''')
        importlib.invalidate_caches()
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import baz
        ibaz = baz.baz()
        self.assertTrue(repr(ibaz).startswith(
            "<%s.baz object at 0x" % baz.__name__))

    def test_method(self):
        self._check_path_limitations('qux')
        eq = self.assertEqual
        write_file(os.path.join(self.subpkgname, 'qux.py'), '''\
class aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:
    def amethod(self): pass
''')
        importlib.invalidate_caches()
        from areallylongpackageandmodulenametotestreprtruncation.areallylongpackageandmodulenametotestreprtruncation import qux
        # Unbound methods first
        r = repr(qux.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod)
        self.assertTrue(r.startswith('<function aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod'), r)
        # Bound method next
        iqux = qux.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa()
        r = repr(iqux.amethod)
        self.assertTrue(r.startswith(
            '<bound method aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.amethod of <%s.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa object at 0x' \
            % (qux.__name__,) ), r)

    def test_builtin_function(self):
        # XXX test built-in functions and methods with really long names
        pass

class ClassWithRepr:
    def __init__(self, s):
        self.s = s
    def __repr__(self):
        return "ClassWithRepr(%r)" % self.s


class ClassWithFailingRepr:
    def __repr__(self):
        raise Exception("This should be caught by Repr.repr_instance")

class MyContainer:
    'Helper class for TestRecursiveRepr'
    def __init__(self, values):
        self.values = list(values)
    def append(self, value):
        self.values.append(value)
    @recursive_repr()
    def __repr__(self):
        return '<' + ', '.join(map(str, self.values)) + '>'

class MyContainer2(MyContainer):
    @recursive_repr('+++')
    def __repr__(self):
        return '<' + ', '.join(map(str, self.values)) + '>'

class TestRecursiveRepr(unittest.TestCase):
    def test_recursive_repr(self):
        m = MyContainer(list('abcde'))
        m.append(m)
        m.append('x')
        m.append(m)
        self.assertEqual(repr(m), '<a, b, c, d, e, ..., x, ...>')
        m = MyContainer2(list('abcde'))
        m.append(m)
        m.append('x')
        m.append(m)
        self.assertEqual(repr(m), '<a, b, c, d, e, +++, x, +++>')

def test_main():
    run_unittest(ReprTests)
    run_unittest(LongReprTest)
    run_unittest(TestRecursiveRepr)


if __name__ == "__main__":
    test_main()

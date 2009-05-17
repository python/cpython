import unittest
from test import support
from weakref import proxy, ref, WeakSet
import operator
import copy
import string
import os
from random import randrange, shuffle
import sys
import warnings
import collections
from collections import UserString as ustr


class Foo:
    pass


class TestWeakSet(unittest.TestCase):

    def setUp(self):
        # need to keep references to them
        self.items = [ustr(c) for c in ('a', 'b', 'c')]
        self.items2 = [ustr(c) for c in ('x', 'y', 'z')]
        self.letters = [ustr(c) for c in string.ascii_letters]
        self.s = WeakSet(self.items)
        self.d = dict.fromkeys(self.items)
        self.obj = ustr('F')
        self.fs = WeakSet([self.obj])

    def test_methods(self):
        weaksetmethods = dir(WeakSet)
        for method in dir(set):
            if method == 'test_c_api' or method.startswith('_'):
                continue
            self.assert_(method in weaksetmethods,
                         "WeakSet missing method " + method)

    def test_new_or_init(self):
        self.assertRaises(TypeError, WeakSet, [], 2)

    def test_len(self):
        self.assertEqual(len(self.s), len(self.d))
        self.assertEqual(len(self.fs), 1)
        del self.obj
        self.assertEqual(len(self.fs), 0)

    def test_contains(self):
        for c in self.letters:
            self.assertEqual(c in self.s, c in self.d)
        self.assertRaises(TypeError, self.s.__contains__, [[]])
        self.assert_(self.obj in self.fs)
        del self.obj
        self.assert_(ustr('F') not in self.fs)

    def test_union(self):
        u = self.s.union(self.items2)
        for c in self.letters:
            self.assertEqual(c in u, c in self.d or c in self.items2)
        self.assertEqual(self.s, WeakSet(self.items))
        self.assertEqual(type(u), WeakSet)
        self.assertRaises(TypeError, self.s.union, [[]])
        for C in set, frozenset, dict.fromkeys, list, tuple:
            x = WeakSet(self.items + self.items2)
            c = C(self.items2)
            self.assertEqual(self.s.union(c), x)

    def test_or(self):
        i = self.s.union(self.items2)
        self.assertEqual(self.s | set(self.items2), i)
        self.assertEqual(self.s | frozenset(self.items2), i)

    def test_intersection(self):
        i = self.s.intersection(self.items2)
        for c in self.letters:
            self.assertEqual(c in i, c in self.d and c in self.items2)
        self.assertEqual(self.s, WeakSet(self.items))
        self.assertEqual(type(i), WeakSet)
        for C in set, frozenset, dict.fromkeys, list, tuple:
            x = WeakSet([])
            self.assertEqual(self.s.intersection(C(self.items2)), x)

    def test_isdisjoint(self):
        self.assert_(self.s.isdisjoint(WeakSet(self.items2)))
        self.assert_(not self.s.isdisjoint(WeakSet(self.letters)))

    def test_and(self):
        i = self.s.intersection(self.items2)
        self.assertEqual(self.s & set(self.items2), i)
        self.assertEqual(self.s & frozenset(self.items2), i)

    def test_difference(self):
        i = self.s.difference(self.items2)
        for c in self.letters:
            self.assertEqual(c in i, c in self.d and c not in self.items2)
        self.assertEqual(self.s, WeakSet(self.items))
        self.assertEqual(type(i), WeakSet)
        self.assertRaises(TypeError, self.s.difference, [[]])

    def test_sub(self):
        i = self.s.difference(self.items2)
        self.assertEqual(self.s - set(self.items2), i)
        self.assertEqual(self.s - frozenset(self.items2), i)

    def test_symmetric_difference(self):
        i = self.s.symmetric_difference(self.items2)
        for c in self.letters:
            self.assertEqual(c in i, (c in self.d) ^ (c in self.items2))
        self.assertEqual(self.s, WeakSet(self.items))
        self.assertEqual(type(i), WeakSet)
        self.assertRaises(TypeError, self.s.symmetric_difference, [[]])

    def test_xor(self):
        i = self.s.symmetric_difference(self.items2)
        self.assertEqual(self.s ^ set(self.items2), i)
        self.assertEqual(self.s ^ frozenset(self.items2), i)

    def test_sub_and_super(self):
        pl, ql, rl = map(lambda s: [ustr(c) for c in s], ['ab', 'abcde', 'def'])
        p, q, r = map(WeakSet, (pl, ql, rl))
        self.assert_(p < q)
        self.assert_(p <= q)
        self.assert_(q <= q)
        self.assert_(q > p)
        self.assert_(q >= p)
        self.failIf(q < r)
        self.failIf(q <= r)
        self.failIf(q > r)
        self.failIf(q >= r)
        self.assert_(set('a').issubset('abc'))
        self.assert_(set('abc').issuperset('a'))
        self.failIf(set('a').issubset('cbs'))
        self.failIf(set('cbs').issuperset('a'))

    def test_gc(self):
        # Create a nest of cycles to exercise overall ref count check
        s = WeakSet(Foo() for i in range(1000))
        for elem in s:
            elem.cycle = s
            elem.sub = elem
            elem.set = WeakSet([elem])

    def test_subclass_with_custom_hash(self):
        # Bug #1257731
        class H(WeakSet):
            def __hash__(self):
                return int(id(self) & 0x7fffffff)
        s=H()
        f=set()
        f.add(s)
        self.assert_(s in f)
        f.remove(s)
        f.add(s)
        f.discard(s)

    def test_init(self):
        s = WeakSet()
        s.__init__(self.items)
        self.assertEqual(s, self.s)
        s.__init__(self.items2)
        self.assertEqual(s, WeakSet(self.items2))
        self.assertRaises(TypeError, s.__init__, s, 2);
        self.assertRaises(TypeError, s.__init__, 1);

    def test_constructor_identity(self):
        s = WeakSet(self.items)
        t = WeakSet(s)
        self.assertNotEqual(id(s), id(t))

    def test_hash(self):
        self.assertRaises(TypeError, hash, self.s)

    def test_clear(self):
        self.s.clear()
        self.assertEqual(self.s, WeakSet([]))
        self.assertEqual(len(self.s), 0)

    def test_copy(self):
        dup = self.s.copy()
        self.assertEqual(self.s, dup)
        self.assertNotEqual(id(self.s), id(dup))

    def test_add(self):
        x = ustr('Q')
        self.s.add(x)
        self.assert_(x in self.s)
        dup = self.s.copy()
        self.s.add(x)
        self.assertEqual(self.s, dup)
        self.assertRaises(TypeError, self.s.add, [])
        self.fs.add(Foo())
        self.assert_(len(self.fs) == 1)
        self.fs.add(self.obj)
        self.assert_(len(self.fs) == 1)

    def test_remove(self):
        x = ustr('a')
        self.s.remove(x)
        self.assert_(x not in self.s)
        self.assertRaises(KeyError, self.s.remove, x)
        self.assertRaises(TypeError, self.s.remove, [])

    def test_discard(self):
        a, q = ustr('a'), ustr('Q')
        self.s.discard(a)
        self.assert_(a not in self.s)
        self.s.discard(q)
        self.assertRaises(TypeError, self.s.discard, [])

    def test_pop(self):
        for i in range(len(self.s)):
            elem = self.s.pop()
            self.assert_(elem not in self.s)
        self.assertRaises(KeyError, self.s.pop)

    def test_update(self):
        retval = self.s.update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            self.assert_(c in self.s)
        self.assertRaises(TypeError, self.s.update, [[]])

    def test_update_set(self):
        self.s.update(set(self.items2))
        for c in (self.items + self.items2):
            self.assert_(c in self.s)

    def test_ior(self):
        self.s |= set(self.items2)
        for c in (self.items + self.items2):
            self.assert_(c in self.s)

    def test_intersection_update(self):
        retval = self.s.intersection_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if c in self.items2 and c in self.items:
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)
        self.assertRaises(TypeError, self.s.intersection_update, [[]])

    def test_iand(self):
        self.s &= set(self.items2)
        for c in (self.items + self.items2):
            if c in self.items2 and c in self.items:
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)

    def test_difference_update(self):
        retval = self.s.difference_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if c in self.items and c not in self.items2:
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)
        self.assertRaises(TypeError, self.s.difference_update, [[]])
        self.assertRaises(TypeError, self.s.symmetric_difference_update, [[]])

    def test_isub(self):
        self.s -= set(self.items2)
        for c in (self.items + self.items2):
            if c in self.items and c not in self.items2:
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)

    def test_symmetric_difference_update(self):
        retval = self.s.symmetric_difference_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if (c in self.items) ^ (c in self.items2):
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)
        self.assertRaises(TypeError, self.s.symmetric_difference_update, [[]])

    def test_ixor(self):
        self.s ^= set(self.items2)
        for c in (self.items + self.items2):
            if (c in self.items) ^ (c in self.items2):
                self.assert_(c in self.s)
            else:
                self.assert_(c not in self.s)

    def test_inplace_on_self(self):
        t = self.s.copy()
        t |= t
        self.assertEqual(t, self.s)
        t &= t
        self.assertEqual(t, self.s)
        t -= t
        self.assertEqual(t, WeakSet())
        t = self.s.copy()
        t ^= t
        self.assertEqual(t, WeakSet())

    def test_eq(self):
        # issue 5964
        self.assertTrue(self.s == self.s)
        self.assertTrue(self.s == WeakSet(self.items))
        self.assertFalse(self.s == set(self.items))
        self.assertFalse(self.s == list(self.items))
        self.assertFalse(self.s == tuple(self.items))
        self.assertFalse(self.s == WeakSet([Foo]))
        self.assertFalse(self.s == 1)


def test_main(verbose=None):
    support.run_unittest(TestWeakSet)

if __name__ == "__main__":
    test_main(verbose=True)

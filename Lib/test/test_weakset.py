import unittest
from test import test_support
from weakref import proxy, ref, WeakSet
import operator
import copy
import string
import os
from random import randrange, shuffle
import sys
import warnings
import collections
import gc
import contextlib
from UserString import UserString as ustr


class Foo:
    pass

class SomeClass(object):
    def __init__(self, value):
        self.value = value
    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return other.value == self.value

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash((SomeClass, self.value))

class RefCycle(object):
    def __init__(self):
        self.cycle = self

class TestWeakSet(unittest.TestCase):

    def setUp(self):
        # need to keep references to them
        self.items = [SomeClass(c) for c in ('a', 'b', 'c')]
        self.items2 = [SomeClass(c) for c in ('x', 'y', 'z')]
        self.letters = [SomeClass(c) for c in string.ascii_letters]
        self.ab_items = [SomeClass(c) for c in 'ab']
        self.abcde_items = [SomeClass(c) for c in 'abcde']
        self.def_items = [SomeClass(c) for c in 'def']
        self.ab_weakset = WeakSet(self.ab_items)
        self.abcde_weakset = WeakSet(self.abcde_items)
        self.def_weakset = WeakSet(self.def_items)
        self.s = WeakSet(self.items)
        self.d = dict.fromkeys(self.items)
        self.obj = SomeClass('F')
        self.fs = WeakSet([self.obj])

    def test_methods(self):
        weaksetmethods = dir(WeakSet)
        for method in dir(set):
            if method == 'test_c_api' or method.startswith('_'):
                continue
            self.assertIn(method, weaksetmethods,
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
        # 1 is not weakref'able, but that TypeError is caught by __contains__
        self.assertNotIn(1, self.s)
        self.assertIn(self.obj, self.fs)
        del self.obj
        self.assertNotIn(SomeClass('F'), self.fs)

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
            del c
        self.assertEqual(len(u), len(self.items) + len(self.items2))
        self.items2.pop()
        gc.collect()
        self.assertEqual(len(u), len(self.items) + len(self.items2))

    def test_or(self):
        i = self.s.union(self.items2)
        self.assertEqual(self.s | set(self.items2), i)
        self.assertEqual(self.s | frozenset(self.items2), i)

    def test_intersection(self):
        s = WeakSet(self.letters)
        i = s.intersection(self.items2)
        for c in self.letters:
            self.assertEqual(c in i, c in self.items2 and c in self.letters)
        self.assertEqual(s, WeakSet(self.letters))
        self.assertEqual(type(i), WeakSet)
        for C in set, frozenset, dict.fromkeys, list, tuple:
            x = WeakSet([])
            self.assertEqual(i.intersection(C(self.items)), x)
        self.assertEqual(len(i), len(self.items2))
        self.items2.pop()
        gc.collect()
        self.assertEqual(len(i), len(self.items2))

    def test_isdisjoint(self):
        self.assertTrue(self.s.isdisjoint(WeakSet(self.items2)))
        self.assertTrue(not self.s.isdisjoint(WeakSet(self.letters)))

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
        self.assertEqual(len(i), len(self.items) + len(self.items2))
        self.items2.pop()
        gc.collect()
        self.assertEqual(len(i), len(self.items) + len(self.items2))

    def test_xor(self):
        i = self.s.symmetric_difference(self.items2)
        self.assertEqual(self.s ^ set(self.items2), i)
        self.assertEqual(self.s ^ frozenset(self.items2), i)

    def test_sub_and_super(self):
        self.assertTrue(self.ab_weakset <= self.abcde_weakset)
        self.assertTrue(self.abcde_weakset <= self.abcde_weakset)
        self.assertTrue(self.abcde_weakset >= self.ab_weakset)
        self.assertFalse(self.abcde_weakset <= self.def_weakset)
        self.assertFalse(self.abcde_weakset >= self.def_weakset)
        self.assertTrue(set('a').issubset('abc'))
        self.assertTrue(set('abc').issuperset('a'))
        self.assertFalse(set('a').issubset('cbs'))
        self.assertFalse(set('cbs').issuperset('a'))

    def test_lt(self):
        self.assertTrue(self.ab_weakset < self.abcde_weakset)
        self.assertFalse(self.abcde_weakset < self.def_weakset)
        self.assertFalse(self.ab_weakset < self.ab_weakset)
        self.assertFalse(WeakSet() < WeakSet())

    def test_gt(self):
        self.assertTrue(self.abcde_weakset > self.ab_weakset)
        self.assertFalse(self.abcde_weakset > self.def_weakset)
        self.assertFalse(self.ab_weakset > self.ab_weakset)
        self.assertFalse(WeakSet() > WeakSet())

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
        self.assertIn(s, f)
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
        x = SomeClass('Q')
        self.s.add(x)
        self.assertIn(x, self.s)
        dup = self.s.copy()
        self.s.add(x)
        self.assertEqual(self.s, dup)
        self.assertRaises(TypeError, self.s.add, [])
        self.fs.add(Foo())
        self.assertTrue(len(self.fs) == 1)
        self.fs.add(self.obj)
        self.assertTrue(len(self.fs) == 1)

    def test_remove(self):
        x = SomeClass('a')
        self.s.remove(x)
        self.assertNotIn(x, self.s)
        self.assertRaises(KeyError, self.s.remove, x)
        self.assertRaises(TypeError, self.s.remove, [])

    def test_discard(self):
        a, q = SomeClass('a'), SomeClass('Q')
        self.s.discard(a)
        self.assertNotIn(a, self.s)
        self.s.discard(q)
        self.assertRaises(TypeError, self.s.discard, [])

    def test_pop(self):
        for i in range(len(self.s)):
            elem = self.s.pop()
            self.assertNotIn(elem, self.s)
        self.assertRaises(KeyError, self.s.pop)

    def test_update(self):
        retval = self.s.update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            self.assertIn(c, self.s)
        self.assertRaises(TypeError, self.s.update, [[]])

    def test_update_set(self):
        self.s.update(set(self.items2))
        for c in (self.items + self.items2):
            self.assertIn(c, self.s)

    def test_ior(self):
        self.s |= set(self.items2)
        for c in (self.items + self.items2):
            self.assertIn(c, self.s)

    def test_intersection_update(self):
        retval = self.s.intersection_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if c in self.items2 and c in self.items:
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)
        self.assertRaises(TypeError, self.s.intersection_update, [[]])

    def test_iand(self):
        self.s &= set(self.items2)
        for c in (self.items + self.items2):
            if c in self.items2 and c in self.items:
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)

    def test_difference_update(self):
        retval = self.s.difference_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if c in self.items and c not in self.items2:
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)
        self.assertRaises(TypeError, self.s.difference_update, [[]])
        self.assertRaises(TypeError, self.s.symmetric_difference_update, [[]])

    def test_isub(self):
        self.s -= set(self.items2)
        for c in (self.items + self.items2):
            if c in self.items and c not in self.items2:
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)

    def test_symmetric_difference_update(self):
        retval = self.s.symmetric_difference_update(self.items2)
        self.assertEqual(retval, None)
        for c in (self.items + self.items2):
            if (c in self.items) ^ (c in self.items2):
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)
        self.assertRaises(TypeError, self.s.symmetric_difference_update, [[]])

    def test_ixor(self):
        self.s ^= set(self.items2)
        for c in (self.items + self.items2):
            if (c in self.items) ^ (c in self.items2):
                self.assertIn(c, self.s)
            else:
                self.assertNotIn(c, self.s)

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
        self.assertFalse(self.s == 1)

    def test_ne(self):
        self.assertTrue(self.s != set(self.items))
        s1 = WeakSet()
        s2 = WeakSet()
        self.assertFalse(s1 != s2)

    def test_weak_destroy_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        # Create new items to be sure no-one else holds a reference
        items = [SomeClass(c) for c in ('a', 'b', 'c')]
        s = WeakSet(items)
        it = iter(s)
        next(it)             # Trigger internal iteration
        # Destroy an item
        del items[-1]
        gc.collect()    # just in case
        # We have removed either the first consumed items, or another one
        self.assertIn(len(list(it)), [len(items), len(items) - 1])
        del it
        # The removal has been committed
        self.assertEqual(len(s), len(items))

    def test_weak_destroy_and_mutate_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        items = [SomeClass(c) for c in string.ascii_letters]
        s = WeakSet(items)
        @contextlib.contextmanager
        def testcontext():
            try:
                it = iter(s)
                next(it)
                # Schedule an item for removal and recreate it
                u = SomeClass(str(items.pop()))
                gc.collect()      # just in case
                yield u
            finally:
                it = None           # should commit all removals

        with testcontext() as u:
            self.assertNotIn(u, s)
        with testcontext() as u:
            self.assertRaises(KeyError, s.remove, u)
        self.assertNotIn(u, s)
        with testcontext() as u:
            s.add(u)
        self.assertIn(u, s)
        t = s.copy()
        with testcontext() as u:
            s.update(t)
        self.assertEqual(len(s), len(t))
        with testcontext() as u:
            s.clear()
        self.assertEqual(len(s), 0)

    def test_len_cycles(self):
        N = 20
        items = [RefCycle() for i in range(N)]
        s = WeakSet(items)
        del items
        it = iter(s)
        try:
            next(it)
        except StopIteration:
            pass
        gc.collect()
        n1 = len(s)
        del it
        gc.collect()
        n2 = len(s)
        # one item may be kept alive inside the iterator
        self.assertIn(n1, (0, 1))
        self.assertEqual(n2, 0)

    def test_len_race(self):
        # Extended sanity checks for len() in the face of cyclic collection
        self.addCleanup(gc.set_threshold, *gc.get_threshold())
        for th in range(1, 100):
            N = 20
            gc.collect(0)
            gc.set_threshold(th, th, th)
            items = [RefCycle() for i in range(N)]
            s = WeakSet(items)
            del items
            # All items will be collected at next garbage collection pass
            it = iter(s)
            try:
                next(it)
            except StopIteration:
                pass
            n1 = len(s)
            del it
            n2 = len(s)
            self.assertGreaterEqual(n1, 0)
            self.assertLessEqual(n1, N)
            self.assertGreaterEqual(n2, 0)
            self.assertLessEqual(n2, n1)

    def test_weak_destroy_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        # Create new items to be sure no-one else holds a reference
        items = [ustr(c) for c in ('a', 'b', 'c')]
        s = WeakSet(items)
        it = iter(s)
        next(it)             # Trigger internal iteration
        # Destroy an item
        del items[-1]
        gc.collect()    # just in case
        # We have removed either the first consumed items, or another one
        self.assertIn(len(list(it)), [len(items), len(items) - 1])
        del it
        # The removal has been committed
        self.assertEqual(len(s), len(items))

    def test_weak_destroy_and_mutate_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        items = [ustr(c) for c in string.ascii_letters]
        s = WeakSet(items)
        @contextlib.contextmanager
        def testcontext():
            try:
                it = iter(s)
                # Start iterator
                yielded = ustr(str(next(it)))
                # Schedule an item for removal and recreate it
                u = ustr(str(items.pop()))
                if yielded == u:
                    # The iterator still has a reference to the removed item,
                    # advance it (issue #20006).
                    next(it)
                gc.collect()      # just in case
                yield u
            finally:
                it = None           # should commit all removals

        with testcontext() as u:
            self.assertFalse(u in s)
        with testcontext() as u:
            self.assertRaises(KeyError, s.remove, u)
        self.assertFalse(u in s)
        with testcontext() as u:
            s.add(u)
        self.assertTrue(u in s)
        t = s.copy()
        with testcontext() as u:
            s.update(t)
        self.assertEqual(len(s), len(t))
        with testcontext() as u:
            s.clear()
        self.assertEqual(len(s), 0)


def test_main(verbose=None):
    test_support.run_unittest(TestWeakSet)

if __name__ == "__main__":
    test_main(verbose=True)

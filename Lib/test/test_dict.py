import unittest
from test import test_support

import sys, UserDict, cStringIO


class DictTest(unittest.TestCase):
    def test_constructor(self):
        # calling built-in types without argument must return empty
        self.assertEqual(dict(), {})
        self.assert_(dict() is not {})

    def test_bool(self):
        self.assert_(not {})
        self.assert_({1: 2})
        self.assert_(bool({}) is False)
        self.assert_(bool({1: 2}) is True)

    def test_keys(self):
        d = {}
        self.assertEqual(d.keys(), [])
        d = {'a': 1, 'b': 2}
        k = d.keys()
        self.assert_(d.has_key('a'))
        self.assert_(d.has_key('b'))

        self.assertRaises(TypeError, d.keys, None)

    def test_values(self):
        d = {}
        self.assertEqual(d.values(), [])
        d = {1:2}
        self.assertEqual(d.values(), [2])

        self.assertRaises(TypeError, d.values, None)

    def test_items(self):
        d = {}
        self.assertEqual(d.items(), [])

        d = {1:2}
        self.assertEqual(d.items(), [(1, 2)])

        self.assertRaises(TypeError, d.items, None)

    def test_has_key(self):
        d = {}
        self.assert_(not d.has_key('a'))
        d = {'a': 1, 'b': 2}
        k = d.keys()
        k.sort()
        self.assertEqual(k, ['a', 'b'])

        self.assertRaises(TypeError, d.has_key)

    def test_contains(self):
        d = {}
        self.assert_(not ('a' in d))
        self.assert_('a' not in d)
        d = {'a': 1, 'b': 2}
        self.assert_('a' in d)
        self.assert_('b' in d)
        self.assert_('c' not in d)

        self.assertRaises(TypeError, d.__contains__)

    def test_len(self):
        d = {}
        self.assertEqual(len(d), 0)
        d = {'a': 1, 'b': 2}
        self.assertEqual(len(d), 2)

    def test_getitem(self):
        d = {'a': 1, 'b': 2}
        self.assertEqual(d['a'], 1)
        self.assertEqual(d['b'], 2)
        d['c'] = 3
        d['a'] = 4
        self.assertEqual(d['c'], 3)
        self.assertEqual(d['a'], 4)
        del d['b']
        self.assertEqual(d, {'a': 4, 'c': 3})

        self.assertRaises(TypeError, d.__getitem__)

        class BadEq(object):
            def __eq__(self, other):
                raise Exc()

        d = {}
        d[BadEq()] = 42
        self.assertRaises(KeyError, d.__getitem__, 23)

        class Exc(Exception): pass

        class BadHash(object):
            fail = False
            def __hash__(self):
                if self.fail:
                    raise Exc()
                else:
                    return 42

        x = BadHash()
        d[x] = 42
        x.fail = True
        self.assertRaises(Exc, d.__getitem__, x)

    def test_clear(self):
        d = {1:1, 2:2, 3:3}
        d.clear()
        self.assertEqual(d, {})

        self.assertRaises(TypeError, d.clear, None)

    def test_update(self):
        d = {}
        d.update({1:100})
        d.update({2:20})
        d.update({1:1, 2:2, 3:3})
        self.assertEqual(d, {1:1, 2:2, 3:3})

        d.update()
        self.assertEqual(d, {1:1, 2:2, 3:3})

        self.assertRaises((TypeError, AttributeError), d.update, None)

        class SimpleUserDict:
            def __init__(self):
                self.d = {1:1, 2:2, 3:3}
            def keys(self):
                return self.d.keys()
            def __getitem__(self, i):
                return self.d[i]
        d.clear()
        d.update(SimpleUserDict())
        self.assertEqual(d, {1:1, 2:2, 3:3})

        class Exc(Exception): pass

        d.clear()
        class FailingUserDict:
            def keys(self):
                raise Exc
        self.assertRaises(Exc, d.update, FailingUserDict())

        class FailingUserDict:
            def keys(self):
                class BogonIter:
                    def __init__(self):
                        self.i = 1
                    def __iter__(self):
                        return self
                    def next(self):
                        if self.i:
                            self.i = 0
                            return 'a'
                        raise Exc
                return BogonIter()
            def __getitem__(self, key):
                return key
        self.assertRaises(Exc, d.update, FailingUserDict())

        class FailingUserDict:
            def keys(self):
                class BogonIter:
                    def __init__(self):
                        self.i = ord('a')
                    def __iter__(self):
                        return self
                    def next(self):
                        if self.i <= ord('z'):
                            rtn = chr(self.i)
                            self.i += 1
                            return rtn
                        raise StopIteration
                return BogonIter()
            def __getitem__(self, key):
                raise Exc
        self.assertRaises(Exc, d.update, FailingUserDict())

        class badseq(object):
            def __iter__(self):
                return self
            def next(self):
                raise Exc()

        self.assertRaises(Exc, {}.update, badseq())

        self.assertRaises(ValueError, {}.update, [(1, 2, 3)])

    def test_fromkeys(self):
        self.assertEqual(dict.fromkeys('abc'), {'a':None, 'b':None, 'c':None})
        d = {}
        self.assert_(not(d.fromkeys('abc') is d))
        self.assertEqual(d.fromkeys('abc'), {'a':None, 'b':None, 'c':None})
        self.assertEqual(d.fromkeys((4,5),0), {4:0, 5:0})
        self.assertEqual(d.fromkeys([]), {})
        def g():
            yield 1
        self.assertEqual(d.fromkeys(g()), {1:None})
        self.assertRaises(TypeError, {}.fromkeys, 3)
        class dictlike(dict): pass
        self.assertEqual(dictlike.fromkeys('a'), {'a':None})
        self.assertEqual(dictlike().fromkeys('a'), {'a':None})
        self.assert_(type(dictlike.fromkeys('a')) is dictlike)
        self.assert_(type(dictlike().fromkeys('a')) is dictlike)
        class mydict(dict):
            def __new__(cls):
                return UserDict.UserDict()
        ud = mydict.fromkeys('ab')
        self.assertEqual(ud, {'a':None, 'b':None})
        self.assert_(isinstance(ud, UserDict.UserDict))
        self.assertRaises(TypeError, dict.fromkeys)

        class Exc(Exception): pass

        class baddict1(dict):
            def __init__(self):
                raise Exc()

        self.assertRaises(Exc, baddict1.fromkeys, [1])

        class BadSeq(object):
            def __iter__(self):
                return self
            def next(self):
                raise Exc()

        self.assertRaises(Exc, dict.fromkeys, BadSeq())

        class baddict2(dict):
            def __setitem__(self, key, value):
                raise Exc()

        self.assertRaises(Exc, baddict2.fromkeys, [1])

    def test_copy(self):
        d = {1:1, 2:2, 3:3}
        self.assertEqual(d.copy(), {1:1, 2:2, 3:3})
        self.assertEqual({}.copy(), {})
        self.assertRaises(TypeError, d.copy, None)

    def test_get(self):
        d = {}
        self.assert_(d.get('c') is None)
        self.assertEqual(d.get('c', 3), 3)
        d = {'a' : 1, 'b' : 2}
        self.assert_(d.get('c') is None)
        self.assertEqual(d.get('c', 3), 3)
        self.assertEqual(d.get('a'), 1)
        self.assertEqual(d.get('a', 3), 1)
        self.assertRaises(TypeError, d.get)
        self.assertRaises(TypeError, d.get, None, None, None)

    def test_setdefault(self):
        # dict.setdefault()
        d = {}
        self.assert_(d.setdefault('key0') is None)
        d.setdefault('key0', [])
        self.assert_(d.setdefault('key0') is None)
        d.setdefault('key', []).append(3)
        self.assertEqual(d['key'][0], 3)
        d.setdefault('key', []).append(4)
        self.assertEqual(len(d['key']), 2)
        self.assertRaises(TypeError, d.setdefault)

        class Exc(Exception): pass

        class BadHash(object):
            fail = False
            def __hash__(self):
                if self.fail:
                    raise Exc()
                else:
                    return 42

        x = BadHash()
        d[x] = 42
        x.fail = True
        self.assertRaises(Exc, d.setdefault, x, [])

    def test_popitem(self):
        # dict.popitem()
        for copymode in -1, +1:
            # -1: b has same structure as a
            # +1: b is a.copy()
            for log2size in range(12):
                size = 2**log2size
                a = {}
                b = {}
                for i in range(size):
                    a[repr(i)] = i
                    if copymode < 0:
                        b[repr(i)] = i
                if copymode > 0:
                    b = a.copy()
                for i in range(size):
                    ka, va = ta = a.popitem()
                    self.assertEqual(va, int(ka))
                    kb, vb = tb = b.popitem()
                    self.assertEqual(vb, int(kb))
                    self.assert_(not(copymode < 0 and ta != tb))
                self.assert_(not a)
                self.assert_(not b)

        d = {}
        self.assertRaises(KeyError, d.popitem)

    def test_pop(self):
        # Tests for pop with specified key
        d = {}
        k, v = 'abc', 'def'
        d[k] = v
        self.assertRaises(KeyError, d.pop, 'ghi')

        self.assertEqual(d.pop(k), v)
        self.assertEqual(len(d), 0)

        self.assertRaises(KeyError, d.pop, k)

        # verify longs/ints get same value when key > 32 bits (for 64-bit archs)
        # see SF bug #689659
        x = 4503599627370496L
        y = 4503599627370496
        h = {x: 'anything', y: 'something else'}
        self.assertEqual(h[x], h[y])

        self.assertEqual(d.pop(k, v), v)
        d[k] = v
        self.assertEqual(d.pop(k, 1), v)

        self.assertRaises(TypeError, d.pop)

        class Exc(Exception): pass

        class BadHash(object):
            fail = False
            def __hash__(self):
                if self.fail:
                    raise Exc()
                else:
                    return 42

        x = BadHash()
        d[x] = 42
        x.fail = True
        self.assertRaises(Exc, d.pop, x)

    def test_mutatingiteration(self):
        d = {}
        d[1] = 1
        try:
            for i in d:
                d[i+1] = 1
        except RuntimeError:
            pass
        else:
            self.fail("changing dict size during iteration doesn't raise Error")

    def test_repr(self):
        d = {}
        self.assertEqual(repr(d), '{}')
        d[1] = 2
        self.assertEqual(repr(d), '{1: 2}')
        d = {}
        d[1] = d
        self.assertEqual(repr(d), '{1: {...}}')

        class Exc(Exception): pass

        class BadRepr(object):
            def __repr__(self):
                raise Exc()

        d = {1: BadRepr()}
        self.assertRaises(Exc, repr, d)

    def test_le(self):
        self.assert_(not ({} < {}))
        self.assert_(not ({1: 2} < {1L: 2L}))

        class Exc(Exception): pass

        class BadCmp(object):
            def __eq__(self, other):
                raise Exc()

        d1 = {BadCmp(): 1}
        d2 = {1: 1}
        try:
            d1 < d2
        except Exc:
            pass
        else:
            self.fail("< didn't raise Exc")

import mapping_tests

class GeneralMappingTests(mapping_tests.BasicTestMappingProtocol):
    type2test = dict

class Dict(dict):
    pass

class SubclassMappingTests(mapping_tests.BasicTestMappingProtocol):
    type2test = Dict

def test_main():
    test_support.run_unittest(
        DictTest,
        GeneralMappingTests,
        SubclassMappingTests,
    )

if __name__ == "__main__":
    test_main()

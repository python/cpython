# Check every path through every method of UserDict

from collections import UserDict
from test import mapping_tests
import unittest
import collections
import types


class UserDictSubclass(UserDict):
    pass

class UserDictSubclass2(UserDict):
    pass


d0 = {}
d1 = {"one": 1}
d2 = {"one": 1, "two": 2}
d3 = {"one": 1, "two": 3, "three": 5}
d4 = {"one": None, "two": None}
d5 = {"one": 1, "two": 1}

class UserDictTest(mapping_tests.TestHashMappingProtocol):
    type2test = collections.UserDict

    def test_all(self):
        # Test constructors
        u = collections.UserDict()
        u0 = collections.UserDict(d0)
        u1 = collections.UserDict(d1)
        u2 = collections.UserDict(d2)

        uu = collections.UserDict(u)
        uu0 = collections.UserDict(u0)
        uu1 = collections.UserDict(u1)
        uu2 = collections.UserDict(u2)

        # keyword arg constructor
        self.assertEqual(collections.UserDict(one=1, two=2), d2)
        # item sequence constructor
        self.assertEqual(collections.UserDict([('one',1), ('two',2)]), d2)
        self.assertEqual(collections.UserDict(dict=[('one',1), ('two',2)]),
                         {'dict': [('one', 1), ('two', 2)]})
        # both together
        self.assertEqual(collections.UserDict([('one',1), ('two',2)], two=3, three=5), d3)

        # alternate constructor
        self.assertEqual(collections.UserDict.fromkeys('one two'.split()), d4)
        self.assertEqual(collections.UserDict().fromkeys('one two'.split()), d4)
        self.assertEqual(collections.UserDict.fromkeys('one two'.split(), 1), d5)
        self.assertEqual(collections.UserDict().fromkeys('one two'.split(), 1), d5)
        self.assertTrue(u1.fromkeys('one two'.split()) is not u1)
        self.assertIsInstance(u1.fromkeys('one two'.split()), collections.UserDict)
        self.assertIsInstance(u2.fromkeys('one two'.split()), collections.UserDict)

        # Test __repr__
        self.assertEqual(str(u0), str(d0))
        self.assertEqual(repr(u1), repr(d1))
        self.assertIn(repr(u2), ("{'one': 1, 'two': 2}",
                                 "{'two': 2, 'one': 1}"))

        # Test rich comparison and __len__
        all = [d0, d1, d2, u, u0, u1, u2, uu, uu0, uu1, uu2]
        for a in all:
            for b in all:
                self.assertEqual(a == b, len(a) == len(b))

        # Test __getitem__
        self.assertEqual(u2["one"], 1)
        self.assertRaises(KeyError, u1.__getitem__, "two")

        # Test __setitem__
        u3 = collections.UserDict(u2)
        u3["two"] = 2
        u3["three"] = 3

        # Test __delitem__
        del u3["three"]
        self.assertRaises(KeyError, u3.__delitem__, "three")

        # Test clear
        u3.clear()
        self.assertEqual(u3, {})

        # Test copy()
        u2a = u2.copy()
        self.assertEqual(u2a, u2)
        u2b = collections.UserDict(x=42, y=23)
        u2c = u2b.copy() # making a copy of a UserDict is special cased
        self.assertEqual(u2b, u2c)

        class MyUserDict(collections.UserDict):
            def display(self): print(self)

        m2 = MyUserDict(u2)
        m2a = m2.copy()
        self.assertEqual(m2a, m2)

        # SF bug #476616 -- copy() of UserDict subclass shared data
        m2['foo'] = 'bar'
        self.assertNotEqual(m2a, m2)

        # Test keys, items, values
        self.assertEqual(sorted(u2.keys()), sorted(d2.keys()))
        self.assertEqual(sorted(u2.items()), sorted(d2.items()))
        self.assertEqual(sorted(u2.values()), sorted(d2.values()))

        # Test "in".
        for i in u2.keys():
            self.assertIn(i, u2)
            self.assertEqual(i in u1, i in d1)
            self.assertEqual(i in u0, i in d0)

        # Test update
        t = collections.UserDict()
        t.update(u2)
        self.assertEqual(t, u2)

        # Test get
        for i in u2.keys():
            self.assertEqual(u2.get(i), u2[i])
            self.assertEqual(u1.get(i), d1.get(i))
            self.assertEqual(u0.get(i), d0.get(i))

        # Test "in" iteration.
        for i in range(20):
            u2[i] = str(i)
        ikeys = []
        for k in u2:
            ikeys.append(k)
        keys = u2.keys()
        self.assertEqual(set(ikeys), set(keys))

        # Test setdefault
        t = collections.UserDict()
        self.assertEqual(t.setdefault("x", 42), 42)
        self.assertIn("x", t)
        self.assertEqual(t.setdefault("x", 23), 42)

        # Test pop
        t = collections.UserDict(x=42)
        self.assertEqual(t.pop("x"), 42)
        self.assertRaises(KeyError, t.pop, "x")
        self.assertEqual(t.pop("x", 1), 1)
        t["x"] = 42
        self.assertEqual(t.pop("x", 1), 42)

        # Test popitem
        t = collections.UserDict(x=42)
        self.assertEqual(t.popitem(), ("x", 42))
        self.assertRaises(KeyError, t.popitem)

    def test_init(self):
        for kw in 'self', 'other', 'iterable':
            self.assertEqual(list(collections.UserDict(**{kw: 42}).items()),
                             [(kw, 42)])
        self.assertEqual(list(collections.UserDict({}, dict=42).items()),
                         [('dict', 42)])
        self.assertEqual(list(collections.UserDict({}, dict=None).items()),
                         [('dict', None)])
        self.assertEqual(list(collections.UserDict(dict={'a': 42}).items()),
                         [('dict', {'a': 42})])
        self.assertRaises(TypeError, collections.UserDict, 42)
        self.assertRaises(TypeError, collections.UserDict, (), ())
        self.assertRaises(TypeError, collections.UserDict.__init__)

    def test_data(self):
        u = UserDict()
        self.assertEqual(u.data, {})
        self.assertIs(type(u.data), dict)
        d = {'a': 1, 'b': 2}
        u = UserDict(d)
        self.assertEqual(u.data, d)
        self.assertIsNot(u.data, d)
        self.assertIs(type(u.data), dict)
        u = UserDict(u)
        self.assertEqual(u.data, d)
        self.assertIs(type(u.data), dict)
        u = UserDict([('a', 1), ('b', 2)])
        self.assertEqual(u.data, d)
        self.assertIs(type(u.data), dict)
        u = UserDict(a=1, b=2)
        self.assertEqual(u.data, d)
        self.assertIs(type(u.data), dict)

    def test_update(self):
        for kw in 'self', 'dict', 'other', 'iterable':
            d = collections.UserDict()
            d.update(**{kw: 42})
            self.assertEqual(list(d.items()), [(kw, 42)])
        self.assertRaises(TypeError, collections.UserDict().update, 42)
        self.assertRaises(TypeError, collections.UserDict().update, {}, {})
        self.assertRaises(TypeError, collections.UserDict.update)

    def test_missing(self):
        # Make sure UserDict doesn't have a __missing__ method
        self.assertNotHasAttr(collections.UserDict, "__missing__")
        # Test several cases:
        # (D) subclass defines __missing__ method returning a value
        # (E) subclass defines __missing__ method raising RuntimeError
        # (F) subclass sets __missing__ instance variable (no effect)
        # (G) subclass doesn't define __missing__ at all
        class D(collections.UserDict):
            def __missing__(self, key):
                return 42
        d = D({1: 2, 3: 4})
        self.assertEqual(d[1], 2)
        self.assertEqual(d[3], 4)
        self.assertNotIn(2, d)
        self.assertNotIn(2, d.keys())
        self.assertEqual(d[2], 42)
        class E(collections.UserDict):
            def __missing__(self, key):
                raise RuntimeError(key)
        e = E()
        try:
            e[42]
        except RuntimeError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("e[42] didn't raise RuntimeError")
        class F(collections.UserDict):
            def __init__(self):
                # An instance variable __missing__ should have no effect
                self.__missing__ = lambda key: None
                collections.UserDict.__init__(self)
        f = F()
        try:
            f[42]
        except KeyError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("f[42] didn't raise KeyError")
        class G(collections.UserDict):
            pass
        g = G()
        try:
            g[42]
        except KeyError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("g[42] didn't raise KeyError")

    test_repr_deep = mapping_tests.TestHashMappingProtocol.test_repr_deep

    def test_mixed_or(self):
        for t in UserDict, dict, types.MappingProxyType:
            with self.subTest(t.__name__):
                u = UserDict({0: 'a', 1: 'b'}) | t({1: 'c', 2: 'd'})
                self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
                self.assertIs(type(u), UserDict)

                u = t({0: 'a', 1: 'b'}) | UserDict({1: 'c', 2: 'd'})
                self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
                self.assertIs(type(u), UserDict)

        u = UserDict({0: 'a', 1: 'b'}) | UserDictSubclass({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDict)

        u = UserDictSubclass({0: 'a', 1: 'b'}) | UserDict({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDictSubclass)

        u = UserDictSubclass({0: 'a', 1: 'b'}) |  UserDictSubclass2({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDictSubclass)

        u = UserDict({1: 'c', 2: 'd'}).__ror__(UserDict({0: 'a', 1: 'b'}))
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDict)

        u = UserDictSubclass({1: 'c', 2: 'd'}).__ror__(UserDictSubclass2({0: 'a', 1: 'b'}))
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDictSubclass)

    def test_mixed_ior(self):
        for t in UserDict, dict, types.MappingProxyType:
            with self.subTest(t.__name__):
                u = u2 = UserDict({0: 'a', 1: 'b'})
                u |= t({1: 'c', 2: 'd'})
                self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
                self.assertIs(type(u), UserDict)
                self.assertIs(u, u2)

        u = dict({0: 'a', 1: 'b'})
        u |= UserDict({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), dict)

        u = u2 = UserDict({0: 'a', 1: 'b'})
        u |= UserDictSubclass({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDict)
        self.assertIs(u, u2)

        u = u2 = UserDictSubclass({0: 'a', 1: 'b'})
        u |= UserDict({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDictSubclass)
        self.assertIs(u, u2)

        u = u2 = UserDictSubclass({0: 'a', 1: 'b'})
        u |= UserDictSubclass2({1: 'c', 2: 'd'})
        self.assertEqual(u, {0: 'a', 1: 'c', 2: 'd'})
        self.assertIs(type(u), UserDictSubclass)
        self.assertIs(u, u2)


if __name__ == "__main__":
    unittest.main()

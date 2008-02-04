# Check every path through every method of UserDict

import unittest
from test import test_support, mapping_tests
import UserDict

d0 = {}
d1 = {"one": 1}
d2 = {"one": 1, "two": 2}
d3 = {"one": 1, "two": 3, "three": 5}
d4 = {"one": None, "two": None}
d5 = {"one": 1, "two": 1}

class UserDictTest(mapping_tests.TestHashMappingProtocol):
    type2test = UserDict.IterableUserDict

    def test_all(self):
        # Test constructors
        u = UserDict.UserDict()
        u0 = UserDict.UserDict(d0)
        u1 = UserDict.UserDict(d1)
        u2 = UserDict.IterableUserDict(d2)

        uu = UserDict.UserDict(u)
        uu0 = UserDict.UserDict(u0)
        uu1 = UserDict.UserDict(u1)
        uu2 = UserDict.UserDict(u2)

        # keyword arg constructor
        self.assertEqual(UserDict.UserDict(one=1, two=2), d2)
        # item sequence constructor
        self.assertEqual(UserDict.UserDict([('one',1), ('two',2)]), d2)
        self.assertEqual(UserDict.UserDict(dict=[('one',1), ('two',2)]), d2)
        # both together
        self.assertEqual(UserDict.UserDict([('one',1), ('two',2)], two=3, three=5), d3)

        # alternate constructor
        self.assertEqual(UserDict.UserDict.fromkeys('one two'.split()), d4)
        self.assertEqual(UserDict.UserDict().fromkeys('one two'.split()), d4)
        self.assertEqual(UserDict.UserDict.fromkeys('one two'.split(), 1), d5)
        self.assertEqual(UserDict.UserDict().fromkeys('one two'.split(), 1), d5)
        self.assert_(u1.fromkeys('one two'.split()) is not u1)
        self.assert_(isinstance(u1.fromkeys('one two'.split()), UserDict.UserDict))
        self.assert_(isinstance(u2.fromkeys('one two'.split()), UserDict.IterableUserDict))

        # Test __repr__
        self.assertEqual(str(u0), str(d0))
        self.assertEqual(repr(u1), repr(d1))
        self.assertEqual(repr(u2), repr(d2))

        # Test __cmp__ and __len__
        all = [d0, d1, d2, u, u0, u1, u2, uu, uu0, uu1, uu2]
        for a in all:
            for b in all:
                self.assertEqual(a == b, len(a) == len(b))

        # Test __getitem__
        self.assertEqual(u2["one"], 1)
        self.assertRaises(KeyError, u1.__getitem__, "two")

        # Test __setitem__
        u3 = UserDict.UserDict(u2)
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
        u2b = UserDict.UserDict(x=42, y=23)
        u2c = u2b.copy() # making a copy of a UserDict is special cased
        self.assertEqual(u2b, u2c)

        class MyUserDict(UserDict.UserDict):
            def display(self): print(self)

        m2 = MyUserDict(u2)
        m2a = m2.copy()
        self.assertEqual(m2a, m2)

        # SF bug #476616 -- copy() of UserDict subclass shared data
        m2['foo'] = 'bar'
        self.assertNotEqual(m2a, m2)

        # Test keys, items, values
        self.assertEqual(u2.keys(), d2.keys())
        self.assertEqual(u2.items(), d2.items())
        self.assertEqual(list(u2.values()), list(d2.values()))

        # Test "in".
        for i in u2.keys():
            self.assert_(i in u2)
            self.assertEqual(i in u1, i in d1)
            self.assertEqual(i in u0, i in d0)

        # Test update
        t = UserDict.UserDict()
        t.update(u2)
        self.assertEqual(t, u2)
        class Items:
            def items(self):
                return (("x", 42), ("y", 23))
        t = UserDict.UserDict()
        t.update(Items())
        self.assertEqual(t, {"x": 42, "y": 23})

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
        t = UserDict.UserDict()
        self.assertEqual(t.setdefault("x", 42), 42)
        self.assert_("x" in t)
        self.assertEqual(t.setdefault("x", 23), 42)

        # Test pop
        t = UserDict.UserDict(x=42)
        self.assertEqual(t.pop("x"), 42)
        self.assertRaises(KeyError, t.pop, "x")
        self.assertEqual(t.pop("x", 1), 1)
        t["x"] = 42
        self.assertEqual(t.pop("x", 1), 42)

        # Test popitem
        t = UserDict.UserDict(x=42)
        self.assertEqual(t.popitem(), ("x", 42))
        self.assertRaises(KeyError, t.popitem)

    def test_missing(self):
        # Make sure UserDict doesn't have a __missing__ method
        self.assertEqual(hasattr(UserDict, "__missing__"), False)
        # Test several cases:
        # (D) subclass defines __missing__ method returning a value
        # (E) subclass defines __missing__ method raising RuntimeError
        # (F) subclass sets __missing__ instance variable (no effect)
        # (G) subclass doesn't define __missing__ at a all
        class D(UserDict.UserDict):
            def __missing__(self, key):
                return 42
        d = D({1: 2, 3: 4})
        self.assertEqual(d[1], 2)
        self.assertEqual(d[3], 4)
        self.assert_(2 not in d)
        self.assert_(2 not in d.keys())
        self.assertEqual(d[2], 42)
        class E(UserDict.UserDict):
            def __missing__(self, key):
                raise RuntimeError(key)
        e = E()
        try:
            e[42]
        except RuntimeError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("e[42] didn't raise RuntimeError")
        class F(UserDict.UserDict):
            def __init__(self):
                # An instance variable __missing__ should have no effect
                self.__missing__ = lambda key: None
                UserDict.UserDict.__init__(self)
        f = F()
        try:
            f[42]
        except KeyError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("f[42] didn't raise KeyError")
        class G(UserDict.UserDict):
            pass
        g = G()
        try:
            g[42]
        except KeyError as err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("g[42] didn't raise KeyError")



def test_main():
    test_support.run_unittest(
        UserDictTest,
    )

if __name__ == "__main__":
    test_main()

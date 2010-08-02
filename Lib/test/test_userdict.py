# Check every path through every method of UserDict

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
                self.assertEqual(cmp(a, b), cmp(len(a), len(b)))

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
            def display(self): print self

        m2 = MyUserDict(u2)
        m2a = m2.copy()
        self.assertEqual(m2a, m2)

        # SF bug #476616 -- copy() of UserDict subclass shared data
        m2['foo'] = 'bar'
        self.assertNotEqual(m2a, m2)

        # Test keys, items, values
        self.assertEqual(u2.keys(), d2.keys())
        self.assertEqual(u2.items(), d2.items())
        self.assertEqual(u2.values(), d2.values())

        # Test has_key and "in".
        for i in u2.keys():
            self.assertTrue(i in u2)
            self.assertEqual(i in u1, i in d1)
            self.assertEqual(i in u0, i in d0)
            with test_support._check_py3k_warnings():
                self.assertTrue(u2.has_key(i))
                self.assertEqual(u1.has_key(i), d1.has_key(i))
                self.assertEqual(u0.has_key(i), d0.has_key(i))

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
        for i in xrange(20):
            u2[i] = str(i)
        ikeys = []
        for k in u2:
            ikeys.append(k)
        keys = u2.keys()
        self.assertEqual(set(ikeys), set(keys))

        # Test setdefault
        t = UserDict.UserDict()
        self.assertEqual(t.setdefault("x", 42), 42)
        self.assert_(t.has_key("x"))
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
        except RuntimeError, err:
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
        except KeyError, err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("f[42] didn't raise KeyError")
        class G(UserDict.UserDict):
            pass
        g = G()
        try:
            g[42]
        except KeyError, err:
            self.assertEqual(err.args, (42,))
        else:
            self.fail("g[42] didn't raise KeyError")

##########################
# Test Dict Mixin

class SeqDict(UserDict.DictMixin):
    """Dictionary lookalike implemented with lists.

    Used to test and demonstrate DictMixin
    """
    def __init__(self, other=None, **kwargs):
        self.keylist = []
        self.valuelist = []
        if other is not None:
            for (key, value) in other:
                self[key] = value
        for (key, value) in kwargs.iteritems():
            self[key] = value
    def __getitem__(self, key):
        try:
            i = self.keylist.index(key)
        except ValueError:
            raise KeyError
        return self.valuelist[i]
    def __setitem__(self, key, value):
        try:
            i = self.keylist.index(key)
            self.valuelist[i] = value
        except ValueError:
            self.keylist.append(key)
            self.valuelist.append(value)
    def __delitem__(self, key):
        try:
            i = self.keylist.index(key)
        except ValueError:
            raise KeyError
        self.keylist.pop(i)
        self.valuelist.pop(i)
    def keys(self):
        return list(self.keylist)
    def copy(self):
        d = self.__class__()
        for key, value in self.iteritems():
            d[key] = value
        return d
    @classmethod
    def fromkeys(cls, keys, value=None):
        d = cls()
        for key in keys:
            d[key] = value
        return d

class UserDictMixinTest(mapping_tests.TestMappingProtocol):
    type2test = SeqDict

    def test_all(self):
        ## Setup test and verify working of the test class

        # check init
        s = SeqDict()

        # exercise setitem
        s[10] = 'ten'
        s[20] = 'twenty'
        s[30] = 'thirty'

        # exercise delitem
        del s[20]
        # check getitem and setitem
        self.assertEqual(s[10], 'ten')
        # check keys() and delitem
        self.assertEqual(s.keys(), [10, 30])

        ## Now, test the DictMixin methods one by one
        # has_key
        self.assert_(s.has_key(10))
        self.assert_(not s.has_key(20))

        # __contains__
        self.assert_(10 in s)
        self.assert_(20 not in s)

        # __iter__
        self.assertEqual([k for k in s], [10, 30])

        # __len__
        self.assertEqual(len(s), 2)

        # iteritems
        self.assertEqual(list(s.iteritems()), [(10,'ten'), (30, 'thirty')])

        # iterkeys
        self.assertEqual(list(s.iterkeys()), [10, 30])

        # itervalues
        self.assertEqual(list(s.itervalues()), ['ten', 'thirty'])

        # values
        self.assertEqual(s.values(), ['ten', 'thirty'])

        # items
        self.assertEqual(s.items(), [(10,'ten'), (30, 'thirty')])

        # get
        self.assertEqual(s.get(10), 'ten')
        self.assertEqual(s.get(15,'fifteen'), 'fifteen')
        self.assertEqual(s.get(15), None)

        # setdefault
        self.assertEqual(s.setdefault(40, 'forty'), 'forty')
        self.assertEqual(s.setdefault(10, 'null'), 'ten')
        del s[40]

        # pop
        self.assertEqual(s.pop(10), 'ten')
        self.assert_(10 not in s)
        s[10] = 'ten'
        self.assertEqual(s.pop("x", 1), 1)
        s["x"] = 42
        self.assertEqual(s.pop("x", 1), 42)

        # popitem
        k, v = s.popitem()
        self.assert_(k not in s)
        s[k] = v

        # clear
        s.clear()
        self.assertEqual(len(s), 0)

        # empty popitem
        self.assertRaises(KeyError, s.popitem)

        # update
        s.update({10: 'ten', 20:'twenty'})
        self.assertEqual(s[10], 'ten')
        self.assertEqual(s[20], 'twenty')

        # cmp
        self.assertEqual(s, {10: 'ten', 20:'twenty'})
        t = SeqDict()
        t[20] = 'twenty'
        t[10] = 'ten'
        self.assertEqual(s, t)

def test_main():
    test_support.run_unittest(
        UserDictTest,
        UserDictMixinTest
    )

if __name__ == "__main__":
    test_main()

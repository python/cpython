# Check every path through every method of UserDict

import test.test_support, unittest
from sets import Set

import UserDict

class TestMappingProtocol(unittest.TestCase):
    # This base class can be used to check that an object conforms to the
    # mapping protocol

    # Functions that can be useful to override to adapt to dictionary
    # semantics
    _tested_class = dict   # which class is being tested

    def _reference(self):
        """Return a dictionary of values which are invariant by storage
        in the object under test."""
        return {1:2, "key1":"value1", "key2":(1,2,3)}
    def _empty_mapping(self):
        """Return an empty mapping object"""
        return self._tested_class()
    def _full_mapping(self, data):
        """Return a mapping object with the value contained in data
        dictionary"""
        x = self._empty_mapping()
        for key, value in data.items():
            x[key] = value
        return x

    def __init__(self, *args, **kw):
        unittest.TestCase.__init__(self, *args, **kw)
        self.reference = self._reference().copy()
        key, value = self.reference.popitem()
        self.other = {key:value}

    def test_read(self):
        # Test for read only operations on mapping
        p = self._empty_mapping()
        p1 = dict(p) #workaround for singleton objects
        d = self._full_mapping(self.reference)
        if d is p:
            p = p1
        #Indexing
        for key, value in self.reference.items():
            self.assertEqual(d[key], value)
        knownkey = self.other.keys()[0]
        self.failUnlessRaises(KeyError, lambda:d[knownkey])
        #len
        self.assertEqual(len(p), 0)
        self.assertEqual(len(d), len(self.reference))
        #has_key
        for k in self.reference:
            self.assert_(d.has_key(k))
            self.assert_(k in d)
        for k in self.other:
            self.failIf(d.has_key(k))
            self.failIf(k in d)
        #cmp
        self.assertEqual(cmp(p,p), 0)
        self.assertEqual(cmp(d,d), 0)
        self.assertEqual(cmp(p,d), -1)
        self.assertEqual(cmp(d,p), 1)
        #__non__zero__
        if p: self.fail("Empty mapping must compare to False")
        if not d: self.fail("Full mapping must compare to True")
        # keys(), items(), iterkeys() ...
        def check_iterandlist(iter, lst, ref):
            self.assert_(hasattr(iter, 'next'))
            self.assert_(hasattr(iter, '__iter__'))
            x = list(iter)
            self.assert_(Set(x)==Set(lst)==Set(ref))
        check_iterandlist(d.iterkeys(), d.keys(), self.reference.keys())
        check_iterandlist(iter(d), d.keys(), self.reference.keys())
        check_iterandlist(d.itervalues(), d.values(), self.reference.values())
        check_iterandlist(d.iteritems(), d.items(), self.reference.items())
        #get
        key, value = d.iteritems().next()
        knownkey, knownvalue = self.other.iteritems().next()
        self.assertEqual(d.get(key, knownvalue), value)
        self.assertEqual(d.get(knownkey, knownvalue), knownvalue)
        self.failIf(knownkey in d)

    def test_write(self):
        # Test for write operations on mapping
        p = self._empty_mapping()
        #Indexing
        for key, value in self.reference.items():
            p[key] = value
            self.assertEqual(p[key], value)
        for key in self.reference.keys():
            del p[key]
            self.failUnlessRaises(KeyError, lambda:p[key])
        p = self._empty_mapping()
        #update
        p.update(self.reference)
        self.assertEqual(dict(p), self.reference)
        d = self._full_mapping(self.reference)
        #setdefaullt
        key, value = d.iteritems().next()
        knownkey, knownvalue = self.other.iteritems().next()
        self.assertEqual(d.setdefault(key, knownvalue), value)
        self.assertEqual(d[key], value)
        self.assertEqual(d.setdefault(knownkey, knownvalue), knownvalue)
        self.assertEqual(d[knownkey], knownvalue)
        #pop
        self.assertEqual(d.pop(knownkey), knownvalue)
        self.failIf(knownkey in d)
        self.assertRaises(KeyError, d.pop, knownkey)
        default = 909
        d[knownkey] = knownvalue
        self.assertEqual(d.pop(knownkey, default), knownvalue)
        self.failIf(knownkey in d)
        self.assertEqual(d.pop(knownkey, default), default)
        #popitem
        key, value = d.popitem()
        self.failIf(key in d)
        self.assertEqual(value, self.reference[key])
        p=self._empty_mapping()
        self.assertRaises(KeyError, p.popitem)

d0 = {}
d1 = {"one": 1}
d2 = {"one": 1, "two": 2}
d3 = {"one": 1, "two": 3, "three": 5}
d4 = {"one": None, "two": None}
d5 = {"one": 1, "two": 1}

class UserDictTest(TestMappingProtocol):
    _tested_class = UserDict.IterableUserDict

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
        self.assertEqual(`u2`, `d2`)

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
            self.assert_(u2.has_key(i))
            self.assert_(i in u2)
            self.assertEqual(u1.has_key(i), d1.has_key(i))
            self.assertEqual(i in u1, i in d1)
            self.assertEqual(u0.has_key(i), d0.has_key(i))
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
        for i in xrange(20):
            u2[i] = str(i)
        ikeys = []
        for k in u2:
            ikeys.append(k)
        keys = u2.keys()
        self.assertEqual(Set(ikeys), Set(keys))

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

##########################
# Test Dict Mixin

class SeqDict(UserDict.DictMixin):
    """Dictionary lookalike implemented with lists.

    Used to test and demonstrate DictMixin
    """
    def __init__(self):
        self.keylist = []
        self.valuelist = []
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

class UserDictMixinTest(TestMappingProtocol):
    _tested_class = SeqDict

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
    test.test_support.run_unittest(
        TestMappingProtocol,
        UserDictTest,
        UserDictMixinTest
    )

if __name__ == "__main__":
    test_main()

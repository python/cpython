import sys
import unittest
import UserList
import weakref

import test_support


class C:
    def method(self):
        pass


class Callable:
    bar = None

    def __call__(self, x):
        self.bar = x


def create_function():
    def f(): pass
    return f

def create_bound_method():
    return C().method

def create_unbound_method():
    return C.method


class TestBase(unittest.TestCase):

    def setUp(self):
        self.cbcalled = 0

    def callback(self, ref):
        self.cbcalled += 1


class ReferencesTestCase(TestBase):

    def test_basic_ref(self):
        self.check_basic_ref(C)
        self.check_basic_ref(create_function)
        self.check_basic_ref(create_bound_method)
        self.check_basic_ref(create_unbound_method)

    def test_basic_callback(self):
        self.check_basic_callback(C)
        self.check_basic_callback(create_function)
        self.check_basic_callback(create_bound_method)
        self.check_basic_callback(create_unbound_method)

    def test_multiple_callbacks(self):
        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del o
        self.assert_(ref1() is None,
                     "expected reference to be invalidated")
        self.assert_(ref2() is None,
                     "expected reference to be invalidated")
        self.assert_(self.cbcalled == 2,
                     "callback not called the right number of times")

    def test_multiple_selfref_callbacks(self):
        """Make sure all references are invalidated before callbacks
        are called."""
        #
        # What's important here is that we're using the first
        # reference in the callback invoked on the second reference
        # (the most recently created ref is cleaned up first).  This
        # tests that all references to the object are invalidated
        # before any of the callbacks are invoked, so that we only
        # have one invocation of _weakref.c:cleanup_helper() active
        # for a particular object at a time.
        #
        def callback(object, self=self):
            self.ref()
        c = C()
        self.ref = weakref.ref(c, callback)
        ref1 = weakref.ref(c, callback)
        del c

    def test_proxy_ref(self):
        o = C()
        o.bar = 1
        ref1 = weakref.proxy(o, self.callback)
        ref2 = weakref.proxy(o, self.callback)
        del o

        def check(proxy):
            proxy.bar

        self.assertRaises(weakref.ReferenceError, check, ref1)
        self.assertRaises(weakref.ReferenceError, check, ref2)
        self.assert_(self.cbcalled == 2)

    def check_basic_ref(self, factory):
        o = factory()
        ref = weakref.ref(o)
        self.assert_(ref() is not None,
                     "weak reference to live object should be live")
        o2 = ref()
        self.assert_(o is o2,
                     "<ref>() should return original object if live")

    def check_basic_callback(self, factory):
        self.cbcalled = 0
        o = factory()
        ref = weakref.ref(o, self.callback)
        del o
        self.assert_(self.cbcalled == 1,
                     "callback did not properly set 'cbcalled'")
        self.assert_(ref() is None,
                     "ref2 should be dead after deleting object reference")

    def test_ref_reuse(self):
        o = C()
        ref1 = weakref.ref(o)
        # create a proxy to make sure that there's an intervening creation
        # between these two; it should make no difference
        proxy = weakref.proxy(o)
        ref2 = weakref.ref(o)
        self.assert_(ref1 is ref2,
                     "reference object w/out callback should be re-used")

        o = C()
        proxy = weakref.proxy(o)
        ref1 = weakref.ref(o)
        ref2 = weakref.ref(o)
        self.assert_(ref1 is ref2,
                     "reference object w/out callback should be re-used")
        self.assert_(weakref.getweakrefcount(o) == 2,
                     "wrong weak ref count for object")
        del proxy
        self.assert_(weakref.getweakrefcount(o) == 1,
                     "wrong weak ref count for object after deleting proxy")

    def test_proxy_reuse(self):
        o = C()
        proxy1 = weakref.proxy(o)
        ref = weakref.ref(o)
        proxy2 = weakref.proxy(o)
        self.assert_(proxy1 is proxy2,
                     "proxy object w/out callback should have been re-used")

    def test_basic_proxy(self):
        o = C()
        self.check_proxy(o, weakref.proxy(o))

        L = UserList.UserList()
        p = weakref.proxy(L)
        self.failIf(p, "proxy for empty UserList should be false")
        p.append(12)
        self.assertEqual(len(L), 1)
        self.failUnless(p, "proxy for non-empty UserList should be true")
        p[:] = [2, 3]
        self.assertEqual(len(L), 2)
        self.assertEqual(len(p), 2)
        self.failUnless(3 in p, "proxy didn't support __contains__() properly")
        p[1] = 5
        self.assertEqual(L[1], 5)
        self.assertEqual(p[1], 5)
        L2 = UserList.UserList(L)
        p2 = weakref.proxy(L2)
        self.assertEqual(p, p2)

    def test_callable_proxy(self):
        o = Callable()
        ref1 = weakref.proxy(o)

        self.check_proxy(o, ref1)

        self.assert_(type(ref1) is weakref.CallableProxyType,
                     "proxy is not of callable type")
        ref1('twinkies!')
        self.assert_(o.bar == 'twinkies!',
                     "call through proxy not passed through to original")
        ref1(x='Splat.')
        self.assert_(o.bar == 'Splat.',
                     "call through proxy not passed through to original")

        # expect due to too few args
        self.assertRaises(TypeError, ref1)

        # expect due to too many args
        self.assertRaises(TypeError, ref1, 1, 2, 3)

    def check_proxy(self, o, proxy):
        o.foo = 1
        self.assert_(proxy.foo == 1,
                     "proxy does not reflect attribute addition")
        o.foo = 2
        self.assert_(proxy.foo == 2,
                     "proxy does not reflect attribute modification")
        del o.foo
        self.assert_(not hasattr(proxy, 'foo'),
                     "proxy does not reflect attribute removal")

        proxy.foo = 1
        self.assert_(o.foo == 1,
                     "object does not reflect attribute addition via proxy")
        proxy.foo = 2
        self.assert_(
            o.foo == 2,
            "object does not reflect attribute modification via proxy")
        del proxy.foo
        self.assert_(not hasattr(o, 'foo'),
                     "object does not reflect attribute removal via proxy")

    def test_getweakrefcount(self):
        o = C()
        ref1 = weakref.ref(o)
        ref2 = weakref.ref(o, self.callback)
        self.assert_(weakref.getweakrefcount(o) == 2,
                     "got wrong number of weak reference objects")

        proxy1 = weakref.proxy(o)
        proxy2 = weakref.proxy(o, self.callback)
        self.assert_(weakref.getweakrefcount(o) == 4,
                     "got wrong number of weak reference objects")

    def test_getweakrefs(self):
        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del ref1
        self.assert_(weakref.getweakrefs(o) == [ref2],
                     "list of refs does not match")

        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del ref2
        self.assert_(weakref.getweakrefs(o) == [ref1],
                     "list of refs does not match")

    def test_newstyle_number_ops(self):
        class F(float):
            pass
        f = F(2.0)
        p = weakref.proxy(f)
        self.assert_(p + 1.0 == 3.0)
        self.assert_(1.0 + p == 3.0)  # this used to SEGV

    def test_callbacks_protected(self):
        """Callbacks protected from already-set exceptions?"""
        # Regression test for SF bug #478534.
        class BogusError(Exception):
            pass
        data = {}
        def remove(k):
            del data[k]
        def encapsulate():
            f = lambda : ()
            data[weakref.ref(f, remove)] = None
            raise BogusError
        try:
            encapsulate()
        except BogusError:
            pass
        else:
            self.fail("exception not properly restored")
        try:
            encapsulate()
        except BogusError:
            pass
        else:
            self.fail("exception not properly restored")


class Object:
    def __init__(self, arg):
        self.arg = arg
    def __repr__(self):
        return "<Object %r>" % self.arg


class MappingTestCase(TestBase):

    COUNT = 10

    def test_weak_values(self):
        #
        #  This exercises d.copy(), d.items(), d[], del d[], len(d).
        #
        dict, objects = self.make_weak_valued_dict()
        for o in objects:
            self.assert_(weakref.getweakrefcount(o) == 1,
                         "wrong number of weak references to %r!" % o)
            self.assert_(o is dict[o.arg],
                         "wrong object returned by weak dict!")
        items1 = dict.items()
        items2 = dict.copy().items()
        items1.sort()
        items2.sort()
        self.assert_(items1 == items2,
                     "cloning of weak-valued dictionary did not work!")
        del items1, items2
        self.assert_(len(dict) == self.COUNT)
        del objects[0]
        self.assert_(len(dict) == (self.COUNT - 1),
                     "deleting object did not cause dictionary update")
        del objects, o
        self.assert_(len(dict) == 0,
                     "deleting the values did not clear the dictionary")
        # regression on SF bug #447152:
        dict = weakref.WeakValueDictionary()
        self.assertRaises(KeyError, dict.__getitem__, 1)
        dict[2] = C()
        self.assertRaises(KeyError, dict.__getitem__, 2)

    def test_weak_keys(self):
        #
        #  This exercises d.copy(), d.items(), d[] = v, d[], del d[],
        #  len(d), d.has_key().
        #
        dict, objects = self.make_weak_keyed_dict()
        for o in objects:
            self.assert_(weakref.getweakrefcount(o) == 1,
                         "wrong number of weak references to %r!" % o)
            self.assert_(o.arg is dict[o],
                         "wrong object returned by weak dict!")
        items1 = dict.items()
        items2 = dict.copy().items()
        items1.sort()
        items2.sort()
        self.assert_(items1 == items2,
                     "cloning of weak-keyed dictionary did not work!")
        del items1, items2
        self.assert_(len(dict) == self.COUNT)
        del objects[0]
        self.assert_(len(dict) == (self.COUNT - 1),
                     "deleting object did not cause dictionary update")
        del objects, o
        self.assert_(len(dict) == 0,
                     "deleting the keys did not clear the dictionary")
        o = Object(42)
        dict[o] = "What is the meaning of the universe?"
        self.assert_(dict.has_key(o))
        self.assert_(not dict.has_key(34))

    def test_weak_keyed_iters(self):
        dict, objects = self.make_weak_keyed_dict()
        self.check_iters(dict)

    def test_weak_valued_iters(self):
        dict, objects = self.make_weak_valued_dict()
        self.check_iters(dict)

    def check_iters(self, dict):
        # item iterator:
        items = dict.items()
        for item in dict.iteritems():
            items.remove(item)
        self.assert_(len(items) == 0, "iteritems() did not touch all items")

        # key iterator, via __iter__():
        keys = dict.keys()
        for k in dict:
            keys.remove(k)
        self.assert_(len(keys) == 0, "__iter__() did not touch all keys")

        # key iterator, via iterkeys():
        keys = dict.keys()
        for k in dict.iterkeys():
            keys.remove(k)
        self.assert_(len(keys) == 0, "iterkeys() did not touch all keys")

        # value iterator:
        values = dict.values()
        for v in dict.itervalues():
            values.remove(v)
        self.assert_(len(values) == 0, "itervalues() did not touch all values")

    def test_make_weak_keyed_dict_from_dict(self):
        o = Object(3)
        dict = weakref.WeakKeyDictionary({o:364})
        self.assert_(dict[o] == 364)

    def test_make_weak_keyed_dict_from_weak_keyed_dict(self):
        o = Object(3)
        dict = weakref.WeakKeyDictionary({o:364})
        dict2 = weakref.WeakKeyDictionary(dict)
        self.assert_(dict[o] == 364)

    def make_weak_keyed_dict(self):
        dict = weakref.WeakKeyDictionary()
        objects = map(Object, range(self.COUNT))
        for o in objects:
            dict[o] = o.arg
        return dict, objects

    def make_weak_valued_dict(self):
        dict = weakref.WeakValueDictionary()
        objects = map(Object, range(self.COUNT))
        for o in objects:
            dict[o.arg] = o
        return dict, objects

    def check_popitem(self, klass, key1, value1, key2, value2):
        weakdict = klass()
        weakdict[key1] = value1
        weakdict[key2] = value2
        self.assert_(len(weakdict) == 2)
        k, v = weakdict.popitem()
        self.assert_(len(weakdict) == 1)
        if k is key1:
            self.assert_(v is value1)
        else:
            self.assert_(v is value2)
        k, v = weakdict.popitem()
        self.assert_(len(weakdict) == 0)
        if k is key1:
            self.assert_(v is value1)
        else:
            self.assert_(v is value2)

    def test_weak_valued_dict_popitem(self):
        self.check_popitem(weakref.WeakValueDictionary,
                           "key1", C(), "key2", C())

    def test_weak_keyed_dict_popitem(self):
        self.check_popitem(weakref.WeakKeyDictionary,
                           C(), "value 1", C(), "value 2")

    def check_setdefault(self, klass, key, value1, value2):
        self.assert_(value1 is not value2,
                     "invalid test"
                     " -- value parameters must be distinct objects")
        weakdict = klass()
        o = weakdict.setdefault(key, value1)
        self.assert_(o is value1)
        self.assert_(weakdict.has_key(key))
        self.assert_(weakdict.get(key) is value1)
        self.assert_(weakdict[key] is value1)

        o = weakdict.setdefault(key, value2)
        self.assert_(o is value1)
        self.assert_(weakdict.has_key(key))
        self.assert_(weakdict.get(key) is value1)
        self.assert_(weakdict[key] is value1)

    def test_weak_valued_dict_setdefault(self):
        self.check_setdefault(weakref.WeakValueDictionary,
                              "key", C(), C())

    def test_weak_keyed_dict_setdefault(self):
        self.check_setdefault(weakref.WeakKeyDictionary,
                              C(), "value 1", "value 2")

    def check_update(self, klass, dict):
        #
        #  This exercises d.update(), len(d), d.keys(), d.has_key(),
        #  d.get(), d[].
        #
        weakdict = klass()
        weakdict.update(dict)
        self.assert_(len(weakdict) == len(dict))
        for k in weakdict.keys():
            self.assert_(dict.has_key(k),
                         "mysterious new key appeared in weak dict")
            v = dict.get(k)
            self.assert_(v is weakdict[k])
            self.assert_(v is weakdict.get(k))
        for k in dict.keys():
            self.assert_(weakdict.has_key(k),
                         "original key disappeared in weak dict")
            v = dict[k]
            self.assert_(v is weakdict[k])
            self.assert_(v is weakdict.get(k))

    def test_weak_valued_dict_update(self):
        self.check_update(weakref.WeakValueDictionary,
                          {1: C(), 'a': C(), C(): C()})

    def test_weak_keyed_dict_update(self):
        self.check_update(weakref.WeakKeyDictionary,
                          {C(): 1, C(): 2, C(): 3})

    def test_weak_keyed_delitem(self):
        d = weakref.WeakKeyDictionary()
        o1 = Object('1')
        o2 = Object('2')
        d[o1] = 'something'
        d[o2] = 'something'
        self.assert_(len(d) == 2)
        del d[o1]
        self.assert_(len(d) == 1)
        self.assert_(d.keys() == [o2])

    def test_weak_valued_delitem(self):
        d = weakref.WeakValueDictionary()
        o1 = Object('1')
        o2 = Object('2')
        d['something'] = o1
        d['something else'] = o2
        self.assert_(len(d) == 2)
        del d['something']
        self.assert_(len(d) == 1)
        self.assert_(d.items() == [('something else', o2)])

    def test_weak_keyed_bad_delitem(self):
        d = weakref.WeakKeyDictionary()
        o = Object('1')
        # An attempt to delete an object that isn't there should raise
        # KeyError.  It didn't before 2.3.
        self.assertRaises(KeyError, d.__delitem__, o)
        self.assertRaises(KeyError, d.__getitem__, o)

        # If a key isn't of a weakly referencable type, __getitem__ and
        # __setitem__ raise TypeError.  __delitem__ should too.
        self.assertRaises(TypeError, d.__delitem__,  13)
        self.assertRaises(TypeError, d.__getitem__,  13)
        self.assertRaises(TypeError, d.__setitem__,  13, 13)

    def test_weak_keyed_cascading_deletes(self):
        # SF bug 742860.  For some reason, before 2.3 __delitem__ iterated
        # over the keys via self.data.iterkeys().  If things vanished from
        # the dict during this (or got added), that caused a RuntimeError.

        d = weakref.WeakKeyDictionary()
        mutate = False

        class C(object):
            def __init__(self, i):
                self.value = i
            def __hash__(self):
                return hash(self.value)
            def __eq__(self, other):
                if mutate:
                    # Side effect that mutates the dict, by removing the
                    # last strong reference to a key.
                    del objs[-1]
                return self.value == other.value

        objs = [C(i) for i in range(4)]
        for o in objs:
            d[o] = o.value
        del o   # now the only strong references to keys are in objs
        # Find the order in which iterkeys sees the keys.
        objs = d.keys()
        # Reverse it, so that the iteration implementation of __delitem__
        # has to keep looping to find the first object we delete.
        objs.reverse()

        # Turn on mutation in C.__eq__.  The first time thru the loop,
        # under the iterkeys() business the first comparison will delete
        # the last item iterkeys() would see, and that causes a
        #     RuntimeError: dictionary changed size during iteration
        # when the iterkeys() loop goes around to try comparing the next
        # key.  After this was fixed, it just deletes the last object *our*
        # "for o in obj" loop would have gotten to.
        mutate = True
        count = 0
        for o in objs:
            count += 1
            del d[o]
        self.assertEqual(len(d), 0)
        self.assertEqual(count, 2)

def test_main():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(ReferencesTestCase))
    suite.addTest(loader.loadTestsFromTestCase(MappingTestCase))
    test_support.run_suite(suite)


if __name__ == "__main__":
    test_main()

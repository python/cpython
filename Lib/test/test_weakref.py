import sys
import unittest
import weakref

from test_support import run_unittest


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

    def test_callable_proxy(self):
        o = Callable()
        ref1 = weakref.proxy(o)

        self.check_proxy(o, ref1)

        self.assert_(type(ref1) is weakref.CallableProxyType,
                     "proxy is not of callable type")
        ref1('twinkies!')
        self.assert_(o.bar == 'twinkies!',
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

    def test_weak_keys(self):
        #
        #  This exercises d.copy(), d.items(), d[] = v, d[], del d[],
        #  len(d).
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
        self.assert_(len(items) == 0, "iterator did not touch all items")

        # key iterator:
        keys = dict.keys()
        for k in dict:
            keys.remove(k)
        self.assert_(len(keys) == 0, "iterator did not touch all keys")

        # value iterator:
        values = dict.values()
        for v in dict.itervalues():
            values.remove(v)
        self.assert_(len(values) == 0, "iterator did not touch all values")

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


run_unittest(ReferencesTestCase)
run_unittest(MappingTestCase)

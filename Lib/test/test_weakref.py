import gc
import sys
import unittest
import collections
import weakref
import operator
import contextlib
import copy

from test import support, script_helper

# Used in ReferencesTestCase.test_ref_created_during_del() .
ref_from_del = None

# Used by FinalizeTestCase as a global that may be replaced by None
# when the interpreter shuts down.
_global_var = 'foobar'

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


class Object:
    def __init__(self, arg):
        self.arg = arg
    def __repr__(self):
        return "<Object %r>" % self.arg
    def __eq__(self, other):
        if isinstance(other, Object):
            return self.arg == other.arg
        return NotImplemented
    def __lt__(self, other):
        if isinstance(other, Object):
            return self.arg < other.arg
        return NotImplemented
    def __hash__(self):
        return hash(self.arg)
    def some_method(self):
        return 4
    def other_method(self):
        return 5


class RefCycle:
    def __init__(self):
        self.cycle = self


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

        # Just make sure the tp_repr handler doesn't raise an exception.
        # Live reference:
        o = C()
        wr = weakref.ref(o)
        repr(wr)
        # Dead reference:
        del o
        repr(wr)

    def test_basic_callback(self):
        self.check_basic_callback(C)
        self.check_basic_callback(create_function)
        self.check_basic_callback(create_bound_method)

    def test_multiple_callbacks(self):
        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del o
        self.assertIsNone(ref1(), "expected reference to be invalidated")
        self.assertIsNone(ref2(), "expected reference to be invalidated")
        self.assertEqual(self.cbcalled, 2,
                     "callback not called the right number of times")

    def test_multiple_selfref_callbacks(self):
        # Make sure all references are invalidated before callbacks are called
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

        self.assertRaises(ReferenceError, check, ref1)
        self.assertRaises(ReferenceError, check, ref2)
        self.assertRaises(ReferenceError, bool, weakref.proxy(C()))
        self.assertEqual(self.cbcalled, 2)

    def check_basic_ref(self, factory):
        o = factory()
        ref = weakref.ref(o)
        self.assertIsNotNone(ref(),
                     "weak reference to live object should be live")
        o2 = ref()
        self.assertIs(o, o2,
                     "<ref>() should return original object if live")

    def check_basic_callback(self, factory):
        self.cbcalled = 0
        o = factory()
        ref = weakref.ref(o, self.callback)
        del o
        self.assertEqual(self.cbcalled, 1,
                     "callback did not properly set 'cbcalled'")
        self.assertIsNone(ref(),
                     "ref2 should be dead after deleting object reference")

    def test_ref_reuse(self):
        o = C()
        ref1 = weakref.ref(o)
        # create a proxy to make sure that there's an intervening creation
        # between these two; it should make no difference
        proxy = weakref.proxy(o)
        ref2 = weakref.ref(o)
        self.assertIs(ref1, ref2,
                     "reference object w/out callback should be re-used")

        o = C()
        proxy = weakref.proxy(o)
        ref1 = weakref.ref(o)
        ref2 = weakref.ref(o)
        self.assertIs(ref1, ref2,
                     "reference object w/out callback should be re-used")
        self.assertEqual(weakref.getweakrefcount(o), 2,
                     "wrong weak ref count for object")
        del proxy
        self.assertEqual(weakref.getweakrefcount(o), 1,
                     "wrong weak ref count for object after deleting proxy")

    def test_proxy_reuse(self):
        o = C()
        proxy1 = weakref.proxy(o)
        ref = weakref.ref(o)
        proxy2 = weakref.proxy(o)
        self.assertIs(proxy1, proxy2,
                     "proxy object w/out callback should have been re-used")

    def test_basic_proxy(self):
        o = C()
        self.check_proxy(o, weakref.proxy(o))

        L = collections.UserList()
        p = weakref.proxy(L)
        self.assertFalse(p, "proxy for empty UserList should be false")
        p.append(12)
        self.assertEqual(len(L), 1)
        self.assertTrue(p, "proxy for non-empty UserList should be true")
        p[:] = [2, 3]
        self.assertEqual(len(L), 2)
        self.assertEqual(len(p), 2)
        self.assertIn(3, p, "proxy didn't support __contains__() properly")
        p[1] = 5
        self.assertEqual(L[1], 5)
        self.assertEqual(p[1], 5)
        L2 = collections.UserList(L)
        p2 = weakref.proxy(L2)
        self.assertEqual(p, p2)
        ## self.assertEqual(repr(L2), repr(p2))
        L3 = collections.UserList(range(10))
        p3 = weakref.proxy(L3)
        self.assertEqual(L3[:], p3[:])
        self.assertEqual(L3[5:], p3[5:])
        self.assertEqual(L3[:5], p3[:5])
        self.assertEqual(L3[2:5], p3[2:5])

    def test_proxy_unicode(self):
        # See bug 5037
        class C(object):
            def __str__(self):
                return "string"
            def __bytes__(self):
                return b"bytes"
        instance = C()
        self.assertIn("__bytes__", dir(weakref.proxy(instance)))
        self.assertEqual(bytes(weakref.proxy(instance)), b"bytes")

    def test_proxy_index(self):
        class C:
            def __index__(self):
                return 10
        o = C()
        p = weakref.proxy(o)
        self.assertEqual(operator.index(p), 10)

    def test_proxy_div(self):
        class C:
            def __floordiv__(self, other):
                return 42
            def __ifloordiv__(self, other):
                return 21
        o = C()
        p = weakref.proxy(o)
        self.assertEqual(p // 5, 42)
        p //= 5
        self.assertEqual(p, 21)

    # The PyWeakref_* C API is documented as allowing either NULL or
    # None as the value for the callback, where either means "no
    # callback".  The "no callback" ref and proxy objects are supposed
    # to be shared so long as they exist by all callers so long as
    # they are active.  In Python 2.3.3 and earlier, this guarantee
    # was not honored, and was broken in different ways for
    # PyWeakref_NewRef() and PyWeakref_NewProxy().  (Two tests.)

    def test_shared_ref_without_callback(self):
        self.check_shared_without_callback(weakref.ref)

    def test_shared_proxy_without_callback(self):
        self.check_shared_without_callback(weakref.proxy)

    def check_shared_without_callback(self, makeref):
        o = Object(1)
        p1 = makeref(o, None)
        p2 = makeref(o, None)
        self.assertIs(p1, p2, "both callbacks were None in the C API")
        del p1, p2
        p1 = makeref(o)
        p2 = makeref(o, None)
        self.assertIs(p1, p2, "callbacks were NULL, None in the C API")
        del p1, p2
        p1 = makeref(o)
        p2 = makeref(o)
        self.assertIs(p1, p2, "both callbacks were NULL in the C API")
        del p1, p2
        p1 = makeref(o, None)
        p2 = makeref(o)
        self.assertIs(p1, p2, "callbacks were None, NULL in the C API")

    def test_callable_proxy(self):
        o = Callable()
        ref1 = weakref.proxy(o)

        self.check_proxy(o, ref1)

        self.assertIs(type(ref1), weakref.CallableProxyType,
                     "proxy is not of callable type")
        ref1('twinkies!')
        self.assertEqual(o.bar, 'twinkies!',
                     "call through proxy not passed through to original")
        ref1(x='Splat.')
        self.assertEqual(o.bar, 'Splat.',
                     "call through proxy not passed through to original")

        # expect due to too few args
        self.assertRaises(TypeError, ref1)

        # expect due to too many args
        self.assertRaises(TypeError, ref1, 1, 2, 3)

    def check_proxy(self, o, proxy):
        o.foo = 1
        self.assertEqual(proxy.foo, 1,
                     "proxy does not reflect attribute addition")
        o.foo = 2
        self.assertEqual(proxy.foo, 2,
                     "proxy does not reflect attribute modification")
        del o.foo
        self.assertFalse(hasattr(proxy, 'foo'),
                     "proxy does not reflect attribute removal")

        proxy.foo = 1
        self.assertEqual(o.foo, 1,
                     "object does not reflect attribute addition via proxy")
        proxy.foo = 2
        self.assertEqual(o.foo, 2,
            "object does not reflect attribute modification via proxy")
        del proxy.foo
        self.assertFalse(hasattr(o, 'foo'),
                     "object does not reflect attribute removal via proxy")

    def test_proxy_deletion(self):
        # Test clearing of SF bug #762891
        class Foo:
            result = None
            def __delitem__(self, accessor):
                self.result = accessor
        g = Foo()
        f = weakref.proxy(g)
        del f[0]
        self.assertEqual(f.result, 0)

    def test_proxy_bool(self):
        # Test clearing of SF bug #1170766
        class List(list): pass
        lyst = List()
        self.assertEqual(bool(weakref.proxy(lyst)), bool(lyst))

    def test_getweakrefcount(self):
        o = C()
        ref1 = weakref.ref(o)
        ref2 = weakref.ref(o, self.callback)
        self.assertEqual(weakref.getweakrefcount(o), 2,
                     "got wrong number of weak reference objects")

        proxy1 = weakref.proxy(o)
        proxy2 = weakref.proxy(o, self.callback)
        self.assertEqual(weakref.getweakrefcount(o), 4,
                     "got wrong number of weak reference objects")

        del ref1, ref2, proxy1, proxy2
        self.assertEqual(weakref.getweakrefcount(o), 0,
                     "weak reference objects not unlinked from"
                     " referent when discarded.")

        # assumes ints do not support weakrefs
        self.assertEqual(weakref.getweakrefcount(1), 0,
                     "got wrong number of weak reference objects for int")

    def test_getweakrefs(self):
        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del ref1
        self.assertEqual(weakref.getweakrefs(o), [ref2],
                     "list of refs does not match")

        o = C()
        ref1 = weakref.ref(o, self.callback)
        ref2 = weakref.ref(o, self.callback)
        del ref2
        self.assertEqual(weakref.getweakrefs(o), [ref1],
                     "list of refs does not match")

        del ref1
        self.assertEqual(weakref.getweakrefs(o), [],
                     "list of refs not cleared")

        # assumes ints do not support weakrefs
        self.assertEqual(weakref.getweakrefs(1), [],
                     "list of refs does not match for int")

    def test_newstyle_number_ops(self):
        class F(float):
            pass
        f = F(2.0)
        p = weakref.proxy(f)
        self.assertEqual(p + 1.0, 3.0)
        self.assertEqual(1.0 + p, 3.0)  # this used to SEGV

    def test_callbacks_protected(self):
        # Callbacks protected from already-set exceptions?
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

    def test_sf_bug_840829(self):
        # "weakref callbacks and gc corrupt memory"
        # subtype_dealloc erroneously exposed a new-style instance
        # already in the process of getting deallocated to gc,
        # causing double-deallocation if the instance had a weakref
        # callback that triggered gc.
        # If the bug exists, there probably won't be an obvious symptom
        # in a release build.  In a debug build, a segfault will occur
        # when the second attempt to remove the instance from the "list
        # of all objects" occurs.

        import gc

        class C(object):
            pass

        c = C()
        wr = weakref.ref(c, lambda ignore: gc.collect())
        del c

        # There endeth the first part.  It gets worse.
        del wr

        c1 = C()
        c1.i = C()
        wr = weakref.ref(c1.i, lambda ignore: gc.collect())

        c2 = C()
        c2.c1 = c1
        del c1  # still alive because c2 points to it

        # Now when subtype_dealloc gets called on c2, it's not enough just
        # that c2 is immune from gc while the weakref callbacks associated
        # with c2 execute (there are none in this 2nd half of the test, btw).
        # subtype_dealloc goes on to call the base classes' deallocs too,
        # so any gc triggered by weakref callbacks associated with anything
        # torn down by a base class dealloc can also trigger double
        # deallocation of c2.
        del c2

    def test_callback_in_cycle_1(self):
        import gc

        class J(object):
            pass

        class II(object):
            def acallback(self, ignore):
                self.J

        I = II()
        I.J = J
        I.wr = weakref.ref(J, I.acallback)

        # Now J and II are each in a self-cycle (as all new-style class
        # objects are, since their __mro__ points back to them).  I holds
        # both a weak reference (I.wr) and a strong reference (I.J) to class
        # J.  I is also in a cycle (I.wr points to a weakref that references
        # I.acallback).  When we del these three, they all become trash, but
        # the cycles prevent any of them from getting cleaned up immediately.
        # Instead they have to wait for cyclic gc to deduce that they're
        # trash.
        #
        # gc used to call tp_clear on all of them, and the order in which
        # it does that is pretty accidental.  The exact order in which we
        # built up these things manages to provoke gc into running tp_clear
        # in just the right order (I last).  Calling tp_clear on II leaves
        # behind an insane class object (its __mro__ becomes NULL).  Calling
        # tp_clear on J breaks its self-cycle, but J doesn't get deleted
        # just then because of the strong reference from I.J.  Calling
        # tp_clear on I starts to clear I's __dict__, and just happens to
        # clear I.J first -- I.wr is still intact.  That removes the last
        # reference to J, which triggers the weakref callback.  The callback
        # tries to do "self.J", and instances of new-style classes look up
        # attributes ("J") in the class dict first.  The class (II) wants to
        # search II.__mro__, but that's NULL.   The result was a segfault in
        # a release build, and an assert failure in a debug build.
        del I, J, II
        gc.collect()

    def test_callback_in_cycle_2(self):
        import gc

        # This is just like test_callback_in_cycle_1, except that II is an
        # old-style class.  The symptom is different then:  an instance of an
        # old-style class looks in its own __dict__ first.  'J' happens to
        # get cleared from I.__dict__ before 'wr', and 'J' was never in II's
        # __dict__, so the attribute isn't found.  The difference is that
        # the old-style II doesn't have a NULL __mro__ (it doesn't have any
        # __mro__), so no segfault occurs.  Instead it got:
        #    test_callback_in_cycle_2 (__main__.ReferencesTestCase) ...
        #    Exception exceptions.AttributeError:
        #   "II instance has no attribute 'J'" in <bound method II.acallback
        #       of <?.II instance at 0x00B9B4B8>> ignored

        class J(object):
            pass

        class II:
            def acallback(self, ignore):
                self.J

        I = II()
        I.J = J
        I.wr = weakref.ref(J, I.acallback)

        del I, J, II
        gc.collect()

    def test_callback_in_cycle_3(self):
        import gc

        # This one broke the first patch that fixed the last two.  In this
        # case, the objects reachable from the callback aren't also reachable
        # from the object (c1) *triggering* the callback:  you can get to
        # c1 from c2, but not vice-versa.  The result was that c2's __dict__
        # got tp_clear'ed by the time the c2.cb callback got invoked.

        class C:
            def cb(self, ignore):
                self.me
                self.c1
                self.wr

        c1, c2 = C(), C()

        c2.me = c2
        c2.c1 = c1
        c2.wr = weakref.ref(c1, c2.cb)

        del c1, c2
        gc.collect()

    def test_callback_in_cycle_4(self):
        import gc

        # Like test_callback_in_cycle_3, except c2 and c1 have different
        # classes.  c2's class (C) isn't reachable from c1 then, so protecting
        # objects reachable from the dying object (c1) isn't enough to stop
        # c2's class (C) from getting tp_clear'ed before c2.cb is invoked.
        # The result was a segfault (C.__mro__ was NULL when the callback
        # tried to look up self.me).

        class C(object):
            def cb(self, ignore):
                self.me
                self.c1
                self.wr

        class D:
            pass

        c1, c2 = D(), C()

        c2.me = c2
        c2.c1 = c1
        c2.wr = weakref.ref(c1, c2.cb)

        del c1, c2, C, D
        gc.collect()

    def test_callback_in_cycle_resurrection(self):
        import gc

        # Do something nasty in a weakref callback:  resurrect objects
        # from dead cycles.  For this to be attempted, the weakref and
        # its callback must also be part of the cyclic trash (else the
        # objects reachable via the callback couldn't be in cyclic trash
        # to begin with -- the callback would act like an external root).
        # But gc clears trash weakrefs with callbacks early now, which
        # disables the callbacks, so the callbacks shouldn't get called
        # at all (and so nothing actually gets resurrected).

        alist = []
        class C(object):
            def __init__(self, value):
                self.attribute = value

            def acallback(self, ignore):
                alist.append(self.c)

        c1, c2 = C(1), C(2)
        c1.c = c2
        c2.c = c1
        c1.wr = weakref.ref(c2, c1.acallback)
        c2.wr = weakref.ref(c1, c2.acallback)

        def C_went_away(ignore):
            alist.append("C went away")
        wr = weakref.ref(C, C_went_away)

        del c1, c2, C   # make them all trash
        self.assertEqual(alist, [])  # del isn't enough to reclaim anything

        gc.collect()
        # c1.wr and c2.wr were part of the cyclic trash, so should have
        # been cleared without their callbacks executing.  OTOH, the weakref
        # to C is bound to a function local (wr), and wasn't trash, so that
        # callback should have been invoked when C went away.
        self.assertEqual(alist, ["C went away"])
        # The remaining weakref should be dead now (its callback ran).
        self.assertEqual(wr(), None)

        del alist[:]
        gc.collect()
        self.assertEqual(alist, [])

    def test_callbacks_on_callback(self):
        import gc

        # Set up weakref callbacks *on* weakref callbacks.
        alist = []
        def safe_callback(ignore):
            alist.append("safe_callback called")

        class C(object):
            def cb(self, ignore):
                alist.append("cb called")

        c, d = C(), C()
        c.other = d
        d.other = c
        callback = c.cb
        c.wr = weakref.ref(d, callback)     # this won't trigger
        d.wr = weakref.ref(callback, d.cb)  # ditto
        external_wr = weakref.ref(callback, safe_callback)  # but this will
        self.assertIs(external_wr(), callback)

        # The weakrefs attached to c and d should get cleared, so that
        # C.cb is never called.  But external_wr isn't part of the cyclic
        # trash, and no cyclic trash is reachable from it, so safe_callback
        # should get invoked when the bound method object callback (c.cb)
        # -- which is itself a callback, and also part of the cyclic trash --
        # gets reclaimed at the end of gc.

        del callback, c, d, C
        self.assertEqual(alist, [])  # del isn't enough to clean up cycles
        gc.collect()
        self.assertEqual(alist, ["safe_callback called"])
        self.assertEqual(external_wr(), None)

        del alist[:]
        gc.collect()
        self.assertEqual(alist, [])

    def test_gc_during_ref_creation(self):
        self.check_gc_during_creation(weakref.ref)

    def test_gc_during_proxy_creation(self):
        self.check_gc_during_creation(weakref.proxy)

    def check_gc_during_creation(self, makeref):
        thresholds = gc.get_threshold()
        gc.set_threshold(1, 1, 1)
        gc.collect()
        class A:
            pass

        def callback(*args):
            pass

        referenced = A()

        a = A()
        a.a = a
        a.wr = makeref(referenced)

        try:
            # now make sure the object and the ref get labeled as
            # cyclic trash:
            a = A()
            weakref.ref(referenced, callback)

        finally:
            gc.set_threshold(*thresholds)

    def test_ref_created_during_del(self):
        # Bug #1377858
        # A weakref created in an object's __del__() would crash the
        # interpreter when the weakref was cleaned up since it would refer to
        # non-existent memory.  This test should not segfault the interpreter.
        class Target(object):
            def __del__(self):
                global ref_from_del
                ref_from_del = weakref.ref(self)

        w = Target()

    def test_init(self):
        # Issue 3634
        # <weakref to class>.__init__() doesn't check errors correctly
        r = weakref.ref(Exception)
        self.assertRaises(TypeError, r.__init__, 0, 0, 0, 0, 0)
        # No exception should be raised here
        gc.collect()

    def test_classes(self):
        # Check that classes are weakrefable.
        class A(object):
            pass
        l = []
        weakref.ref(int)
        a = weakref.ref(A, l.append)
        A = None
        gc.collect()
        self.assertEqual(a(), None)
        self.assertEqual(l, [a])

    def test_equality(self):
        # Alive weakrefs defer equality testing to their underlying object.
        x = Object(1)
        y = Object(1)
        z = Object(2)
        a = weakref.ref(x)
        b = weakref.ref(y)
        c = weakref.ref(z)
        d = weakref.ref(x)
        # Note how we directly test the operators here, to stress both
        # __eq__ and __ne__.
        self.assertTrue(a == b)
        self.assertFalse(a != b)
        self.assertFalse(a == c)
        self.assertTrue(a != c)
        self.assertTrue(a == d)
        self.assertFalse(a != d)
        del x, y, z
        gc.collect()
        for r in a, b, c:
            # Sanity check
            self.assertIs(r(), None)
        # Dead weakrefs compare by identity: whether `a` and `d` are the
        # same weakref object is an implementation detail, since they pointed
        # to the same original object and didn't have a callback.
        # (see issue #16453).
        self.assertFalse(a == b)
        self.assertTrue(a != b)
        self.assertFalse(a == c)
        self.assertTrue(a != c)
        self.assertEqual(a == d, a is d)
        self.assertEqual(a != d, a is not d)

    def test_ordering(self):
        # weakrefs cannot be ordered, even if the underlying objects can.
        ops = [operator.lt, operator.gt, operator.le, operator.ge]
        x = Object(1)
        y = Object(1)
        a = weakref.ref(x)
        b = weakref.ref(y)
        for op in ops:
            self.assertRaises(TypeError, op, a, b)
        # Same when dead.
        del x, y
        gc.collect()
        for op in ops:
            self.assertRaises(TypeError, op, a, b)

    def test_hashing(self):
        # Alive weakrefs hash the same as the underlying object
        x = Object(42)
        y = Object(42)
        a = weakref.ref(x)
        b = weakref.ref(y)
        self.assertEqual(hash(a), hash(42))
        del x, y
        gc.collect()
        # Dead weakrefs:
        # - retain their hash is they were hashed when alive;
        # - otherwise, cannot be hashed.
        self.assertEqual(hash(a), hash(42))
        self.assertRaises(TypeError, hash, b)

    def test_trashcan_16602(self):
        # Issue #16602: when a weakref's target was part of a long
        # deallocation chain, the trashcan mechanism could delay clearing
        # of the weakref and make the target object visible from outside
        # code even though its refcount had dropped to 0.  A crash ensued.
        class C:
            def __init__(self, parent):
                if not parent:
                    return
                wself = weakref.ref(self)
                def cb(wparent):
                    o = wself()
                self.wparent = weakref.ref(parent, cb)

        d = weakref.WeakKeyDictionary()
        root = c = C(None)
        for n in range(100):
            d[c] = c = C(c)
        del root
        gc.collect()

    def test_callback_attribute(self):
        x = Object(1)
        callback = lambda ref: None
        ref1 = weakref.ref(x, callback)
        self.assertIs(ref1.__callback__, callback)

        ref2 = weakref.ref(x)
        self.assertIsNone(ref2.__callback__)

    def test_callback_attribute_after_deletion(self):
        x = Object(1)
        ref = weakref.ref(x, self.callback)
        self.assertIsNotNone(ref.__callback__)
        del x
        support.gc_collect()
        self.assertIsNone(ref.__callback__)

    def test_set_callback_attribute(self):
        x = Object(1)
        callback = lambda ref: None
        ref1 = weakref.ref(x, callback)
        with self.assertRaises(AttributeError):
            ref1.__callback__ = lambda ref: None


class SubclassableWeakrefTestCase(TestBase):

    def test_subclass_refs(self):
        class MyRef(weakref.ref):
            def __init__(self, ob, callback=None, value=42):
                self.value = value
                super().__init__(ob, callback)
            def __call__(self):
                self.called = True
                return super().__call__()
        o = Object("foo")
        mr = MyRef(o, value=24)
        self.assertIs(mr(), o)
        self.assertTrue(mr.called)
        self.assertEqual(mr.value, 24)
        del o
        self.assertIsNone(mr())
        self.assertTrue(mr.called)

    def test_subclass_refs_dont_replace_standard_refs(self):
        class MyRef(weakref.ref):
            pass
        o = Object(42)
        r1 = MyRef(o)
        r2 = weakref.ref(o)
        self.assertIsNot(r1, r2)
        self.assertEqual(weakref.getweakrefs(o), [r2, r1])
        self.assertEqual(weakref.getweakrefcount(o), 2)
        r3 = MyRef(o)
        self.assertEqual(weakref.getweakrefcount(o), 3)
        refs = weakref.getweakrefs(o)
        self.assertEqual(len(refs), 3)
        self.assertIs(r2, refs[0])
        self.assertIn(r1, refs[1:])
        self.assertIn(r3, refs[1:])

    def test_subclass_refs_dont_conflate_callbacks(self):
        class MyRef(weakref.ref):
            pass
        o = Object(42)
        r1 = MyRef(o, id)
        r2 = MyRef(o, str)
        self.assertIsNot(r1, r2)
        refs = weakref.getweakrefs(o)
        self.assertIn(r1, refs)
        self.assertIn(r2, refs)

    def test_subclass_refs_with_slots(self):
        class MyRef(weakref.ref):
            __slots__ = "slot1", "slot2"
            def __new__(type, ob, callback, slot1, slot2):
                return weakref.ref.__new__(type, ob, callback)
            def __init__(self, ob, callback, slot1, slot2):
                self.slot1 = slot1
                self.slot2 = slot2
            def meth(self):
                return self.slot1 + self.slot2
        o = Object(42)
        r = MyRef(o, None, "abc", "def")
        self.assertEqual(r.slot1, "abc")
        self.assertEqual(r.slot2, "def")
        self.assertEqual(r.meth(), "abcdef")
        self.assertFalse(hasattr(r, "__dict__"))

    def test_subclass_refs_with_cycle(self):
        # Bug #3110
        # An instance of a weakref subclass can have attributes.
        # If such a weakref holds the only strong reference to the object,
        # deleting the weakref will delete the object. In this case,
        # the callback must not be called, because the ref object is
        # being deleted.
        class MyRef(weakref.ref):
            pass

        # Use a local callback, for "regrtest -R::"
        # to detect refcounting problems
        def callback(w):
            self.cbcalled += 1

        o = C()
        r1 = MyRef(o, callback)
        r1.o = o
        del o

        del r1 # Used to crash here

        self.assertEqual(self.cbcalled, 0)

        # Same test, with two weakrefs to the same object
        # (since code paths are different)
        o = C()
        r1 = MyRef(o, callback)
        r2 = MyRef(o, callback)
        r1.r = r2
        r2.o = o
        del o
        del r2

        del r1 # Used to crash here

        self.assertEqual(self.cbcalled, 0)


class WeakMethodTestCase(unittest.TestCase):

    def _subclass(self):
        """Return a Object subclass overriding `some_method`."""
        class C(Object):
            def some_method(self):
                return 6
        return C

    def test_alive(self):
        o = Object(1)
        r = weakref.WeakMethod(o.some_method)
        self.assertIsInstance(r, weakref.ReferenceType)
        self.assertIsInstance(r(), type(o.some_method))
        self.assertIs(r().__self__, o)
        self.assertIs(r().__func__, o.some_method.__func__)
        self.assertEqual(r()(), 4)

    def test_object_dead(self):
        o = Object(1)
        r = weakref.WeakMethod(o.some_method)
        del o
        gc.collect()
        self.assertIs(r(), None)

    def test_method_dead(self):
        C = self._subclass()
        o = C(1)
        r = weakref.WeakMethod(o.some_method)
        del C.some_method
        gc.collect()
        self.assertIs(r(), None)

    def test_callback_when_object_dead(self):
        # Test callback behaviour when object dies first.
        C = self._subclass()
        calls = []
        def cb(arg):
            calls.append(arg)
        o = C(1)
        r = weakref.WeakMethod(o.some_method, cb)
        del o
        gc.collect()
        self.assertEqual(calls, [r])
        # Callback is only called once.
        C.some_method = Object.some_method
        gc.collect()
        self.assertEqual(calls, [r])

    def test_callback_when_method_dead(self):
        # Test callback behaviour when method dies first.
        C = self._subclass()
        calls = []
        def cb(arg):
            calls.append(arg)
        o = C(1)
        r = weakref.WeakMethod(o.some_method, cb)
        del C.some_method
        gc.collect()
        self.assertEqual(calls, [r])
        # Callback is only called once.
        del o
        gc.collect()
        self.assertEqual(calls, [r])

    @support.cpython_only
    def test_no_cycles(self):
        # A WeakMethod doesn't create any reference cycle to itself.
        o = Object(1)
        def cb(_):
            pass
        r = weakref.WeakMethod(o.some_method, cb)
        wr = weakref.ref(r)
        del r
        self.assertIs(wr(), None)

    def test_equality(self):
        def _eq(a, b):
            self.assertTrue(a == b)
            self.assertFalse(a != b)
        def _ne(a, b):
            self.assertTrue(a != b)
            self.assertFalse(a == b)
        x = Object(1)
        y = Object(1)
        a = weakref.WeakMethod(x.some_method)
        b = weakref.WeakMethod(y.some_method)
        c = weakref.WeakMethod(x.other_method)
        d = weakref.WeakMethod(y.other_method)
        # Objects equal, same method
        _eq(a, b)
        _eq(c, d)
        # Objects equal, different method
        _ne(a, c)
        _ne(a, d)
        _ne(b, c)
        _ne(b, d)
        # Objects unequal, same or different method
        z = Object(2)
        e = weakref.WeakMethod(z.some_method)
        f = weakref.WeakMethod(z.other_method)
        _ne(a, e)
        _ne(a, f)
        _ne(b, e)
        _ne(b, f)
        del x, y, z
        gc.collect()
        # Dead WeakMethods compare by identity
        refs = a, b, c, d, e, f
        for q in refs:
            for r in refs:
                self.assertEqual(q == r, q is r)
                self.assertEqual(q != r, q is not r)

    def test_hashing(self):
        # Alive WeakMethods are hashable if the underlying object is
        # hashable.
        x = Object(1)
        y = Object(1)
        a = weakref.WeakMethod(x.some_method)
        b = weakref.WeakMethod(y.some_method)
        c = weakref.WeakMethod(y.other_method)
        # Since WeakMethod objects are equal, the hashes should be equal.
        self.assertEqual(hash(a), hash(b))
        ha = hash(a)
        # Dead WeakMethods retain their old hash value
        del x, y
        gc.collect()
        self.assertEqual(hash(a), ha)
        self.assertEqual(hash(b), ha)
        # If it wasn't hashed when alive, a dead WeakMethod cannot be hashed.
        self.assertRaises(TypeError, hash, c)


class MappingTestCase(TestBase):

    COUNT = 10

    def check_len_cycles(self, dict_type, cons):
        N = 20
        items = [RefCycle() for i in range(N)]
        dct = dict_type(cons(o) for o in items)
        # Keep an iterator alive
        it = dct.items()
        try:
            next(it)
        except StopIteration:
            pass
        del items
        gc.collect()
        n1 = len(dct)
        del it
        gc.collect()
        n2 = len(dct)
        # one item may be kept alive inside the iterator
        self.assertIn(n1, (0, 1))
        self.assertEqual(n2, 0)

    def test_weak_keyed_len_cycles(self):
        self.check_len_cycles(weakref.WeakKeyDictionary, lambda k: (k, 1))

    def test_weak_valued_len_cycles(self):
        self.check_len_cycles(weakref.WeakValueDictionary, lambda k: (1, k))

    def check_len_race(self, dict_type, cons):
        # Extended sanity checks for len() in the face of cyclic collection
        self.addCleanup(gc.set_threshold, *gc.get_threshold())
        for th in range(1, 100):
            N = 20
            gc.collect(0)
            gc.set_threshold(th, th, th)
            items = [RefCycle() for i in range(N)]
            dct = dict_type(cons(o) for o in items)
            del items
            # All items will be collected at next garbage collection pass
            it = dct.items()
            try:
                next(it)
            except StopIteration:
                pass
            n1 = len(dct)
            del it
            n2 = len(dct)
            self.assertGreaterEqual(n1, 0)
            self.assertLessEqual(n1, N)
            self.assertGreaterEqual(n2, 0)
            self.assertLessEqual(n2, n1)

    def test_weak_keyed_len_race(self):
        self.check_len_race(weakref.WeakKeyDictionary, lambda k: (k, 1))

    def test_weak_valued_len_race(self):
        self.check_len_race(weakref.WeakValueDictionary, lambda k: (1, k))

    def test_weak_values(self):
        #
        #  This exercises d.copy(), d.items(), d[], del d[], len(d).
        #
        dict, objects = self.make_weak_valued_dict()
        for o in objects:
            self.assertEqual(weakref.getweakrefcount(o), 1)
            self.assertIs(o, dict[o.arg],
                         "wrong object returned by weak dict!")
        items1 = list(dict.items())
        items2 = list(dict.copy().items())
        items1.sort()
        items2.sort()
        self.assertEqual(items1, items2,
                     "cloning of weak-valued dictionary did not work!")
        del items1, items2
        self.assertEqual(len(dict), self.COUNT)
        del objects[0]
        self.assertEqual(len(dict), self.COUNT - 1,
                     "deleting object did not cause dictionary update")
        del objects, o
        self.assertEqual(len(dict), 0,
                     "deleting the values did not clear the dictionary")
        # regression on SF bug #447152:
        dict = weakref.WeakValueDictionary()
        self.assertRaises(KeyError, dict.__getitem__, 1)
        dict[2] = C()
        self.assertRaises(KeyError, dict.__getitem__, 2)

    def test_weak_keys(self):
        #
        #  This exercises d.copy(), d.items(), d[] = v, d[], del d[],
        #  len(d), k in d.
        #
        dict, objects = self.make_weak_keyed_dict()
        for o in objects:
            self.assertEqual(weakref.getweakrefcount(o), 1,
                         "wrong number of weak references to %r!" % o)
            self.assertIs(o.arg, dict[o],
                         "wrong object returned by weak dict!")
        items1 = dict.items()
        items2 = dict.copy().items()
        self.assertEqual(set(items1), set(items2),
                     "cloning of weak-keyed dictionary did not work!")
        del items1, items2
        self.assertEqual(len(dict), self.COUNT)
        del objects[0]
        self.assertEqual(len(dict), (self.COUNT - 1),
                     "deleting object did not cause dictionary update")
        del objects, o
        self.assertEqual(len(dict), 0,
                     "deleting the keys did not clear the dictionary")
        o = Object(42)
        dict[o] = "What is the meaning of the universe?"
        self.assertIn(o, dict)
        self.assertNotIn(34, dict)

    def test_weak_keyed_iters(self):
        dict, objects = self.make_weak_keyed_dict()
        self.check_iters(dict)

        # Test keyrefs()
        refs = dict.keyrefs()
        self.assertEqual(len(refs), len(objects))
        objects2 = list(objects)
        for wr in refs:
            ob = wr()
            self.assertIn(ob, dict)
            self.assertIn(ob, dict)
            self.assertEqual(ob.arg, dict[ob])
            objects2.remove(ob)
        self.assertEqual(len(objects2), 0)

        # Test iterkeyrefs()
        objects2 = list(objects)
        self.assertEqual(len(list(dict.keyrefs())), len(objects))
        for wr in dict.keyrefs():
            ob = wr()
            self.assertIn(ob, dict)
            self.assertIn(ob, dict)
            self.assertEqual(ob.arg, dict[ob])
            objects2.remove(ob)
        self.assertEqual(len(objects2), 0)

    def test_weak_valued_iters(self):
        dict, objects = self.make_weak_valued_dict()
        self.check_iters(dict)

        # Test valuerefs()
        refs = dict.valuerefs()
        self.assertEqual(len(refs), len(objects))
        objects2 = list(objects)
        for wr in refs:
            ob = wr()
            self.assertEqual(ob, dict[ob.arg])
            self.assertEqual(ob.arg, dict[ob.arg].arg)
            objects2.remove(ob)
        self.assertEqual(len(objects2), 0)

        # Test itervaluerefs()
        objects2 = list(objects)
        self.assertEqual(len(list(dict.itervaluerefs())), len(objects))
        for wr in dict.itervaluerefs():
            ob = wr()
            self.assertEqual(ob, dict[ob.arg])
            self.assertEqual(ob.arg, dict[ob.arg].arg)
            objects2.remove(ob)
        self.assertEqual(len(objects2), 0)

    def check_iters(self, dict):
        # item iterator:
        items = list(dict.items())
        for item in dict.items():
            items.remove(item)
        self.assertFalse(items, "items() did not touch all items")

        # key iterator, via __iter__():
        keys = list(dict.keys())
        for k in dict:
            keys.remove(k)
        self.assertFalse(keys, "__iter__() did not touch all keys")

        # key iterator, via iterkeys():
        keys = list(dict.keys())
        for k in dict.keys():
            keys.remove(k)
        self.assertFalse(keys, "iterkeys() did not touch all keys")

        # value iterator:
        values = list(dict.values())
        for v in dict.values():
            values.remove(v)
        self.assertFalse(values,
                     "itervalues() did not touch all values")

    def check_weak_destroy_while_iterating(self, dict, objects, iter_name):
        n = len(dict)
        it = iter(getattr(dict, iter_name)())
        next(it)             # Trigger internal iteration
        # Destroy an object
        del objects[-1]
        gc.collect()    # just in case
        # We have removed either the first consumed object, or another one
        self.assertIn(len(list(it)), [len(objects), len(objects) - 1])
        del it
        # The removal has been committed
        self.assertEqual(len(dict), n - 1)

    def check_weak_destroy_and_mutate_while_iterating(self, dict, testcontext):
        # Check that we can explicitly mutate the weak dict without
        # interfering with delayed removal.
        # `testcontext` should create an iterator, destroy one of the
        # weakref'ed objects and then return a new key/value pair corresponding
        # to the destroyed object.
        with testcontext() as (k, v):
            self.assertNotIn(k, dict)
        with testcontext() as (k, v):
            self.assertRaises(KeyError, dict.__delitem__, k)
        self.assertNotIn(k, dict)
        with testcontext() as (k, v):
            self.assertRaises(KeyError, dict.pop, k)
        self.assertNotIn(k, dict)
        with testcontext() as (k, v):
            dict[k] = v
        self.assertEqual(dict[k], v)
        ddict = copy.copy(dict)
        with testcontext() as (k, v):
            dict.update(ddict)
        self.assertEqual(dict, ddict)
        with testcontext() as (k, v):
            dict.clear()
        self.assertEqual(len(dict), 0)

    def test_weak_keys_destroy_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        dict, objects = self.make_weak_keyed_dict()
        self.check_weak_destroy_while_iterating(dict, objects, 'keys')
        self.check_weak_destroy_while_iterating(dict, objects, 'items')
        self.check_weak_destroy_while_iterating(dict, objects, 'values')
        self.check_weak_destroy_while_iterating(dict, objects, 'keyrefs')
        dict, objects = self.make_weak_keyed_dict()
        @contextlib.contextmanager
        def testcontext():
            try:
                it = iter(dict.items())
                next(it)
                # Schedule a key/value for removal and recreate it
                v = objects.pop().arg
                gc.collect()      # just in case
                yield Object(v), v
            finally:
                it = None           # should commit all removals
        self.check_weak_destroy_and_mutate_while_iterating(dict, testcontext)

    def test_weak_values_destroy_while_iterating(self):
        # Issue #7105: iterators shouldn't crash when a key is implicitly removed
        dict, objects = self.make_weak_valued_dict()
        self.check_weak_destroy_while_iterating(dict, objects, 'keys')
        self.check_weak_destroy_while_iterating(dict, objects, 'items')
        self.check_weak_destroy_while_iterating(dict, objects, 'values')
        self.check_weak_destroy_while_iterating(dict, objects, 'itervaluerefs')
        self.check_weak_destroy_while_iterating(dict, objects, 'valuerefs')
        dict, objects = self.make_weak_valued_dict()
        @contextlib.contextmanager
        def testcontext():
            try:
                it = iter(dict.items())
                next(it)
                # Schedule a key/value for removal and recreate it
                k = objects.pop().arg
                gc.collect()      # just in case
                yield k, Object(k)
            finally:
                it = None           # should commit all removals
        self.check_weak_destroy_and_mutate_while_iterating(dict, testcontext)

    def test_make_weak_keyed_dict_from_dict(self):
        o = Object(3)
        dict = weakref.WeakKeyDictionary({o:364})
        self.assertEqual(dict[o], 364)

    def test_make_weak_keyed_dict_from_weak_keyed_dict(self):
        o = Object(3)
        dict = weakref.WeakKeyDictionary({o:364})
        dict2 = weakref.WeakKeyDictionary(dict)
        self.assertEqual(dict[o], 364)

    def make_weak_keyed_dict(self):
        dict = weakref.WeakKeyDictionary()
        objects = list(map(Object, range(self.COUNT)))
        for o in objects:
            dict[o] = o.arg
        return dict, objects

    def test_make_weak_valued_dict_from_dict(self):
        o = Object(3)
        dict = weakref.WeakValueDictionary({364:o})
        self.assertEqual(dict[364], o)

    def test_make_weak_valued_dict_from_weak_valued_dict(self):
        o = Object(3)
        dict = weakref.WeakValueDictionary({364:o})
        dict2 = weakref.WeakValueDictionary(dict)
        self.assertEqual(dict[364], o)

    def make_weak_valued_dict(self):
        dict = weakref.WeakValueDictionary()
        objects = list(map(Object, range(self.COUNT)))
        for o in objects:
            dict[o.arg] = o
        return dict, objects

    def check_popitem(self, klass, key1, value1, key2, value2):
        weakdict = klass()
        weakdict[key1] = value1
        weakdict[key2] = value2
        self.assertEqual(len(weakdict), 2)
        k, v = weakdict.popitem()
        self.assertEqual(len(weakdict), 1)
        if k is key1:
            self.assertIs(v, value1)
        else:
            self.assertIs(v, value2)
        k, v = weakdict.popitem()
        self.assertEqual(len(weakdict), 0)
        if k is key1:
            self.assertIs(v, value1)
        else:
            self.assertIs(v, value2)

    def test_weak_valued_dict_popitem(self):
        self.check_popitem(weakref.WeakValueDictionary,
                           "key1", C(), "key2", C())

    def test_weak_keyed_dict_popitem(self):
        self.check_popitem(weakref.WeakKeyDictionary,
                           C(), "value 1", C(), "value 2")

    def check_setdefault(self, klass, key, value1, value2):
        self.assertIsNot(value1, value2,
                     "invalid test"
                     " -- value parameters must be distinct objects")
        weakdict = klass()
        o = weakdict.setdefault(key, value1)
        self.assertIs(o, value1)
        self.assertIn(key, weakdict)
        self.assertIs(weakdict.get(key), value1)
        self.assertIs(weakdict[key], value1)

        o = weakdict.setdefault(key, value2)
        self.assertIs(o, value1)
        self.assertIn(key, weakdict)
        self.assertIs(weakdict.get(key), value1)
        self.assertIs(weakdict[key], value1)

    def test_weak_valued_dict_setdefault(self):
        self.check_setdefault(weakref.WeakValueDictionary,
                              "key", C(), C())

    def test_weak_keyed_dict_setdefault(self):
        self.check_setdefault(weakref.WeakKeyDictionary,
                              C(), "value 1", "value 2")

    def check_update(self, klass, dict):
        #
        #  This exercises d.update(), len(d), d.keys(), k in d,
        #  d.get(), d[].
        #
        weakdict = klass()
        weakdict.update(dict)
        self.assertEqual(len(weakdict), len(dict))
        for k in weakdict.keys():
            self.assertIn(k, dict, "mysterious new key appeared in weak dict")
            v = dict.get(k)
            self.assertIs(v, weakdict[k])
            self.assertIs(v, weakdict.get(k))
        for k in dict.keys():
            self.assertIn(k, weakdict, "original key disappeared in weak dict")
            v = dict[k]
            self.assertIs(v, weakdict[k])
            self.assertIs(v, weakdict.get(k))

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
        self.assertEqual(len(d), 2)
        del d[o1]
        self.assertEqual(len(d), 1)
        self.assertEqual(list(d.keys()), [o2])

    def test_weak_valued_delitem(self):
        d = weakref.WeakValueDictionary()
        o1 = Object('1')
        o2 = Object('2')
        d['something'] = o1
        d['something else'] = o2
        self.assertEqual(len(d), 2)
        del d['something']
        self.assertEqual(len(d), 1)
        self.assertEqual(list(d.items()), [('something else', o2)])

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
        objs = list(d.keys())
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

from test import mapping_tests

class WeakValueDictionaryTestCase(mapping_tests.BasicTestMappingProtocol):
    """Check that WeakValueDictionary conforms to the mapping protocol"""
    __ref = {"key1":Object(1), "key2":Object(2), "key3":Object(3)}
    type2test = weakref.WeakValueDictionary
    def _reference(self):
        return self.__ref.copy()

class WeakKeyDictionaryTestCase(mapping_tests.BasicTestMappingProtocol):
    """Check that WeakKeyDictionary conforms to the mapping protocol"""
    __ref = {Object("key1"):1, Object("key2"):2, Object("key3"):3}
    type2test = weakref.WeakKeyDictionary
    def _reference(self):
        return self.__ref.copy()


class FinalizeTestCase(unittest.TestCase):

    class A:
        pass

    def _collect_if_necessary(self):
        # we create no ref-cycles so in CPython no gc should be needed
        if sys.implementation.name != 'cpython':
            support.gc_collect()

    def test_finalize(self):
        def add(x,y,z):
            res.append(x + y + z)
            return x + y + z

        a = self.A()

        res = []
        f = weakref.finalize(a, add, 67, 43, z=89)
        self.assertEqual(f.alive, True)
        self.assertEqual(f.peek(), (a, add, (67,43), {'z':89}))
        self.assertEqual(f(), 199)
        self.assertEqual(f(), None)
        self.assertEqual(f(), None)
        self.assertEqual(f.peek(), None)
        self.assertEqual(f.detach(), None)
        self.assertEqual(f.alive, False)
        self.assertEqual(res, [199])

        res = []
        f = weakref.finalize(a, add, 67, 43, 89)
        self.assertEqual(f.peek(), (a, add, (67,43,89), {}))
        self.assertEqual(f.detach(), (a, add, (67,43,89), {}))
        self.assertEqual(f(), None)
        self.assertEqual(f(), None)
        self.assertEqual(f.peek(), None)
        self.assertEqual(f.detach(), None)
        self.assertEqual(f.alive, False)
        self.assertEqual(res, [])

        res = []
        f = weakref.finalize(a, add, x=67, y=43, z=89)
        del a
        self._collect_if_necessary()
        self.assertEqual(f(), None)
        self.assertEqual(f(), None)
        self.assertEqual(f.peek(), None)
        self.assertEqual(f.detach(), None)
        self.assertEqual(f.alive, False)
        self.assertEqual(res, [199])

    def test_order(self):
        a = self.A()
        res = []

        f1 = weakref.finalize(a, res.append, 'f1')
        f2 = weakref.finalize(a, res.append, 'f2')
        f3 = weakref.finalize(a, res.append, 'f3')
        f4 = weakref.finalize(a, res.append, 'f4')
        f5 = weakref.finalize(a, res.append, 'f5')

        # make sure finalizers can keep themselves alive
        del f1, f4

        self.assertTrue(f2.alive)
        self.assertTrue(f3.alive)
        self.assertTrue(f5.alive)

        self.assertTrue(f5.detach())
        self.assertFalse(f5.alive)

        f5()                       # nothing because previously unregistered
        res.append('A')
        f3()                       # => res.append('f3')
        self.assertFalse(f3.alive)
        res.append('B')
        f3()                       # nothing because previously called
        res.append('C')
        del a
        self._collect_if_necessary()
                                   # => res.append('f4')
                                   # => res.append('f2')
                                   # => res.append('f1')
        self.assertFalse(f2.alive)
        res.append('D')
        f2()                       # nothing because previously called by gc

        expected = ['A', 'f3', 'B', 'C', 'f4', 'f2', 'f1', 'D']
        self.assertEqual(res, expected)

    def test_all_freed(self):
        # we want a weakrefable subclass of weakref.finalize
        class MyFinalizer(weakref.finalize):
            pass

        a = self.A()
        res = []
        def callback():
            res.append(123)
        f = MyFinalizer(a, callback)

        wr_callback = weakref.ref(callback)
        wr_f = weakref.ref(f)
        del callback, f

        self.assertIsNotNone(wr_callback())
        self.assertIsNotNone(wr_f())

        del a
        self._collect_if_necessary()

        self.assertIsNone(wr_callback())
        self.assertIsNone(wr_f())
        self.assertEqual(res, [123])

    @classmethod
    def run_in_child(cls):
        def error():
            # Create an atexit finalizer from inside a finalizer called
            # at exit.  This should be the next to be run.
            g1 = weakref.finalize(cls, print, 'g1')
            print('f3 error')
            1/0

        # cls should stay alive till atexit callbacks run
        f1 = weakref.finalize(cls, print, 'f1', _global_var)
        f2 = weakref.finalize(cls, print, 'f2', _global_var)
        f3 = weakref.finalize(cls, error)
        f4 = weakref.finalize(cls, print, 'f4', _global_var)

        assert f1.atexit == True
        f2.atexit = False
        assert f3.atexit == True
        assert f4.atexit == True

    def test_atexit(self):
        prog = ('from test.test_weakref import FinalizeTestCase;'+
                'FinalizeTestCase.run_in_child()')
        rc, out, err = script_helper.assert_python_ok('-c', prog)
        out = out.decode('ascii').splitlines()
        self.assertEqual(out, ['f4 foobar', 'f3 error', 'g1', 'f1 foobar'])
        self.assertTrue(b'ZeroDivisionError' in err)


libreftest = """ Doctest for examples in the library reference: weakref.rst

>>> import weakref
>>> class Dict(dict):
...     pass
...
>>> obj = Dict(red=1, green=2, blue=3)   # this object is weak referencable
>>> r = weakref.ref(obj)
>>> print(r() is obj)
True

>>> import weakref
>>> class Object:
...     pass
...
>>> o = Object()
>>> r = weakref.ref(o)
>>> o2 = r()
>>> o is o2
True
>>> del o, o2
>>> print(r())
None

>>> import weakref
>>> class ExtendedRef(weakref.ref):
...     def __init__(self, ob, callback=None, **annotations):
...         super().__init__(ob, callback)
...         self.__counter = 0
...         for k, v in annotations.items():
...             setattr(self, k, v)
...     def __call__(self):
...         '''Return a pair containing the referent and the number of
...         times the reference has been called.
...         '''
...         ob = super().__call__()
...         if ob is not None:
...             self.__counter += 1
...             ob = (ob, self.__counter)
...         return ob
...
>>> class A:   # not in docs from here, just testing the ExtendedRef
...     pass
...
>>> a = A()
>>> r = ExtendedRef(a, foo=1, bar="baz")
>>> r.foo
1
>>> r.bar
'baz'
>>> r()[1]
1
>>> r()[1]
2
>>> r()[0] is a
True


>>> import weakref
>>> _id2obj_dict = weakref.WeakValueDictionary()
>>> def remember(obj):
...     oid = id(obj)
...     _id2obj_dict[oid] = obj
...     return oid
...
>>> def id2obj(oid):
...     return _id2obj_dict[oid]
...
>>> a = A()             # from here, just testing
>>> a_id = remember(a)
>>> id2obj(a_id) is a
True
>>> del a
>>> try:
...     id2obj(a_id)
... except KeyError:
...     print('OK')
... else:
...     print('WeakValueDictionary error')
OK

"""

__test__ = {'libreftest' : libreftest}

def test_main():
    support.run_unittest(
        ReferencesTestCase,
        WeakMethodTestCase,
        MappingTestCase,
        WeakValueDictionaryTestCase,
        WeakKeyDictionaryTestCase,
        SubclassableWeakrefTestCase,
        FinalizeTestCase,
        )
    support.run_doctest(sys.modules[__name__])


if __name__ == "__main__":
    test_main()

import unittest
from test.support import verbose, run_unittest
import sys
import gc
import weakref

### Support code
###############################################################################

# Bug 1055820 has several tests of longstanding bugs involving weakrefs and
# cyclic gc.

# An instance of C1055820 has a self-loop, so becomes cyclic trash when
# unreachable.
class C1055820(object):
    def __init__(self, i):
        self.i = i
        self.loop = self

class GC_Detector(object):
    # Create an instance I.  Then gc hasn't happened again so long as
    # I.gc_happened is false.

    def __init__(self):
        self.gc_happened = False

        def it_happened(ignored):
            self.gc_happened = True

        # Create a piece of cyclic trash that triggers it_happened when
        # gc collects it.
        self.wr = weakref.ref(C1055820(666), it_happened)


### Tests
###############################################################################

class GCTests(unittest.TestCase):
    def test_list(self):
        l = []
        l.append(l)
        gc.collect()
        del l
        self.assertEqual(gc.collect(), 1)

    def test_dict(self):
        d = {}
        d[1] = d
        gc.collect()
        del d
        self.assertEqual(gc.collect(), 1)

    def test_tuple(self):
        # since tuples are immutable we close the loop with a list
        l = []
        t = (l,)
        l.append(t)
        gc.collect()
        del t
        del l
        self.assertEqual(gc.collect(), 2)

    def test_class(self):
        class A:
            pass
        A.a = A
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_newstyleclass(self):
        class A(object):
            pass
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_instance(self):
        class A:
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    def test_newinstance(self):
        class A(object):
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        class B(list):
            pass
        class C(B, A):
            pass
        a = C()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        del B, C
        self.assertNotEqual(gc.collect(), 0)
        A.a = A()
        del A
        self.assertNotEqual(gc.collect(), 0)
        self.assertEqual(gc.collect(), 0)

    def test_method(self):
        # Tricky: self.__init__ is a bound method, it references the instance.
        class A:
            def __init__(self):
                self.init = self.__init__
        a = A()
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    def test_finalizer(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        class A:
            def __del__(self): pass
        class B:
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    def test_finalizer_newclass(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        class A(object):
            def __del__(self): pass
        class B(object):
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    def test_function(self):
        # Tricky: f -> d -> f, code should call d.clear() after the exec to
        # break the cycle.
        d = {}
        exec("def f(): pass\n", d)
        gc.collect()
        del d
        self.assertEqual(gc.collect(), 2)

    def test_frame(self):
        def f():
            frame = sys._getframe()
        gc.collect()
        f()
        self.assertEqual(gc.collect(), 1)

    def test_saveall(self):
        # Verify that cyclic garbage like lists show up in gc.garbage if the
        # SAVEALL option is enabled.

        # First make sure we don't save away other stuff that just happens to
        # be waiting for collection.
        gc.collect()
        # if this fails, someone else created immortal trash
        self.assertEqual(gc.garbage, [])

        L = []
        L.append(L)
        id_L = id(L)

        debug = gc.get_debug()
        gc.set_debug(debug | gc.DEBUG_SAVEALL)
        del L
        gc.collect()
        gc.set_debug(debug)

        self.assertEqual(len(gc.garbage), 1)
        obj = gc.garbage.pop()
        self.assertEqual(id(obj), id_L)

    def test_del(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A:
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    def test_del_newclass(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A(object):
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    # The following two tests are fragile:
    # They precisely count the number of allocations,
    # which is highly implementation-dependent.
    # For example, disposed tuples are not freed, but reused.
    # To minimize variations, though, we first store the get_count() results
    # and check them at the end.
    def test_get_count(self):
        gc.collect()
        a, b, c = gc.get_count()
        x = []
        d, e, f = gc.get_count()
        self.assertEqual((b, c), (0, 0))
        self.assertEqual((e, f), (0, 0))
        # This is less fragile than asserting that a equals 0.
        self.assertLess(a, 5)
        # Between the two calls to get_count(), at least one object was
        # created (the list).
        self.assertGreater(d, a)

    def test_collect_generations(self):
        gc.collect()
        # This object will "trickle" into generation N + 1 after
        # each call to collect(N)
        x = []
        gc.collect(0)
        # x is now in gen 1
        a, b, c = gc.get_count()
        gc.collect(1)
        # x is now in gen 2
        d, e, f = gc.get_count()
        gc.collect(2)
        # x is now in gen 3
        g, h, i = gc.get_count()
        # We don't check a, d, g since their exact values depends on
        # internal implementation details of the interpreter.
        self.assertEqual((b, c), (1, 0))
        self.assertEqual((e, f), (0, 1))
        self.assertEqual((h, i), (0, 0))

    def test_trashcan(self):
        class Ouch:
            n = 0
            def __del__(self):
                Ouch.n = Ouch.n + 1
                if Ouch.n % 17 == 0:
                    gc.collect()

        # "trashcan" is a hack to prevent stack overflow when deallocating
        # very deeply nested tuples etc.  It works in part by abusing the
        # type pointer and refcount fields, and that can yield horrible
        # problems when gc tries to traverse the structures.
        # If this test fails (as it does in 2.0, 2.1 and 2.2), it will
        # most likely die via segfault.

        # Note:  In 2.3 the possibility for compiling without cyclic gc was
        # removed, and that in turn allows the trashcan mechanism to work
        # via much simpler means (e.g., it never abuses the type pointer or
        # refcount fields anymore).  Since it's much less likely to cause a
        # problem now, the various constants in this expensive (we force a lot
        # of full collections) test are cut back from the 2.2 version.
        gc.enable()
        N = 150
        for count in range(2):
            t = []
            for i in range(N):
                t = [t, Ouch()]
            u = []
            for i in range(N):
                u = [u, Ouch()]
            v = {}
            for i in range(N):
                v = {1: v, 2: Ouch()}
        gc.disable()

    def test_boom(self):
        class Boom:
            def __getattr__(self, someattribute):
                del self.attr
                raise AttributeError

        a = Boom()
        b = Boom()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # a<->b are in a trash cycle now.  Collection will invoke
        # Boom.__getattr__ (to see whether a and b have __del__ methods), and
        # __getattr__ deletes the internal "attr" attributes as a side effect.
        # That causes the trash cycle to get reclaimed via refcounts falling to
        # 0, thus mutating the trash graph as a side effect of merely asking
        # whether __del__ exists.  This used to (before 2.3b1) crash Python.
        # Now __getattr__ isn't called.
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom2(self):
        class Boom2:
            def __init__(self):
                self.x = 0

            def __getattr__(self, someattribute):
                self.x += 1
                if self.x > 1:
                    del self.attr
                raise AttributeError

        a = Boom2()
        b = Boom2()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # Much like test_boom(), except that __getattr__ doesn't break the
        # cycle until the second time gc checks for __del__.  As of 2.3b1,
        # there isn't a second time, so this simply cleans up the trash cycle.
        # We expect a, b, a.__dict__ and b.__dict__ (4 objects) to get
        # reclaimed this way.
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom_new(self):
        # boom__new and boom2_new are exactly like boom and boom2, except use
        # new-style classes.

        class Boom_New(object):
            def __getattr__(self, someattribute):
                del self.attr
                raise AttributeError

        a = Boom_New()
        b = Boom_New()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom2_new(self):
        class Boom2_New(object):
            def __init__(self):
                self.x = 0

            def __getattr__(self, someattribute):
                self.x += 1
                if self.x > 1:
                    del self.attr
                raise AttributeError

        a = Boom2_New()
        b = Boom2_New()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_get_referents(self):
        alist = [1, 3, 5]
        got = gc.get_referents(alist)
        got.sort()
        self.assertEqual(got, alist)

        atuple = tuple(alist)
        got = gc.get_referents(atuple)
        got.sort()
        self.assertEqual(got, alist)

        adict = {1: 3, 5: 7}
        expected = [1, 3, 5, 7]
        got = gc.get_referents(adict)
        got.sort()
        self.assertEqual(got, expected)

        got = gc.get_referents([1, 2], {3: 4}, (0, 0, 0))
        got.sort()
        self.assertEqual(got, [0, 0] + list(range(5)))

        self.assertEqual(gc.get_referents(1, 'a', 4j), [])

    def test_is_tracked(self):
        # Atomic built-in types are not tracked, user-defined objects and
        # mutable containers are.
        # NOTE: types with special optimizations (e.g. tuple) have tests
        # in their own test files instead.
        self.assertFalse(gc.is_tracked(None))
        self.assertFalse(gc.is_tracked(1))
        self.assertFalse(gc.is_tracked(1.0))
        self.assertFalse(gc.is_tracked(1.0 + 5.0j))
        self.assertFalse(gc.is_tracked(True))
        self.assertFalse(gc.is_tracked(False))
        self.assertFalse(gc.is_tracked(b"a"))
        self.assertFalse(gc.is_tracked("a"))
        self.assertFalse(gc.is_tracked(bytearray(b"a")))
        self.assertFalse(gc.is_tracked(type))
        self.assertFalse(gc.is_tracked(int))
        self.assertFalse(gc.is_tracked(object))
        self.assertFalse(gc.is_tracked(object()))

        class UserClass:
            pass
        self.assertTrue(gc.is_tracked(gc))
        self.assertTrue(gc.is_tracked(UserClass))
        self.assertTrue(gc.is_tracked(UserClass()))
        self.assertTrue(gc.is_tracked([]))
        self.assertTrue(gc.is_tracked(set()))

    def test_bug1055820b(self):
        # Corresponds to temp2b.py in the bug report.

        ouch = []
        def callback(ignored):
            ouch[:] = [wr() for wr in WRs]

        Cs = [C1055820(i) for i in range(2)]
        WRs = [weakref.ref(c, callback) for c in Cs]
        c = None

        gc.collect()
        self.assertEqual(len(ouch), 0)
        # Make the two instances trash, and collect again.  The bug was that
        # the callback materialized a strong reference to an instance, but gc
        # cleared the instance's dict anyway.
        Cs = None
        gc.collect()
        self.assertEqual(len(ouch), 2)  # else the callbacks didn't run
        for x in ouch:
            # If the callback resurrected one of these guys, the instance
            # would be damaged, with an empty __dict__.
            self.assertEqual(x, None)

class GCTogglingTests(unittest.TestCase):
    def setUp(self):
        gc.enable()

    def tearDown(self):
        gc.disable()

    def test_bug1055820c(self):
        # Corresponds to temp2c.py in the bug report.  This is pretty
        # elaborate.

        c0 = C1055820(0)
        # Move c0 into generation 2.
        gc.collect()

        c1 = C1055820(1)
        c1.keep_c0_alive = c0
        del c0.loop # now only c1 keeps c0 alive

        c2 = C1055820(2)
        c2wr = weakref.ref(c2) # no callback!

        ouch = []
        def callback(ignored):
            ouch[:] = [c2wr()]

        # The callback gets associated with a wr on an object in generation 2.
        c0wr = weakref.ref(c0, callback)

        c0 = c1 = c2 = None

        # What we've set up:  c0, c1, and c2 are all trash now.  c0 is in
        # generation 2.  The only thing keeping it alive is that c1 points to
        # it. c1 and c2 are in generation 0, and are in self-loops.  There's a
        # global weakref to c2 (c2wr), but that weakref has no callback.
        # There's also a global weakref to c0 (c0wr), and that does have a
        # callback, and that callback references c2 via c2wr().
        #
        #               c0 has a wr with callback, which references c2wr
        #               ^
        #               |
        #               |     Generation 2 above dots
        #. . . . . . . .|. . . . . . . . . . . . . . . . . . . . . . . .
        #               |     Generation 0 below dots
        #               |
        #               |
        #            ^->c1   ^->c2 has a wr but no callback
        #            |  |    |  |
        #            <--v    <--v
        #
        # So this is the nightmare:  when generation 0 gets collected, we see
        # that c2 has a callback-free weakref, and c1 doesn't even have a
        # weakref.  Collecting generation 0 doesn't see c0 at all, and c0 is
        # the only object that has a weakref with a callback.  gc clears c1
        # and c2.  Clearing c1 has the side effect of dropping the refcount on
        # c0 to 0, so c0 goes away (despite that it's in an older generation)
        # and c0's wr callback triggers.  That in turn materializes a reference
        # to c2 via c2wr(), but c2 gets cleared anyway by gc.

        # We want to let gc happen "naturally", to preserve the distinction
        # between generations.
        junk = []
        i = 0
        detector = GC_Detector()
        while not detector.gc_happened:
            i += 1
            if i > 10000:
                self.fail("gc didn't happen after 10000 iterations")
            self.assertEqual(len(ouch), 0)
            junk.append([])  # this will eventually trigger gc

        self.assertEqual(len(ouch), 1)  # else the callback wasn't invoked
        for x in ouch:
            # If the callback resurrected c2, the instance would be damaged,
            # with an empty __dict__.
            self.assertEqual(x, None)

    def test_bug1055820d(self):
        # Corresponds to temp2d.py in the bug report.  This is very much like
        # test_bug1055820c, but uses a __del__ method instead of a weakref
        # callback to sneak in a resurrection of cyclic trash.

        ouch = []
        class D(C1055820):
            def __del__(self):
                ouch[:] = [c2wr()]

        d0 = D(0)
        # Move all the above into generation 2.
        gc.collect()

        c1 = C1055820(1)
        c1.keep_d0_alive = d0
        del d0.loop # now only c1 keeps d0 alive

        c2 = C1055820(2)
        c2wr = weakref.ref(c2) # no callback!

        d0 = c1 = c2 = None

        # What we've set up:  d0, c1, and c2 are all trash now.  d0 is in
        # generation 2.  The only thing keeping it alive is that c1 points to
        # it.  c1 and c2 are in generation 0, and are in self-loops.  There's
        # a global weakref to c2 (c2wr), but that weakref has no callback.
        # There are no other weakrefs.
        #
        #               d0 has a __del__ method that references c2wr
        #               ^
        #               |
        #               |     Generation 2 above dots
        #. . . . . . . .|. . . . . . . . . . . . . . . . . . . . . . . .
        #               |     Generation 0 below dots
        #               |
        #               |
        #            ^->c1   ^->c2 has a wr but no callback
        #            |  |    |  |
        #            <--v    <--v
        #
        # So this is the nightmare:  when generation 0 gets collected, we see
        # that c2 has a callback-free weakref, and c1 doesn't even have a
        # weakref.  Collecting generation 0 doesn't see d0 at all.  gc clears
        # c1 and c2.  Clearing c1 has the side effect of dropping the refcount
        # on d0 to 0, so d0 goes away (despite that it's in an older
        # generation) and d0's __del__ triggers.  That in turn materializes
        # a reference to c2 via c2wr(), but c2 gets cleared anyway by gc.

        # We want to let gc happen "naturally", to preserve the distinction
        # between generations.
        detector = GC_Detector()
        junk = []
        i = 0
        while not detector.gc_happened:
            i += 1
            if i > 10000:
                self.fail("gc didn't happen after 10000 iterations")
            self.assertEqual(len(ouch), 0)
            junk.append([])  # this will eventually trigger gc

        self.assertEqual(len(ouch), 1)  # else __del__ wasn't invoked
        for x in ouch:
            # If __del__ resurrected c2, the instance would be damaged, with an
            # empty __dict__.
            self.assertEqual(x, None)

def test_main():
    enabled = gc.isenabled()
    gc.disable()
    assert not gc.isenabled()
    debug = gc.get_debug()
    gc.set_debug(debug & ~gc.DEBUG_LEAK) # this test is supposed to leak

    try:
        gc.collect() # Delete 2nd generation garbage
        run_unittest(GCTests, GCTogglingTests)
    finally:
        gc.set_debug(debug)
        # test gc.enable() even if GC is disabled by default
        if verbose:
            print("restoring automatic collection")
        # make sure to always test gc.enable()
        gc.enable()
        assert gc.isenabled()
        if not enabled:
            gc.disable()

if __name__ == "__main__":
    test_main()

from test.test_support import verify, verbose, TestFailed, vereq
import sys
import gc
import weakref

def expect(actual, expected, name):
    if actual != expected:
        raise TestFailed, "test_%s: actual %r, expected %r" % (
            name, actual, expected)

def expect_nonzero(actual, name):
    if actual == 0:
        raise TestFailed, "test_%s: unexpected zero" % name

def run_test(name, thunk):
    if verbose:
        print "testing %s..." % name,
    thunk()
    if verbose:
        print "ok"

def test_list():
    l = []
    l.append(l)
    gc.collect()
    del l
    expect(gc.collect(), 1, "list")

def test_dict():
    d = {}
    d[1] = d
    gc.collect()
    del d
    expect(gc.collect(), 1, "dict")

def test_tuple():
    # since tuples are immutable we close the loop with a list
    l = []
    t = (l,)
    l.append(t)
    gc.collect()
    del t
    del l
    expect(gc.collect(), 2, "tuple")

def test_class():
    class A:
        pass
    A.a = A
    gc.collect()
    del A
    expect_nonzero(gc.collect(), "class")

def test_newstyleclass():
    class A(object):
        pass
    gc.collect()
    del A
    expect_nonzero(gc.collect(), "staticclass")

def test_instance():
    class A:
        pass
    a = A()
    a.a = a
    gc.collect()
    del a
    expect_nonzero(gc.collect(), "instance")

def test_newinstance():
    class A(object):
        pass
    a = A()
    a.a = a
    gc.collect()
    del a
    expect_nonzero(gc.collect(), "newinstance")
    class B(list):
        pass
    class C(B, A):
        pass
    a = C()
    a.a = a
    gc.collect()
    del a
    expect_nonzero(gc.collect(), "newinstance(2)")
    del B, C
    expect_nonzero(gc.collect(), "newinstance(3)")
    A.a = A()
    del A
    expect_nonzero(gc.collect(), "newinstance(4)")
    expect(gc.collect(), 0, "newinstance(5)")

def test_method():
    # Tricky: self.__init__ is a bound method, it references the instance.
    class A:
        def __init__(self):
            self.init = self.__init__
    a = A()
    gc.collect()
    del a
    expect_nonzero(gc.collect(), "method")

def test_finalizer():
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
    expect_nonzero(gc.collect(), "finalizer")
    for obj in gc.garbage:
        if id(obj) == id_a:
            del obj.a
            break
    else:
        raise TestFailed, "didn't find obj in garbage (finalizer)"
    gc.garbage.remove(obj)

def test_finalizer_newclass():
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
    expect_nonzero(gc.collect(), "finalizer")
    for obj in gc.garbage:
        if id(obj) == id_a:
            del obj.a
            break
    else:
        raise TestFailed, "didn't find obj in garbage (finalizer)"
    gc.garbage.remove(obj)

def test_function():
    # Tricky: f -> d -> f, code should call d.clear() after the exec to
    # break the cycle.
    d = {}
    exec("def f(): pass\n") in d
    gc.collect()
    del d
    expect(gc.collect(), 2, "function")

def test_frame():
    def f():
        frame = sys._getframe()
    gc.collect()
    f()
    expect(gc.collect(), 1, "frame")


def test_saveall():
    # Verify that cyclic garbage like lists show up in gc.garbage if the
    # SAVEALL option is enabled.

    # First make sure we don't save away other stuff that just happens to
    # be waiting for collection.
    gc.collect()
    vereq(gc.garbage, []) # if this fails, someone else created immortal trash

    L = []
    L.append(L)
    id_L = id(L)

    debug = gc.get_debug()
    gc.set_debug(debug | gc.DEBUG_SAVEALL)
    del L
    gc.collect()
    gc.set_debug(debug)

    vereq(len(gc.garbage), 1)
    obj = gc.garbage.pop()
    vereq(id(obj), id_L)

def test_del():
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

def test_del_newclass():
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

class Ouch:
    n = 0
    def __del__(self):
        Ouch.n = Ouch.n + 1
        if Ouch.n % 17 == 0:
            gc.collect()

def test_trashcan():
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

class Boom:
    def __getattr__(self, someattribute):
        del self.attr
        raise AttributeError

def test_boom():
    a = Boom()
    b = Boom()
    a.attr = b
    b.attr = a

    gc.collect()
    garbagelen = len(gc.garbage)
    del a, b
    # a<->b are in a trash cycle now.  Collection will invoke Boom.__getattr__
    # (to see whether a and b have __del__ methods), and __getattr__ deletes
    # the internal "attr" attributes as a side effect.  That causes the
    # trash cycle to get reclaimed via refcounts falling to 0, thus mutating
    # the trash graph as a side effect of merely asking whether __del__
    # exists.  This used to (before 2.3b1) crash Python.  Now __getattr__
    # isn't called.
    expect(gc.collect(), 4, "boom")
    expect(len(gc.garbage), garbagelen, "boom")

class Boom2:
    def __init__(self):
        self.x = 0

    def __getattr__(self, someattribute):
        self.x += 1
        if self.x > 1:
            del self.attr
        raise AttributeError

def test_boom2():
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
    # We expect a, b, a.__dict__ and b.__dict__ (4 objects) to get reclaimed
    # this way.
    expect(gc.collect(), 4, "boom2")
    expect(len(gc.garbage), garbagelen, "boom2")

# boom__new and boom2_new are exactly like boom and boom2, except use
# new-style classes.

class Boom_New(object):
    def __getattr__(self, someattribute):
        del self.attr
        raise AttributeError

def test_boom_new():
    a = Boom_New()
    b = Boom_New()
    a.attr = b
    b.attr = a

    gc.collect()
    garbagelen = len(gc.garbage)
    del a, b
    expect(gc.collect(), 4, "boom_new")
    expect(len(gc.garbage), garbagelen, "boom_new")

class Boom2_New(object):
    def __init__(self):
        self.x = 0

    def __getattr__(self, someattribute):
        self.x += 1
        if self.x > 1:
            del self.attr
        raise AttributeError

def test_boom2_new():
    a = Boom2_New()
    b = Boom2_New()
    a.attr = b
    b.attr = a

    gc.collect()
    garbagelen = len(gc.garbage)
    del a, b
    expect(gc.collect(), 4, "boom2_new")
    expect(len(gc.garbage), garbagelen, "boom2_new")

def test_get_referents():
    alist = [1, 3, 5]
    got = gc.get_referents(alist)
    got.sort()
    expect(got, alist, "get_referents")

    atuple = tuple(alist)
    got = gc.get_referents(atuple)
    got.sort()
    expect(got, alist, "get_referents")

    adict = {1: 3, 5: 7}
    expected = [1, 3, 5, 7]
    got = gc.get_referents(adict)
    got.sort()
    expect(got, expected, "get_referents")

    got = gc.get_referents([1, 2], {3: 4}, (0, 0, 0))
    got.sort()
    expect(got, [0, 0] + range(5), "get_referents")

    expect(gc.get_referents(1, 'a', 4j), [], "get_referents")

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

def test_bug1055820b():
    # Corresponds to temp2b.py in the bug report.

    ouch = []
    def callback(ignored):
        ouch[:] = [wr() for wr in WRs]

    Cs = [C1055820(i) for i in range(2)]
    WRs = [weakref.ref(c, callback) for c in Cs]
    c = None

    gc.collect()
    expect(len(ouch), 0, "bug1055820b")
    # Make the two instances trash, and collect again.  The bug was that
    # the callback materialized a strong reference to an instance, but gc
    # cleared the instance's dict anyway.
    Cs = None
    gc.collect()
    expect(len(ouch), 2, "bug1055820b")  # else the callbacks didn't run
    for x in ouch:
        # If the callback resurrected one of these guys, the instance
        # would be damaged, with an empty __dict__.
        expect(x, None, "bug1055820b")

def test_bug1055820c():
    # Corresponds to temp2c.py in the bug report.  This is pretty elaborate.

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
    # generation 2.  The only thing keeping it alive is that c1 points to it.
    # c1 and c2 are in generation 0, and are in self-loops.  There's a global
    # weakref to c2 (c2wr), but that weakref has no callback.  There's also
    # a global weakref to c0 (c0wr), and that does have a callback, and that
    # callback references c2 via c2wr().
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
    # So this is the nightmare:  when generation 0 gets collected, we see that
    # c2 has a callback-free weakref, and c1 doesn't even have a weakref.
    # Collecting generation 0 doesn't see c0 at all, and c0 is the only object
    # that has a weakref with a callback.  gc clears c1 and c2.  Clearing c1
    # has the side effect of dropping the refcount on c0 to 0, so c0 goes
    # away (despite that it's in an older generation) and c0's wr callback
    # triggers.  That in turn materializes a reference to c2 via c2wr(), but
    # c2 gets cleared anyway by gc.

    # We want to let gc happen "naturally", to preserve the distinction
    # between generations.
    junk = []
    i = 0
    detector = GC_Detector()
    while not detector.gc_happened:
        i += 1
        if i > 10000:
            raise TestFailed("gc didn't happen after 10000 iterations")
        expect(len(ouch), 0, "bug1055820c")
        junk.append([])  # this will eventually trigger gc

    expect(len(ouch), 1, "bug1055820c")  # else the callback wasn't invoked
    for x in ouch:
        # If the callback resurrected c2, the instance would be damaged,
        # with an empty __dict__.
        expect(x, None, "bug1055820c")

def test_bug1055820d():
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
    # generation 2.  The only thing keeping it alive is that c1 points to it.
    # c1 and c2 are in generation 0, and are in self-loops.  There's a global
    # weakref to c2 (c2wr), but that weakref has no callback.  There are no
    # other weakrefs.
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
    # So this is the nightmare:  when generation 0 gets collected, we see that
    # c2 has a callback-free weakref, and c1 doesn't even have a weakref.
    # Collecting generation 0 doesn't see d0 at all.  gc clears c1 and c2.
    # Clearing c1 has the side effect of dropping the refcount on d0 to 0, so
    # d0 goes away (despite that it's in an older generation) and d0's __del__
    # triggers.  That in turn materializes a reference to c2 via c2wr(), but
    # c2 gets cleared anyway by gc.

    # We want to let gc happen "naturally", to preserve the distinction
    # between generations.
    detector = GC_Detector()
    junk = []
    i = 0
    while not detector.gc_happened:
        i += 1
        if i > 10000:
            raise TestFailed("gc didn't happen after 10000 iterations")
        expect(len(ouch), 0, "bug1055820d")
        junk.append([])  # this will eventually trigger gc

    expect(len(ouch), 1, "bug1055820d")  # else __del__ wasn't invoked
    for x in ouch:
        # If __del__ resurrected c2, the instance would be damaged, with an
        # empty __dict__.
        expect(x, None, "bug1055820d")


def test_all():
    gc.collect() # Delete 2nd generation garbage
    run_test("lists", test_list)
    run_test("dicts", test_dict)
    run_test("tuples", test_tuple)
    run_test("classes", test_class)
    run_test("new style classes", test_newstyleclass)
    run_test("instances", test_instance)
    run_test("new instances", test_newinstance)
    run_test("methods", test_method)
    run_test("functions", test_function)
    run_test("frames", test_frame)
    run_test("finalizers", test_finalizer)
    run_test("finalizers (new class)", test_finalizer_newclass)
    run_test("__del__", test_del)
    run_test("__del__ (new class)", test_del_newclass)
    run_test("saveall", test_saveall)
    run_test("trashcan", test_trashcan)
    run_test("boom", test_boom)
    run_test("boom2", test_boom2)
    run_test("boom_new", test_boom_new)
    run_test("boom2_new", test_boom2_new)
    run_test("get_referents", test_get_referents)
    run_test("bug1055820b", test_bug1055820b)

    gc.enable()
    try:
        run_test("bug1055820c", test_bug1055820c)
    finally:
        gc.disable()

    gc.enable()
    try:
        run_test("bug1055820d", test_bug1055820d)
    finally:
        gc.disable()

def test():
    if verbose:
        print "disabling automatic collection"
    enabled = gc.isenabled()
    gc.disable()
    verify(not gc.isenabled())
    debug = gc.get_debug()
    gc.set_debug(debug & ~gc.DEBUG_LEAK) # this test is supposed to leak

    try:
        test_all()
    finally:
        gc.set_debug(debug)
        # test gc.enable() even if GC is disabled by default
        if verbose:
            print "restoring automatic collection"
        # make sure to always test gc.enable()
        gc.enable()
        verify(gc.isenabled())
        if not enabled:
            gc.disable()


test()

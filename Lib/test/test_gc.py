from test.test_support import verify, verbose, TestFailed, vereq
import sys
import gc

def expect(actual, expected, name):
    if actual != expected:
        raise TestFailed, "test_%s: actual %d, expected %d" % (
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

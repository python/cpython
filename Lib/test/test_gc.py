from test_support import verbose, TestFailed
import gc

def run_test(name, thunk):
    if verbose:
        print "testing %s..." % name,
    try:
        thunk()
    except TestFailed:
        if verbose:
            print "failed (expected %s but got %s)" % (result,
                                                       test_result)
        raise TestFailed, name
    else:
        if verbose:
            print "ok"

def test_list():
    l = []
    l.append(l)
    gc.collect()
    del l
    if gc.collect() != 1:
        raise TestFailed

def test_dict():
    d = {}
    d[1] = d
    gc.collect()
    del d
    if gc.collect() != 1:
        raise TestFailed

def test_tuple():
    # since tuples are immutable we close the loop with a list
    l = []
    t = (l,)
    l.append(t)
    gc.collect()
    del t
    del l
    if gc.collect() != 2:
        raise TestFailed

def test_class():
    class A:
        pass
    A.a = A
    gc.collect()
    del A
    if gc.collect() == 0:
        raise TestFailed

def test_instance():
    class A:
        pass
    a = A()
    a.a = a
    gc.collect()
    del a
    if gc.collect() == 0:
        raise TestFailed

def test_method():
    # Tricky: self.__init__ is a bound method, it references the instance.
    class A:
        def __init__(self):
            self.init = self.__init__
    a = A()
    gc.collect()
    del a
    if gc.collect() == 0:
        raise TestFailed

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
    if gc.collect() == 0:
        raise TestFailed
    for obj in gc.garbage:
        if id(obj) == id_a:
            del obj.a
            break
    else:
        raise TestFailed
    gc.garbage.remove(obj)

def test_function():
    # Tricky: f -> d -> f, code should call d.clear() after the exec to
    # break the cycle.
    d = {}
    exec("def f(): pass\n") in d
    gc.collect()
    del d
    if gc.collect() != 2:
        raise TestFailed

def test_saveall():
    # Verify that cyclic garbage like lists show up in gc.garbage if the
    # SAVEALL option is enabled.
    debug = gc.get_debug()
    gc.set_debug(debug | gc.DEBUG_SAVEALL)
    l = []
    l.append(l)
    id_l = id(l)
    del l
    gc.collect()
    try:
        for obj in gc.garbage:
            if id(obj) == id_l:
                del obj[:]
                break
        else:
            raise TestFailed
        gc.garbage.remove(obj)
    finally:
        gc.set_debug(debug)

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
    apply(gc.set_threshold, thresholds)
    

def test_all():
    run_test("lists", test_list)
    run_test("dicts", test_dict)
    run_test("tuples", test_tuple)
    run_test("classes", test_class)
    run_test("instances", test_instance)
    run_test("methods", test_method)
    run_test("functions", test_function)
    run_test("finalizers", test_finalizer)
    run_test("__del__", test_del)
    run_test("saveall", test_saveall)

def test():
    if verbose:
        print "disabling automatic collection"
    enabled = gc.isenabled()
    gc.disable()
    assert not gc.isenabled() 
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
        assert gc.isenabled()
        if not enabled:
            gc.disable()


test()

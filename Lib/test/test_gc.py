import gc

def test_list():
    l = []
    l.append(l)
    gc.collect()
    del l
    assert gc.collect() == 1

def test_dict():
    d = {}
    d[1] = d
    gc.collect()
    del d
    assert gc.collect() == 1

def test_tuple():
    # since tuples are immutable we close the loop with a list
    l = []
    t = (l,)
    l.append(t)
    gc.collect()
    del t
    del l
    assert gc.collect() == 2

def test_class():
    class A:
        pass
    A.a = A
    gc.collect()
    del A
    assert gc.collect() > 0

def test_instance():
    class A:
        pass
    a = A()
    a.a = a
    gc.collect()
    del a
    assert gc.collect() > 0

def test_method():
    # Tricky: self.__init__ is a bound method, it references the instance.
    class A:
        def __init__(self):
            self.init = self.__init__
    a = A()
    gc.collect()
    del a
    assert gc.collect() > 0

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
    gc.garbage[:] = []
    del a
    del b
    assert gc.collect() > 0
    assert id(gc.garbage[0]) == id_a

def test_function():
    # Tricky: f -> d -> f, code should call d.clear() after the exec to
    # break the cycle.
    d = {}
    exec("def f(): pass\n") in d
    gc.collect()
    del d
    assert gc.collect() == 2

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

    enabled = gc.isenabled()
    gc.disable()
    assert not gc.isenabled()

    test_list()
    test_dict()
    test_tuple()
    test_class()
    test_instance()
    test_method()
    test_finalizer()
    test_function()
    test_del()

    # test gc.enable() even if GC is disabled by default
    gc.enable()
    assert gc.isenabled()
    if not enabled:
        gc.disable()


test_all()

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
    class A:
        def __init__(self):
            self.init = self.__init__
    a = A()
    gc.collect()
    del a
    assert gc.collect() > 0

def test_finalizer():
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
    d = {}
    exec("def f(): pass\n") in d
    gc.collect()
    del d
    assert gc.collect() == 2


def test_all():
    test_list()
    test_dict()
    test_tuple()
    test_class()
    test_instance()
    test_method()
    test_finalizer()
    test_function()

test_all()

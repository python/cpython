import _testcapi
from threading import Thread
from time import time

def test_1():
    class Foo:
        def __init__(self, x):
            self.x = x

    niter = 5 * 1000 * 1000

    def benchmark(n):
        for i in range(n):
            Foo(x=1)

    for nth in (1, 4):
        t0 = time()
        threads = [Thread(target=benchmark, args=(niter,)) for _ in range(nth)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        print(f"{nth=} {(time() - t0) / nth}")
        
def test_2():
    class Foo2:
        def __init__(self, x):
                pass
        pass
    
    _Foo2_x = int
    
    create_str = """def create_init(_Foo2_x,):
        def __init__(self, x: _Foo2_x):
            self.x = x
        return (__init__,)
    """
    ns = {}
    exec(create_str, globals(), ns)
    fn = ns['create_init']({**locals()})
    setattr(Foo2, '__init__', fn[0])
    niter = 5 * 1000 * 1000
    def benchmark(n):
        for i in range(n):
            Foo2(x=1)
    
    for nth in (1, 4):
        t0 = time()
        threads = [Thread(target=benchmark, args=(niter,)) for _ in range(nth)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        print(f"{nth=} {(time() - t0) / nth}")
        
def test_3():
    class Foo3:
        def __init__(self, x):
                pass
        pass
    
    _Foo3_x = int
    
    create_str = """def create_init(_Foo3_x,):
        def __init__(self, x: _Foo3_x):
            self.x = x
        return (__init__,)
    """
    ns = {}
    exec(create_str, globals(), ns)
    fn = ns['create_init']({**locals()})
    setattr(Foo3, '__init__', fn[0])
    _testcapi.pyobject_enable_deferred_refcount(Foo3.__init__)
    niter = 5 * 1000 * 1000
    def benchmark(n):
        for i in range(n):
            Foo3(x=1)
    
    for nth in (1, 4):
        t0 = time()
        threads = [Thread(target=benchmark, args=(niter,)) for _ in range(nth)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        print(f"{nth=} {(time() - t0) / nth}")
        
if __name__ == "__main__":
    print("------test_1-------")
    test_1()
    print("------test_2-------")
    test_2()
    print("------test_3-------")
    test_3()
import unittest
from doctest import DocTestSuite
from test import test_support
import threading
import weakref
import gc

class Weak(object):
    pass

def target(local, weaklist):
    weak = Weak()
    local.weak = weak
    weaklist.append(weakref.ref(weak))

class ThreadingLocalTest(unittest.TestCase):

    def test_local_refs(self):
        self._local_refs(20)
        self._local_refs(50)
        self._local_refs(100)

    def _local_refs(self, n):
        local = threading.local()
        weaklist = []
        for i in range(n):
            t = threading.Thread(target=target, args=(local, weaklist))
            t.start()
            t.join()
        del t

        gc.collect()
        self.assertEqual(len(weaklist), n)

        # XXX threading.local keeps the local of the last stopped thread alive.
        deadlist = [weak for weak in weaklist if weak() is None]
        self.assertEqual(len(deadlist), n-1)

        # Assignment to the same thread local frees it sometimes (!)
        local.someothervar = None
        gc.collect()
        deadlist = [weak for weak in weaklist if weak() is None]
        self.assert_(len(deadlist) in (n-1, n), (n, len(deadlist)))

    def test_derived(self):
        # Issue 3088: if there is a threads switch inside the __init__
        # of a threading.local derived class, the per-thread dictionary
        # is created but not correctly set on the object.
        # The first member set may be bogus.
        import time
        class Local(threading.local):
            def __init__(self):
                time.sleep(0.01)
        local = Local()

        def f(i):
            local.x = i
            # Simply check that the variable is correctly set
            self.assertEqual(local.x, i)

        threads= []
        for i in range(10):
            t = threading.Thread(target=f, args=(i,))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

    def test_derived_cycle_dealloc(self):
        # http://bugs.python.org/issue6990
        class Local(threading.local):
            pass
        locals = None
        passed = [False]
        e1 = threading.Event()
        e2 = threading.Event()

        def f():
            # 1) Involve Local in a cycle
            cycle = [Local()]
            cycle.append(cycle)
            cycle[0].foo = 'bar'

            # 2) GC the cycle (triggers threadmodule.c::local_clear
            # before local_dealloc)
            del cycle
            gc.collect()
            e1.set()
            e2.wait()

            # 4) New Locals should be empty
            passed[0] = all(not hasattr(local, 'foo') for local in locals)

        t = threading.Thread(target=f)
        t.start()
        e1.wait()

        # 3) New Locals should recycle the original's address. Creating
        # them in the thread overwrites the thread state and avoids the
        # bug
        locals = [Local() for i in range(10)]
        e2.set()
        t.join()

        self.assertTrue(passed[0])


def test_main():
    suite = unittest.TestSuite()
    suite.addTest(DocTestSuite('_threading_local'))
    suite.addTest(unittest.makeSuite(ThreadingLocalTest))

    try:
        from thread import _local
    except ImportError:
        pass
    else:
        import _threading_local
        local_orig = _threading_local.local
        def setUp(test):
            _threading_local.local = _local
        def tearDown(test):
            _threading_local.local = local_orig
        suite.addTest(DocTestSuite('_threading_local',
                                   setUp=setUp, tearDown=tearDown)
                      )

    test_support.run_unittest(suite)

if __name__ == '__main__':
    test_main()

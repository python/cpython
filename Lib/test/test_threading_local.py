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

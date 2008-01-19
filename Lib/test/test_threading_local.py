import unittest
from doctest import DocTestSuite
from test import test_support
import threading
import weakref

class Weak(object):
    pass

def target(local, weaklist):
    weak = Weak()
    local.weak = weak
    weaklist.append(weakref.ref(weak))

class ThreadingLocalTest(unittest.TestCase):
    def test_local_refs(self):
        local = threading.local()
        weaklist = []
        n = 20
        for i in range(n):
            t = threading.Thread(target=target, args=(local, weaklist))
            t.start()
            t.join()
        self.assertEqual(len(weaklist), n)
        deadlist = [weak for weak in weaklist if weak() is None]
        # XXX threading.local keeps the local of the last stopped thread alive
        self.assertEqual(len(deadlist), n-1)

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

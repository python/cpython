import unittest
from doctest import DocTestSuite
from test import test_support

class ThreadingLocalTest(unittest.TestCase):
    def test_derived(self):
        # Issue 3088: if there is a threads switch inside the __init__
        # of a threading.local derived class, the per-thread dictionary
        # is created but not correctly set on the object.
        # The first member set may be bogus.
        import threading
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


def test_main():
    suite = DocTestSuite('_threading_local')

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
        suite.addTest(unittest.makeSuite(ThreadingLocalTest))

    test_support.run_suite(suite)

if __name__ == '__main__':
    test_main()

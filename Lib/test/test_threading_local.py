import sys
import unittest
from doctest import DocTestSuite
from test import support
from test.support import threading_helper
from test.support.import_helper import import_module
import weakref

# Modules under test
import _thread
import threading
import _threading_local


threading_helper.requires_working_threading(module=True)


class Weak(object):
    pass

def target(local, weaklist):
    weak = Weak()
    local.weak = weak
    weaklist.append(weakref.ref(weak))


class BaseLocalTest:

    def test_local_refs(self):
        self._local_refs(20)
        self._local_refs(50)
        self._local_refs(100)

    def _local_refs(self, n):
        local = self._local()
        weaklist = []
        for i in range(n):
            t = threading.Thread(target=target, args=(local, weaklist))
            t.start()
            t.join()
        del t

        support.gc_collect()  # For PyPy or other GCs.
        self.assertEqual(len(weaklist), n)

        # XXX _threading_local keeps the local of the last stopped thread alive.
        deadlist = [weak for weak in weaklist if weak() is None]
        self.assertIn(len(deadlist), (n-1, n))

        # Assignment to the same thread local frees it sometimes (!)
        local.someothervar = None
        support.gc_collect()  # For PyPy or other GCs.
        deadlist = [weak for weak in weaklist if weak() is None]
        self.assertIn(len(deadlist), (n-1, n), (n, len(deadlist)))

    def test_derived(self):
        # Issue 3088: if there is a threads switch inside the __init__
        # of a threading.local derived class, the per-thread dictionary
        # is created but not correctly set on the object.
        # The first member set may be bogus.
        import time
        class Local(self._local):
            def __init__(self):
                time.sleep(0.01)
        local = Local()

        def f(i):
            local.x = i
            # Simply check that the variable is correctly set
            self.assertEqual(local.x, i)

        with threading_helper.start_threads(threading.Thread(target=f, args=(i,))
                                            for i in range(10)):
            pass

    def test_derived_cycle_dealloc(self):
        # http://bugs.python.org/issue6990
        class Local(self._local):
            pass
        locals = None
        passed = False
        e1 = threading.Event()
        e2 = threading.Event()

        def f():
            nonlocal passed
            # 1) Involve Local in a cycle
            cycle = [Local()]
            cycle.append(cycle)
            cycle[0].foo = 'bar'

            # 2) GC the cycle (triggers threadmodule.c::local_clear
            # before local_dealloc)
            del cycle
            support.gc_collect()  # For PyPy or other GCs.
            e1.set()
            e2.wait()

            # 4) New Locals should be empty
            passed = all(not hasattr(local, 'foo') for local in locals)

        t = threading.Thread(target=f)
        t.start()
        e1.wait()

        # 3) New Locals should recycle the original's address. Creating
        # them in the thread overwrites the thread state and avoids the
        # bug
        locals = [Local() for i in range(10)]
        e2.set()
        t.join()

        self.assertTrue(passed)

    def test_arguments(self):
        # Issue 1522237
        class MyLocal(self._local):
            def __init__(self, *args, **kwargs):
                pass

        MyLocal(a=1)
        MyLocal(1)
        self.assertRaises(TypeError, self._local, a=1)
        self.assertRaises(TypeError, self._local, 1)

    def _test_one_class(self, c):
        self._failed = "No error message set or cleared."
        obj = c()
        e1 = threading.Event()
        e2 = threading.Event()

        def f1():
            obj.x = 'foo'
            obj.y = 'bar'
            del obj.y
            e1.set()
            e2.wait()

        def f2():
            try:
                foo = obj.x
            except AttributeError:
                # This is expected -- we haven't set obj.x in this thread yet!
                self._failed = ""  # passed
            else:
                self._failed = ('Incorrectly got value %r from class %r\n' %
                                (foo, c))
                sys.stderr.write(self._failed)

        t1 = threading.Thread(target=f1)
        t1.start()
        e1.wait()
        t2 = threading.Thread(target=f2)
        t2.start()
        t2.join()
        # The test is done; just let t1 know it can exit, and wait for it.
        e2.set()
        t1.join()

        self.assertFalse(self._failed, self._failed)

    def test_threading_local(self):
        self._test_one_class(self._local)

    def test_threading_local_subclass(self):
        class LocalSubclass(self._local):
            """To test that subclasses behave properly."""
        self._test_one_class(LocalSubclass)

    def _test_dict_attribute(self, cls):
        obj = cls()
        obj.x = 5
        self.assertEqual(obj.__dict__, {'x': 5})
        with self.assertRaises(AttributeError):
            obj.__dict__ = {}
        with self.assertRaises(AttributeError):
            del obj.__dict__

    def test_dict_attribute(self):
        self._test_dict_attribute(self._local)

    def test_dict_attribute_subclass(self):
        class LocalSubclass(self._local):
            """To test that subclasses behave properly."""
        self._test_dict_attribute(LocalSubclass)

    def test_cycle_collection(self):
        class X:
            pass

        x = X()
        x.local = self._local()
        x.local.x = x
        wr = weakref.ref(x)
        del x
        support.gc_collect()  # For PyPy or other GCs.
        self.assertIsNone(wr())


    def test_threading_local_clear_race(self):
        # See https://github.com/python/cpython/issues/100892

        _testcapi = import_module('_testcapi')
        _testcapi.call_in_temporary_c_thread(lambda: None, False)

        for _ in range(1000):
            _ = threading.local()

        _testcapi.join_temporary_c_thread()

    @support.cpython_only
    def test_error(self):
        class Loop(self._local):
            attr = 1

        # Trick the "if name == '__dict__':" test of __setattr__()
        # to always be true
        class NameCompareTrue:
            def __eq__(self, other):
                return True

        loop = Loop()
        with self.assertRaisesRegex(AttributeError, 'Loop.*read-only'):
            loop.__setattr__(NameCompareTrue(), 2)


class ThreadLocalTest(unittest.TestCase, BaseLocalTest):
    _local = _thread._local

class PyThreadingLocalTest(unittest.TestCase, BaseLocalTest):
    _local = _threading_local.local


def load_tests(loader, tests, pattern):
    tests.addTest(DocTestSuite('_threading_local'))

    local_orig = _threading_local.local
    def setUp(test):
        _threading_local.local = _thread._local
    def tearDown(test):
        _threading_local.local = local_orig
    tests.addTests(DocTestSuite('_threading_local',
                                setUp=setUp, tearDown=tearDown)
                   )
    return tests


if __name__ == '__main__':
    unittest.main()

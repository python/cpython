from multiprocessing import Process
from multiprocessing.managers import SyncManager
from test.support import reap_children
import test._test_multiprocessing
import unittest


class TestSyncManagerTypes(unittest.TestCase):
    """Test all the types which can be shared between parent and child
    process by using a manager which acts as an intermediary between
    the two.

    In the following tests the base type is created in the parent
    process. "worker" function represents how the base type is
    received (and handled) from within the child process. E.g.:

    def test_list(self):
        def worker(obj):
            # === within the child process ===
            assert obj[0] == 1

        # === within the parent process ===
        o = self.manager.list()
        o.append(1)
        self.run_test(worker, o)
    """
    manager_class = SyncManager

    def setUp(self):
        self.manager = self.manager_class()
        self.manager.start()
        self.proc = None

    def tearDown(self):
        self.manager.shutdown()
        if self.proc is not None and self.proc.is_alive():
            self.proc.terminate()
            self.proc.join()

    @classmethod
    def tearDownClass(cls):
        reap_children()

    def run_test(self, worker, obj):
        self.proc = Process(target=worker, args=(obj, ))
        self.proc.start()
        self.proc.join()
        self.assertEqual(self.proc.exitcode, 0)

    def test_queue(self, qname="Queue"):
        def worker(obj):
            assert obj.qsize() == 2
            assert obj.full()
            assert not obj.empty()
            assert obj.get() == 5
            assert not obj.empty()
            assert obj.get() == 6
            assert obj.empty()

        o = getattr(self.manager, qname)(2)
        o.put(5)
        o.put(6)
        self.run_test(worker, o)
        assert o.empty()
        assert not o.full()

    def test_joinable_queue(self):
        self.test_queue("JoinableQueue")

    def test_event(self):
        def worker(obj):
            assert obj.is_set()
            obj.wait()
            obj.clear()
            obj.wait(0.001)

        o = self.manager.Event()
        o.set()
        self.run_test(worker, o)
        assert not o.is_set()
        o.wait(0.001)

    def test_lock(self, lname="Lock"):
        def worker(obj):
            o.acquire()

        o = getattr(self.manager, lname)()
        self.run_test(worker, o)
        o.release()
        self.assertRaises(RuntimeError, o.release)  # already released

    def test_rlock(self):
        self.test_lock(lname="RLock")

    def test_semaphore(self, sname="Semaphore"):
        def worker(obj):
            obj.acquire()

        o = getattr(self.manager, sname)()
        self.run_test(worker, o)
        o.release()

    def test_bounded_semaphore(self):
        self.test_semaphore(sname="BoundedSemaphore")

    def test_condition(self):
        def worker(obj):
            obj.acquire()

        o = self.manager.Condition()
        self.run_test(worker, o)
        o.release()
        self.assertRaises(RuntimeError, o.release)  # already released

    def test_barrier(self):
        def worker(obj):
            assert obj.parties == 5
            obj.reset()

        o = self.manager.Barrier(5)
        self.run_test(worker, o)

    def test_pool(self):
        def worker(obj):
            # TODO: fix https://bugs.python.org/issue35919
            with obj:
                pass

        o = self.manager.Pool(processes=4)
        self.run_test(worker, o)

    def test_list(self):
        def worker(obj):
            assert obj[0] == 5
            assert obj.count(5) == 1
            assert obj.index(5) == 0
            obj.sort()
            obj.reverse()
            for x in obj:
                pass
            assert len(obj) == 1
            assert obj.pop(0) == 5

        o = self.manager.list()
        o.append(5)
        self.run_test(worker, o)
        assert not o
        self.assertEqual(len(o), 0)

    def test_dict(self):
        def worker(obj):
            assert len(obj) == 1
            assert obj['foo'] == 5
            assert obj.get('foo') == 5
            # TODO: fix https://bugs.python.org/issue35918
            # assert obj.has_key('foo')
            assert list(obj.items()) == [('foo', 5)]
            assert list(obj.keys()) == ['foo']
            assert list(obj.values()) == [5]
            assert obj.copy() == {'foo': 5}
            assert obj.popitem() == ('foo', 5)

        o = self.manager.dict()
        o['foo'] = 5
        self.run_test(worker, o)
        assert not o
        self.assertEqual(len(o), 0)

    def test_value(self):
        def worker(obj):
            assert obj.value == 1
            assert obj.get() == 1
            obj.set(2)

        o = self.manager.Value('i', 1)
        self.run_test(worker, o)
        self.assertEqual(o.value, 2)
        self.assertEqual(o.get(), 2)

    def test_array(self):
        def worker(obj):
            assert obj[0] == 0
            assert obj[1] == 1
            assert len(obj) == 2
            assert list(obj) == [0, 1]

        o = self.manager.Array('i', [0, 1])
        self.run_test(worker, o)

    def test_namespace(self):
        def worker(obj):
            assert obj.x == 0
            assert obj.y == 1

        o = self.manager.Namespace()
        o.x = 0
        o.y = 1
        self.run_test(worker, o)


try:
    from multiprocessing.shared_memory import SharedMemoryManager
except ImportError:
    @unittest.skip("SharedMemoryManager not available on this platform")
    class TestSharedMemoryManagerTypes(TestSyncManagerTypes):
        pass
else:
    class TestSharedMemoryManagerTypes(TestSyncManagerTypes):
        """Same as above but by using SharedMemoryManager."""
        manager_class = SharedMemoryManager


def tearDownModule():
    reap_children()


test._test_multiprocessing.install_tests_in_module_dict(globals(), 'managers')


if __name__ == '__main__':
    unittest.main()

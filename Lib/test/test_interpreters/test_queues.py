import threading
from textwrap import dedent
import unittest
import time

from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
_queues = import_helper.import_module('_xxinterpqueues')
from test.support import interpreters
from test.support.interpreters import queues
from .utils import _run_output, TestBase


class TestBase(TestBase):
    def tearDown(self):
        for qid in _queues.list_all():
            try:
                _queues.destroy(qid)
            except Exception:
                pass


class QueueTests(TestBase):

    def test_create(self):
        with self.subTest('vanilla'):
            queue = queues.create()
            self.assertEqual(queue.maxsize, 0)

        with self.subTest('small maxsize'):
            queue = queues.create(3)
            self.assertEqual(queue.maxsize, 3)

        with self.subTest('big maxsize'):
            queue = queues.create(100)
            self.assertEqual(queue.maxsize, 100)

        with self.subTest('no maxsize'):
            queue = queues.create(0)
            self.assertEqual(queue.maxsize, 0)

        with self.subTest('negative maxsize'):
            queue = queues.create(-10)
            self.assertEqual(queue.maxsize, -10)

        with self.subTest('bad maxsize'):
            with self.assertRaises(TypeError):
                queues.create('1')

    def test_shareable(self):
        queue1 = queues.create()

        interp = interpreters.create()
        interp.exec_sync(dedent(f"""
            from test.support.interpreters import queues
            queue1 = queues.Queue({queue1.id})
            """));

        with self.subTest('same interpreter'):
            queue2 = queues.create()
            queue1.put(queue2)
            queue3 = queue1.get()
            self.assertIs(queue3, queue2)

        with self.subTest('from current interpreter'):
            queue4 = queues.create()
            queue1.put(queue4)
            out = _run_output(interp, dedent("""
                queue4 = queue1.get()
                print(queue4.id)
                """))
            qid = int(out)
            self.assertEqual(qid, queue4.id)

        with self.subTest('from subinterpreter'):
            out = _run_output(interp, dedent("""
                queue5 = queues.create()
                queue1.put(queue5)
                print(queue5.id)
                """))
            qid = int(out)
            queue5 = queue1.get()
            self.assertEqual(queue5.id, qid)

    def test_id_type(self):
        queue = queues.create()
        self.assertIsInstance(queue.id, int)

    def test_custom_id(self):
        with self.assertRaises(queues.QueueNotFoundError):
            queues.Queue(1_000_000)

    def test_id_readonly(self):
        queue = queues.create()
        with self.assertRaises(AttributeError):
            queue.id = 1_000_000

    def test_maxsize_readonly(self):
        queue = queues.create(10)
        with self.assertRaises(AttributeError):
            queue.maxsize = 1_000_000

    def test_hashable(self):
        queue = queues.create()
        expected = hash(queue.id)
        actual = hash(queue)
        self.assertEqual(actual, expected)

    def test_equality(self):
        queue1 = queues.create()
        queue2 = queues.create()
        self.assertEqual(queue1, queue1)
        self.assertNotEqual(queue1, queue2)


class TestQueueOps(TestBase):

    def test_empty(self):
        queue = queues.create()
        before = queue.empty()
        queue.put(None)
        during = queue.empty()
        queue.get()
        after = queue.empty()

        self.assertIs(before, True)
        self.assertIs(during, False)
        self.assertIs(after, True)

    def test_full(self):
        expected = [False, False, False, True, False, False, False]
        actual = []
        queue = queues.create(3)
        for _ in range(3):
            actual.append(queue.full())
            queue.put(None)
        actual.append(queue.full())
        for _ in range(3):
            queue.get()
            actual.append(queue.full())

        self.assertEqual(actual, expected)

    def test_qsize(self):
        expected = [0, 1, 2, 3, 2, 3, 2, 1, 0, 1, 0]
        actual = []
        queue = queues.create()
        for _ in range(3):
            actual.append(queue.qsize())
            queue.put(None)
        actual.append(queue.qsize())
        queue.get()
        actual.append(queue.qsize())
        queue.put(None)
        actual.append(queue.qsize())
        for _ in range(3):
            queue.get()
            actual.append(queue.qsize())
        queue.put(None)
        actual.append(queue.qsize())
        queue.get()
        actual.append(queue.qsize())

        self.assertEqual(actual, expected)

    def test_put_get_main(self):
        expected = list(range(20))
        queue = queues.create()
        for i in range(20):
            queue.put(i)
        actual = [queue.get() for _ in range(20)]

        self.assertEqual(actual, expected)

    def test_put_timeout(self):
        queue = queues.create(2)
        queue.put(None)
        queue.put(None)
        with self.assertRaises(queues.QueueFull):
            queue.put(None, timeout=0.1)
        queue.get()
        queue.put(None)

    def test_put_nowait(self):
        queue = queues.create(2)
        queue.put_nowait(None)
        queue.put_nowait(None)
        with self.assertRaises(queues.QueueFull):
            queue.put_nowait(None)
        queue.get()
        queue.put_nowait(None)

    def test_get_timeout(self):
        queue = queues.create()
        with self.assertRaises(queues.QueueEmpty):
            queue.get(timeout=0.1)

    def test_get_nowait(self):
        queue = queues.create()
        with self.assertRaises(queues.QueueEmpty):
            queue.get_nowait()

    def test_put_get_same_interpreter(self):
        interp = interpreters.create()
        interp.exec_sync(dedent("""
            from test.support.interpreters import queues
            queue = queues.create()
            orig = b'spam'
            queue.put(orig)
            obj = queue.get()
            assert obj == orig, 'expected: obj == orig'
            assert obj is not orig, 'expected: obj is not orig'
            """))

    def test_put_get_different_interpreters(self):
        interp = interpreters.create()
        queue1 = queues.create()
        queue2 = queues.create()
        self.assertEqual(len(queues.list_all()), 2)

        obj1 = b'spam'
        queue1.put(obj1)

        out = _run_output(
            interp,
            dedent(f"""
                from test.support.interpreters import queues
                queue1 = queues.Queue({queue1.id})
                queue2 = queues.Queue({queue2.id})
                assert queue1.qsize() == 1, 'expected: queue1.qsize() == 1'
                obj = queue1.get()
                assert queue1.qsize() == 0, 'expected: queue1.qsize() == 0'
                assert obj == b'spam', 'expected: obj == obj1'
                # When going to another interpreter we get a copy.
                assert id(obj) != {id(obj1)}, 'expected: obj is not obj1'
                obj2 = b'eggs'
                print(id(obj2))
                assert queue2.qsize() == 0, 'expected: queue2.qsize() == 0'
                queue2.put(obj2)
                assert queue2.qsize() == 1, 'expected: queue2.qsize() == 1'
                """))
        self.assertEqual(len(queues.list_all()), 2)
        self.assertEqual(queue1.qsize(), 0)
        self.assertEqual(queue2.qsize(), 1)

        obj2 = queue2.get()
        self.assertEqual(obj2, b'eggs')
        self.assertNotEqual(id(obj2), int(out))

    def test_put_cleared_with_subinterpreter(self):
        interp = interpreters.create()
        queue = queues.create()

        out = _run_output(
            interp,
            dedent(f"""
                from test.support.interpreters import queues
                queue = queues.Queue({queue.id})
                obj1 = b'spam'
                obj2 = b'eggs'
                queue.put(obj1)
                queue.put(obj2)
                """))
        self.assertEqual(queue.qsize(), 2)

        obj1 = queue.get()
        self.assertEqual(obj1, b'spam')
        self.assertEqual(queue.qsize(), 1)

        del interp
        self.assertEqual(queue.qsize(), 0)

    def test_put_get_different_threads(self):
        queue1 = queues.create()
        queue2 = queues.create()

        def f():
            while True:
                try:
                    obj = queue1.get(timeout=0.1)
                    break
                except queues.QueueEmpty:
                    continue
            queue2.put(obj)
        t = threading.Thread(target=f)
        t.start()

        orig = b'spam'
        queue1.put(orig)
        obj = queue2.get()
        t.join()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()

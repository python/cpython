import importlib
import pickle
import threading
from textwrap import dedent
import unittest
import time

from test.support import import_helper, Py_DEBUG
# Raise SkipTest if subinterpreters not supported.
_queues = import_helper.import_module('_interpqueues')
from test.support import interpreters
from test.support.interpreters import queues, _crossinterp
from .utils import _run_output, TestBase as _TestBase


REPLACE = _crossinterp._UNBOUND_CONSTANT_TO_FLAG[_crossinterp.UNBOUND]


def get_num_queues():
    return len(_queues.list_all())


class TestBase(_TestBase):
    def tearDown(self):
        for qid, _, _ in _queues.list_all():
            try:
                _queues.destroy(qid)
            except Exception:
                pass


class LowLevelTests(TestBase):

    # The behaviors in the low-level module are important in as much
    # as they are exercised by the high-level module.  Therefore the
    # most important testing happens in the high-level tests.
    # These low-level tests cover corner cases that are not
    # encountered by the high-level module, thus they
    # mostly shouldn't matter as much.

    def test_highlevel_reloaded(self):
        # See gh-115490 (https://github.com/python/cpython/issues/115490).
        importlib.reload(queues)

    def test_create_destroy(self):
        qid = _queues.create(2, 0, REPLACE)
        _queues.destroy(qid)
        self.assertEqual(get_num_queues(), 0)
        with self.assertRaises(queues.QueueNotFoundError):
            _queues.get(qid)
        with self.assertRaises(queues.QueueNotFoundError):
            _queues.destroy(qid)

    def test_not_destroyed(self):
        # It should have cleaned up any remaining queues.
        stdout, stderr = self.assert_python_ok(
            '-c',
            dedent(f"""
                import {_queues.__name__} as _queues
                _queues.create(2, 0, {REPLACE})
                """),
        )
        self.assertEqual(stdout, '')
        if Py_DEBUG:
            self.assertNotEqual(stderr, '')
        else:
            self.assertEqual(stderr, '')

    def test_bind_release(self):
        with self.subTest('typical'):
            qid = _queues.create(2, 0, REPLACE)
            _queues.bind(qid)
            _queues.release(qid)
            self.assertEqual(get_num_queues(), 0)

        with self.subTest('bind too much'):
            qid = _queues.create(2, 0, REPLACE)
            _queues.bind(qid)
            _queues.bind(qid)
            _queues.release(qid)
            _queues.destroy(qid)
            self.assertEqual(get_num_queues(), 0)

        with self.subTest('nested'):
            qid = _queues.create(2, 0, REPLACE)
            _queues.bind(qid)
            _queues.bind(qid)
            _queues.release(qid)
            _queues.release(qid)
            self.assertEqual(get_num_queues(), 0)

        with self.subTest('release without binding'):
            qid = _queues.create(2, 0, REPLACE)
            with self.assertRaises(queues.QueueError):
                _queues.release(qid)


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
        interp.exec(dedent(f"""
            from test.support.interpreters import queues
            queue1 = queues.Queue({queue1.id})
            """));

        with self.subTest('same interpreter'):
            queue2 = queues.create()
            queue1.put(queue2, syncobj=True)
            queue3 = queue1.get()
            self.assertIs(queue3, queue2)

        with self.subTest('from current interpreter'):
            queue4 = queues.create()
            queue1.put(queue4, syncobj=True)
            out = _run_output(interp, dedent("""
                queue4 = queue1.get()
                print(queue4.id)
                """))
            qid = int(out)
            self.assertEqual(qid, queue4.id)

        with self.subTest('from subinterpreter'):
            out = _run_output(interp, dedent("""
                queue5 = queues.create()
                queue1.put(queue5, syncobj=True)
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

    def test_pickle(self):
        queue = queues.create()
        data = pickle.dumps(queue)
        unpickled = pickle.loads(data)
        self.assertEqual(unpickled, queue)


class TestQueueOps(TestBase):

    def test_empty(self):
        queue = queues.create()
        before = queue.empty()
        queue.put(None, syncobj=True)
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
            queue.put(None, syncobj=True)
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
            queue.put(None, syncobj=True)
        actual.append(queue.qsize())
        queue.get()
        actual.append(queue.qsize())
        queue.put(None, syncobj=True)
        actual.append(queue.qsize())
        for _ in range(3):
            queue.get()
            actual.append(queue.qsize())
        queue.put(None, syncobj=True)
        actual.append(queue.qsize())
        queue.get()
        actual.append(queue.qsize())

        self.assertEqual(actual, expected)

    def test_put_get_main(self):
        expected = list(range(20))
        for syncobj in (True, False):
            kwds = dict(syncobj=syncobj)
            with self.subTest(f'syncobj={syncobj}'):
                queue = queues.create()
                for i in range(20):
                    queue.put(i, **kwds)
                actual = [queue.get() for _ in range(20)]

                self.assertEqual(actual, expected)

    def test_put_timeout(self):
        for syncobj in (True, False):
            kwds = dict(syncobj=syncobj)
            with self.subTest(f'syncobj={syncobj}'):
                queue = queues.create(2)
                queue.put(None, **kwds)
                queue.put(None, **kwds)
                with self.assertRaises(queues.QueueFull):
                    queue.put(None, timeout=0.1, **kwds)
                queue.get()
                queue.put(None, **kwds)

    def test_put_nowait(self):
        for syncobj in (True, False):
            kwds = dict(syncobj=syncobj)
            with self.subTest(f'syncobj={syncobj}'):
                queue = queues.create(2)
                queue.put_nowait(None, **kwds)
                queue.put_nowait(None, **kwds)
                with self.assertRaises(queues.QueueFull):
                    queue.put_nowait(None, **kwds)
                queue.get()
                queue.put_nowait(None, **kwds)

    def test_put_syncobj(self):
        for obj in [
            None,
            True,
            10,
            'spam',
            b'spam',
            (0, 'a'),
        ]:
            with self.subTest(repr(obj)):
                queue = queues.create()

                queue.put(obj, syncobj=True)
                obj2 = queue.get()
                self.assertEqual(obj2, obj)

                queue.put(obj, syncobj=True)
                obj2 = queue.get_nowait()
                self.assertEqual(obj2, obj)

        for obj in [
            [1, 2, 3],
            {'a': 13, 'b': 17},
        ]:
            with self.subTest(repr(obj)):
                queue = queues.create()
                with self.assertRaises(interpreters.NotShareableError):
                    queue.put(obj, syncobj=True)

    def test_put_not_syncobj(self):
        for obj in [
            None,
            True,
            10,
            'spam',
            b'spam',
            (0, 'a'),
            # not shareable
            [1, 2, 3],
            {'a': 13, 'b': 17},
        ]:
            with self.subTest(repr(obj)):
                queue = queues.create()

                queue.put(obj, syncobj=False)
                obj2 = queue.get()
                self.assertEqual(obj2, obj)

                queue.put(obj, syncobj=False)
                obj2 = queue.get_nowait()
                self.assertEqual(obj2, obj)

    def test_get_timeout(self):
        queue = queues.create()
        with self.assertRaises(queues.QueueEmpty):
            queue.get(timeout=0.1)

    def test_get_nowait(self):
        queue = queues.create()
        with self.assertRaises(queues.QueueEmpty):
            queue.get_nowait()

    def test_put_get_default_syncobj(self):
        expected = list(range(20))
        queue = queues.create(syncobj=True)
        for methname in ('get', 'get_nowait'):
            with self.subTest(f'{methname}()'):
                get = getattr(queue, methname)
                for i in range(20):
                    queue.put(i)
                actual = [get() for _ in range(20)]
                self.assertEqual(actual, expected)

        obj = [1, 2, 3]  # lists are not shareable
        with self.assertRaises(interpreters.NotShareableError):
            queue.put(obj)

    def test_put_get_default_not_syncobj(self):
        expected = list(range(20))
        queue = queues.create(syncobj=False)
        for methname in ('get', 'get_nowait'):
            with self.subTest(f'{methname}()'):
                get = getattr(queue, methname)

                for i in range(20):
                    queue.put(i)
                actual = [get() for _ in range(20)]
                self.assertEqual(actual, expected)

                obj = [1, 2, 3]  # lists are not shareable
                queue.put(obj)
                obj2 = get()
                self.assertEqual(obj, obj2)
                self.assertIsNot(obj, obj2)

    def test_put_get_same_interpreter(self):
        interp = interpreters.create()
        interp.exec(dedent("""
            from test.support.interpreters import queues
            queue = queues.create()
            """))
        for methname in ('get', 'get_nowait'):
            with self.subTest(f'{methname}()'):
                interp.exec(dedent(f"""
                    orig = b'spam'
                    queue.put(orig, syncobj=True)
                    obj = queue.{methname}()
                    assert obj == orig, 'expected: obj == orig'
                    assert obj is not orig, 'expected: obj is not orig'
                    """))

    def test_put_get_different_interpreters(self):
        interp = interpreters.create()
        queue1 = queues.create()
        queue2 = queues.create()
        self.assertEqual(len(queues.list_all()), 2)

        for methname in ('get', 'get_nowait'):
            with self.subTest(f'{methname}()'):
                obj1 = b'spam'
                queue1.put(obj1, syncobj=True)

                out = _run_output(
                    interp,
                    dedent(f"""
                        from test.support.interpreters import queues
                        queue1 = queues.Queue({queue1.id})
                        queue2 = queues.Queue({queue2.id})
                        assert queue1.qsize() == 1, 'expected: queue1.qsize() == 1'
                        obj = queue1.{methname}()
                        assert queue1.qsize() == 0, 'expected: queue1.qsize() == 0'
                        assert obj == b'spam', 'expected: obj == obj1'
                        # When going to another interpreter we get a copy.
                        assert id(obj) != {id(obj1)}, 'expected: obj is not obj1'
                        obj2 = b'eggs'
                        print(id(obj2))
                        assert queue2.qsize() == 0, 'expected: queue2.qsize() == 0'
                        queue2.put(obj2, syncobj=True)
                        assert queue2.qsize() == 1, 'expected: queue2.qsize() == 1'
                        """))
                self.assertEqual(len(queues.list_all()), 2)
                self.assertEqual(queue1.qsize(), 0)
                self.assertEqual(queue2.qsize(), 1)

                get = getattr(queue2, methname)
                obj2 = get()
                self.assertEqual(obj2, b'eggs')
                self.assertNotEqual(id(obj2), int(out))

    def test_put_cleared_with_subinterpreter(self):
        def common(queue, unbound=None, presize=0):
            if not unbound:
                extraargs = ''
            elif unbound is queues.UNBOUND:
                extraargs = ', unbound=queues.UNBOUND'
            elif unbound is queues.UNBOUND_ERROR:
                extraargs = ', unbound=queues.UNBOUND_ERROR'
            elif unbound is queues.UNBOUND_REMOVE:
                extraargs = ', unbound=queues.UNBOUND_REMOVE'
            else:
                raise NotImplementedError(repr(unbound))
            interp = interpreters.create()

            _run_output(interp, dedent(f"""
                from test.support.interpreters import queues
                queue = queues.Queue({queue.id})
                obj1 = b'spam'
                obj2 = b'eggs'
                queue.put(obj1, syncobj=True{extraargs})
                queue.put(obj2, syncobj=True{extraargs})
                """))
            self.assertEqual(queue.qsize(), presize + 2)

            if presize == 0:
                obj1 = queue.get()
                self.assertEqual(obj1, b'spam')
                self.assertEqual(queue.qsize(), presize + 1)

            return interp

        with self.subTest('default'):  # UNBOUND
            queue = queues.create()
            interp = common(queue)
            del interp
            obj1 = queue.get()
            self.assertIs(obj1, queues.UNBOUND)
            self.assertEqual(queue.qsize(), 0)
            with self.assertRaises(queues.QueueEmpty):
                queue.get_nowait()

        with self.subTest('UNBOUND'):
            queue = queues.create()
            interp = common(queue, queues.UNBOUND)
            del interp
            obj1 = queue.get()
            self.assertIs(obj1, queues.UNBOUND)
            self.assertEqual(queue.qsize(), 0)
            with self.assertRaises(queues.QueueEmpty):
                queue.get_nowait()

        with self.subTest('UNBOUND_ERROR'):
            queue = queues.create()
            interp = common(queue, queues.UNBOUND_ERROR)

            del interp
            self.assertEqual(queue.qsize(), 1)
            with self.assertRaises(queues.ItemInterpreterDestroyed):
                queue.get()

            self.assertEqual(queue.qsize(), 0)
            with self.assertRaises(queues.QueueEmpty):
                queue.get_nowait()

        with self.subTest('UNBOUND_REMOVE'):
            queue = queues.create()

            interp = common(queue, queues.UNBOUND_REMOVE)
            del interp
            self.assertEqual(queue.qsize(), 0)
            with self.assertRaises(queues.QueueEmpty):
                queue.get_nowait()

            queue.put(b'ham', unbound=queues.UNBOUND_REMOVE)
            self.assertEqual(queue.qsize(), 1)
            interp = common(queue, queues.UNBOUND_REMOVE, 1)
            self.assertEqual(queue.qsize(), 3)
            queue.put(42, unbound=queues.UNBOUND_REMOVE)
            self.assertEqual(queue.qsize(), 4)
            del interp
            self.assertEqual(queue.qsize(), 2)
            obj1 = queue.get()
            obj2 = queue.get()
            self.assertEqual(obj1, b'ham')
            self.assertEqual(obj2, 42)
            self.assertEqual(queue.qsize(), 0)
            with self.assertRaises(queues.QueueEmpty):
                queue.get_nowait()

    def test_put_cleared_with_subinterpreter_mixed(self):
        queue = queues.create()
        interp = interpreters.create()
        _run_output(interp, dedent(f"""
            from test.support.interpreters import queues
            queue = queues.Queue({queue.id})
            queue.put(1, syncobj=True, unbound=queues.UNBOUND)
            queue.put(2, syncobj=True, unbound=queues.UNBOUND_ERROR)
            queue.put(3, syncobj=True)
            queue.put(4, syncobj=True, unbound=queues.UNBOUND_REMOVE)
            queue.put(5, syncobj=True, unbound=queues.UNBOUND)
            """))
        self.assertEqual(queue.qsize(), 5)

        del interp
        self.assertEqual(queue.qsize(), 4)

        obj1 = queue.get()
        self.assertIs(obj1, queues.UNBOUND)
        self.assertEqual(queue.qsize(), 3)

        with self.assertRaises(queues.ItemInterpreterDestroyed):
            queue.get()
        self.assertEqual(queue.qsize(), 2)

        obj2 = queue.get()
        self.assertIs(obj2, queues.UNBOUND)
        self.assertEqual(queue.qsize(), 1)

        obj3 = queue.get()
        self.assertIs(obj3, queues.UNBOUND)
        self.assertEqual(queue.qsize(), 0)

    def test_put_cleared_with_subinterpreter_multiple(self):
        queue = queues.create()
        interp1 = interpreters.create()
        interp2 = interpreters.create()

        queue.put(1, syncobj=True)
        _run_output(interp1, dedent(f"""
            from test.support.interpreters import queues
            queue = queues.Queue({queue.id})
            obj1 = queue.get()
            queue.put(2, syncobj=True, unbound=queues.UNBOUND)
            queue.put(obj1, syncobj=True, unbound=queues.UNBOUND_REMOVE)
            """))
        _run_output(interp2, dedent(f"""
            from test.support.interpreters import queues
            queue = queues.Queue({queue.id})
            obj2 = queue.get()
            obj1 = queue.get()
            """))
        self.assertEqual(queue.qsize(), 0)
        queue.put(3)
        _run_output(interp1, dedent("""
            queue.put(4, syncobj=True, unbound=queues.UNBOUND)
            # interp closed here
            queue.put(5, syncobj=True, unbound=queues.UNBOUND_REMOVE)
            queue.put(6, syncobj=True, unbound=queues.UNBOUND)
            """))
        _run_output(interp2, dedent("""
            queue.put(7, syncobj=True, unbound=queues.UNBOUND_ERROR)
            # interp closed here
            queue.put(obj1, syncobj=True, unbound=queues.UNBOUND_ERROR)
            queue.put(obj2, syncobj=True, unbound=queues.UNBOUND_REMOVE)
            queue.put(8, syncobj=True, unbound=queues.UNBOUND)
            """))
        _run_output(interp1, dedent("""
            queue.put(9, syncobj=True, unbound=queues.UNBOUND_REMOVE)
            queue.put(10, syncobj=True, unbound=queues.UNBOUND)
            """))
        self.assertEqual(queue.qsize(), 10)

        obj3 = queue.get()
        self.assertEqual(obj3, 3)
        self.assertEqual(queue.qsize(), 9)

        obj4 = queue.get()
        self.assertEqual(obj4, 4)
        self.assertEqual(queue.qsize(), 8)

        del interp1
        self.assertEqual(queue.qsize(), 6)

        # obj5 was removed

        obj6 = queue.get()
        self.assertIs(obj6, queues.UNBOUND)
        self.assertEqual(queue.qsize(), 5)

        obj7 = queue.get()
        self.assertEqual(obj7, 7)
        self.assertEqual(queue.qsize(), 4)

        del interp2
        self.assertEqual(queue.qsize(), 3)

        # obj1
        with self.assertRaises(queues.ItemInterpreterDestroyed):
            queue.get()
        self.assertEqual(queue.qsize(), 2)

        # obj2 was removed

        obj8 = queue.get()
        self.assertIs(obj8, queues.UNBOUND)
        self.assertEqual(queue.qsize(), 1)

        # obj9 was removed

        obj10 = queue.get()
        self.assertIs(obj10, queues.UNBOUND)
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
            queue2.put(obj, syncobj=True)
        t = threading.Thread(target=f)
        t.start()

        orig = b'spam'
        queue1.put(orig, syncobj=True)
        obj = queue2.get()
        t.join()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()

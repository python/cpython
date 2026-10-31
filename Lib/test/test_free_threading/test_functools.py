import random
import unittest

from functools import Placeholder, lru_cache, partial
from threading import Barrier, Thread

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestLRUCache(unittest.TestCase):

    def _test_concurrent_operations(self, maxsize):
        num_threads = 10
        b = Barrier(num_threads)
        @lru_cache(maxsize=maxsize)
        def func(arg=0):
            return object()


        def thread_func():
            b.wait()
            for i in range(1000):
                r = random.randint(0, 1000)
                if i < 800:
                    func(i)
                elif i < 900:
                    func.cache_info()
                else:
                    func.cache_clear()

        threads = []
        for i in range(num_threads):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_concurrent_operations_unbounded(self):
        self._test_concurrent_operations(maxsize=None)

    def test_concurrent_operations_bounded(self):
        self._test_concurrent_operations(maxsize=128)

    def _test_reentrant_cache_clear(self, maxsize):
        num_threads = 10
        b = Barrier(num_threads)
        @lru_cache(maxsize=maxsize)
        def func(arg=0):
            func.cache_clear()
            return object()


        def thread_func():
            b.wait()
            for i in range(1000):
                func(random.randint(0, 10000))

        threads = []
        for i in range(num_threads):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_reentrant_cache_clear_unbounded(self):
        self._test_reentrant_cache_clear(maxsize=None)

    def test_reentrant_cache_clear_bounded(self):
        self._test_reentrant_cache_clear(maxsize=128)


def partial_target(x, y=0, *extra, z=0):
    return x, y, z, True


class PartialCallable:
    def __call__(self, x, y=0, *extra, z=0):
        return x, y, z, False


@threading_helper.requires_working_threading()
class TestPartial(unittest.TestCase):
    def _race_setstate(self, p, read, *, placeholders=False, readers=4):
        iterations = 2000
        barrier = Barrier(readers + 2)
        errors = []
        callable_object = PartialCallable()

        def reader():
            try:
                barrier.wait()
                for _ in range(iterations):
                    read(p)
            except Exception as exc:
                errors.append(exc)

        def writer(offset):
            try:
                barrier.wait()
                for i in range(iterations):
                    value = 10**12 + offset * iterations + i
                    if placeholders and i % 2:
                        args = (Placeholder, value)
                    else:
                        args = (value, value)
                    func = partial_target if i % 2 else callable_object
                    p.__setstate__((func, args, {"z": value}, {"tag": value}))
            except Exception as exc:
                errors.append(exc)

        threads = [Thread(target=reader) for _ in range(readers)]
        threads += [Thread(target=writer, args=(i,)) for i in range(2)]
        with threading_helper.start_threads(threads):
            pass
        self.assertEqual(errors, [])

    def _partial(self):
        p = partial(PartialCallable(), 0, 0, z=0)
        p.tag = 0
        return p

    def test_concurrent_setstate(self):
        p = self._partial()
        self._race_setstate(p, None, readers=0)
        x, y, z, marker = p()
        self.assertEqual((x, x), (y, z))
        self.assertEqual(marker, bool(y % 2))

    def test_concurrent_setstate_and_repr(self):
        self._race_setstate(self._partial(), repr)

    def test_concurrent_setstate_and_call(self):
        def read(p):
            x, y, z, marker = p()
            self.assertEqual((x, x), (y, z))
            self.assertEqual(marker, bool(y % 2))

        self._race_setstate(self._partial(), read)

    def test_concurrent_setstate_and_placeholders(self):
        def read(p):
            x, y, z, marker = p(99)
            self.assertEqual(y, z)
            self.assertIn(x, (99, y))
            self.assertEqual(marker, bool(y % 2))

        self._race_setstate(self._partial(), read, placeholders=True)

    def test_concurrent_setstate_and_reduce(self):
        def read(p):
            constructor_args, state = p.__reduce__()[1:]
            func, args, kw, instance_dict = state
            self.assertIs(constructor_args[0], func)
            self.assertEqual(func is partial_target, bool(args[0] % 2))
            self.assertEqual((args[0], args[0]), (args[1], kw["z"]))
            self.assertEqual(args[0], instance_dict["tag"])

        self._race_setstate(self._partial(), read)

    def test_concurrent_setstate_and_flatten(self):
        p = partial(partial_target, 0, 0, z=0)
        iterations = 2000
        barrier = Barrier(5)
        errors = []

        def writer():
            try:
                barrier.wait()
                for i in range(iterations):
                    value = 10**12 + i
                    args = (Placeholder, value) if i % 2 else (value, value)
                    p.__setstate__((partial_target, args, {"z": value}, None))
            except Exception as exc:
                errors.append(exc)

        def reader():
            try:
                barrier.wait()
                for _ in range(iterations):
                    q = partial(p, 99)
                    x, y, z, marker = q()
                    self.assertIs(q.func, partial_target)
                    self.assertEqual(y, z)
                    self.assertIn(x, (99, y))
                    self.assertTrue(marker)
            except Exception as exc:
                errors.append(exc)

        threads = [Thread(target=reader) for _ in range(4)]
        threads.append(Thread(target=writer))
        with threading_helper.start_threads(threads):
            pass
        self.assertEqual(errors, [])

    def test_concurrent_setstate_and_members(self):
        def read(p):
            self.assertTrue(callable(p.func))
            self.assertEqual(len(p.args), 2)
            self.assertIn("z", p.keywords)

        self._race_setstate(self._partial(), read)

    def test_concurrent_setstate_and_instance_dict(self):
        def read(p):
            self.assertIsInstance(p.tag, int)
            self.assertIn("tag", p.__dict__)

        self._race_setstate(self._partial(), read)


if __name__ == "__main__":
    unittest.main()

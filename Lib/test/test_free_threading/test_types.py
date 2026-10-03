import unittest
from typing import TypeVar
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestGenericAlias(unittest.TestCase):
    def test_parameters_race(self):
        # gh-153298

        T = TypeVar('T')
        slot = [list[T]]

        def access():
            for _ in range(2000):
                try:
                    _ = slot[0].__parameters__
                except Exception:
                    pass

        def refresh():
            for _ in range(2000):
                slot[0] = list[T]

        threading_helper.run_concurrently([
            *[access for _ in range(6)],
            *[refresh for _ in range(2)],
        ])

    def test_getitem_parameters_race(self):
        # gh-153298: ga_getitem() lazily initializes __parameters__;
        # racing subscriptions must not race on the write or leak.
        T = TypeVar('T')
        for _ in range(100):
            alias = list[T]

            def subscribe():
                self.assertEqual(alias[int], list[int])

            threading_helper.run_concurrently(subscribe, nthreads=8)

    def test_iter_next_reduce_race(self):
        # gh-154916: next() clears the iterator's reference to the alias
        # while __reduce__() reads it; the alias must not be freed in
        # between (the iterator can hold the last reference).
        def use(it):
            it.__reduce__()
            try:
                next(it)
            except StopIteration:
                pass
            it.__reduce__()

        for _ in range(100):
            it = iter(list[int])
            threading_helper.run_concurrently(use, nthreads=8, args=(it,))


if __name__ == "__main__":
    unittest.main()

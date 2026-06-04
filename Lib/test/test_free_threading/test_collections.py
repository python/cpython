import unittest
from collections import deque
from copy import copy
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestDeque(unittest.TestCase):
    def test_copy_race(self):
        # gh-144809: Test that deque copy is thread safe. It previously
        # could raise a "deque mutated during iteration" error.
        d = deque(range(100))

        def mutate():
            for i in range(1000):
                d.append(i)
                if len(d) > 200:
                    d.popleft()

        def copy_loop():
            for _ in range(1000):
                copy(d)

        threading_helper.run_concurrently([mutate, copy_loop])

    def test_index_race_in_ac(self):
        # gh-150750: There was a c_default specified as `Py_SIZE(self)`,
        # it was used without a critical section.

        d = deque(range(100))

        def index():
            for _ in range(10000):
                try:
                    d.index(50)
                except ValueError:
                    pass

        def mutate():
            for _ in range(10000):
                d.append(0)
                d.clear()
                d.extend(range(100))
                d.appendleft(-1)

        threading_helper.run_concurrently(
            [index, *[mutate for _ in range(3)]],
        )


if __name__ == "__main__":
    unittest.main()

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


if __name__ == "__main__":
    unittest.main()

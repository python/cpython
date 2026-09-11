import unittest

from test import support
from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently


sqlite3 = import_helper.import_module("sqlite3")


NAME = "FREE_THREADING_RACE"
NCOLUMNS = 64
NITER = 3000
QUERY = "select " + ", ".join(
    f"'value' as 'c{i} [{NAME}]'" for i in range(NCOLUMNS)
)


@threading_helper.requires_working_threading()
class TestSQLite3(unittest.TestCase):
    def test_concurrent_converter_replacement(self):
        # gh-155781: Converter lookups must retain a strong reference while
        # another thread updates the public converter registry.
        class Converter:
            __slots__ = ("value",)

            def __init__(self, value):
                self.value = value

            def __call__(self, value):
                return self.value

        def reader():
            con = sqlite3.connect(":memory:",
                                  detect_types=sqlite3.PARSE_COLNAMES)
            try:
                for _ in range(NITER):
                    row = con.execute(QUERY).fetchone()
                    self.assertEqual(len(row), NCOLUMNS)
            finally:
                con.close()

        def mutator():
            for i in range(NITER * NCOLUMNS):
                sqlite3.register_converter(NAME, Converter(i))
                if i % 3 == 0:
                    sqlite3.converters.pop(NAME, None)

        with support.swap_item(sqlite3.converters, NAME, Converter(0)):
            run_concurrently([reader] * 8 + [mutator] * 2)


if __name__ == "__main__":
    unittest.main()

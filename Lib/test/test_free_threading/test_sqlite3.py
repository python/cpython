import threading
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

    def test_close_during_statement_cache_lookup(self):
        # execute() must load the statement cache under the connection lock,
        # since close() on another thread clears it.  Params.__iter__ runs
        # after execute() has checked the connection and before it loads the
        # cache, so the handshake orders only the checks before close().
        class Stop(Exception):
            pass

        class SQL(str):
            # Raise at the cache lookup, before SQLite is used.
            def __hash__(self):
                raise Stop

        def run():
            cx = sqlite3.connect(":memory:", check_same_thread=False)
            cu = cx.cursor()
            close = cx.close
            ready = threading.Lock()
            ready.acquire()
            raised = []

            class Params:
                def __iter__(self):
                    ready.release()
                    return iter(())

            def reader():
                try:
                    cu.executemany(SQL("select 1"), Params())
                except Exception as exc:
                    raised.append(exc)

            t = threading.Thread(target=reader)
            t.start()
            try:
                # Ensure we don't close too early before other thread
                # has got past check_cursor()
                self.assertTrue(ready.acquire(timeout=support.SHORT_TIMEOUT))
                close()
            finally:
                t.join()
            self.assertEqual(len(raised), 1)
            self.assertIsInstance(raised[0], (Stop, sqlite3.ProgrammingError))

        # A GC pause between the two threads would also order them.
        with support.disable_gc():
            for _ in range(5):
                run()


if __name__ == "__main__":
    unittest.main()

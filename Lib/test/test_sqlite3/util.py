import contextlib
import functools
import io
import re
import sqlite3
import test.support
import unittest


# Helper for temporary memory databases
def memory_database(*args, **kwargs):
    cx = sqlite3.connect(":memory:", *args, **kwargs)
    return contextlib.closing(cx)


# Temporarily limit a database connection parameter
@contextlib.contextmanager
def cx_limit(cx, category=sqlite3.SQLITE_LIMIT_SQL_LENGTH, limit=128):
    try:
        _prev = cx.setlimit(category, limit)
        yield limit
    finally:
        cx.setlimit(category, _prev)


def with_tracebacks(exc, regex="", name=""):
    """Convenience decorator for testing callback tracebacks."""
    def decorator(func):
        _regex = re.compile(regex) if regex else None
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            with test.support.catch_unraisable_exception() as cm:
                # First, run the test with traceback enabled.
                with check_tracebacks(self, cm, exc, _regex, name):
                    func(self, *args, **kwargs)

            # Then run the test with traceback disabled.
            func(self, *args, **kwargs)
        return wrapper
    return decorator


@contextlib.contextmanager
def check_tracebacks(self, cm, exc, regex, obj_name):
    """Convenience context manager for testing callback tracebacks."""
    sqlite3.enable_callback_tracebacks(True)
    try:
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            yield

        self.assertEqual(cm.unraisable.exc_type, exc)
        if regex:
            msg = str(cm.unraisable.exc_value)
            self.assertIsNotNone(regex.search(msg))
        if obj_name:
            self.assertEqual(cm.unraisable.object.__name__, obj_name)
    finally:
        sqlite3.enable_callback_tracebacks(False)


class MemoryDatabaseMixin:

    def setUp(self):
        self.con = sqlite3.connect(":memory:")
        self.cur = self.con.cursor()

    def tearDown(self):
        self.cur.close()
        self.con.close()

    @property
    def cx(self):
        return self.con

    @property
    def cu(self):
        return self.cur


def requires_virtual_table(module):
    with memory_database() as cx:
        supported = (module,) in list(cx.execute("PRAGMA module_list"))
        reason = f"Requires {module!r} virtual table support"
        return unittest.skipUnless(supported, reason)

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


def with_tracebacks(exc, regex="", name="", msg_regex=""):
    """Convenience decorator for testing callback tracebacks."""
    def decorator(func):
        exc_regex = re.compile(regex) if regex else None
        _msg_regex = re.compile(msg_regex) if msg_regex else None
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            with test.support.catch_unraisable_exception() as cm:
                # First, run the test with traceback enabled.
                with check_tracebacks(self, cm, exc, exc_regex, _msg_regex, name):
                    func(self, *args, **kwargs)

            # Then run the test with traceback disabled.
            func(self, *args, **kwargs)
        return wrapper
    return decorator


@contextlib.contextmanager
def check_tracebacks(self, cm, exc, exc_regex, msg_regex, obj_name):
    """Convenience context manager for testing callback tracebacks."""
    sqlite3.enable_callback_tracebacks(True)
    try:
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            yield

        self.assertEqual(cm.unraisable.exc_type, exc)
        if exc_regex:
            msg = str(cm.unraisable.exc_value)
            self.assertIsNotNone(exc_regex.search(msg), (exc_regex, msg))
        if msg_regex:
            msg = cm.unraisable.err_msg
            self.assertIsNotNone(msg_regex.search(msg), (msg_regex, msg))
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

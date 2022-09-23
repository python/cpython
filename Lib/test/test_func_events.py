import unittest

from contextlib import contextmanager
from test.support import catch_unraisable_exception, import_helper

_testcapi = import_helper.import_module("_testcapi")

from _testcapi import (
    PYFUNC_EVENT_CREATED,
    PYFUNC_EVENT_DESTROY,
    PYFUNC_EVENT_MODIFY_CODE,
    PYFUNC_EVENT_MODIFY_DEFAULTS,
    PYFUNC_EVENT_MODIFY_KWDEFAULTS,
    _add_func_watcher,
    _clear_func_watcher,
)


class FuncEventsTest(unittest.TestCase):
    @contextmanager
    def add_watcher(self, func):
        wid = _add_func_watcher(func)
        try:
            yield
        finally:
            _clear_func_watcher(wid)

    def test_func_events_dispatched(self):
        events = []
        def watcher(*args):
            events.append(args)

        with self.add_watcher(watcher):
            def myfunc():
                pass
            self.assertIn((PYFUNC_EVENT_CREATED, myfunc, None), events)
            myfunc_id = id(myfunc)

            new_code = self.test_func_events_dispatched.__code__
            myfunc.__code__ = new_code
            self.assertIn((PYFUNC_EVENT_MODIFY_CODE, myfunc, new_code), events)

            new_defaults = (123,)
            myfunc.__defaults__ = new_defaults
            self.assertIn((PYFUNC_EVENT_MODIFY_DEFAULTS, myfunc, new_defaults), events)

            new_kwdefaults = {"self": 123}
            myfunc.__kwdefaults__ = new_kwdefaults
            self.assertIn((PYFUNC_EVENT_MODIFY_KWDEFAULTS, myfunc, new_kwdefaults), events)

            # Clear events reference to func
            events = []
            del myfunc
            self.assertIn((PYFUNC_EVENT_DESTROY, myfunc_id, None), events)

    def test_multiple_watchers(self):
        events0 = []
        def first_watcher(*args):
            events0.append(args)

        events1 = []
        def second_watcher(*args):
            events1.append(args)

        with self.add_watcher(first_watcher):
            with self.add_watcher(second_watcher):
                def myfunc():
                    pass

                event = (PYFUNC_EVENT_CREATED, myfunc, None)
                self.assertIn(event, events0)
                self.assertIn(event, events1)

    def test_watcher_raises_error(self):
        class MyError(Exception):
            pass

        def watcher(*args):
            raise MyError("testing 123")

        with self.add_watcher(watcher):
            with catch_unraisable_exception() as cm:
                def myfunc():
                    pass

                self.assertIs(cm.unraisable.object, myfunc)
                self.assertIsInstance(cm.unraisable.exc_value, MyError)

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"Invalid func watcher ID -1"):
            _clear_func_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"Invalid func watcher ID 8"):
            _clear_func_watcher(8)  # FUNC_MAX_WATCHERS = 8

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"No func watcher set for ID 1"):
            _clear_func_watcher(1)

import unittest
from _testcapi import (
    PYFUNC_EVENT_CREATED,
    PYFUNC_EVENT_DESTROY,
    PYFUNC_EVENT_MODIFY_CODE,
    PYFUNC_EVENT_MODIFY_DEFAULTS,
    PYFUNC_EVENT_MODIFY_KWDEFAULTS,
    restore_func_watch_callback,
    set_func_watch_callback,
)


class FuncEventsTest(unittest.TestCase):
    def test_func_events_dispatched(self):
        events = []
        def handle_func_event(*args):
            events.append(args)
        set_func_watch_callback(handle_func_event)

        try:
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
        finally:
            restore_func_watch_callback()

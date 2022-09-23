import unittest
from _testcapi import (
    PYFUNC_EVENT_CREATED,
    PYFUNC_EVENT_DESTROY,
    PYFUNC_EVENT_MODIFY_CODE,
    PYFUNC_EVENT_MODIFY_DEFAULTS,
    PYFUNC_EVENT_MODIFY_KWDEFAULTS,
    restore_func_event_callback,
    set_func_event_callback,
)


class FuncEventsTest(unittest.TestCase):
    def test_func_events_dispatched(self):
        event = None
        def handle_func_event(*args):
            nonlocal event
            event = args
        set_func_event_callback(handle_func_event)

        try:
            def myfunc():
                pass
            self.assertEqual(event, (PYFUNC_EVENT_CREATED, myfunc, None))
            myfunc_id = id(myfunc)

            new_code = self.test_func_events_dispatched.__code__
            myfunc.__code__ = new_code
            self.assertEqual(event, (PYFUNC_EVENT_MODIFY_CODE, myfunc, new_code))

            new_defaults = (123,)
            myfunc.__defaults__ = new_defaults
            self.assertEqual(event, (PYFUNC_EVENT_MODIFY_DEFAULTS, myfunc, new_defaults))

            new_kwdefaults = {"self": 123}
            myfunc.__kwdefaults__ = new_kwdefaults
            self.assertEqual(event, (PYFUNC_EVENT_MODIFY_KWDEFAULTS, myfunc, new_kwdefaults))

            # Clear event's reference to func
            event = None
            del myfunc
            self.assertEqual(event, (PYFUNC_EVENT_DESTROY, myfunc_id, None))
        finally:
            restore_func_event_callback()

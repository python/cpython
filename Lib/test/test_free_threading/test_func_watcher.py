import gc
import unittest

from test.support import import_helper, threading_helper

_testcapi = import_helper.import_module("_testcapi")

NTHREADS = 8
ITERS = 200


def template(x=1, *, y=2):
    return x


@threading_helper.requires_working_threading()
class TestFuncWatcherThreadSafety(unittest.TestCase):
    """MODIFY events and stores to function fields must not interleave.

    When a function watcher receives a MODIFY event, the function must still
    hold the value announced by the previous event for the same attribute.
    Otherwise watchers that track a function's state (e.g. a JIT) can end up
    believing it holds a different value than it does.
    """

    def setUp(self):
        # Stopping the world for a garbage collection can suspend the
        # function's critical section between an event and its store.  Keep
        # the GC out of the way so this checks what the setters guarantee.
        if gc.isenabled():
            gc.disable()
            self.addCleanup(gc.enable)

    def check_events_match_stores(self, event, getter, setter, make_value):
        def target(x=1, *, y=2):
            return x

        def writer():
            for _ in range(ITERS):
                setter(target, make_value())

        _testcapi.start_func_order_watcher(target, event)
        try:
            threading_helper.run_concurrently(writer, NTHREADS)
        finally:
            history, mismatches = _testcapi.stop_func_order_watcher()

        self.assertEqual(mismatches, 0,
                         f"{mismatches} of {len(history) - 1} events were "
                         f"delivered before the previous event's store")
        self.assertEqual(len(history), NTHREADS * ITERS + 1)
        self.assertIs(getter(target), history[-1])

    def check_attr(self, attr, event, make_value):
        self.check_events_match_stores(
            event,
            lambda f: getattr(f, attr),
            lambda f, v: setattr(f, attr, v),
            make_value)

    def test_code(self):
        # Every store gets a distinct code object so that identity checks
        # can't pass by coincidence.
        code = template.__code__
        self.check_attr(
            "__code__", _testcapi.PYFUNC_EVENT_MODIFY_CODE,
            lambda: code.replace(co_name=f"template_{id(object())}"))

    def test_defaults(self):
        self.check_attr(
            "__defaults__", _testcapi.PYFUNC_EVENT_MODIFY_DEFAULTS,
            lambda: (object(),))

    def test_kwdefaults(self):
        self.check_attr(
            "__kwdefaults__", _testcapi.PYFUNC_EVENT_MODIFY_KWDEFAULTS,
            lambda: {"y": object()})

    def test_qualname(self):
        self.check_attr(
            "__qualname__", _testcapi.PYFUNC_EVENT_MODIFY_QUALNAME,
            lambda: str(object()))

    def test_defaults_via_capi(self):
        self.check_events_match_stores(
            _testcapi.PYFUNC_EVENT_MODIFY_DEFAULTS,
            lambda f: f.__defaults__,
            _testcapi.set_func_defaults_via_capi,
            lambda: (object(),))

    def test_kwdefaults_via_capi(self):
        self.check_events_match_stores(
            _testcapi.PYFUNC_EVENT_MODIFY_KWDEFAULTS,
            lambda f: f.__kwdefaults__,
            _testcapi.set_func_kwdefaults_via_capi,
            lambda: {"y": object()})


if __name__ == "__main__":
    unittest.main()

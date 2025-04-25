import unittest
import contextvars

from contextlib import contextmanager, ExitStack
from test.support import (
    catch_unraisable_exception, import_helper,
    gc_collect)


# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class TestDictWatchers(unittest.TestCase):
    # types of watchers testcapimodule can add:
    EVENTS = 0   # appends dict events as strings to global event list
    ERROR = 1    # unconditionally sets and signals a RuntimeException
    SECOND = 2   # always appends "second" to global event list

    def add_watcher(self, kind=EVENTS):
        return _testcapi.add_dict_watcher(kind)

    def clear_watcher(self, watcher_id):
        _testcapi.clear_dict_watcher(watcher_id)

    @contextmanager
    def watcher(self, kind=EVENTS):
        wid = self.add_watcher(kind)
        try:
            yield wid
        finally:
            self.clear_watcher(wid)

    def assert_events(self, expected):
        actual = _testcapi.get_dict_watcher_events()
        self.assertEqual(actual, expected)

    def watch(self, wid, d):
        _testcapi.watch_dict(wid, d)

    def unwatch(self, wid, d):
        _testcapi.unwatch_dict(wid, d)

    def test_set_new_item(self):
        d = {}
        with self.watcher() as wid:
            self.watch(wid, d)
            d["foo"] = "bar"
            self.assert_events(["new:foo:bar"])

    def test_set_existing_item(self):
        d = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            d["foo"] = "baz"
            self.assert_events(["mod:foo:baz"])

    def test_clone(self):
        d = {}
        d2 = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            d.update(d2)
            self.assert_events(["clone"])

    def test_no_event_if_not_watched(self):
        d = {}
        with self.watcher() as wid:
            d["foo"] = "bar"
            self.assert_events([])

    def test_del(self):
        d = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            del d["foo"]
            self.assert_events(["del:foo"])

    def test_pop(self):
        d = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            d.pop("foo")
            self.assert_events(["del:foo"])

    def test_clear(self):
        d = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            d.clear()
            self.assert_events(["clear"])

    def test_dealloc(self):
        d = {"foo": "bar"}
        with self.watcher() as wid:
            self.watch(wid, d)
            del d
            self.assert_events(["dealloc"])

    def test_object_dict(self):
        class MyObj: pass
        o = MyObj()

        with self.watcher() as wid:
            self.watch(wid, o.__dict__)
            o.foo = "bar"
            o.foo = "baz"
            del o.foo
            self.assert_events(["new:foo:bar", "mod:foo:baz", "del:foo"])

        with self.watcher() as wid:
            self.watch(wid, o.__dict__)
            for _ in range(100):
                o.foo = "bar"
            self.assert_events(["new:foo:bar"] + ["mod:foo:bar"] * 99)

    def test_unwatch(self):
        d = {}
        with self.watcher() as wid:
            self.watch(wid, d)
            d["foo"] = "bar"
            self.unwatch(wid, d)
            d["hmm"] = "baz"
            self.assert_events(["new:foo:bar"])

    def test_error(self):
        d = {}
        with self.watcher(kind=self.ERROR) as wid:
            self.watch(wid, d)
            with catch_unraisable_exception() as cm:
                d["foo"] = "bar"
                self.assertIn(
                    "Exception ignored in "
                    "PyDict_EVENT_ADDED watcher callback for <dict at ",
                    cm.unraisable.err_msg
                )
                self.assertIsNone(cm.unraisable.object)
                self.assertEqual(str(cm.unraisable.exc_value), "boom!")
            self.assert_events([])

    def test_dealloc_error(self):
        d = {}
        with self.watcher(kind=self.ERROR) as wid:
            self.watch(wid, d)
            with catch_unraisable_exception() as cm:
                del d
                self.assertEqual(str(cm.unraisable.exc_value), "boom!")

    def test_two_watchers(self):
        d1 = {}
        d2 = {}
        with self.watcher() as wid1:
            with self.watcher(kind=self.SECOND) as wid2:
                self.watch(wid1, d1)
                self.watch(wid2, d2)
                d1["foo"] = "bar"
                d2["hmm"] = "baz"
                self.assert_events(["new:foo:bar", "second"])

    def test_watch_non_dict(self):
        with self.watcher() as wid:
            with self.assertRaisesRegex(ValueError, r"Cannot watch non-dictionary"):
                self.watch(wid, 1)

    def test_watch_out_of_range_watcher_id(self):
        d = {}
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID -1"):
            self.watch(-1, d)
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID 8"):
            self.watch(8, d)  # DICT_MAX_WATCHERS = 8

    def test_watch_unassigned_watcher_id(self):
        d = {}
        with self.assertRaisesRegex(ValueError, r"No dict watcher set for ID 3"):
            self.watch(3, d)

    def test_unwatch_non_dict(self):
        with self.watcher() as wid:
            with self.assertRaisesRegex(ValueError, r"Cannot watch non-dictionary"):
                self.unwatch(wid, 1)

    def test_unwatch_out_of_range_watcher_id(self):
        d = {}
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID -1"):
            self.unwatch(-1, d)
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID 8"):
            self.unwatch(8, d)  # DICT_MAX_WATCHERS = 8

    def test_unwatch_unassigned_watcher_id(self):
        d = {}
        with self.assertRaisesRegex(ValueError, r"No dict watcher set for ID 3"):
            self.unwatch(3, d)

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID -1"):
            self.clear_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"Invalid dict watcher ID 8"):
            self.clear_watcher(8)  # DICT_MAX_WATCHERS = 8

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"No dict watcher set for ID 3"):
            self.clear_watcher(3)


class TestTypeWatchers(unittest.TestCase):
    # types of watchers testcapimodule can add:
    TYPES = 0    # appends modified types to global event list
    ERROR = 1    # unconditionally sets and signals a RuntimeException
    WRAP = 2     # appends modified type wrapped in list to global event list

    # duplicating the C constant
    TYPE_MAX_WATCHERS = 8

    def add_watcher(self, kind=TYPES):
        return _testcapi.add_type_watcher(kind)

    def clear_watcher(self, watcher_id):
        _testcapi.clear_type_watcher(watcher_id)

    @contextmanager
    def watcher(self, kind=TYPES):
        wid = self.add_watcher(kind)
        try:
            yield wid
        finally:
            self.clear_watcher(wid)

    def assert_events(self, expected):
        actual = _testcapi.get_type_modified_events()
        self.assertEqual(actual, expected)

    def watch(self, wid, t):
        _testcapi.watch_type(wid, t)

    def unwatch(self, wid, t):
        _testcapi.unwatch_type(wid, t)

    def test_watch_type(self):
        class C: pass
        with self.watcher() as wid:
            self.watch(wid, C)
            C.foo = "bar"
            self.assert_events([C])

    def test_event_aggregation(self):
        class C: pass
        with self.watcher() as wid:
            self.watch(wid, C)
            C.foo = "bar"
            C.bar = "baz"
            # only one event registered for both modifications
            self.assert_events([C])

    def test_lookup_resets_aggregation(self):
        class C: pass
        with self.watcher() as wid:
            self.watch(wid, C)
            C.foo = "bar"
            # lookup resets type version tag
            self.assertEqual(C.foo, "bar")
            C.bar = "baz"
            # both events registered
            self.assert_events([C, C])

    def test_unwatch_type(self):
        class C: pass
        with self.watcher() as wid:
            self.watch(wid, C)
            C.foo = "bar"
            self.assertEqual(C.foo, "bar")
            self.assert_events([C])
            self.unwatch(wid, C)
            C.bar = "baz"
            self.assert_events([C])

    def test_clear_watcher(self):
        class C: pass
        # outer watcher is unused, it's just to keep events list alive
        with self.watcher() as _:
            with self.watcher() as wid:
                self.watch(wid, C)
                C.foo = "bar"
                self.assertEqual(C.foo, "bar")
                self.assert_events([C])
            C.bar = "baz"
            # Watcher on C has been cleared, no new event
            self.assert_events([C])

    def test_watch_type_subclass(self):
        class C: pass
        class D(C): pass
        with self.watcher() as wid:
            self.watch(wid, D)
            C.foo = "bar"
            self.assert_events([D])

    def test_error(self):
        class C: pass
        with self.watcher(kind=self.ERROR) as wid:
            self.watch(wid, C)
            with catch_unraisable_exception() as cm:
                C.foo = "bar"
                self.assertEqual(
                    cm.unraisable.err_msg,
                    f"Exception ignored in type watcher callback #1 for {C!r}",
                )
                self.assertIs(cm.unraisable.object, None)
                self.assertEqual(str(cm.unraisable.exc_value), "boom!")
            self.assert_events([])

    def test_two_watchers(self):
        class C1: pass
        class C2: pass
        with self.watcher() as wid1:
            with self.watcher(kind=self.WRAP) as wid2:
                self.assertNotEqual(wid1, wid2)
                self.watch(wid1, C1)
                self.watch(wid2, C2)
                C1.foo = "bar"
                C2.hmm = "baz"
                self.assert_events([C1, [C2]])

    def test_all_watchers(self):
        class C: pass
        with ExitStack() as stack:
            last_wid = -1
            # don't make assumptions about how many watchers are already
            # registered, just go until we reach the max ID
            while last_wid < self.TYPE_MAX_WATCHERS - 1:
                last_wid = stack.enter_context(self.watcher())
            self.watch(last_wid, C)
            C.foo = "bar"
            self.assert_events([C])

    def test_watch_non_type(self):
        with self.watcher() as wid:
            with self.assertRaisesRegex(ValueError, r"Cannot watch non-type"):
                self.watch(wid, 1)

    def test_watch_out_of_range_watcher_id(self):
        class C: pass
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID -1"):
            self.watch(-1, C)
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID 8"):
            self.watch(self.TYPE_MAX_WATCHERS, C)

    def test_watch_unassigned_watcher_id(self):
        class C: pass
        with self.assertRaisesRegex(ValueError, r"No type watcher set for ID 1"):
            self.watch(1, C)

    def test_unwatch_non_type(self):
        with self.watcher() as wid:
            with self.assertRaisesRegex(ValueError, r"Cannot watch non-type"):
                self.unwatch(wid, 1)

    def test_unwatch_out_of_range_watcher_id(self):
        class C: pass
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID -1"):
            self.unwatch(-1, C)
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID 8"):
            self.unwatch(self.TYPE_MAX_WATCHERS, C)

    def test_unwatch_unassigned_watcher_id(self):
        class C: pass
        with self.assertRaisesRegex(ValueError, r"No type watcher set for ID 1"):
            self.unwatch(1, C)

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID -1"):
            self.clear_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"Invalid type watcher ID 8"):
            self.clear_watcher(self.TYPE_MAX_WATCHERS)

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"No type watcher set for ID 1"):
            self.clear_watcher(1)

    def test_no_more_ids_available(self):
        with self.assertRaisesRegex(RuntimeError, r"no more type watcher IDs"):
            with ExitStack() as stack:
                for _ in range(self.TYPE_MAX_WATCHERS + 1):
                    stack.enter_context(self.watcher())


class TestCodeObjectWatchers(unittest.TestCase):
    @contextmanager
    def code_watcher(self, which_watcher):
        wid = _testcapi.add_code_watcher(which_watcher)
        try:
            yield wid
        finally:
            _testcapi.clear_code_watcher(wid)

    def assert_event_counts(self, exp_created_0, exp_destroyed_0,
                            exp_created_1, exp_destroyed_1):
        gc_collect()  # code objects are collected by GC in free-threaded build
        self.assertEqual(
            exp_created_0, _testcapi.get_code_watcher_num_created_events(0))
        self.assertEqual(
            exp_destroyed_0, _testcapi.get_code_watcher_num_destroyed_events(0))
        self.assertEqual(
            exp_created_1, _testcapi.get_code_watcher_num_created_events(1))
        self.assertEqual(
            exp_destroyed_1, _testcapi.get_code_watcher_num_destroyed_events(1))

    def test_code_object_events_dispatched(self):
        # verify that all counts are zero before any watchers are registered
        self.assert_event_counts(0, 0, 0, 0)

        # verify that all counts remain zero when a code object is
        # created and destroyed with no watchers registered
        co1 = _testcapi.code_newempty("test_watchers", "dummy1", 0)
        self.assert_event_counts(0, 0, 0, 0)
        del co1
        self.assert_event_counts(0, 0, 0, 0)

        # verify counts are as expected when first watcher is registered
        with self.code_watcher(0):
            self.assert_event_counts(0, 0, 0, 0)
            co2 = _testcapi.code_newempty("test_watchers", "dummy2", 0)
            self.assert_event_counts(1, 0, 0, 0)
            del co2
            self.assert_event_counts(1, 1, 0, 0)

            # again with second watcher registered
            with self.code_watcher(1):
                self.assert_event_counts(1, 1, 0, 0)
                co3 = _testcapi.code_newempty("test_watchers", "dummy3", 0)
                self.assert_event_counts(2, 1, 1, 0)
                del co3
                self.assert_event_counts(2, 2, 1, 1)

        # verify counts are reset and don't change after both watchers are cleared
        co4 = _testcapi.code_newempty("test_watchers", "dummy4", 0)
        self.assert_event_counts(0, 0, 0, 0)
        del co4
        self.assert_event_counts(0, 0, 0, 0)

    def test_error(self):
        with self.code_watcher(2):
            with catch_unraisable_exception() as cm:
                co = _testcapi.code_newempty("test_watchers", "dummy0", 0)

                self.assertEqual(
                    cm.unraisable.err_msg,
                    f"Exception ignored in "
                    f"PY_CODE_EVENT_CREATE watcher callback for {co!r}"
                )
                self.assertIsNone(cm.unraisable.object)
                self.assertEqual(str(cm.unraisable.exc_value), "boom!")

    def test_dealloc_error(self):
        co = _testcapi.code_newempty("test_watchers", "dummy0", 0)
        with self.code_watcher(2):
            with catch_unraisable_exception() as cm:
                del co
                gc_collect()

                self.assertEqual(str(cm.unraisable.exc_value), "boom!")

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"Invalid code watcher ID -1"):
            _testcapi.clear_code_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"Invalid code watcher ID 8"):
            _testcapi.clear_code_watcher(8)  # CODE_MAX_WATCHERS = 8

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"No code watcher set for ID 1"):
            _testcapi.clear_code_watcher(1)

    def test_allocate_too_many_watchers(self):
        with self.assertRaisesRegex(RuntimeError, r"no more code watcher IDs available"):
            _testcapi.allocate_too_many_code_watchers()


class TestFuncWatchers(unittest.TestCase):
    @contextmanager
    def add_watcher(self, func):
        wid = _testcapi.add_func_watcher(func)
        try:
            yield
        finally:
            _testcapi.clear_func_watcher(wid)

    def test_func_events_dispatched(self):
        events = []
        def watcher(*args):
            events.append(args)

        with self.add_watcher(watcher):
            def myfunc():
                pass
            self.assertIn((_testcapi.PYFUNC_EVENT_CREATE, myfunc, None), events)
            myfunc_id = id(myfunc)

            new_code = self.test_func_events_dispatched.__code__
            myfunc.__code__ = new_code
            self.assertIn((_testcapi.PYFUNC_EVENT_MODIFY_CODE, myfunc, new_code), events)

            new_defaults = (123,)
            myfunc.__defaults__ = new_defaults
            self.assertIn((_testcapi.PYFUNC_EVENT_MODIFY_DEFAULTS, myfunc, new_defaults), events)

            new_defaults = (456,)
            _testcapi.set_func_defaults_via_capi(myfunc, new_defaults)
            self.assertIn((_testcapi.PYFUNC_EVENT_MODIFY_DEFAULTS, myfunc, new_defaults), events)

            new_kwdefaults = {"self": 123}
            myfunc.__kwdefaults__ = new_kwdefaults
            self.assertIn((_testcapi.PYFUNC_EVENT_MODIFY_KWDEFAULTS, myfunc, new_kwdefaults), events)

            new_kwdefaults = {"self": 456}
            _testcapi.set_func_kwdefaults_via_capi(myfunc, new_kwdefaults)
            self.assertIn((_testcapi.PYFUNC_EVENT_MODIFY_KWDEFAULTS, myfunc, new_kwdefaults), events)

            # Clear events reference to func
            events = []
            del myfunc
            self.assertIn((_testcapi.PYFUNC_EVENT_DESTROY, myfunc_id, None), events)

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

                event = (_testcapi.PYFUNC_EVENT_CREATE, myfunc, None)
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

                self.assertEqual(
                    cm.unraisable.err_msg,
                    f"Exception ignored in "
                    f"PyFunction_EVENT_CREATE watcher callback for {repr(myfunc)[1:-1]}"
                )
                self.assertIsNone(cm.unraisable.object)

    def test_dealloc_watcher_raises_error(self):
        class MyError(Exception):
            pass

        def watcher(*args):
            raise MyError("testing 123")

        def myfunc():
            pass

        with self.add_watcher(watcher):
            with catch_unraisable_exception() as cm:
                del myfunc

                self.assertIsInstance(cm.unraisable.exc_value, MyError)

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"invalid func watcher ID -1"):
            _testcapi.clear_func_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"invalid func watcher ID 8"):
            _testcapi.clear_func_watcher(8)  # FUNC_MAX_WATCHERS = 8

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"no func watcher set for ID 1"):
            _testcapi.clear_func_watcher(1)

    def test_allocate_too_many_watchers(self):
        with self.assertRaisesRegex(RuntimeError, r"no more func watcher IDs"):
            _testcapi.allocate_too_many_func_watchers()


class TestContextObjectWatchers(unittest.TestCase):
    @contextmanager
    def context_watcher(self, which_watcher):
        wid = _testcapi.add_context_watcher(which_watcher)
        try:
            switches = _testcapi.get_context_switches(which_watcher)
        except ValueError:
            switches = None
        try:
            yield switches
        finally:
            _testcapi.clear_context_watcher(wid)

    def assert_event_counts(self, want_0, want_1):
        self.assertEqual(len(_testcapi.get_context_switches(0)), want_0)
        self.assertEqual(len(_testcapi.get_context_switches(1)), want_1)

    def test_context_object_events_dispatched(self):
        # verify that all counts are zero before any watchers are registered
        self.assert_event_counts(0, 0)

        # verify that all counts remain zero when a context object is
        # entered and exited with no watchers registered
        ctx = contextvars.copy_context()
        ctx.run(self.assert_event_counts, 0, 0)
        self.assert_event_counts(0, 0)

        # verify counts are as expected when first watcher is registered
        with self.context_watcher(0):
            self.assert_event_counts(0, 0)
            ctx.run(self.assert_event_counts, 1, 0)
            self.assert_event_counts(2, 0)

            # again with second watcher registered
            with self.context_watcher(1):
                self.assert_event_counts(2, 0)
                ctx.run(self.assert_event_counts, 3, 1)
                self.assert_event_counts(4, 2)

        # verify counts are reset and don't change after both watchers are cleared
        ctx.run(self.assert_event_counts, 0, 0)
        self.assert_event_counts(0, 0)

    def test_callback_error(self):
        ctx_outer = contextvars.copy_context()
        ctx_inner = contextvars.copy_context()
        unraisables = []

        def _in_outer():
            with self.context_watcher(2):
                with catch_unraisable_exception() as cm:
                    ctx_inner.run(lambda: unraisables.append(cm.unraisable))
                    unraisables.append(cm.unraisable)

        try:
            ctx_outer.run(_in_outer)
            self.assertEqual([x.err_msg for x in unraisables],
                             ["Exception ignored in Py_CONTEXT_SWITCHED "
                              f"watcher callback for {ctx!r}"
                              for ctx in [ctx_inner, ctx_outer]])
            self.assertEqual([str(x.exc_value) for x in unraisables],
                             ["boom!", "boom!"])
        finally:
            # Break reference cycle
            unraisables = None

    def test_clear_out_of_range_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"Invalid context watcher ID -1"):
            _testcapi.clear_context_watcher(-1)
        with self.assertRaisesRegex(ValueError, r"Invalid context watcher ID 8"):
            _testcapi.clear_context_watcher(8)  # CONTEXT_MAX_WATCHERS = 8

    def test_clear_unassigned_watcher_id(self):
        with self.assertRaisesRegex(ValueError, r"No context watcher set for ID 1"):
            _testcapi.clear_context_watcher(1)

    def test_allocate_too_many_watchers(self):
        with self.assertRaisesRegex(RuntimeError, r"no more context watcher IDs available"):
            _testcapi.allocate_too_many_context_watchers()

    def test_exit_base_context(self):
        ctx = contextvars.Context()
        _testcapi.clear_context_stack()
        with self.context_watcher(0) as switches:
            ctx.run(lambda: None)
        self.assertEqual(switches, [ctx, None])

if __name__ == "__main__":
    unittest.main()

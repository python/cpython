import re
import sys
import types
import unittest
import weakref

from test import support
from test.support import import_helper


class ClearTest(unittest.TestCase):
    """
    Tests for frame.clear().
    """

    def inner(self, x=5, **kwargs):
        1/0

    def outer(self, **kwargs):
        try:
            self.inner(**kwargs)
        except ZeroDivisionError as e:
            exc = e
        return exc

    def clear_traceback_frames(self, tb):
        """
        Clear all frames in a traceback.
        """
        while tb is not None:
            tb.tb_frame.clear()
            tb = tb.tb_next

    def test_clear_locals(self):
        class C:
            pass
        c = C()
        wr = weakref.ref(c)
        exc = self.outer(c=c)
        del c
        support.gc_collect()
        # A reference to c is held through the frames
        self.assertIsNot(None, wr())
        self.clear_traceback_frames(exc.__traceback__)
        support.gc_collect()
        # The reference was released by .clear()
        self.assertIs(None, wr())

    def test_clear_does_not_clear_specials(self):
        class C:
            pass
        c = C()
        exc = self.outer(c=c)
        del c
        f = exc.__traceback__.tb_frame
        f.clear()
        self.assertIsNot(f.f_code, None)
        self.assertIsNot(f.f_locals, None)
        self.assertIsNot(f.f_builtins, None)
        self.assertIsNot(f.f_globals, None)

    def test_clear_generator(self):
        endly = False
        def g():
            nonlocal endly
            try:
                yield
                self.inner()
            finally:
                endly = True
        gen = g()
        next(gen)
        self.assertFalse(endly)
        # Clearing the frame closes the generator
        gen.gi_frame.clear()
        self.assertTrue(endly)

    def test_clear_executing(self):
        # Attempting to clear an executing frame is forbidden.
        try:
            1/0
        except ZeroDivisionError as e:
            f = e.__traceback__.tb_frame
        with self.assertRaises(RuntimeError):
            f.clear()
        with self.assertRaises(RuntimeError):
            f.f_back.clear()

    def test_clear_executing_generator(self):
        # Attempting to clear an executing generator frame is forbidden.
        endly = False
        def g():
            nonlocal endly
            try:
                1/0
            except ZeroDivisionError as e:
                f = e.__traceback__.tb_frame
                with self.assertRaises(RuntimeError):
                    f.clear()
                with self.assertRaises(RuntimeError):
                    f.f_back.clear()
                yield f
            finally:
                endly = True
        gen = g()
        f = next(gen)
        self.assertFalse(endly)
        # Clearing the frame closes the generator
        f.clear()
        self.assertTrue(endly)

    def test_lineno_with_tracing(self):
        def record_line():
            f = sys._getframe(1)
            lines.append(f.f_lineno-f.f_code.co_firstlineno)

        def test(trace):
            record_line()
            if trace:
                sys._getframe(0).f_trace = True
            record_line()
            record_line()

        expected_lines = [1, 4, 5]
        lines = []
        test(False)
        self.assertEqual(lines, expected_lines)
        lines = []
        test(True)
        self.assertEqual(lines, expected_lines)

    @support.cpython_only
    def test_clear_refcycles(self):
        # .clear() doesn't leave any refcycle behind
        with support.disable_gc():
            class C:
                pass
            c = C()
            wr = weakref.ref(c)
            exc = self.outer(c=c)
            del c
            self.assertIsNot(None, wr())
            self.clear_traceback_frames(exc.__traceback__)
            self.assertIs(None, wr())


class FrameAttrsTest(unittest.TestCase):

    def make_frames(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        return frames

    def test_locals(self):
        f, outer, inner = self.make_frames()
        outer_locals = outer.f_locals
        self.assertIsInstance(outer_locals.pop('inner'), types.FunctionType)
        self.assertEqual(outer_locals, {'x': 5, 'y': 6})
        inner_locals = inner.f_locals
        self.assertEqual(inner_locals, {'x': 5, 'z': 7})

    def test_clear_locals(self):
        # Test f_locals after clear() (issue #21897)
        f, outer, inner = self.make_frames()
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_locals_clear_locals(self):
        # Test f_locals before and after clear() (to exercise caching)
        f, outer, inner = self.make_frames()
        self.assertNotEqual(outer.f_locals, {})
        self.assertNotEqual(inner.f_locals, {})
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_f_lineno_del_segfault(self):
        f, _, _ = self.make_frames()
        with self.assertRaises(AttributeError):
            del f.f_lineno

class FastLocalsProxyTest(unittest.TestCase):

    def check_proxy_contents(self, proxy, expected_contents):
        # These checks should never implicitly resync the frame proxy's cache,
        # even if the proxy is referenced as a local variable in the frame
        # However, the first executed check may trigger the initial lazy sync
        self.assertEqual(len(proxy), len(expected_contents))
        self.assertCountEqual(proxy.items(), expected_contents.items())

    def test_dict_query_operations(self):
        # Check retrieval of individual keys via the proxy
        proxy = sys._getframe().f_locals
        self.assertIs(proxy["self"], self)
        self.assertIs(proxy.get("self"), self)
        self.assertIs(proxy.get("no-such-key"), None)
        self.assertIs(proxy.get("no-such-key", Ellipsis), Ellipsis)

        # Proxy value cache is lazily refreshed on the first operation that cares
        # about the full contents of the mapping (such as querying the length)
        expected_proxy_contents = {"self": self, "proxy": proxy}
        expected_proxy_contents["expected_proxy_contents"] = expected_proxy_contents
        self.check_proxy_contents(proxy, expected_proxy_contents)

        # Ensuring copying the proxy produces a plain dict instance
        dict_copy = proxy.copy()
        self.assertIsInstance(dict_copy, dict)
        self.check_proxy_contents(dict_copy, expected_proxy_contents)

        # The proxy automatically updates its cache for O(n) operations like copying,
        # but won't pick up new local variables until it is resync'ed with the frame
        # or that particular key is accessed or queried
        self.check_proxy_contents(proxy, dict_copy)
        self.assertIn("dict_copy", proxy) # Implicitly updates cache for this key
        expected_proxy_contents["dict_copy"] = dict_copy
        self.check_proxy_contents(proxy, expected_proxy_contents)

        # Check forward iteration (order is abitrary, so only check overall contents)
        # Note: len() and the items() method are covered by "check_proxy_contents"
        self.assertCountEqual(proxy, expected_proxy_contents)
        self.assertCountEqual(proxy.keys(), expected_proxy_contents.keys())
        self.assertCountEqual(proxy.values(), expected_proxy_contents.values())
        # Check reversed iteration (order should be reverse of forward iteration)
        self.assertEqual(list(reversed(proxy)), list(reversed(list(proxy))))

        # Check dict union operations (these implicitly refresh the value cache)
        extra_contents = dict(a=1, b=2)
        expected_proxy_contents["extra_contents"] = extra_contents
        self.check_proxy_contents(proxy | proxy, expected_proxy_contents)
        self.assertIsInstance(proxy | proxy, dict)
        self.check_proxy_contents(proxy | extra_contents, expected_proxy_contents | extra_contents)
        self.check_proxy_contents(extra_contents | proxy, expected_proxy_contents | extra_contents)

    def test_dict_mutation_operations(self):
        # Check mutation of local variables via proxy
        proxy = sys._getframe().f_locals
        if not len(proxy): # Trigger the initial implicit cache update
            # This code block never actually runs
            unbound_local = None
        self.assertNotIn("unbound_local", proxy)
        proxy["unbound_local"] = "set via proxy"
        self.assertEqual(unbound_local, "set via proxy")
        del proxy["unbound_local"]
        with self.assertRaises(UnboundLocalError):
            unbound_local
        # Check mutation of cell variables via proxy
        cell_variable = None
        proxy["cell_variable"] = "set via proxy"
        self.assertEqual(cell_variable, "set via proxy")
        def inner():
            return cell_variable
        self.assertEqual(inner(), "set via proxy")
        del proxy["cell_variable"]
        with self.assertRaises(UnboundLocalError):
            cell_variable
        with self.assertRaises(NameError):
            inner()
        # Check storage of additional variables in the frame value cache via proxy
        proxy["extra_variable"] = "added via proxy"
        self.assertEqual(proxy["extra_variable"], "added via proxy")
        with self.assertRaises(NameError):
            extra_variable
        del proxy["extra_variable"]
        self.assertNotIn("extra_variable", proxy)

        # Check pop() on all 3 kinds of variable
        unbound_local = "set directly"
        self.assertEqual(proxy.pop("unbound_local"), "set directly")
        self.assertIs(proxy.pop("unbound_local", None), None)
        with self.assertRaises(KeyError):
            proxy.pop("unbound_local")
        cell_variable = "set directly"
        self.assertEqual(proxy.pop("cell_variable"), "set directly")
        self.assertIs(proxy.pop("cell_variable", None), None)
        with self.assertRaises(KeyError):
            proxy.pop("cell_variable")
        proxy["extra_variable"] = "added via proxy"
        self.assertEqual(proxy.pop("extra_variable"), "added via proxy")
        self.assertIs(proxy.pop("extra_variable", None), None)
        with self.assertRaises(KeyError):
            proxy.pop("extra_variable")

        # Check updating all 3 kinds of variable via update()
        updated_keys = {
            "unbound_local": "set via proxy.update()",
            "cell_variable": "set via proxy.update()",
            "extra_variable": "set via proxy.update()",
        }
        proxy.update(updated_keys)
        self.assertEqual(unbound_local, "set via proxy.update()")
        self.assertEqual(cell_variable, "set via proxy.update()")
        self.assertEqual(proxy["extra_variable"], "set via proxy.update()")

        # Check updating all 3 kinds of variable via an in-place dict union
        updated_keys = {
            "unbound_local": "set via proxy |=",
            "cell_variable": "set via proxy |=",
            "extra_variable": "set via proxy |=",
        }
        proxy |= updated_keys
        self.assertEqual(unbound_local, "set via proxy |=")
        self.assertEqual(cell_variable, "set via proxy |=")
        self.assertEqual(proxy["extra_variable"], "set via proxy |=")

        # Check clearing all variables via the proxy
        # Use a nested generator to allow the test case reference to be
        # restored even after the frame variables are cleared
        def clear_frame_via_proxy(test_case_arg):
            inner_proxy = sys._getframe().f_locals
            inner_proxy["extra_variable"] = "added via inner_proxy"
            test_case_arg.assertEqual(inner_proxy, {
                "inner_proxy": inner_proxy,
                "cell_variable": cell_variable,
                "test_case_arg": test_case_arg,
                "extra_variable": "added via inner_proxy",
            })
            inner_proxy.clear()
            test_case = yield None
            with test_case.assertRaises(UnboundLocalError):
                inner_proxy
            with test_case.assertRaises(UnboundLocalError):
                test_case_arg
            with test_case.assertRaises(NameError):
                cell_variable
            inner_proxy = sys._getframe().f_locals
            test_case.assertNotIn("extra_variable", inner_proxy)
        # Clearing the inner frame even clears the cell in the outer frame
        clear_iter = clear_frame_via_proxy(self)
        next(clear_iter)
        with self.assertRaises(UnboundLocalError):
            cell_variable
        # Run the final checks in the inner frame
        try:
            clear_iter.send(self)
            self.fail("Inner proxy clearing iterator didn't stop")
        except StopIteration:
            pass



        self.fail("PEP 558 TODO: Implement proxy setdefault() test")
        self.fail("PEP 558 TODO: Implement proxy popitem() test")

    def test_popitem(self):
        # Check popitem() in a controlled inner frame
        # This is a separate test case so it can be skipped if the test case
        # detects that something is injecting extra keys into the frame state
        if len(sys._getframe().f_locals) != 1:
            self.skipTest("Locals other than 'self' detected, test case will be unreliable")

        # With no local variables, trying to pop one should fail
        def popitem_exception():
            return sys._getframe().f_locals.popitem()
        with self.assertRaises(KeyError):
            popitem_exception()

        # With exactly one local variable, it should be popped
        def popitem_local(arg="only proxy entry"):
            return sys._getframe().f_locals.popitem(), list(sys._getframe().f_locals)
        popped_item, remaining_vars = popitem_local()
        self.assertEqual(popped_item, ("arg", "only proxy entry"))
        self.assertEqual(remaining_vars, [])

        # With exactly one cell variable, it should be popped
        cell_variable = initial_cell_ref = "only proxy entry"
        def popitem_cell():
            return cell_variable, sys._getframe().f_locals.popitem(), list(sys._getframe().f_locals)
        cell_ref, popped_item, remaining_vars = popitem_cell()
        self.assertEqual(popped_item, ("cell_variable", "only proxy entry"))
        self.assertEqual(remaining_vars, [])
        self.assertIs(cell_ref, initial_cell_ref)
        with self.assertRaises(UnboundLocalError):
            cell_variable

        # With exactly one extra variable, it should be popped
        def popitem_extra():
            sys._getframe().f_locals["extra_variable"] = "only proxy entry"
            return sys._getframe().f_locals.popitem(), list(sys._getframe().f_locals)
        popped_item, remaining_vars = popitem_extra()
        self.assertEqual(popped_item, ("extra_variable", "only proxy entry"))
        self.assertEqual(remaining_vars, [])

    def test_sync_frame_cache(self):
        proxy = sys._getframe().f_locals
        self.assertEqual(len(proxy), 2) # Trigger the initial implicit cache update
        new_variable = None
        # No implicit value cache refresh
        self.assertNotIn("new_variable", set(proxy))
        # But an explicit refresh adds the new key
        proxy.sync_frame_cache()
        self.assertIn("new_variable", set(proxy))

    def test_proxy_sizeof(self):
        self.fail("TODO: Implement proxy sys.getsizeof() test")

    def test_active_frame_c_apis(self):
        # Use ctypes to access the C APIs under test
        ctypes = import_helper.import_module('ctypes')
        Py_IncRef = ctypes.pythonapi.Py_IncRef
        PyEval_GetLocals = ctypes.pythonapi.PyEval_GetLocals
        PyLocals_Get = ctypes.pythonapi.PyLocals_Get
        PyLocals_GetKind = ctypes.pythonapi.PyLocals_GetKind
        PyLocals_GetCopy = ctypes.pythonapi.PyLocals_GetCopy
        PyLocals_GetView = ctypes.pythonapi.PyLocals_GetView
        for capi_func in (Py_IncRef,):
            capi_func.argtypes = (ctypes.py_object,)
        for capi_func in (PyEval_GetLocals,
                          PyLocals_Get, PyLocals_GetCopy, PyLocals_GetView):
            capi_func.restype = ctypes.py_object

        # PyEval_GetLocals() always accesses the running frame,
        # so Py_IncRef has to be called inline (no helper function)

        # This test covers the retrieval APIs, the behavioural tests are covered
        # elsewhere using the `frame.f_locals` attribute and the locals() builtin

        # Test retrieval API behaviour in an optimised scope
        c_locals_cache = PyEval_GetLocals()
        Py_IncRef(c_locals_cache) # Make the borrowed reference a real one
        Py_IncRef(c_locals_cache) # Account for next check's borrowed reference
        self.assertIs(PyEval_GetLocals(), c_locals_cache)
        self.assertEqual(PyLocals_GetKind(), 1) # PyLocals_SHALLOW_COPY
        locals_get = PyLocals_Get()
        self.assertIsInstance(locals_get, dict)
        self.assertIsNot(locals_get, c_locals_cache)
        locals_copy = PyLocals_GetCopy()
        self.assertIsInstance(locals_copy, dict)
        self.assertIsNot(locals_copy, c_locals_cache)
        locals_view = PyLocals_GetView()
        self.assertIsInstance(locals_view, types.MappingProxyType)

        # Test API behaviour in an unoptimised scope
        class ExecFrame:
            c_locals_cache = PyEval_GetLocals()
            Py_IncRef(c_locals_cache) # Make the borrowed reference a real one
            Py_IncRef(c_locals_cache) # Account for next check's borrowed reference
            self.assertIs(PyEval_GetLocals(), c_locals_cache)
            self.assertEqual(PyLocals_GetKind(), 0) # PyLocals_DIRECT_REFERENCE
            locals_get = PyLocals_Get()
            self.assertIs(locals_get, c_locals_cache)
            locals_copy = PyLocals_GetCopy()
            self.assertIsInstance(locals_copy, dict)
            self.assertIsNot(locals_copy, c_locals_cache)
            locals_view = PyLocals_GetView()
            self.assertIsInstance(locals_view, types.MappingProxyType)

    def test_arbitrary_frame_c_apis(self):
        # Use ctypes to access the C APIs under test
        ctypes = import_helper.import_module('ctypes')
        Py_IncRef = ctypes.pythonapi.Py_IncRef
        _PyFrame_BorrowLocals = ctypes.pythonapi._PyFrame_BorrowLocals
        PyFrame_GetLocals = ctypes.pythonapi.PyFrame_GetLocals
        PyFrame_GetLocalsKind = ctypes.pythonapi.PyFrame_GetLocalsKind
        PyFrame_GetLocalsCopy = ctypes.pythonapi.PyFrame_GetLocalsCopy
        PyFrame_GetLocalsView = ctypes.pythonapi.PyFrame_GetLocalsView
        for capi_func in (Py_IncRef, _PyFrame_BorrowLocals,
                          PyFrame_GetLocals, PyFrame_GetLocalsKind,
                          PyFrame_GetLocalsCopy, PyFrame_GetLocalsView):
            capi_func.argtypes = (ctypes.py_object,)
        for capi_func in (_PyFrame_BorrowLocals, PyFrame_GetLocals,
                          PyFrame_GetLocalsCopy, PyFrame_GetLocalsView):
            capi_func.restype = ctypes.py_object

        def get_c_locals(frame):
            c_locals = _PyFrame_BorrowLocals(frame)
            Py_IncRef(c_locals) # Make the borrowed reference a real one
            return c_locals

        # This test covers the retrieval APIs, the behavioural tests are covered
        # elsewhere using the `frame.f_locals` attribute and the locals() builtin

        # Test querying an optimised frame from an unoptimised scope
        func_frame = sys._getframe()
        cls_frame = None
        def set_cls_frame(f):
            nonlocal cls_frame
            cls_frame = f
        class ExecFrame:
            c_locals_cache = get_c_locals(func_frame)
            self.assertIs(get_c_locals(func_frame), c_locals_cache)
            self.assertEqual(PyFrame_GetLocalsKind(func_frame), 1) # PyLocals_SHALLOW_COPY
            locals_get = PyFrame_GetLocals(func_frame)
            self.assertIsInstance(locals_get, dict)
            self.assertIsNot(locals_get, c_locals_cache)
            locals_copy = PyFrame_GetLocalsCopy(func_frame)
            self.assertIsInstance(locals_copy, dict)
            self.assertIsNot(locals_copy, c_locals_cache)
            locals_view = PyFrame_GetLocalsView(func_frame)
            self.assertIsInstance(locals_view, types.MappingProxyType)

            # Keep the class frame alive for the functions below to access
            set_cls_frame(sys._getframe())

        # Test querying an unoptimised frame from an optimised scope
        c_locals_cache = get_c_locals(cls_frame)
        self.assertIs(get_c_locals(cls_frame), c_locals_cache)
        self.assertEqual(PyFrame_GetLocalsKind(cls_frame), 0) # PyLocals_DIRECT_REFERENCE
        locals_get = PyFrame_GetLocals(cls_frame)
        self.assertIs(locals_get, c_locals_cache)
        locals_copy = PyFrame_GetLocalsCopy(cls_frame)
        self.assertIsInstance(locals_copy, dict)
        self.assertIsNot(locals_copy, c_locals_cache)
        locals_view = PyFrame_GetLocalsView(cls_frame)
        self.assertIsInstance(locals_view, types.MappingProxyType)


class ReprTest(unittest.TestCase):
    """
    Tests for repr(frame).
    """

    def test_repr(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()

        offset = outer.__code__.co_firstlineno
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        else:
            self.fail("should have raised")

        f_this, f_outer, f_inner = frames
        file_repr = re.escape(repr(__file__))
        self.assertRegex(repr(f_this),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code test_repr>$"
                         % (file_repr, offset + 23))
        self.assertRegex(repr(f_outer),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code outer>$"
                         % (file_repr, offset + 7))
        self.assertRegex(repr(f_inner),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code inner>$"
                         % (file_repr, offset + 5))


if __name__ == "__main__":
    unittest.main()

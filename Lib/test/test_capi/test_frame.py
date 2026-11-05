import sys
import unittest
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')


class FrameTest(unittest.TestCase):
    def getframe(self):
        return sys._getframe()

    def test_frame_getters(self):
        frame = self.getframe()
        self.assertEqual(frame.f_locals, _testcapi.frame_getlocals(frame))
        self.assertIs(frame.f_globals, _testcapi.frame_getglobals(frame))
        self.assertIs(frame.f_builtins, _testcapi.frame_getbuiltins(frame))
        self.assertEqual(frame.f_lasti, _testcapi.frame_getlasti(frame))

    def test_getvar(self):
        current_frame = sys._getframe()
        x = 1
        self.assertEqual(_testcapi.frame_getvar(current_frame, "x"), 1)
        self.assertEqual(_testcapi.frame_getvarstring(current_frame, b"x"), 1)
        with self.assertRaises(NameError):
            _testcapi.frame_getvar(current_frame, "y")
        with self.assertRaises(NameError):
            _testcapi.frame_getvarstring(current_frame, b"y")

        # wrong name type
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, b'x')
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, 123)

    def getgenframe(self):
        yield sys._getframe()

    def test_frame_get_generator(self):
        gen = self.getgenframe()
        frame = next(gen)
        self.assertIs(gen, _testcapi.frame_getgenerator(frame))

    def test_frame_fback_api(self):
        """Test that accessing `f_back` does not cause a segmentation fault on
        a frame created with `PyFrame_New` (GH-99110)."""
        def dummy():
            pass

        frame = _testcapi.frame_new(dummy.__code__, globals(), locals())
        # The following line should not cause a segmentation fault.
        self.assertIsNone(frame.f_back)


class ExternalFrameTest(unittest.TestCase):
    def test_introspector_new(self):
        intro = _testcapi.external_frame_introspector_new()
        self.assertEqual(type(intro).__name__, "external_frame_introspector")
        self.assertTrue(_testcapi.external_frame_introspector_check(intro))
        self.assertFalse(_testcapi.external_frame_introspector_check(None))
        self.assertFalse(_testcapi.external_frame_introspector_check(42))
        self.assertFalse(_testcapi.external_frame_introspector_check([]))

    def test_introspector_new_null(self):
        with self.assertRaises(ValueError):
            _testcapi.external_frame_introspector_new_null()

    def test_introspector_roundtrip(self):
        self.assertTrue(_testcapi.external_frame_introspector_roundtrip())

    def test_introspector_get_visitor(self):
        intro = _testcapi.external_frame_introspector_new()
        addr = _testcapi.external_frame_introspector_get_visitor(intro)
        self.assertIsInstance(addr, int)
        self.assertNotEqual(addr, 0)
        with self.assertRaises(TypeError):
            _testcapi.external_frame_introspector_get_visitor(None)
        with self.assertRaises(TypeError):
            _testcapi.external_frame_introspector_get_visitor(42)

    def test_gc_visit_external_frame(self):
        _testinternalcapi = import_helper.import_module('_testinternalcapi')
        if sys._is_gil_enabled():
            self.skipTest("requires free-threaded build")
        obj = [1, 2, 3]
        calls, cb_calls = _testinternalcapi.external_frame_gc_test(obj, 0)
        self.assertGreaterEqual(calls, 1)
        self.assertGreaterEqual(cb_calls, 1)

    def test_gc_visit_external_frame_incomplete(self):
        _testinternalcapi = import_helper.import_module('_testinternalcapi')
        if sys._is_gil_enabled():
            self.skipTest("requires free-threaded build")
        obj = [1, 2, 3]
        calls, cb_calls = _testinternalcapi.external_frame_gc_test(obj, 1)
        self.assertGreaterEqual(calls, 1)
        self.assertGreaterEqual(cb_calls, 1)

    def test_frame_init_clears_external_bits(self):
        # VM-created frames must never carry the external bit, otherwise
        # the GC would read a non-existent external header (segfault).
        _testinternalcapi = import_helper.import_module('_testinternalcapi')
        def func():
            pass
        self.assertIsNone(
            _testinternalcapi.frame_init_clears_external_bits(func))

    def test_no_external_frames_on_stack(self):
        _testinternalcapi = import_helper.import_module('_testinternalcapi')
        self.assertIsNone(
            _testinternalcapi.frame_no_external_frames_on_stack())


if __name__ == "__main__":
    unittest.main()

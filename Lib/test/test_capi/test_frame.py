import sys
import unittest
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')


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


class InterpreterFrameTest(unittest.TestCase):

    @staticmethod
    def func():
        return sys._getframe()

    def test_code(self):
        frame = self.func()
        code = _testinternalcapi.iframe_getcode(frame)
        self.assertIs(code, self.func.__code__)

    def test_lasti(self):
        frame = self.func()
        lasti = _testinternalcapi.iframe_getlasti(frame)
        self.assertGreater(lasti, 0)
        self.assertLess(lasti, len(self.func.__code__.co_code))

    def test_line(self):
        frame = self.func()
        line = _testinternalcapi.iframe_getline(frame)
        firstline = self.func.__code__.co_firstlineno
        self.assertEqual(line, firstline + 2)

    def test_frame_object(self):
        frame = sys._getframe()
        obj = _testinternalcapi.iframe_getframeobject(frame)
        self.assertIs(obj, frame)


if __name__ == "__main__":
    unittest.main()

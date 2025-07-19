import unittest
from test.support import is_emscripten

unittest.skipUnless(is_emscripten, "only available on Emscripten")

from pathlib import Path


class EmscriptenAsyncInputDeviceTest(unittest.TestCase):
    def test_emscripten_async_input_device(self):
        from _testinternalcapi import emscripten_set_up_async_input_device
        supported = emscripten_set_up_async_input_device()
        p = Path("/dev/blah")
        self.addCleanup(p.unlink)
        if not supported:
            with open(p, "r") as f:
                self.assertRaises(OSError, f.readline)
            return

        with open(p, "r") as f:
            for _ in range(10):
                self.assertEqual(f.readline().strip(), "ab")
                self.assertEqual(f.readline().strip(), "fi")
                self.assertEqual(f.readline().strip(), "xy")

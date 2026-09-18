import unittest
from test.support import is_emscripten

if not is_emscripten:
    raise unittest.SkipTest("Emscripten-only test")

from _testinternalcapi import emscripten_set_up_async_input_device
from pathlib import Path


class EmscriptenAsyncInputDeviceTest(unittest.TestCase):
    def test_emscripten_async_input_device(self):
        jspi_supported = emscripten_set_up_async_input_device()
        p = Path("/dev/blah")
        self.addCleanup(p.unlink)
        if not jspi_supported:
            with open(p, "r") as f:
                self.assertRaises(OSError, f.readline)
            return

        with open(p, "r") as f:
            for _ in range(10):
                self.assertEqual(f.readline().strip(), "ab")
                self.assertEqual(f.readline().strip(), "fi")
                self.assertEqual(f.readline().strip(), "xy")

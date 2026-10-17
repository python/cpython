import signal
import unittest
from test.support import is_emscripten

if not is_emscripten:
    raise unittest.SkipTest("Emscripten-only test")

from _testinternalcapi import (
    emscripten_check_signal_buffer,
    emscripten_set_up_async_input_device,
)
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


class EmscriptenSignalBufferTest(unittest.TestCase):
    def check_signal_buffer(self, shared):
        received = []

        def handler(signum, frame):
            received.append(signum)

        old_handler = signal.signal(signal.SIGUSR1, handler)
        self.addCleanup(signal.signal, signal.SIGUSR1, old_handler)

        left_in_buffer = emscripten_check_signal_buffer(signal.SIGUSR1, shared)
        for _ in range(1000):
            if received:
                break

        self.assertEqual(left_in_buffer, 0)
        self.assertEqual(received, [signal.SIGUSR1])

    def test_shared_buffer(self):
        self.check_signal_buffer(shared=True)

    def test_unshared_buffer(self):
        self.check_signal_buffer(shared=False)

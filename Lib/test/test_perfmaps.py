import os
import sysconfig
import unittest

try:
    from _testinternalcapi import perf_map_state_teardown, write_perf_map_entry
except ImportError:
    raise unittest.SkipTest("requires _testinternalcapi")

def supports_trampoline_profiling():
    perf_trampoline = sysconfig.get_config_var("PY_HAVE_PERF_TRAMPOLINE")
    if not perf_trampoline:
        return False
    return int(perf_trampoline) == 1

if not supports_trampoline_profiling():
    raise unittest.SkipTest("perf trampoline profiling not supported")

class TestPerfMapWriting(unittest.TestCase):
    def test_write_perf_map_entry(self):
        self.assertEqual(write_perf_map_entry(0x1234, 5678, "entry1"), 0)
        self.assertEqual(write_perf_map_entry(0x2345, 6789, "entry2"), 0)
        with open(f"/tmp/perf-{os.getpid()}.map") as f:
            perf_file_contents = f.read()
            self.assertIn("1234 162e entry1", perf_file_contents)
            self.assertIn("2345 1a85 entry2", perf_file_contents)
        perf_map_state_teardown()

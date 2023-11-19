import os
import sys
import unittest

from _testinternalcapi import perf_map_state_teardown, write_perf_map_entry

if sys.platform != 'linux':
    raise unittest.SkipTest('Linux only')

def entry1():
    pass

def entry2():
    pass

class TestPerfMapWriting(unittest.TestCase):
    def test_write_perf_map_entry(self):
        sys.activate_stack_trampoline("perf")
        try:
            self.assertEqual(write_perf_map_entry(0x1234, 5678, entry1.__code__), 0)
            self.assertEqual(write_perf_map_entry(0x2345, 6789, entry2.__code__), 0)
            with open(f"/tmp/perf-{os.getpid()}.map") as f:
                perf_file_contents = f.read()
                self.assertIn("1234 162e py::entry1", perf_file_contents)
                self.assertIn("2345 1a85 py::entry2", perf_file_contents)
            perf_map_state_teardown()
        finally:
            sys.deactivate_stack_trampoline()

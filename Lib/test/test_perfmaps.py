import os
import unittest


class TestPerfMapWriting(unittest.TestCase):
    def test_write_perf_map_entry(self):
        from _testinternalcapi import write_perf_map_entry
        self.assertEqual(write_perf_map_entry(0x1234, 0x5678, "entry1"), 0)
        self.assertEqual(write_perf_map_entry(0x2345, 0x6789, "entry2"), 0)
        with open(f"/tmp/perf-{os.getpid()}.map") as f:
            self.assertEqual(f.read().strip(), "0x1234 5678 entry1\n0x2345 6789 entry2")

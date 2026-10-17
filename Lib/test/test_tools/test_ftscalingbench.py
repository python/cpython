"""Tests for Tools/ftscalingbench/ftscalingbench.py."""

import sys
import unittest
from unittest import mock

from test.test_tools import skip_if_missing, imports_under_tool

skip_if_missing('ftscalingbench')

with imports_under_tool('ftscalingbench'):
    import ftscalingbench


def lscpu(rows):
    """Build `lscpu -p=cpu,node,core,MAXMHZ` output from (cpu, node, core, mhz)."""
    lines = ['# cpu,node,core,MAXMHZ']
    lines += [f'{cpu},{node},{core},{mhz}' for cpu, node, core, mhz in rows]
    return '\n'.join(lines) + '\n'


def smt_rows(count, mhz, first_cpu=0, first_core=0):
    """Rows for `count` cores with two hardware threads each."""
    rows = []
    for i in range(count):
        cpu = first_cpu + i * 2
        rows.append((cpu, 0, first_core + i, mhz))
        rows.append((cpu + 1, 0, first_core + i, mhz))
    return rows


class DetermineAffinityTests(unittest.TestCase):

    def select(self, output):
        with (mock.patch('subprocess.check_output', return_value=output),
              mock.patch.object(sys, 'platform', 'linux')):
            return ftscalingbench.determine_num_threads_and_affinity()

    def test_performance_cores_binned_at_different_clocks(self):
        # Two of the eight performance cores clock higher than the rest.
        rows = smt_rows(4, '5000.0000')
        rows += smt_rows(2, '5200.0000', first_cpu=8, first_core=4)
        rows += smt_rows(2, '5000.0000', first_cpu=12, first_core=6)
        rows += [(16 + i, 0, 8 + i, '3700.0000') for i in range(8)]
        self.assertEqual(self.select(lscpu(rows)),
                         [0, 2, 4, 6, 8, 10, 12, 14])

    def test_efficiency_cores_are_skipped(self):
        rows = smt_rows(4, '4800.0000')
        rows += [(8 + i, 0, 4 + i, '3600.0000') for i in range(4)]
        self.assertEqual(self.select(lscpu(rows)), [0, 2, 4, 6])

    def test_one_thread_per_physical_core(self):
        self.assertEqual(self.select(lscpu(smt_rows(8, '3700.0000'))),
                         [0, 2, 4, 6, 8, 10, 12, 14])

    def test_missing_max_clock(self):
        # MAXMHZ is empty on some kernels and in many virtual machines.
        rows = [(i, 0, i, '') for i in range(4)]
        self.assertEqual(self.select(lscpu(rows)), [0, 1, 2, 3])

    def test_second_numa_node_is_ignored(self):
        rows = [(i, 0, i, '3000.0000') for i in range(4)]
        rows += [(4 + i, 1, 4 + i, '3000.0000') for i in range(4)]
        self.assertEqual(self.select(lscpu(rows)), [0, 1, 2, 3])

    def test_lscpu_missing(self):
        with (mock.patch('subprocess.check_output', side_effect=FileNotFoundError),
              mock.patch.object(sys, 'platform', 'linux')):
            cpus = ftscalingbench.determine_num_threads_and_affinity()
        self.assertTrue(all(cpu is None for cpu in cpus))


if __name__ == '__main__':
    unittest.main()

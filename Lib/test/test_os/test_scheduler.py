"""
Test the scheduler: sched_setaffinity(), sched_yield(), cpu_count(), etc.
"""

import copy
import errno
import os
import pickle
import sys
import unittest
from test import support
from test.support.script_helper import assert_python_ok
from .utils import requires_sched

try:
    import posix
except ImportError:
    import nt as posix


class PosixTester(unittest.TestCase):

    requires_sched_h = unittest.skipUnless(hasattr(posix, 'sched_yield'),
                                           "don't have scheduling support")
    requires_sched_affinity = unittest.skipUnless(hasattr(posix, 'sched_setaffinity'),
                                                  "don't have sched affinity support")

    @requires_sched_h
    def test_sched_yield(self):
        # This has no error conditions (at least on Linux).
        posix.sched_yield()

    @requires_sched_h
    @unittest.skipUnless(hasattr(posix, 'sched_get_priority_max'),
                         "requires sched_get_priority_max()")
    def test_sched_priority(self):
        # Round-robin usually has interesting priorities.
        pol = posix.SCHED_RR
        lo = posix.sched_get_priority_min(pol)
        hi = posix.sched_get_priority_max(pol)
        self.assertIsInstance(lo, int)
        self.assertIsInstance(hi, int)
        self.assertGreaterEqual(hi, lo)
        # Apple platforms return 15 without checking the argument.
        if not support.is_apple:
            self.assertRaises(OSError, posix.sched_get_priority_min, -23)
            self.assertRaises(OSError, posix.sched_get_priority_max, -23)

    @requires_sched
    def test_get_and_set_scheduler_and_param(self):
        possible_schedulers = [sched for name, sched in posix.__dict__.items()
                               if name.startswith("SCHED_")]
        mine = posix.sched_getscheduler(0)
        self.assertIn(mine, possible_schedulers)
        try:
            parent = posix.sched_getscheduler(os.getppid())
        except PermissionError:
            # POSIX specifies EPERM, but Android returns EACCES. Both errno
            # values are mapped to PermissionError.
            pass
        else:
            self.assertIn(parent, possible_schedulers)
        self.assertRaises(OSError, posix.sched_getscheduler, -1)
        self.assertRaises(OSError, posix.sched_getparam, -1)
        param = posix.sched_getparam(0)
        self.assertIsInstance(param.sched_priority, int)

        # POSIX states that calling sched_setparam() or sched_setscheduler() on
        # a process with a scheduling policy other than SCHED_FIFO or SCHED_RR
        # is implementation-defined: NetBSD and FreeBSD can return EINVAL.
        if not sys.platform.startswith(('freebsd', 'netbsd')):
            try:
                posix.sched_setscheduler(0, mine, param)
                posix.sched_setparam(0, param)
            except PermissionError:
                pass
            self.assertRaises(OSError, posix.sched_setparam, -1, param)

        self.assertRaises(OSError, posix.sched_setscheduler, -1, mine, param)
        self.assertRaises(TypeError, posix.sched_setscheduler, 0, mine, None)
        self.assertRaises(TypeError, posix.sched_setparam, 0, 43)
        param = posix.sched_param(None)
        self.assertRaises(TypeError, posix.sched_setparam, 0, param)
        large = 214748364700
        param = posix.sched_param(large)
        self.assertRaises(OverflowError, posix.sched_setparam, 0, param)
        param = posix.sched_param(sched_priority=-large)
        self.assertRaises(OverflowError, posix.sched_setparam, 0, param)

    @requires_sched
    def test_sched_param(self):
        param = posix.sched_param(1)
        for proto in range(pickle.HIGHEST_PROTOCOL+1):
            newparam = pickle.loads(pickle.dumps(param, proto))
            self.assertEqual(newparam, param)
        newparam = copy.copy(param)
        self.assertIsNot(newparam, param)
        self.assertEqual(newparam, param)
        newparam = copy.deepcopy(param)
        self.assertIsNot(newparam, param)
        self.assertEqual(newparam, param)
        newparam = copy.replace(param)
        self.assertIsNot(newparam, param)
        self.assertEqual(newparam, param)
        newparam = copy.replace(param, sched_priority=0)
        self.assertNotEqual(newparam, param)
        self.assertEqual(newparam.sched_priority, 0)

    @unittest.skipUnless(hasattr(posix, "sched_rr_get_interval"), "no function")
    def test_sched_rr_get_interval(self):
        try:
            interval = posix.sched_rr_get_interval(0)
        except OSError as e:
            # This likely means that sched_rr_get_interval is only valid for
            # processes with the SCHED_RR scheduler in effect.
            if e.errno != errno.EINVAL:
                raise
            self.skipTest("only works on SCHED_RR processes")
        self.assertIsInstance(interval, float)
        # Reasonable constraints, I think.
        self.assertGreaterEqual(interval, 0.)
        self.assertLess(interval, 1.)

    @requires_sched_affinity
    def test_sched_getaffinity(self):
        mask = posix.sched_getaffinity(0)
        self.assertIsInstance(mask, set)
        self.assertGreaterEqual(len(mask), 1)
        if not sys.platform.startswith("freebsd"):
            # bpo-47205: does not raise OSError on FreeBSD
            self.assertRaises(OSError, posix.sched_getaffinity, -1)
        for cpu in mask:
            self.assertIsInstance(cpu, int)
            self.assertGreaterEqual(cpu, 0)
            self.assertLess(cpu, 1 << 32)

    @requires_sched_affinity
    def test_sched_setaffinity(self):
        mask = posix.sched_getaffinity(0)
        self.addCleanup(posix.sched_setaffinity, 0, list(mask))

        if len(mask) > 1:
            # Empty masks are forbidden
            mask.pop()
        posix.sched_setaffinity(0, mask)
        self.assertEqual(posix.sched_getaffinity(0), mask)

        try:
            posix.sched_setaffinity(0, [])
            # gh-117061: On RHEL9, sched_setaffinity(0, []) does not fail
        except OSError:
            # sched_setaffinity() manual page documents EINVAL error
            # when the mask is empty.
            pass

        self.assertRaises(ValueError, posix.sched_setaffinity, 0, [-10])
        self.assertRaises(ValueError, posix.sched_setaffinity, 0, map(int, "0X"))
        self.assertRaises(OverflowError, posix.sched_setaffinity, 0, [1<<128])
        if not sys.platform.startswith("freebsd"):
            # bpo-47205: does not raise OSError on FreeBSD
            self.assertRaises(OSError, posix.sched_setaffinity, -1, mask)


class CPUCountTests(unittest.TestCase):
    def check_cpu_count(self, cpus):
        if cpus is None:
            self.skipTest("Could not determine the number of CPUs")

        self.assertIsInstance(cpus, int)
        self.assertGreater(cpus, 0)

    def test_cpu_count(self):
        cpus = os.cpu_count()
        self.check_cpu_count(cpus)

    def test_process_cpu_count(self):
        cpus = os.process_cpu_count()
        self.assertLessEqual(cpus, os.cpu_count())
        self.check_cpu_count(cpus)

    @unittest.skipUnless(hasattr(os, 'sched_setaffinity'),
                         "don't have sched affinity support")
    def test_process_cpu_count_affinity(self):
        affinity1 = os.process_cpu_count()
        if affinity1 is None:
            self.skipTest("Could not determine the number of CPUs")

        # Disable one CPU
        mask = os.sched_getaffinity(0)
        if len(mask) <= 1:
            self.skipTest(f"sched_getaffinity() returns less than "
                          f"2 CPUs: {sorted(mask)}")
        self.addCleanup(os.sched_setaffinity, 0, list(mask))
        mask.pop()
        os.sched_setaffinity(0, mask)

        # test process_cpu_count()
        affinity2 = os.process_cpu_count()
        self.assertEqual(affinity2, affinity1 - 1)


@unittest.skipUnless(hasattr(os, 'getpriority') and hasattr(os, 'setpriority'),
                     "needs os.getpriority and os.setpriority")
class ProgramPriorityTests(unittest.TestCase):
    """Tests for os.getpriority() and os.setpriority()."""

    def test_set_get_priority(self):
        base = os.getpriority(os.PRIO_PROCESS, os.getpid())
        code = f"""if 1:
        import os
        os.setpriority(os.PRIO_PROCESS, os.getpid(), {base} + 1)
        print(os.getpriority(os.PRIO_PROCESS, os.getpid()))
        """

        # Subprocess inherits the current process' priority.
        _, out, _ = assert_python_ok("-c", code)
        new_prio = int(out)
        # nice value cap is 19 for linux and 20 for FreeBSD
        if base >= 19 and new_prio <= base:
            raise unittest.SkipTest("unable to reliably test setpriority "
                                    "at current nice level of %s" % base)
        else:
            self.assertEqual(new_prio, base + 1)


if __name__ == '__main__':
    unittest.main()

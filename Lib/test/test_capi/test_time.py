import time
import unittest
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')


PyTime_MIN = _testcapi.PyTime_MIN
PyTime_MAX = _testcapi.PyTime_MAX
SEC_TO_NS = 10 ** 9
DAY_TO_SEC = (24 * 60 * 60)
# Worst clock resolution: maximum delta between two clock reads.
CLOCK_RES = 0.050


class CAPITest(unittest.TestCase):
    def test_min_max(self):
        # PyTime_t is just int64_t
        self.assertEqual(PyTime_MIN, -2**63)
        self.assertEqual(PyTime_MAX, 2**63 - 1)

    def test_assecondsdouble(self):
        # Test PyTime_AsSecondsDouble()
        def ns_to_sec(ns):
            if abs(ns) % SEC_TO_NS == 0:
                return float(ns // SEC_TO_NS)
            else:
                return float(ns) / SEC_TO_NS

        seconds = (
            0,
            1,
            DAY_TO_SEC,
            365 * DAY_TO_SEC,
        )
        values = {
            PyTime_MIN,
            PyTime_MIN + 1,
            PyTime_MAX - 1,
            PyTime_MAX,
        }
        for second in seconds:
            ns = second * SEC_TO_NS
            values.add(ns)
            # test nanosecond before/after to test rounding
            values.add(ns - 1)
            values.add(ns + 1)
        for ns in list(values):
            if (-ns) > PyTime_MAX:
                continue
            values.add(-ns)
        for ns in sorted(values):
            with self.subTest(ns=ns):
                self.assertEqual(_testcapi.PyTime_AsSecondsDouble(ns),
                                 ns_to_sec(ns))

    def check_clock(self, c_func, py_func):
        t1 = c_func()
        t2 = py_func()
        self.assertAlmostEqual(t1, t2, delta=CLOCK_RES)

    def test_monotonic(self):
        # Test PyTime_Monotonic() and PyTime_MonotonicRaw()
        self.check_clock(_testcapi.PyTime_Monotonic, time.monotonic)
        self.check_clock(_testcapi.PyTime_MonotonicRaw, time.monotonic)

    def test_perf_counter(self):
        # Test PyTime_PerfCounter() and PyTime_PerfCounterRaw()
        self.check_clock(_testcapi.PyTime_PerfCounter, time.perf_counter)
        self.check_clock(_testcapi.PyTime_PerfCounterRaw, time.perf_counter)

    def test_time(self):
        # Test PyTime_Time() and PyTime_TimeRaw()
        self.check_clock(_testcapi.PyTime_Time, time.time)
        self.check_clock(_testcapi.PyTime_TimeRaw, time.time)


if __name__ == "__main__":
    unittest.main()

# Test the internal _wmi module on Windows
# This is used by the platform module, and potentially others

import time
import unittest
from test import support
from test.support import import_helper


# Do this first so test will be skipped if module doesn't exist
_wmi = import_helper.import_module('_wmi', required_on=['win'])


def wmi_exec_query(query):
    # gh-112278: WMI maybe slow response when first call.
    for _ in support.sleeping_retry(support.LONG_TIMEOUT):
        try:
            return _wmi.exec_query(query)
        except BrokenPipeError:
            pass
            # retry on pipe error
        except WindowsError as exc:
            if exc.winerror != 258:
                raise
            # retry on timeout


class WmiTests(unittest.TestCase):
    def test_wmi_query_os_version(self):
        r = wmi_exec_query("SELECT Version FROM Win32_OperatingSystem").split("\0")
        self.assertEqual(1, len(r))
        k, eq, v = r[0].partition("=")
        self.assertEqual("=", eq, r[0])
        self.assertEqual("Version", k, r[0])
        # Best we can check for the version is that it's digits, dot, digits, anything
        # Otherwise, we are likely checking the result of the query against itself
        self.assertRegex(v, r"\d+\.\d+.+$", r[0])

    def test_wmi_query_repeated(self):
        # Repeated queries should not break
        for _ in range(10):
            self.test_wmi_query_os_version()

    def test_wmi_query_error(self):
        # Invalid queries fail with OSError
        try:
            wmi_exec_query("SELECT InvalidColumnName FROM InvalidTableName")
        except OSError as ex:
            if ex.winerror & 0xFFFFFFFF == 0x80041010:
                # This is the expected error code. All others should fail the test
                return
        self.fail("Expected OSError")

    def test_wmi_query_repeated_error(self):
        for _ in range(10):
            self.test_wmi_query_error()

    def test_wmi_query_not_select(self):
        # Queries other than SELECT are blocked to avoid potential exploits
        with self.assertRaises(ValueError):
            wmi_exec_query("not select, just in case someone tries something")

    @support.requires_resource('cpu')
    def test_wmi_query_overflow(self):
        # Ensure very big queries fail
        # Test multiple times to ensure consistency
        for _ in range(2):
            with self.assertRaises(OSError):
                wmi_exec_query("SELECT * FROM CIM_DataFile")

    def test_wmi_query_multiple_rows(self):
        # Multiple instances should have an extra null separator
        r = wmi_exec_query("SELECT ProcessId FROM Win32_Process WHERE ProcessId < 1000")
        self.assertFalse(r.startswith("\0"), r)
        self.assertFalse(r.endswith("\0"), r)
        it = iter(r.split("\0"))
        try:
            while True:
                self.assertRegex(next(it), r"ProcessId=\d+")
                self.assertEqual("", next(it))
        except StopIteration:
            pass

    def test_wmi_query_threads(self):
        from concurrent.futures import ThreadPoolExecutor
        query = "SELECT ProcessId FROM Win32_Process WHERE ProcessId < 1000"
        with ThreadPoolExecutor(4) as pool:
            task = [pool.submit(wmi_exec_query, query) for _ in range(32)]
            for t in task:
                self.assertRegex(t.result(), "ProcessId=")

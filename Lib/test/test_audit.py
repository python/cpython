"""Tests for sys.audit and sys.addaudithook
"""

import subprocess
import sys
import unittest
from test import support
from test.support import import_helper
from test.support import os_helper


if not hasattr(sys, "addaudithook") or not hasattr(sys, "audit"):
    raise unittest.SkipTest("test only relevant when sys.audit is available")

AUDIT_TESTS_PY = support.findfile("audit-tests.py")


class AuditTest(unittest.TestCase):
    maxDiff = None

    @support.requires_subprocess()
    def run_test_in_subprocess(self, *args):
        with subprocess.Popen(
            [sys.executable, "-X utf8", AUDIT_TESTS_PY, *args],
            encoding="utf-8",
            errors="backslashreplace",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.wait()
            return p, p.stdout.read(), p.stderr.read()

    def do_test(self, *args):
        proc, stdout, stderr = self.run_test_in_subprocess(*args)

        sys.stdout.write(stdout)
        sys.stderr.write(stderr)
        if proc.returncode:
            self.fail(stderr)

    def run_python(self, *args, expect_stderr=False):
        events = []
        proc, stdout, stderr = self.run_test_in_subprocess(*args)
        if not expect_stderr or support.verbose:
            sys.stderr.write(stderr)
        return (
            proc.returncode,
            [line.strip().partition(" ") for line in stdout.splitlines()],
            stderr,
        )

    def test_basic(self):
        self.do_test("test_basic")

    def test_block_add_hook(self):
        self.do_test("test_block_add_hook")

    def test_block_add_hook_baseexception(self):
        self.do_test("test_block_add_hook_baseexception")

    def test_marshal(self):
        import_helper.import_module("marshal")

        self.do_test("test_marshal")

    def test_pickle(self):
        import_helper.import_module("pickle")

        self.do_test("test_pickle")

    def test_monkeypatch(self):
        self.do_test("test_monkeypatch")

    def test_open(self):
        self.do_test("test_open", os_helper.TESTFN)

    def test_cantrace(self):
        self.do_test("test_cantrace")

    def test_mmap(self):
        self.do_test("test_mmap")

    def test_excepthook(self):
        returncode, events, stderr = self.run_python("test_excepthook")
        if not returncode:
            self.fail(f"Expected fatal exception\n{stderr}")

        self.assertSequenceEqual(
            [("sys.excepthook", " ", "RuntimeError('fatal-error')")], events
        )

    def test_unraisablehook(self):
        import_helper.import_module("_testcapi")
        returncode, events, stderr = self.run_python("test_unraisablehook")
        if returncode:
            self.fail(stderr)

        self.assertEqual(events[0][0], "sys.unraisablehook")
        self.assertEqual(
            events[0][2],
            "RuntimeError('nonfatal-error') Exception ignored for audit hook test",
        )

    def test_winreg(self):
        import_helper.import_module("winreg")
        returncode, events, stderr = self.run_python("test_winreg")
        if returncode:
            self.fail(stderr)

        self.assertEqual(events[0][0], "winreg.OpenKey")
        self.assertEqual(events[1][0], "winreg.OpenKey/result")
        expected = events[1][2]
        self.assertTrue(expected)
        self.assertSequenceEqual(["winreg.EnumKey", " ", f"{expected} 0"], events[2])
        self.assertSequenceEqual(["winreg.EnumKey", " ", f"{expected} 10000"], events[3])
        self.assertSequenceEqual(["winreg.PyHKEY.Detach", " ", expected], events[4])

    def test_socket(self):
        import_helper.import_module("socket")
        returncode, events, stderr = self.run_python("test_socket")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        self.assertEqual(events[0][0], "socket.gethostname")
        self.assertEqual(events[1][0], "socket.__new__")
        self.assertEqual(events[2][0], "socket.bind")
        self.assertTrue(events[2][2].endswith("('127.0.0.1', 8080)"))

    def test_gc(self):
        returncode, events, stderr = self.run_python("test_gc")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        self.assertEqual(
            [event[0] for event in events],
            ["gc.get_objects", "gc.get_referrers", "gc.get_referents"]
        )


    @support.requires_resource('network')
    def test_http(self):
        import_helper.import_module("http.client")
        returncode, events, stderr = self.run_python("test_http_client")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        self.assertEqual(events[0][0], "http.client.connect")
        self.assertEqual(events[0][2], "www.python.org 80")
        self.assertEqual(events[1][0], "http.client.send")
        if events[1][2] != '[cannot send]':
            self.assertIn('HTTP', events[1][2])


    def test_sqlite3(self):
        sqlite3 = import_helper.import_module("sqlite3")
        returncode, events, stderr = self.run_python("test_sqlite3")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [ev[0] for ev in events]
        expected = ["sqlite3.connect", "sqlite3.connect/handle"] * 2

        if hasattr(sqlite3.Connection, "enable_load_extension"):
            expected += [
                "sqlite3.enable_load_extension",
                "sqlite3.load_extension",
            ]
        self.assertEqual(actual, expected)


    def test_sys_getframe(self):
        returncode, events, stderr = self.run_python("test_sys_getframe")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("sys._getframe", "test_sys_getframe")]

        self.assertEqual(actual, expected)

    def test_sys_getframemodulename(self):
        returncode, events, stderr = self.run_python("test_sys_getframemodulename")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("sys._getframemodulename", "0")]

        self.assertEqual(actual, expected)


    def test_threading(self):
        returncode, events, stderr = self.run_python("test_threading")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [
            ("_thread.start_new_thread", "(<test_func>, (), None)"),
            ("test.test_func", "()"),
            ("_thread.start_joinable_thread", "(<test_func>, 1, None)"),
            ("test.test_func", "()"),
        ]

        self.assertEqual(actual, expected)


    def test_wmi_exec_query(self):
        import_helper.import_module("_wmi")
        returncode, events, stderr = self.run_python("test_wmi_exec_query")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("_wmi.exec_query", "SELECT * FROM Win32_OperatingSystem")]

        self.assertEqual(actual, expected)

    def test_syslog(self):
        syslog = import_helper.import_module("syslog")

        returncode, events, stderr = self.run_python("test_syslog")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print('Events:', *events, sep='\n  ')

        self.assertSequenceEqual(
            events,
            [('syslog.openlog', ' ', f'python 0 {syslog.LOG_USER}'),
            ('syslog.syslog', ' ', f'{syslog.LOG_INFO} test'),
            ('syslog.setlogmask', ' ', f'{syslog.LOG_DEBUG}'),
            ('syslog.closelog', '', ''),
            ('syslog.syslog', ' ', f'{syslog.LOG_INFO} test2'),
            ('syslog.openlog', ' ', f'audit-tests.py 0 {syslog.LOG_USER}'),
            ('syslog.openlog', ' ', f'audit-tests.py {syslog.LOG_NDELAY} {syslog.LOG_LOCAL0}'),
            ('syslog.openlog', ' ', f'None 0 {syslog.LOG_USER}'),
            ('syslog.closelog', '', '')]
        )

    def test_not_in_gc(self):
        returncode, _, stderr = self.run_python("test_not_in_gc")
        if returncode:
            self.fail(stderr)

    def test_time(self):
        returncode, events, stderr = self.run_python("test_time", "print")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')

        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("time.sleep", "0"),
                    ("time.sleep", "0.0625"),
                    ("time.sleep", "-1")]

        self.assertEqual(actual, expected)

    def test_time_fail(self):
        returncode, events, stderr = self.run_python("test_time", "fail",
                                                     expect_stderr=True)
        self.assertNotEqual(returncode, 0)
        self.assertIn('hook failed', stderr.splitlines()[-1])

    def test_sys_monitoring_register_callback(self):
        returncode, events, stderr = self.run_python("test_sys_monitoring_register_callback")
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("sys.monitoring.register_callback", "(None,)")]

        self.assertEqual(actual, expected)

    def test_winapi_createnamedpipe(self):
        winapi = import_helper.import_module("_winapi")

        pipe_name = r"\\.\pipe\LOCAL\test_winapi_createnamed_pipe"
        returncode, events, stderr = self.run_python("test_winapi_createnamedpipe", pipe_name)
        if returncode:
            self.fail(stderr)

        if support.verbose:
            print(*events, sep='\n')
        actual = [(ev[0], ev[2]) for ev in events]
        expected = [("_winapi.CreateNamedPipe", f"({pipe_name!r}, 3, 8)")]

        self.assertEqual(actual, expected)

    def test_assert_unicode(self):
        # See gh-126018
        returncode, _, stderr = self.run_python("test_assert_unicode")
        if returncode:
            self.fail(stderr)


if __name__ == "__main__":
    unittest.main()

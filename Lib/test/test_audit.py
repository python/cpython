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

    @support.requires_subprocess()
    def do_test(self, *args):
        with subprocess.Popen(
            [sys.executable, "-X utf8", AUDIT_TESTS_PY, *args],
            encoding="utf-8",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.wait()
            sys.stdout.writelines(p.stdout)
            sys.stderr.writelines(p.stderr)
            if p.returncode:
                self.fail("".join(p.stderr))

    @support.requires_subprocess()
    def run_python(self, *args):
        events = []
        with subprocess.Popen(
            [sys.executable, "-X utf8", AUDIT_TESTS_PY, *args],
            encoding="utf-8",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.wait()
            sys.stderr.writelines(p.stderr)
            return (
                p.returncode,
                [line.strip().partition(" ") for line in p.stdout],
                "".join(p.stderr),
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

if __name__ == "__main__":
    unittest.main()

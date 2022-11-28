"""sqlite3 CLI tests."""

import sqlite3 as sqlite
import subprocess
import sys
import unittest

from test.support import SHORT_TIMEOUT, requires_subprocess
from test.support.os_helper import TESTFN, unlink


@requires_subprocess()
class CommandLineInterface(unittest.TestCase):

    def _do_test(self, *args, expect_success=True):
        with subprocess.Popen(
            [sys.executable, "-Xutf8", "-m", "sqlite3", *args],
            encoding="utf-8",
            bufsize=0,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as proc:
            proc.wait()
            if expect_success == bool(proc.returncode):
                self.fail("".join(proc.stderr))
            stdout = proc.stdout.read()
            stderr = proc.stderr.read()
            if expect_success:
                self.assertEqual(stderr, "")
            else:
                self.assertEqual(stdout, "")
            return stdout, stderr

    def expect_success(self, *args):
        out, _ = self._do_test(*args)
        return out

    def expect_failure(self, *args):
        _, err = self._do_test(*args, expect_success=False)
        return err

    def test_cli_help(self):
        out = self.expect_success("-h")
        self.assertIn("usage: python -m sqlite3", out)

    def test_cli_version(self):
        out = self.expect_success("-v")
        self.assertIn(sqlite.sqlite_version, out)

    def test_cli_execute_sql(self):
        out = self.expect_success(":memory:", "select 1")
        self.assertIn("(1,)", out)

    def test_cli_execute_too_much_sql(self):
        stderr = self.expect_failure(":memory:", "select 1; select 2")
        err = "ProgrammingError: You can only execute one statement at a time"
        self.assertIn(err, stderr)

    def test_cli_execute_incomplete_sql(self):
        stderr = self.expect_failure(":memory:", "sel")
        self.assertIn("OperationalError (SQLITE_ERROR)", stderr)

    def test_cli_on_disk_db(self):
        self.addCleanup(unlink, TESTFN)
        out = self.expect_success(TESTFN, "create table t(t)")
        self.assertEqual(out, "")
        out = self.expect_success(TESTFN, "select count(t) from t")
        self.assertIn("(0,)", out)


@requires_subprocess()
class InteractiveSession(unittest.TestCase):
    TIMEOUT = SHORT_TIMEOUT / 10.
    MEMORY_DB_MSG = "Connected to a transient in-memory database"
    PS1 = "sqlite> "
    PS2 = "... "

    def start_cli(self, *args):
        return subprocess.Popen(
            [sys.executable, "-Xutf8", "-m", "sqlite3", *args],
            encoding="utf-8",
            bufsize=0,
            stdin=subprocess.PIPE,
            # Note: the banner is printed to stderr, the prompt to stdout.
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def expect_success(self, proc):
        proc.wait()
        if proc.returncode:
            self.fail("".join(proc.stderr))

    def test_interact(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn(self.PS1, out)
            self.expect_success(proc)

    def test_interact_quit(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(input=".quit", timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn(self.PS1, out)
            self.expect_success(proc)

    def test_interact_version(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(input=".version", timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn(sqlite.sqlite_version, out)
            self.expect_success(proc)

    def test_interact_valid_sql(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(input="select 1;",
                                        timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn("(1,)", out)
            self.expect_success(proc)

    def test_interact_valid_multiline_sql(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(input="select 1\n;",
                                        timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn(self.PS2, out)
            self.assertIn("(1,)", out)
            self.expect_success(proc)

    def test_interact_invalid_sql(self):
        with self.start_cli() as proc:
            out, err = proc.communicate(input="sel;", timeout=self.TIMEOUT)
            self.assertIn(self.MEMORY_DB_MSG, err)
            self.assertIn("OperationalError (SQLITE_ERROR)", err)
            self.expect_success(proc)

    def test_interact_on_disk_file(self):
        self.addCleanup(unlink, TESTFN)
        with self.start_cli(TESTFN) as proc:
            out, err = proc.communicate(input="create table t(t);",
                                        timeout=self.TIMEOUT)
            self.assertIn(TESTFN, err)
            self.assertIn(self.PS1, out)
            self.expect_success(proc)
        with self.start_cli(TESTFN, "select count(t) from t") as proc:
            out = proc.stdout.read()
            err = proc.stderr.read()
            self.assertIn("(0,)", out)
            self.expect_success(proc)


if __name__ == "__main__":
    unittest.main()

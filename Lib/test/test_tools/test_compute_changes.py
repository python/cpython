"""Tests to cover the Tools/build/compute-changes.py script."""

import importlib
import os
import unittest
from pathlib import Path
from unittest.mock import patch

from test.test_tools import skip_if_missing, imports_under_tool

skip_if_missing("build")

with patch.dict(os.environ, {"GITHUB_DEFAULT_BRANCH": "main"}):
    with imports_under_tool("build"):
        compute_changes = importlib.import_module("compute-changes")

process_changed_files = compute_changes.process_changed_files
Outputs = compute_changes.Outputs
ANDROID_DIRS = compute_changes.ANDROID_DIRS
IOS_DIRS = compute_changes.IOS_DIRS
MACOS_DIRS = compute_changes.MACOS_DIRS
WASI_DIRS = compute_changes.WASI_DIRS
RUN_TESTS_IGNORE = compute_changes.RUN_TESTS_IGNORE
UNIX_BUILD_SYSTEM_FILE_NAMES = compute_changes.UNIX_BUILD_SYSTEM_FILE_NAMES
LIBRARY_FUZZER_PATHS = compute_changes.LIBRARY_FUZZER_PATHS


class TestProcessChangedFiles(unittest.TestCase):

    def test_windows(self):
        f = {Path(".github/workflows/reusable-windows.yml")}
        result = process_changed_files(f)
        self.assertTrue(result.run_tests)
        self.assertTrue(result.run_windows_tests)

    def test_docs(self):
        for f in (
            ".github/workflows/reusable-docs.yml",
            "Doc/library/datetime.rst",
            "Doc/Makefile",
        ):
            with self.subTest(f=f):
                result = process_changed_files({Path(f)})
                self.assertTrue(result.run_docs)
                self.assertFalse(result.run_tests)

    def test_ci_fuzz_stdlib(self):
        for p in LIBRARY_FUZZER_PATHS:
            with self.subTest(p=p):
                if p.is_dir():
                    f = p / "file"
                elif p.is_file():
                    f = p
                else:
                    continue
                result = process_changed_files({f})
                self.assertTrue(result.run_ci_fuzz_stdlib)

    def test_android(self):
        for d in ANDROID_DIRS:
            with self.subTest(d=d):
                result = process_changed_files({Path(d) / "file"})
                self.assertTrue(result.run_tests)
                self.assertTrue(result.run_android)
                self.assertFalse(result.run_windows_tests)

    def test_ios(self):
        for d in IOS_DIRS:
            with self.subTest(d=d):
                result = process_changed_files({Path(d) / "file"})
                self.assertTrue(result.run_tests)
                self.assertTrue(result.run_ios)
                self.assertFalse(result.run_windows_tests)

    def test_macos(self):
        f = {Path(".github/workflows/reusable-macos.yml")}
        result = process_changed_files(f)
        self.assertTrue(result.run_tests)
        self.assertTrue(result.run_macos)

        for d in MACOS_DIRS:
            with self.subTest(d=d):
                result = process_changed_files({Path(d) / "file"})
                self.assertTrue(result.run_tests)
                self.assertTrue(result.run_macos)
                self.assertFalse(result.run_windows_tests)

    def test_wasi(self):
        f = {Path(".github/workflows/reusable-wasi.yml")}
        result = process_changed_files(f)
        self.assertTrue(result.run_tests)
        self.assertTrue(result.run_wasi)

        for d in WASI_DIRS:
            with self.subTest(d=d):
                result = process_changed_files({d / "file"})
                self.assertTrue(result.run_tests)
                self.assertTrue(result.run_wasi)
                self.assertFalse(result.run_windows_tests)

    def test_unix(self):
        for f in UNIX_BUILD_SYSTEM_FILE_NAMES:
            with self.subTest(f=f):
                result = process_changed_files({f})
                self.assertTrue(result.run_tests)
                self.assertFalse(result.run_windows_tests)

    def test_msi(self):
        for f in (
            ".github/workflows/reusable-windows-msi.yml",
            "Tools/msi/build.bat",
        ):
            with self.subTest(f=f):
                result = process_changed_files({Path(f)})
                self.assertTrue(result.run_windows_msi)

    def test_all_run(self):
        for f in (
            ".github/workflows/some-new-workflow.yml",
            ".github/workflows/build.yml",
        ):
            with self.subTest(f=f):
                result = process_changed_files({Path(f)})
                self.assertTrue(result.run_tests)
                self.assertTrue(result.run_android)
                self.assertTrue(result.run_ios)
                self.assertTrue(result.run_macos)
                self.assertTrue(result.run_ubuntu)
                self.assertTrue(result.run_wasi)

    def test_all_ignored(self):
        for f in RUN_TESTS_IGNORE:
            with self.subTest(f=f):
                self.assertEqual(process_changed_files({Path(f)}), Outputs())

    def test_wasi_and_android(self):
        f = {Path(".github/workflows/reusable-wasi.yml"), Path("Android/file")}
        result = process_changed_files(f)
        self.assertTrue(result.run_tests)
        self.assertTrue(result.run_wasi)


if __name__ == "__main__":
    unittest.main()

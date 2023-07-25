#!/usr/bin/env python3
"""
Check the output of running Sphinx in nit-picky mode (missing references).
"""
import argparse
import csv
import os
import re
import sys
from pathlib import Path

# Exclude these whether they're dirty or clean,
# because they trigger a rebuild of dirty files.
EXCLUDE_FILES = {
    "Doc/whatsnew/changelog.rst",
}

# Subdirectories of Doc/ to exclude.
EXCLUDE_SUBDIRS = {
    ".env",
    ".venv",
    "env",
    "includes",
    "venv",
}

PATTERN = re.compile(r"(?P<file>[^:]+):(?P<line>\d+): WARNING: (?P<msg>.+)")


def check_and_annotate(warnings: list[str], files_to_check: str) -> None:
    """
    Convert Sphinx warning messages to GitHub Actions.

    Converts lines like:
        .../Doc/library/cgi.rst:98: WARNING: reference target not found
    to:
        ::warning file=.../Doc/library/cgi.rst,line=98::reference target not found

    Non-matching lines are echoed unchanged.

    see:
    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#setting-a-warning-message
    """
    files_to_check = next(csv.reader([files_to_check]))
    for warning in warnings:
        if any(filename in warning for filename in files_to_check):
            if match := PATTERN.fullmatch(warning):
                print("::warning file={file},line={line}::{msg}".format_map(match))


def fail_if_regression(
    warnings: list[str], files_with_expected_nits: set[str], files_with_nits: set[str]
) -> int:
    """
    Ensure some files always pass Sphinx nit-picky mode (no missing references).
    These are files which are *not* in .nitignore.
    """
    all_rst = {
        str(rst)
        for rst in Path("Doc/").rglob("*.rst")
        if rst.parts[1] not in EXCLUDE_SUBDIRS
    }
    should_be_clean = all_rst - files_with_expected_nits - EXCLUDE_FILES
    problem_files = sorted(should_be_clean & files_with_nits)
    if problem_files:
        print("\nError: must not contain warnings:\n")
        for filename in problem_files:
            print(filename)
            for warning in warnings:
                if filename in warning:
                    if match := PATTERN.fullmatch(warning):
                        print("  {line}: {msg}".format_map(match))
        return -1
    return 0


def fail_if_improved(
    files_with_expected_nits: set[str], files_with_nits: set[str]
) -> int:
    """
    We may have fixed warnings in some files so that the files are now completely clean.
    Good news! Let's add them to .nitignore to prevent regression.
    """
    files_with_no_nits = files_with_expected_nits - files_with_nits
    if files_with_no_nits:
        print("\nCongratulations! You improved:\n")
        for filename in sorted(files_with_no_nits):
            print(filename)
        print("\nPlease remove from Doc/tools/.nitignore\n")
        return -1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check-and-annotate",
        help="Comma-separated list of files to check, "
        "and annotate those with warnings on GitHub Actions",
    )
    parser.add_argument(
        "--fail-if-regression",
        action="store_true",
        help="Fail if known-good files have warnings",
    )
    parser.add_argument(
        "--fail-if-improved",
        action="store_true",
        help="Fail if new files with no nits are found",
    )
    args = parser.parse_args()
    exit_code = 0

    wrong_directory_msg = "Must run this script from the repo root"
    assert Path("Doc").exists() and Path("Doc").is_dir(), wrong_directory_msg

    with Path("Doc/sphinx-warnings.txt").open() as f:
        warnings = f.read().splitlines()

    cwd = str(Path.cwd()) + os.path.sep
    files_with_nits = {
        warning.removeprefix(cwd).split(":")[0]
        for warning in warnings
        if "Doc/" in warning
    }

    with Path("Doc/tools/.nitignore").open() as clean_files:
        files_with_expected_nits = {
            filename.strip()
            for filename in clean_files
            if filename.strip() and not filename.startswith("#")
        }

    if args.check_and_annotate:
        check_and_annotate(warnings, args.check_and_annotate)

    if args.fail_if_regression:
        exit_code += fail_if_regression(
            warnings, files_with_expected_nits, files_with_nits
        )

    if args.fail_if_improved:
        exit_code += fail_if_improved(files_with_expected_nits, files_with_nits)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

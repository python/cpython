#!/usr/bin/env python3
"""
Check the output of running Sphinx in nit-picky mode (missing references).
"""
from __future__ import annotations

import argparse
import itertools
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import TextIO

# Fail if NEWS nit found before this line number
NEWS_NIT_THRESHOLD = 200

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

# Regex pattern to match the parts of a Sphinx warning
WARNING_PATTERN = re.compile(
    r"(?P<file>([A-Za-z]:[\\/])?[^:]+):(?P<line>\d+): WARNING: (?P<msg>.+)"
)

# Regex pattern to match the line numbers in a Git unified diff
DIFF_PATTERN = re.compile(
    r"^@@ -(?P<linea>\d+)(?:,(?P<removed>\d+))? \+(?P<lineb>\d+)(?:,(?P<added>\d+))? @@",
    flags=re.MULTILINE,
)


def get_diff_files(ref_a: str, ref_b: str, filter_mode: str = "") -> set[Path]:
    """List the files changed between two Git refs, filtered by change type."""
    added_files_result = subprocess.run(
        [
            "git",
            "diff",
            f"--diff-filter={filter_mode}",
            "--name-only",
            f"{ref_a}...{ref_b}",
            "--",
        ],
        stdout=subprocess.PIPE,
        check=True,
        text=True,
        encoding="UTF-8",
    )

    added_files = added_files_result.stdout.strip().split("\n")
    return {Path(file.strip()) for file in added_files if file.strip()}


def get_diff_lines(ref_a: str, ref_b: str, file: Path) -> list[int]:
    """List the lines changed between two Git refs for a specific file."""
    diff_output = subprocess.run(
        [
            "git",
            "diff",
            "--unified=0",
            f"{ref_a}...{ref_b}",
            "--",
            str(file),
        ],
        stdout=subprocess.PIPE,
        check=True,
        text=True,
        encoding="UTF-8",
    )

    # Scrape line offsets + lengths from diff and convert to line numbers
    line_matches = DIFF_PATTERN.finditer(diff_output.stdout)
    # Removed and added line counts are 1 if not printed
    line_match_values = [
        line_match.groupdict(default=1) for line_match in line_matches
    ]
    line_ints = [
        (int(match_value["lineb"]), int(match_value["added"]))
        for match_value in line_match_values
    ]
    line_ranges = [
        range(line_b, line_b + added) for line_b, added in line_ints
    ]
    line_numbers = list(itertools.chain(*line_ranges))

    return line_numbers


def get_para_line_numbers(file_obj: TextIO) -> list[list[int]]:
    """Get the line numbers of text in a file object, grouped by paragraph."""
    paragraphs = []
    prev_line = None
    for lineno, line in enumerate(file_obj):
        lineno = lineno + 1
        if prev_line is None or (line.strip() and not prev_line.strip()):
            paragraph = [lineno - 1]
            paragraphs.append(paragraph)
        paragraph.append(lineno)
        prev_line = line
    return paragraphs


def filter_and_parse_warnings(
    warnings: list[str], files: set[Path]
) -> list[re.Match[str]]:
    """Get the warnings matching passed files and parse them with regex."""
    filtered_warnings = [
        warning
        for warning in warnings
        if any(str(file) in warning for file in files)
    ]
    warning_matches = [
        WARNING_PATTERN.fullmatch(warning.strip())
        for warning in filtered_warnings
    ]
    non_null_matches = [warning for warning in warning_matches if warning]
    return non_null_matches


def filter_warnings_by_diff(
    warnings: list[re.Match[str]], ref_a: str, ref_b: str, file: Path
) -> list[re.Match[str]]:
    """Filter the passed per-file warnings to just those on changed lines."""
    diff_lines = get_diff_lines(ref_a, ref_b, file)
    with file.open(encoding="UTF-8") as file_obj:
        paragraphs = get_para_line_numbers(file_obj)
    touched_paras = [
        para_lines
        for para_lines in paragraphs
        if set(diff_lines) & set(para_lines)
    ]
    touched_para_lines = set(itertools.chain(*touched_paras))
    warnings_infile = [
        warning for warning in warnings if str(file) in warning["file"]
    ]
    warnings_touched = [
        warning
        for warning in warnings_infile
        if int(warning["line"]) in touched_para_lines
    ]
    return warnings_touched


def process_touched_warnings(
    warnings: list[str], ref_a: str, ref_b: str
) -> list[re.Match[str]]:
    """Filter a list of Sphinx warnings to those affecting touched lines."""
    added_files, modified_files = tuple(
        get_diff_files(ref_a, ref_b, filter_mode=mode) for mode in ("A", "M")
    )

    warnings_added = filter_and_parse_warnings(warnings, added_files)
    warnings_modified = filter_and_parse_warnings(warnings, modified_files)

    modified_files_warned = {
        file
        for file in modified_files
        if any(str(file) in warning["file"] for warning in warnings_modified)
    }

    warnings_modified_touched = [
        filter_warnings_by_diff(warnings_modified, ref_a, ref_b, file)
        for file in modified_files_warned
    ]
    warnings_touched = warnings_added + list(
        itertools.chain(*warnings_modified_touched)
    )

    return warnings_touched


def annotate_diff(
    warnings: list[str], ref_a: str = "main", ref_b: str = "HEAD"
) -> None:
    """
    Convert Sphinx warning messages to GitHub Actions for changed paragraphs.

    Converts lines like:
        .../Doc/library/cgi.rst:98: WARNING: reference target not found
    to:
        ::warning file=.../Doc/library/cgi.rst,line=98::reference target not found

    See:
    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#setting-a-warning-message
    """
    warnings_touched = process_touched_warnings(warnings, ref_a, ref_b)
    print("Emitting doc warnings matching modified lines:")
    for warning in warnings_touched:
        print("::warning file={file},line={line}::{msg}".format_map(warning))
        print(warning[0])
    if not warnings_touched:
        print("None")


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
                    if match := WARNING_PATTERN.fullmatch(warning):
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


def fail_if_new_news_nit(warnings: list[str], threshold: int) -> int:
    """
    Ensure no warnings are found in the NEWS file before a given line number.
    """
    news_nits = (
        warning
        for warning in warnings
        if "/build/NEWS:" in warning
    )

    # Nits found before the threshold line
    new_news_nits = [
        nit
        for nit in news_nits
        if int(nit.split(":")[1]) <= threshold
    ]

    if new_news_nits:
        print("\nError: new NEWS nits:\n")
        for warning in new_news_nits:
            print(warning)
        return -1

    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--annotate-diff",
        nargs="*",
        metavar=("BASE_REF", "HEAD_REF"),
        help="Add GitHub Actions annotations on the diff for warnings on "
        "lines changed between the given refs (main and HEAD, by default)",
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
    parser.add_argument(
        "--fail-if-new-news-nit",
        metavar="threshold",
        type=int,
        nargs="?",
        const=NEWS_NIT_THRESHOLD,
        help="Fail if new NEWS nit found before threshold line number",
    )

    args = parser.parse_args(argv)
    if args.annotate_diff is not None and len(args.annotate_diff) > 2:
        parser.error(
            "--annotate-diff takes between 0 and 2 ref args, not "
            f"{len(args.annotate_diff)} {tuple(args.annotate_diff)}"
        )
    exit_code = 0

    wrong_directory_msg = "Must run this script from the repo root"
    assert Path("Doc").exists() and Path("Doc").is_dir(), wrong_directory_msg

    with Path("Doc/sphinx-warnings.txt").open(encoding="UTF-8") as f:
        warnings = f.read().splitlines()

    cwd = str(Path.cwd()) + os.path.sep
    files_with_nits = {
        warning.removeprefix(cwd).split(":")[0]
        for warning in warnings
        if "Doc/" in warning
    }

    with Path("Doc/tools/.nitignore").open(encoding="UTF-8") as clean_files:
        files_with_expected_nits = {
            filename.strip()
            for filename in clean_files
            if filename.strip() and not filename.startswith("#")
        }

    if args.annotate_diff is not None:
        annotate_diff(warnings, *args.annotate_diff)

    if args.fail_if_regression:
        exit_code += fail_if_regression(
            warnings, files_with_expected_nits, files_with_nits
        )

    if args.fail_if_improved:
        exit_code += fail_if_improved(files_with_expected_nits, files_with_nits)

    if args.fail_if_new_news_nit:
        exit_code += fail_if_new_news_nit(warnings, args.fail_if_new_news_nit)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

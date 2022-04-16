#!/usr/bin/env python3.8

import argparse
import ast
import os
import sys
import time
import traceback
import tokenize
from glob import glob, escape
from pathlib import PurePath

from typing import List, Optional, Any, Tuple

sys.path.insert(0, os.getcwd())
from pegen.ast_dump import ast_dump
from pegen.testutil import print_memstats

SUCCESS = "\033[92m"
FAIL = "\033[91m"
ENDC = "\033[0m"

COMPILE = 2
PARSE = 1
NOTREE = 0

argparser = argparse.ArgumentParser(
    prog="test_parse_directory",
    description="Helper program to test directories or files for pegen",
)
argparser.add_argument("-d", "--directory", help="Directory path containing files to test")
argparser.add_argument(
    "-e", "--exclude", action="append", default=[], help="Glob(s) for matching files to exclude"
)
argparser.add_argument(
    "-s", "--short", action="store_true", help="Only show errors, in a more Emacs-friendly format"
)
argparser.add_argument(
    "-v", "--verbose", action="store_true", help="Display detailed errors for failures"
)


def report_status(
    succeeded: bool,
    file: str,
    verbose: bool,
    error: Optional[Exception] = None,
    short: bool = False,
) -> None:
    if short and succeeded:
        return

    if succeeded is True:
        status = "OK"
        COLOR = SUCCESS
    else:
        status = "Fail"
        COLOR = FAIL

    if short:
        lineno = 0
        offset = 0
        if isinstance(error, SyntaxError):
            lineno = error.lineno or 1
            offset = error.offset or 1
            message = error.args[0]
        else:
            message = f"{error.__class__.__name__}: {error}"
        print(f"{file}:{lineno}:{offset}: {message}")
    else:
        print(f"{COLOR}{file:60} {status}{ENDC}")

        if error and verbose:
            print(f"  {str(error.__class__.__name__)}: {error}")


def parse_file(source: str, file: str) -> Tuple[Any, float]:
    t0 = time.time()
    result = ast.parse(source, filename=file)
    t1 = time.time()
    return result, t1 - t0


def generate_time_stats(files, total_seconds) -> None:
    total_files = len(files)
    total_bytes = 0
    total_lines = 0
    for file in files:
        # Count lines and bytes separately
        with open(file, "rb") as f:
            total_lines += sum(1 for _ in f)
            total_bytes += f.tell()

    print(
        f"Checked {total_files:,} files, {total_lines:,} lines,",
        f"{total_bytes:,} bytes in {total_seconds:,.3f} seconds.",
    )
    if total_seconds > 0:
        print(
            f"That's {total_lines / total_seconds :,.0f} lines/sec,",
            f"or {total_bytes / total_seconds :,.0f} bytes/sec.",
        )


def parse_directory(directory: str, verbose: bool, excluded_files: List[str], short: bool) -> int:
    # For a given directory, traverse files and attempt to parse each one
    # - Output success/failure for each file
    errors = 0
    files = []
    total_seconds = 0

    for file in sorted(glob(os.path.join(escape(directory), f"**/*.py"), recursive=True)):
        # Only attempt to parse Python files and files that are not excluded
        if any(PurePath(file).match(pattern) for pattern in excluded_files):
            continue

        with tokenize.open(file) as f:
            source = f.read()

        try:
            result, dt = parse_file(source, file)
            total_seconds += dt
            report_status(succeeded=True, file=file, verbose=verbose, short=short)
        except SyntaxError as error:
            report_status(succeeded=False, file=file, verbose=verbose, error=error, short=short)
            errors += 1
        files.append(file)

    generate_time_stats(files, total_seconds)
    if short:
        print_memstats()

    if errors:
        print(f"Encountered {errors} failures.", file=sys.stderr)
        return 1

    return 0


def main() -> None:
    args = argparser.parse_args()
    directory = args.directory
    verbose = args.verbose
    excluded_files = args.exclude
    short = args.short
    sys.exit(parse_directory(directory, verbose, excluded_files, short))


if __name__ == "__main__":
    main()

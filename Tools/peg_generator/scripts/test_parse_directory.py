#!/usr/bin/env python3.8

import argparse
import ast
import os
import sys
import time
import tokenize
import _peg_parser
from glob import glob, escape
from pathlib import PurePath

from typing import List, Optional, Any, Tuple

sys.path.insert(0, os.getcwd())
from pegen.ast_dump import ast_dump
from pegen.testutil import print_memstats
from scripts import show_parse

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
argparser.add_argument(
    "-t", "--tree", action="count", help="Compare parse tree to official AST", default=0
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


def compare_trees(
    actual_tree: ast.AST, file: str, verbose: bool, include_attributes: bool = False,
) -> int:
    with open(file) as f:
        expected_tree = _peg_parser.parse_string(f.read(), oldparser=True)

    expected_text = ast_dump(expected_tree, include_attributes=include_attributes)
    actual_text = ast_dump(actual_tree, include_attributes=include_attributes)
    if actual_text == expected_text:
        if verbose:
            print("Tree for {file}:")
            print(show_parse.format_tree(actual_tree, include_attributes))
        return 0

    print(f"Diffing ASTs for {file} ...")

    expected = show_parse.format_tree(expected_tree, include_attributes)
    actual = show_parse.format_tree(actual_tree, include_attributes)

    if verbose:
        print("Expected for {file}:")
        print(expected)
        print("Actual for {file}:")
        print(actual)
        print(f"Diff for {file}:")

    diff = show_parse.diff_trees(expected_tree, actual_tree, include_attributes)
    for line in diff:
        print(line)

    return 1


def parse_file(source: str, file: str, mode: int, oldparser: bool) -> Tuple[Any, float]:
    t0 = time.time()
    if mode == COMPILE:
        result = _peg_parser.compile_string(
            source,
            filename=file,
            oldparser=oldparser,
        )
    else:
        result = _peg_parser.parse_string(
            source,
            filename=file,
            oldparser=oldparser,
            ast=(mode == PARSE),
        )
    t1 = time.time()
    return result, t1 - t0


def is_parsing_failure(source: str) -> bool:
    try:
        _peg_parser.parse_string(source, mode="exec", oldparser=True)
    except SyntaxError:
        return False
    return True


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


def parse_directory(
    directory: str,
    verbose: bool,
    excluded_files: List[str],
    tree_arg: int,
    short: bool,
    mode: int,
    oldparser: bool,
) -> int:
    if tree_arg:
        assert mode == PARSE, "Mode should be 1 (parse), when comparing the generated trees"

    if oldparser and tree_arg:
        print("Cannot specify tree argument with the cpython parser.", file=sys.stderr)
        return 1

    # For a given directory, traverse files and attempt to parse each one
    # - Output success/failure for each file
    errors = 0
    files = []
    trees = {}  # Trees to compare (after everything else is done)
    total_seconds = 0

    for file in sorted(glob(os.path.join(escape(directory), f"**/*.py"), recursive=True)):
        # Only attempt to parse Python files and files that are not excluded
        if any(PurePath(file).match(pattern) for pattern in excluded_files):
            continue

        with tokenize.open(file) as f:
            source = f.read()

        try:
            result, dt = parse_file(source, file, mode, oldparser)
            total_seconds += dt
            if tree_arg:
                trees[file] = result
            report_status(succeeded=True, file=file, verbose=verbose, short=short)
        except SyntaxError as error:
            if is_parsing_failure(source):
                print(f"File {file} cannot be parsed by either parser.")
            else:
                report_status(
                    succeeded=False, file=file, verbose=verbose, error=error, short=short
                )
                errors += 1
        files.append(file)

    t1 = time.time()

    generate_time_stats(files, total_seconds)
    if short:
        print_memstats()

    if errors:
        print(f"Encountered {errors} failures.", file=sys.stderr)

    # Compare trees (the dict is empty unless -t is given)
    compare_trees_errors = 0
    for file, tree in trees.items():
        if not short:
            print("Comparing ASTs for", file)
        if compare_trees(tree, file, verbose, tree_arg >= 2) == 1:
            compare_trees_errors += 1

    if errors or compare_trees_errors:
        return 1

    return 0


def main() -> None:
    args = argparser.parse_args()
    directory = args.directory
    verbose = args.verbose
    excluded_files = args.exclude
    tree = args.tree
    short = args.short
    mode = 1 if args.tree else 2
    sys.exit(
        parse_directory(
            directory,
            verbose,
            excluded_files,
            tree,
            short,
            mode,
            oldparser=False,
        )
    )


if __name__ == "__main__":
    main()

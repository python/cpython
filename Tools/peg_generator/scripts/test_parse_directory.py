#!/usr/bin/env python3.8

import argparse
import ast
import os
import sys
import tempfile
import time
import traceback
from glob import glob
from pathlib import PurePath

from typing import List, Optional, Any

sys.path.insert(0, os.getcwd())
from pegen.build import build_parser_and_generator
from pegen.testutil import print_memstats
from scripts import show_parse

SUCCESS = "\033[92m"
FAIL = "\033[91m"
ENDC = "\033[0m"

argparser = argparse.ArgumentParser(
    prog="test_parse_directory",
    description="Helper program to test directories or files for pegen",
)
argparser.add_argument("-d", "--directory", help="Directory path containing files to test")
argparser.add_argument("-g", "--grammar-file", help="Grammar file path")
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
    "--skip-actions", action="store_true", help="Suppress code emission for rule actions",
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
        expected_tree = ast.parse(f.read())

    expected_text = ast.dump(expected_tree, include_attributes=include_attributes)
    actual_text = ast.dump(actual_tree, include_attributes=include_attributes)
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


def parse_directory(
    directory: str,
    grammar_file: str,
    verbose: bool,
    excluded_files: List[str],
    skip_actions: bool,
    tree_arg: int,
    short: bool,
    extension: Any,
    mode: int,
    parser: str,
) -> int:
    if parser == "cpython" and (tree_arg or mode == 0):
        print("Cannot specify tree argument or mode=0 with the cpython parser.", file=sys.stderr)
        return 1

    if not directory:
        print("You must specify a directory of files to test.", file=sys.stderr)
        return 1

    if grammar_file:
        if not os.path.exists(grammar_file):
            print(f"The specified grammar file, {grammar_file}, does not exist.", file=sys.stderr)
            return 1

        try:
            if not extension and parser == "pegen":
                build_parser_and_generator(
                    grammar_file,
                    "peg_extension/parse.c",
                    compile_extension=True,
                    skip_actions=skip_actions,
                )
        except Exception as err:
            print(
                f"{FAIL}The following error occurred when generating the parser. Please check your grammar file.\n{ENDC}",
                file=sys.stderr,
            )
            traceback.print_exception(err.__class__, err, None)

            return 1

    else:
        print("A grammar file was not provided - attempting to use existing file...\n")

    if parser == "pegen":
        try:
            from peg_extension import parse  # type: ignore
        except:
            print(
                "An existing parser was not found. Please run `make` or specify a grammar file with the `-g` flag.",
                file=sys.stderr,
            )
            return 1

    # For a given directory, traverse files and attempt to parse each one
    # - Output success/failure for each file
    errors = 0
    files = []
    trees = {}  # Trees to compare (after everything else is done)

    t0 = time.time()
    for file in sorted(glob(f"{directory}/**/*.py", recursive=True)):
        # Only attempt to parse Python files and files that are not excluded
        should_exclude_file = False
        for pattern in excluded_files:
            if PurePath(file).match(pattern):
                should_exclude_file = True
                break

        if not should_exclude_file:
            try:
                if tree_arg:
                    mode = 1
                if parser == "cpython":
                    with open(file, "r") as f:
                        source = f.read()
                        if mode == 2:
                            compile(source, file, "exec")
                        elif mode == 1:
                            ast.parse(source, file, "exec")
                else:
                    tree = parse.parse_file(file, mode=mode)
                if tree_arg:
                    trees[file] = tree
                if not short:
                    report_status(succeeded=True, file=file, verbose=verbose)
            except Exception as error:
                try:
                    ast.parse(file)
                except Exception:
                    if not short:
                        print(f"File {file} cannot be parsed by either pegen or the ast module.")
                else:
                    report_status(
                        succeeded=False, file=file, verbose=verbose, error=error, short=short
                    )
                    errors += 1
            files.append(file)
    t1 = time.time()

    total_seconds = t1 - t0
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

    if parser == "pegen":
        # Dump memo stats to @data.
        with open("@data", "w") as datafile:
            for i, count in enumerate(parse.get_memo_stats()):
                if count:
                    datafile.write(f"{i:4d} {count:9d}\n")

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
    grammar_file = args.grammar_file
    verbose = args.verbose
    excluded_files = args.exclude
    skip_actions = args.skip_actions
    tree = args.tree
    short = args.short
    sys.exit(
        parse_directory(
            directory,
            grammar_file,
            verbose,
            excluded_files,
            skip_actions,
            tree,
            short,
            None,
            0,
            "pegen",
        )
    )


if __name__ == "__main__":
    main()

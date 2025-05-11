"""
Parses compiler output from Clang or GCC and checks that warnings
exist only in files that are expected to have warnings.
"""

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import NamedTuple


class IgnoreRule(NamedTuple):
    file_path: str
    count: int
    ignore_all: bool = False
    is_directory: bool = False


def parse_warning_ignore_file(file_path: str) -> set[IgnoreRule]:
    """
    Parses the warning ignore file and returns a set of IgnoreRules
    """
    files_with_expected_warnings = set()
    with Path(file_path).open(encoding="UTF-8") as ignore_rules_file:
        files_with_expected_warnings = set()
        for i, line in enumerate(ignore_rules_file):
            line = line.strip()
            if line and not line.startswith("#"):
                line_parts = line.split()
                if len(line_parts) >= 2:
                    file_name = line_parts[0]
                    count = line_parts[1]
                    ignore_all = count == "*"
                    is_directory = file_name.endswith("/")

                    # Directories must have a wildcard count
                    if is_directory and count != "*":
                        print(
                            f"Error parsing ignore file: {file_path} "
                            f"at line: {i}"
                        )
                        print(
                            f"Directory {file_name} must have count set to *"
                        )
                        sys.exit(1)
                    if ignore_all:
                        count = 0

                    files_with_expected_warnings.add(
                        IgnoreRule(
                            file_name, int(count), ignore_all, is_directory
                        )
                    )

    return files_with_expected_warnings


def extract_warnings_from_compiler_output(
    compiler_output: str,
    compiler_output_type: str,
    path_prefix: str = "",
) -> list[dict]:
    """
    Extracts warnings from the compiler output based on compiler
    output type. Removes path prefix from file paths if provided.
    Compatible with GCC and Clang compiler output.
    """
    # Choose pattern and compile regex for particular compiler output
    if compiler_output_type == "gcc":
        regex_pattern = (
            r"(?P<file>.*):(?P<line>\d+):(?P<column>\d+): warning: "
            r"(?P<message>.*?)(?: (?P<option>\[-[^\]]+\]))?$"
        )
    elif compiler_output_type == "clang":
        regex_pattern = (
            r"(?P<file>.*):(?P<line>\d+):(?P<column>\d+): warning: "
            r"(?P<message>.*) (?P<option>\[-[^\]]+\])$"
        )
    compiled_regex = re.compile(regex_pattern)
    compiler_warnings = []
    for i, line in enumerate(compiler_output.splitlines(), start=1):
        if match := compiled_regex.match(line):
            try:
                compiler_warnings.append({
                    "file": match.group("file").removeprefix(path_prefix),
                    "line": match.group("line"),
                    "column": match.group("column"),
                    "message": match.group("message"),
                    "option": match.group("option").lstrip("[").rstrip("]"),
                })
            except AttributeError:
                print(
                    f"Error parsing compiler output. "
                    f"Unable to extract warning on line {i}:\n{line}"
                )
                sys.exit(1)

    return compiler_warnings


def get_warnings_by_file(warnings: list[dict]) -> dict[str, list[dict]]:
    """
    Returns a dictionary where the key is the file and the data is the
    warnings in that file. Does not include duplicate warnings for a
    file from list of provided warnings.
    """
    warnings_by_file = defaultdict(list)
    warnings_added = set()
    for warning in warnings:
        warning_key = (
            f"{warning['file']}-{warning['line']}-"
            f"{warning['column']}-{warning['option']}"
        )
        if warning_key not in warnings_added:
            warnings_added.add(warning_key)
            warnings_by_file[warning["file"]].append(warning)

    return warnings_by_file


def is_file_ignored(
    file_path: str, ignore_rules: set[IgnoreRule]
) -> IgnoreRule | None:
    """Return the IgnoreRule object for the file path.

    Return ``None`` if there is no related rule for that path.
    """
    for rule in ignore_rules:
        if rule.is_directory:
            if file_path.startswith(rule.file_path):
                return rule
        elif file_path == rule.file_path:
            return rule
    return None


def get_unexpected_warnings(
    ignore_rules: set[IgnoreRule],
    files_with_warnings: set[IgnoreRule],
) -> int:
    """
    Returns failure status if warnings discovered in list of warnings
    are associated with a file that is not found in the list of files
    with expected warnings
    """
    unexpected_warnings = {}
    for file in files_with_warnings.keys():
        rule = is_file_ignored(file, ignore_rules)

        if rule:
            if rule.ignore_all:
                continue

            if len(files_with_warnings[file]) > rule.count:
                unexpected_warnings[file] = (
                    files_with_warnings[file],
                    rule.count,
                )
            continue
        elif rule is None:
            # If the file is not in the ignore list, then it is unexpected
            unexpected_warnings[file] = (files_with_warnings[file], 0)

    if unexpected_warnings:
        print("Unexpected warnings:")
        for file in unexpected_warnings:
            print(
                f"{file} expected {unexpected_warnings[file][1]} warnings,"
                f" found {len(unexpected_warnings[file][0])}"
            )
            for warning in unexpected_warnings[file][0]:
                print(warning)

        return 1

    return 0


def get_unexpected_improvements(
    ignore_rules: set[IgnoreRule],
    files_with_warnings: set[IgnoreRule],
) -> int:
    """
    Returns failure status if the number of warnings for a file is greater
    than the expected number of warnings for that file based on the ignore
    rules
    """
    unexpected_improvements = []
    for rule in ignore_rules:
        if (
            not rule.ignore_all
            and rule.file_path not in files_with_warnings.keys()
        ):
            if rule.file_path not in files_with_warnings.keys():
                unexpected_improvements.append((rule.file_path, rule.count, 0))
            elif len(files_with_warnings[rule.file_path]) < rule.count:
                unexpected_improvements.append((
                    rule.file_path,
                    rule.count,
                    len(files_with_warnings[rule.file_path]),
                ))

    if unexpected_improvements:
        print("Unexpected improvements:")
        for file in unexpected_improvements:
            print(f"{file[0]} expected {file[1]} warnings, found {file[2]}")
        return 1

    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-c",
        "--compiler-output-file-path",
        type=str,
        required=True,
        help="Path to the compiler output file",
    )
    parser.add_argument(
        "-i",
        "--warning-ignore-file-path",
        type=str,
        help="Path to the warning ignore file",
    )
    parser.add_argument(
        "-x",
        "--fail-on-regression",
        action="store_true",
        default=False,
        help="Flag to fail if new warnings are found",
    )
    parser.add_argument(
        "-X",
        "--fail-on-improvement",
        action="store_true",
        default=False,
        help="Flag to fail if files that were expected "
        "to have warnings have no warnings",
    )
    parser.add_argument(
        "-t",
        "--compiler-output-type",
        type=str,
        required=True,
        choices=["gcc", "clang"],
        help="Type of compiler output file (GCC or Clang)",
    )
    parser.add_argument(
        "-p",
        "--path-prefix",
        type=str,
        help="Path prefix to remove from the start of file paths"
        " in compiler output",
    )

    args = parser.parse_args(argv)

    exit_code = 0

    # Check that the compiler output file is a valid path
    if not Path(args.compiler_output_file_path).is_file():
        print(
            f"Compiler output file does not exist:"
            f" {args.compiler_output_file_path}"
        )
        return 1

    # Check that a warning ignore file was specified and if so is a valid path
    if not args.warning_ignore_file_path:
        print(
            "Warning ignore file not specified."
            " Continuing without it (no warnings ignored)."
        )
        ignore_rules = set()
    else:
        if not Path(args.warning_ignore_file_path).is_file():
            print(
                f"Warning ignore file does not exist:"
                f" {args.warning_ignore_file_path}"
            )
            return 1
        ignore_rules = parse_warning_ignore_file(args.warning_ignore_file_path)

    with Path(args.compiler_output_file_path).open(encoding="UTF-8") as f:
        compiler_output_file_contents = f.read()

    warnings = extract_warnings_from_compiler_output(
        compiler_output_file_contents,
        args.compiler_output_type,
        args.path_prefix,
    )

    files_with_warnings = get_warnings_by_file(warnings)

    status = get_unexpected_warnings(ignore_rules, files_with_warnings)
    if args.fail_on_regression:
        exit_code |= status

    status = get_unexpected_improvements(ignore_rules, files_with_warnings)
    if args.fail_on_improvement:
        exit_code |= status

    print(
        "For information about this tool and its configuration"
        " visit https://devguide.python.org/development-tools/warnings/"
    )

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

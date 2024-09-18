"""
Parses compiler output from Clang or GCC and checks that warnings
exist only in files that are expected to have warnings.
"""

import argparse
from collections import defaultdict
import re
import sys
from pathlib import Path
from typing import NamedTuple


class FileWarnings(NamedTuple):
    name: str
    count: int


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
                compiler_warnings.append(
                    {
                        "file": match.group("file").removeprefix(path_prefix),
                        "line": match.group("line"),
                        "column": match.group("column"),
                        "message": match.group("message"),
                        "option": match.group("option").lstrip("[").rstrip("]"),
                    }
                )
            except:
                print(f"Error parsing compiler output. Unable to extract warning on line {i}:\n{line}")
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


def get_unexpected_warnings(
    files_with_expected_warnings: set[FileWarnings],
    files_with_warnings: set[FileWarnings],
) -> int:
    """
    Returns failure status if warnings discovered in list of warnings
    are associated with a file that is not found in the list of files
    with expected warnings
    """
    unexpected_warnings = {}
    for file in files_with_warnings.keys():
        found_file_in_ignore_list = False
        for ignore_file in files_with_expected_warnings:
            if file == ignore_file.name:
                if len(files_with_warnings[file]) > ignore_file.count:
                    unexpected_warnings[file] = (files_with_warnings[file], ignore_file.count)
                found_file_in_ignore_list = True
                break
        if not found_file_in_ignore_list:
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
    files_with_expected_warnings: set[FileWarnings],
    files_with_warnings: set[FileWarnings],
) -> int:
    """
    Returns failure status if there are no warnings in the list of warnings
    for a file that is in the list of files with expected warnings
    """
    unexpected_improvements = []
    for file in files_with_expected_warnings:
        if file.name not in files_with_warnings.keys():
            unexpected_improvements.append((file.name, file.count, 0))
        elif len(files_with_warnings[file.name]) < file.count:
            unexpected_improvements.append((file.name, file.count, len(files_with_warnings[file.name])))

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
        files_with_expected_warnings = set()
    else:
        if not Path(args.warning_ignore_file_path).is_file():
            print(
                f"Warning ignore file does not exist:"
                f" {args.warning_ignore_file_path}"
            )
            return 1
        with Path(args.warning_ignore_file_path).open(
            encoding="UTF-8"
        ) as clean_files:
            # Files with expected warnings are stored as a set of tuples
            # where the first element is the file name and the second element
            # is the number of warnings expected in that file
            files_with_expected_warnings = {
                FileWarnings(
                    file.strip().split()[0], int(file.strip().split()[1])
                )
                for file in clean_files
                if file.strip() and not file.startswith("#")
            }

    with Path(args.compiler_output_file_path).open(encoding="UTF-8") as f:
        compiler_output_file_contents = f.read()

    warnings = extract_warnings_from_compiler_output(
        compiler_output_file_contents,
        args.compiler_output_type,
        args.path_prefix
    )

    files_with_warnings = get_warnings_by_file(warnings)

    status = get_unexpected_warnings(
        files_with_expected_warnings, files_with_warnings
    )
    if args.fail_on_regression:
        exit_code |= status

    status = get_unexpected_improvements(
        files_with_expected_warnings, files_with_warnings
    )
    if args.fail_on_improvement:
        exit_code |= status

    print(
        "For information about this tool and its configuration"
        " visit https://devguide.python.org/development-tools/warnings/"
        )

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

"""
Parses compiler output with -fdiagnostics-format=json and checks that warnings
exist only in files that are expected to have warnings.
"""

import argparse
from collections import defaultdict
import json
import re
import sys
from pathlib import Path
from typing import NamedTuple

class FileWarnings(NamedTuple):
    name: str
    count: int


def extract_warnings_from_compiler_output_clang(
    compiler_output: str,
) -> list[dict]:
    """
    Extracts warnings from the compiler output when using clang
    """
    # Regex to find warnings in the compiler output
    clang_warning_regex = re.compile(
        r"(?P<file>.*):(?P<line>\d+):(?P<column>\d+): warning: "
        r"(?P<message>.*) (?P<option>\[-[^\]]+\])$"
    )
    compiler_warnings = []
    for line in compiler_output.splitlines():
        if match := clang_warning_regex.match(line):
            compiler_warnings.append(
                {
                    "file": match.group("file"),
                    "line": match.group("line"),
                    "column": match.group("column"),
                    "message": match.group("message"),
                    "option": match.group("option").lstrip("[").rstrip("]"),
                }
            )

    return compiler_warnings


def extract_warnings_from_compiler_output_json(
    compiler_output: str,
) -> list[dict]:
    """
    Extracts warnings from the compiler output when using
    -fdiagnostics-format=json.

    Compiler output as a whole is not a valid json document,
    but includes many json objects and may include other output
    that is not json.
    """
    # Regex to find json arrays at the top level of the file
    # in the compiler output
    json_arrays = re.findall(r"\[(?:[^[\]]|\[[^]]*])*]", compiler_output)
    compiler_warnings = []
    for array in json_arrays:
        try:
            json_data = json.loads(array)
            json_objects_in_array = [entry for entry in json_data]
            warning_list = [
                entry
                for entry in json_objects_in_array
                if entry.get("kind") == "warning"
            ]
            for warning in warning_list:
                locations = warning["locations"]
                for location in locations:
                    for key in ["caret", "start", "end"]:
                        if key in location:
                            compiler_warnings.append(
                                {
                                    # Remove leading current directory if present
                                    "file": location[key]["file"].lstrip("./"),
                                    "line": location[key]["line"],
                                    "column": location[key]["column"],
                                    "message": warning["message"],
                                    "option": warning["option"],
                                }
                            )
                            # Found a caret, start, or end in location so
                            # break out completely to address next warning
                            break
                    else:
                        continue
                    break

        except json.JSONDecodeError:
            continue  # Skip malformed JSON

    return compiler_warnings


def get_warnings_by_file(warnings: list[dict]) -> dict[str, list[dict]]:
    """
    Returns a dictionary where the key is the file and the data is the warnings
    in that file. Does not include duplicate warnings for a file from list of
    provided warnings.
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
    unexpected_warnings = []
    for file in files_with_warnings.keys():
        found_file_in_ignore_list = False
        for ignore_file in files_with_expected_warnings:
            if file == ignore_file.name:
                if len(files_with_warnings[file]) > ignore_file.count:
                    unexpected_warnings.extend(files_with_warnings[file])
                found_file_in_ignore_list = True
                break
        if not found_file_in_ignore_list:
            unexpected_warnings.extend(files_with_warnings[file])

    if unexpected_warnings:
        print("Unexpected warnings:")
        for warning in unexpected_warnings:
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
            unexpected_improvements.append(file)
        elif len(files_with_warnings[file.name]) < file.count:
                unexpected_improvements.append(file)

    if unexpected_improvements:
        print("Unexpected improvements:")
        for file in unexpected_improvements:
            print(file.name)
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
        choices=["json", "clang"],
        help="Type of compiler output file (json or clang)",
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
                FileWarnings(file.strip().split()[0], int(file.strip().split()[1]))
                for file in clean_files
                if file.strip() and not file.startswith("#")
            }

    with Path(args.compiler_output_file_path).open(encoding="UTF-8") as f:
        compiler_output_file_contents = f.read()

    if args.compiler_output_type == "json":
        warnings = extract_warnings_from_compiler_output_json(
            compiler_output_file_contents
        )
    elif args.compiler_output_type == "clang":
        warnings = extract_warnings_from_compiler_output_clang(
            compiler_output_file_contents
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

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

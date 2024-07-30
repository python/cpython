"""
Parses compiler output with -fdiagnostics-format=json and checks that warnings
exist only in files that are expected to have warnings.
"""
import argparse
import json
import re
import sys
from pathlib import Path


def extract_warnings_from_compiler_output(compiler_output: str) -> list[dict]:
    """
    Extracts warnings from the compiler output when using
    -fdiagnostics-format=json

    Compiler output as a whole is not a valid json document, but includes many
    json objects and may include other output that is not json.
    """

    # Regex to find json arrays at the top level of the file
    # in the compiler output
    json_arrays = re.findall(
        r"\[(?:[^\[\]]|\[(?:[^\[\]]|\[[^\[\]]*\])*\])*\]", compiler_output
    )
    compiler_warnings = []
    for array in json_arrays:
        try:
            json_data = json.loads(array)
            json_objects_in_array = [entry for entry in json_data]
            compiler_warnings.extend(
                [
                    entry
                    for entry in json_objects_in_array
                    if entry.get("kind") == "warning"
                ]
            )
        except json.JSONDecodeError:
            continue  # Skip malformed JSON

    return compiler_warnings


def get_warnings_by_file(warnings: list[dict]) -> dict[str, list[dict]]:
    """
    Returns a dictionary where the key is the file and the data is the warnings
    in that file
    """
    warnings_by_file = {}
    for warning in warnings:
        locations = warning["locations"]
        for location in locations:
            for key in ["caret", "start", "end"]:
                if key in location:
                    file = location[key]["file"]
                    file = file.lstrip(
                        "./"
                    )  # Remove leading current directory if present
                    if file not in warnings_by_file:
                        warnings_by_file[file] = []
                    warnings_by_file[file].append(warning)

    return warnings_by_file


def get_unexpected_warnings(
    warnings: list[dict],
    files_with_expected_warnings: set[str],
    files_with_warnings: set[str],
) -> int:
    """
    Returns failure status if warnings discovered in list of warnings
    are associated with a file that is not found in the list of files
    with expected warnings
    """
    unexpected_warnings = []
    for file in files_with_warnings.keys():
        if file not in files_with_expected_warnings:
            unexpected_warnings.extend(files_with_warnings[file])

    if unexpected_warnings:
        print("Unexpected warnings:")
        for warning in unexpected_warnings:
            print(warning)
        return 1

    return 0


def get_unexpected_improvements(
    warnings: list[dict],
    files_with_expected_warnings: set[str],
    files_with_warnings: set[str],
) -> int:
    """
    Returns failure status if there are no warnings in the list of warnings for
    a file that is in the list of files with expected warnings
    """
    unexpected_improvements = []
    for file in files_with_expected_warnings:
        if file not in files_with_warnings.keys():
            unexpected_improvements.append(file)

    if unexpected_improvements:
        print("Unexpected improvements:")
        for file in unexpected_improvements:
            print(file)
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
        required=True,
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

    args = parser.parse_args(argv)

    exit_code = 0

    # Check that the compiler output file is a valid path
    if not Path(args.compiler_output_file_path).is_file():
        print(
            "Compiler output file does not exist: "
            f"{args.compiler_output_file_path}"
        )
        return 1

    # Check that the warning ignore file is a valid path
    if not Path(args.warning_ignore_file_path).is_file():
        print(
            "Warning ignore file does not exist: "
            f"{args.warning_ignore_file_path}"
        )
        return 1

    with Path(args.compiler_output_file_path).open(encoding="UTF-8") as f:
        compiler_output_file_contents = f.read()

    with Path(args.warning_ignore_file_path).open(
        encoding="UTF-8"
    ) as clean_files:
        files_with_expected_warnings = {
            file.strip()
            for file in clean_files
            if file.strip() and not file.startswith("#")
        }

    warnings = extract_warnings_from_compiler_output(
        compiler_output_file_contents
    )
    files_with_warnings = get_warnings_by_file(warnings)

    status = get_unexpected_warnings(
        warnings, files_with_expected_warnings, files_with_warnings
    )
    if args.fail_on_regression:
        exit_code |= status

    status = get_unexpected_improvements(
        warnings, files_with_expected_warnings, files_with_warnings
    )
    if args.fail_on_improvement:
        exit_code |= status

    return exit_code


if __name__ == "__main__":
    sys.exit(main())

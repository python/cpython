#!/usr/bin/env python3
import sys
from pathlib import Path
import argparse
import json


def extract_json_objects(compiler_output: str) -> list[dict]:

    return json_objects


def extract_warnings_from_compiler_output(compiler_output: str) -> list[dict]:
    """
    Extracts warnings from the compiler output when using -fdiagnostics-format=json

    Compiler output as a whole is not a valid json document, but includes many json
    objects and may include other output that is not json.
    """
    # Extract JSON objects from the raw compiler output
    compiler_output_json_objects = []
    stack = []
    start_index = None
    for index, char in enumerate(compiler_output):
        if char == '[':
            if len(stack) == 0:
                start_index = index  # Start of a new JSON array
            stack.append(char)
        elif char == ']':
            if len(stack) > 0:
                stack.pop()
            if len(stack) == 0 and start_index is not None:
                try:
                    json_data = json.loads(compiler_output[start_index:index+1])
                    compiler_output_json_objects.extend(json_data)
                    start_index = None
                except json.JSONDecodeError:
                    continue  # Skip malformed JSON

    compiler_warnings = [entry for entry in compiler_output_json_objects if entry.get('kind') == 'warning']

    return compiler_warnings



def get_unexpected_warnings(
    warnings: list[dict],
    files_with_expected_warnings: set[str],
) -> int:
    """
    Fails if there are unexpected warnings
    """
    unexpected_warnings = []
    for warning in warnings:
        locations = warning['locations']
        for location in locations:
            for key in ['caret', 'start', 'end']:
                if key in location:
                    filename = location[key]['file']
                    if filename not in files_with_expected_warnings:
                        unexpected_warnings.append(warning)

    if unexpected_warnings:
        print("Unexpected warnings:")
        for warning in unexpected_warnings:
            print(warning)
        return 1


    return 0


def get_unexpected_improvements(
    warnings: list[dict],
    files_with_expected_warnings: set[str],
) -> int:
    """
    Fails if files that were expected to have warnings have no warnings
    """

    # Create set of files with warnings
    files_with_ewarnings = set()
    for warning in warnings:
        locations = warning['locations']
        for location in locations:
            for key in ['caret', 'start', 'end']:
                if key in location:
                    filename = location[key]['file']
                    files_with_ewarnings.add(filename)



    unexpected_improvements = []
    for filename in files_with_expected_warnings:
        if filename not in files_with_ewarnings:
            unexpected_improvements.append(filename)

    if unexpected_improvements:
        print("Unexpected improvements:")
        for filename in unexpected_improvements:
            print(filename)
        return 1


    return 0






def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--compiler-output-file-path",
        type=str,
        required=True,
        help="Path to the file"
    )
    parser.add_argument(
        "--warning-ignore-file-path",
        type=str,
        required=True,
        help="Path to the warning ignore file"
    )
    

    args = parser.parse_args(argv)

    exit_code = 0

    # Check that the compiler output file is a valid path
    if not Path(args.compiler_output_file_path).is_file():
        print(f"Compiler output file does not exist: {args.compiler_output_file_path}")
        return 1
    
    # Check that the warning ignore file is a valid path
    if not Path(args.warning_ignore_file_path).is_file():
        print(f"Warning ignore file does not exist: {args.warning_ignore_file_path}")
        return 1
    
    with Path(args.compiler_output_file_path).open(encoding="UTF-8") as f:
        compiler_output_file_contents = f.read()

    with Path(args.warning_ignore_file_path).open(encoding="UTF-8") as clean_files:
        files_with_expected_warnings = {
            filename.strip()
            for filename in clean_files
            if filename.strip() and not filename.startswith("#")
        }


    if len(compiler_output_file_contents) > 0:
        print("Successfully got compiler output")
    else:
        exit_code = 1

    if len(files_with_expected_warnings) > 0:
        print("we have some exceptions")


    warnings = extract_warnings_from_compiler_output(compiler_output_file_contents)

    exit_code = get_unexpected_warnings(warnings, files_with_expected_warnings)

    exit_code = get_unexpected_improvements(warnings, files_with_expected_warnings)

    


    return exit_code








if __name__ == "__main__":
    sys.exit(main())
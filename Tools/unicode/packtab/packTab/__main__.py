# Copyright 2019 Facebook Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from . import *
import argparse
import sys


def main(args=None):
    parser = argparse.ArgumentParser(
        prog="packTab",
        description="Pack a list of integers into compact lookup tables.",
    )
    parser.add_argument(
        "data",
        nargs="*",
        help=(
            "integer data values to pack, or index:value pairs with --sparse "
            "(reads from stdin if not provided)"
        ),
    )
    parser.add_argument(
        "--language",
        choices=["c", "rust"],
        default="c",
        help="output language (default: c)",
    )
    # Keep --rust as a shorthand for --language=rust.
    parser.add_argument(
        "--rust", action="store_true", help="shorthand for --language=rust"
    )
    parser.add_argument(
        "--unsafe",
        action="store_true",
        help="use unsafe array access (Rust only)",
    )
    parser.add_argument(
        "--default",
        type=int,
        default=0,
        help="default value for out-of-range indices (default: 0)",
    )
    parser.add_argument(
        "--compression",
        type=str,
        default="1",
        help=(
            "size vs speed tradeoff: 0 = flat, 1..9 = heuristic, "
            "10 = minimum bytes (default: 1). "
            "For C: use '1,9' to generate both variants with "
            "#ifdef __OPTIMIZE_SIZE__"
        ),
    )
    parser.add_argument(
        "--optimize-size",
        action="store_true",
        help="shortcut for --compression 9 (maximum size optimization)",
    )
    parser.add_argument(
        "--name",
        default="data",
        help="namespace prefix for generated symbols (default: data)",
    )
    parser.add_argument(
        "--analyze",
        action="store_true",
        help="show compression statistics instead of generating code",
    )
    parser.add_argument(
        "--sparse",
        action="store_true",
        help="treat data as sparse: 'index:value' pairs (requires --default)",
    )
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        metavar="FILE",
        help="read data from FILE (default: positional args or stdin)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        metavar="FILE",
        help="write output to FILE instead of stdout",
    )

    parsed = parser.parse_args(args)

    # Read data from input file, positional args, or stdin
    if parsed.input:
        # Read from input file
        with open(parsed.input, "r") as f:
            parsed.data = f.read().strip().split()
        if not parsed.data:
            parser.error(f"no data in input file: {parsed.input}")
    elif not parsed.data:
        # Read from stdin
        stdin_text = sys.stdin.read().strip()
        if not stdin_text:
            parser.error("no data provided (use positional args, -i, or stdin)")
        parsed.data = stdin_text.split()

    # Handle sparse data format
    if parsed.sparse:
        # Parse index:value pairs
        sparse_dict = {}
        try:
            for item in parsed.data:
                if ":" not in item:
                    parser.error(f"--sparse requires 'index:value' format, got: {item}")
                index_str, value_str = item.split(":", 1)
                index = int(index_str)
                value = int(value_str)
                if index < 0:
                    parser.error(f"negative index not allowed: {index}")
                sparse_dict[index] = value
        except ValueError as e:
            parser.error(f"invalid index:value pair: {e}")

        if not sparse_dict:
            parser.error("no sparse data provided")

        # Convert sparse dict to list using pack_table's dict support
        parsed.data = sparse_dict
    else:
        # Parse as regular integers
        try:
            parsed.data = [int(x) for x in parsed.data]
        except ValueError as e:
            parser.error(f"invalid integer in data: {e}")
        if not parsed.data:
            parser.error("no data provided")

    language = "rust" if parsed.rust else parsed.language

    # Handle --optimize-size shortcut
    if parsed.optimize_size:
        parsed.compression = "9"

    # Parse compression value(s)
    compression_values = []
    try:
        for val in parsed.compression.split(","):
            compression_values.append(float(val.strip()))
    except ValueError as e:
        parser.error(f"invalid compression value: {e}")

    # Validate dual-compression (C-only for now)
    if len(compression_values) > 1:
        if len(compression_values) != 2:
            parser.error("compression can have at most 2 values (e.g., '1,9')")
        if language != "c":
            parser.error(
                "dual compression (e.g., '1,9') is only supported for C output"
            )

    if parsed.analyze:
        # Get all Pareto-optimal solutions for analysis
        solutions = pack_table(parsed.data, parsed.default, compression=None)

        # Handle both list and dict input
        if isinstance(parsed.data, dict):
            original_size = max(parsed.data.keys()) + 1 if parsed.data else 0
            values = list(parsed.data.values())
        else:
            original_size = len(parsed.data)
            values = parsed.data

        # Calculate bits needed for the data range
        minV = min(values) if values else 0
        maxV = max(values) if values else 0
        bits_needed = binaryBitsFor(minV, maxV)
        # Round up to next byte boundary for original storage
        original_bytes = original_size * max(1, (bits_needed + 7) // 8)

        print("Compression Analysis")
        print("=" * 70)
        print(f"Original data: {original_size} values, range [{minV}..{maxV}]")
        print(
            f"Original storage: {bits_needed} bits/value, "
            f"{original_bytes} bytes total"
        )
        print(f"Default value: {parsed.default}")
        print()
        print(f"Found {len(solutions)} Pareto-optimal solutions:")
        print()
        print(
            f"{'#':<3} {'Lookups':<8} {'ExtraOps':<9} {'Bytes':<6} "
            f"{'FullCost':<8} {'Ratio':<7} {'Score':<8}"
        )
        print("-" * 70)

        for i, sol in enumerate(solutions):
            ratio = original_bytes / sol.cost if sol.cost > 0 else float("inf")
            score = sol.nLookups + compression_values[0] * (sol.fullCost.bit_length() - 1)
            print(
                f"{i+1:<3} {sol.nLookups:<8} {sol.nExtraOps:<9} {sol.cost:<6} {sol.fullCost:<8} {ratio:>6.2f}x {score:>7.1f}"
            )

        print()
        # Highlight the chosen solution for the current compression parameter
        chosen = pick_solution(solutions, compression_values[0])
        chosen_idx = solutions.index(chosen)
        print(f"Best solution for compression={parsed.compression}: #{chosen_idx+1}")
        print(
            f"  {chosen.nLookups} lookups, {chosen.nExtraOps} extra ops, {chosen.cost} bytes"
        )
        if chosen.cost > 0:
            print(f"  Compression ratio: {original_bytes/chosen.cost:.2f}x")
        else:
            print("  Compression ratio: ∞ (computed inline, no storage)")

        return 0

    # Normal code generation path
    lang = languageClasses[language](unsafe_array_access=parsed.unsafe)

    # Determine output file
    output_file = open(parsed.output, "w") if parsed.output else sys.stdout

    try:
        if len(compression_values) == 1:
            # Single compression - generate normally
            solution = pack_table(parsed.data, parsed.default, compression=compression_values[0])
            code = Code(parsed.name)
            solution.genCode(code, "get", language=lang, private=False)
            code.print_code(language=lang, file=output_file)
        else:
            # Dual compression - generate both with #ifdef
            solution_speed = pack_table(parsed.data, parsed.default, compression=compression_values[0])
            solution_size = pack_table(parsed.data, parsed.default, compression=compression_values[1])

            code_speed = Code(parsed.name)
            solution_speed.genCode(code_speed, "get", language=lang, private=False)

            code_size = Code(parsed.name)
            solution_size.genCode(code_size, "get", language=lang, private=False)

            # Print with #ifdef wrapper
            print("#ifdef __OPTIMIZE_SIZE__", file=output_file)
            print(file=output_file)
            code_size.print_code(language=lang, file=output_file)
            print(file=output_file)
            print("#else  /* optimize for speed */", file=output_file)
            print(file=output_file)
            code_speed.print_code(language=lang, file=output_file)
            print(file=output_file)
            print("#endif", file=output_file)
    finally:
        if parsed.output:
            output_file.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())

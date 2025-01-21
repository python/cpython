"""Generate the main interpreter labels.
Reads the label definitions from bytecodes.c.
Writes the labels to generated_labels.c.h, which is #included in ceval.c.
"""

import argparse

from analyzer import (
    Analysis,
    analyze_files,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Python/generated_labels.c.h"


def generate_labels(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 2, False)
    out.emit("\n")
    for name, label in analysis.labels.items():
        out.emit(f"{name}:\n")
        for tkn in label.body:
            out.emit(tkn)
        out.emit("\n")
        out.emit("\n")
    out.emit("\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter labels.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)


def generate_labels_from_files(
    filenames: list[str], outfilename: str
) -> None:
    data = analyze_files(filenames)
    with open(outfilename, "w") as outfile:
        generate_labels(filenames, data, outfile)


if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_labels(args.input, data, outfile)

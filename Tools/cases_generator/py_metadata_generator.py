"""Generate opcode metadata for Python.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to _opcode_metadata.py by default.
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


DEFAULT_OUTPUT = ROOT / "Lib/_opcode_metadata.py"


def get_specialized(analysis: Analysis) -> set[str]:
    specialized: set[str] = set()
    for family in analysis.families.values():
        for member in family.members:
            specialized.add(member.name)
    return specialized


def generate_specializations(analysis: Analysis, out: CWriter) -> None:
    out.emit("_specializations = {\n")
    for family in analysis.families.values():
        out.emit(f'"{family.name}": [\n')
        for member in family.members:
            out.emit(f'    "{member.name}",\n')
        out.emit("],\n")
    out.emit("}\n\n")


def generate_specialized_opmap(analysis: Analysis, out: CWriter) -> None:
    out.emit("_specialized_opmap = {\n")
    names = []
    for family in analysis.families.values():
        for member in family.members:
            if member.name == family.name:
                continue
            names.append(member.name)
    for name in sorted(names):
        out.emit(f"'{name}': {analysis.opmap[name]},\n")
    out.emit("}\n\n")


def generate_opmap(analysis: Analysis, out: CWriter) -> None:
    specialized = get_specialized(analysis)
    out.emit("opmap = {\n")
    for inst, op in analysis.opmap.items():
        if inst not in specialized:
            out.emit(f"'{inst}': {analysis.opmap[inst]},\n")
    out.emit("}\n\n")


def generate_py_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile, "#")
    out = CWriter(outfile, 0, False)
    generate_specializations(analysis, out)
    generate_specialized_opmap(analysis, out)
    generate_opmap(analysis, out)
    out.emit(f"HAVE_ARGUMENT = {analysis.have_arg}\n")
    out.emit(f"MIN_INSTRUMENTED_OPCODE = {analysis.min_instrumented}\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the Python file with opcode metadata.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_py_metadata(args.input, data, outfile)

"""Generate the main interpreter switch."""

# Version 1 reads a switch statement from a file, writes one to another file.
#
# Version 2 will extract the switch from ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py.

import argparse
import difflib
import dis
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input", type=str, default="Python/ceval.c")
parser.add_argument("-o", "--output", type=str, default="bytecodes.inst")
parser.add_argument("-c", "--compare", action="store_true")
parser.add_argument("-q", "--quiet", action="store_true")


def eopen(filename, mode="r"):
    if filename == "-":
        if "r" in mode:
            return sys.stdin
        else:
            return sys.stdout
    return open(filename, mode)


def leading_whitespace(line):
    return len(line) - len(line.lstrip())


def extract_opcode_name(line):
    m = re.match(r"\A\s*TARGET\((\w+)\)\s*{\s*\Z", line)
    if m:
        opcode_name = m.group(1)
        if opcode_name not in dis._all_opmap:
            raise ValueError(f"error: unknown opcode {opcode_name}")
        return opcode_name
    raise ValueError(f"error: no opcode in {line.strip()}")


def figure_stack_effect(opcode_name):
    # If stack effect is constant, return it as an int.
    # If it is variable or unknown, raise ValueError.
    # (It may be dis.stack_effect() that raises ValueError.)
    opcode = dis._all_opmap[opcode_name]
    if opcode in dis.hasarg:
        try:
            se = dis.stack_effect(opcode, 0)
        except ValueError as err:
            raise ValueError(f"{err} for {opcode_name}")
        for i in range(1, 33):
            if dis.stack_effect(opcode, i) != se:
                raise ValueError(f"{opcode_name} has variable stack effect")
        return se
    else:
        try:
            return dis.stack_effect(opcode)
        except ValueError as err:
            raise ValueError(f"{err} for {opcode_name}")


START_MARKER = "/* Start instructions */"  # The '{' is on the preceding line.
END_MARKER = "/* End regular instructions */"

def read_cases(f):
    cases = []
    case = None
    started = False
    # TODO: Count line numbers
    for line in f:
        stripped = line.strip()
        if not started:
            if stripped == START_MARKER:
                started = True
            continue
        if stripped == END_MARKER:
            break
        if stripped.startswith("TARGET("):
            if case:
                cases.append(case)
            indent = " " * leading_whitespace(line)
            case = ""
            opcode_name = extract_opcode_name(line)
            try:
                se = figure_stack_effect(opcode_name)
            except ValueError as err:
                case += f"{indent}// error: {err}\n"
                case += f"{indent}inst({opcode_name}, ?? -- ??) {{\n"
            else:
                inputs = []
                outputs = []
                if se > 0:
                    for i in range(se):
                        outputs.append(f"__{i}")
                elif se < 0:
                    for i in range(-se):
                        inputs.append(f"__{i}")
                input = ", ".join(inputs)
                output = ", ".join(outputs)
                case += f"{indent}inst({opcode_name}, ({input} -- {output})) {{\n"
        else:
            if case:
                case += line
    if case:
        cases.append(case)
    return cases


def write_cases(f, cases):
    for case in cases:
        print(case.rstrip() + "\n", file=f)


def compare(oldfile, newfile, quiet=False):
    with open(oldfile) as f:
        oldlines = f.readlines()
    for top, line in enumerate(oldlines):
        if line.strip() == START_MARKER:
            break
    else:
        print(f"No start marker found in {oldfile}", file=sys.stderr)
        return
    del oldlines[:top]
    for bottom, line in enumerate(oldlines):
        if line.strip() == END_MARKER:
            break
    else:
        print(f"No end marker found in {oldfile}", file=sys.stderr)
        return
    del oldlines[bottom:]
    if not quiet:
        print(
            f"// {oldfile} has {len(oldlines)} lines after stripping top/bottom",
            file=sys.stderr,
        )
    with open(newfile) as f:
        newlines = f.readlines()
    if not quiet:
        print(f"// {newfile} has {len(newlines)} lines", file=sys.stderr)
    for line in difflib.unified_diff(oldlines, newlines, fromfile=oldfile, tofile=newfile):
        sys.stdout.write(line)


def main():
    args = parser.parse_args()
    input = args.input
    output = args.output
    with eopen(input) as f:
        cases = read_cases(f)
    if not args.quiet:
        print(f"// Read {len(cases)} cases from {input}", file=sys.stderr)
    with eopen(output, "w") as f:
        write_cases(f, cases)
    if not args.quiet:
        print(f"// Wrote {len(cases)} cases to {output}", file=sys.stderr)
    if args.compare:
        compare(input, output, args.quiet)


main()

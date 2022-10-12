"""Generate the main interpreter switch."""

# Version 1 reads a switch statement from a file, writes one to another file.
#
# Version 2 will extract the switch from ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py.

import difflib
import dis
import re
import sys


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
            case = "// <CASE>\n"  # For debugging
            opcode_name = extract_opcode_name(line)
            try:
                se = figure_stack_effect(opcode_name)
            except ValueError as err:
                case += f"// error: {err}\n"
                case += f"inst({opcode_name}) {{\n"
            else:
                if se == 0:
                    case += f"inst({opcode_name}, --) {{\n"
                else:
                    case += f"// stack effect: {se}\n"
                    case += f"inst({opcode_name}) {{\n"
        else:
            if case:
                case += line
    if case:
        cases.append(case)
    return cases


def write_cases(f, cases):
    print("// <HEADER>\n", file=f)
    for case in cases:
        print(case.rstrip() + "\n", file=f)
    print("// <TRAILER>", file=f)


def compare(oldfile, newfile):
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
    print(f"// {oldfile} has {len(oldlines)} lines after stripping top/bottom", file=sys.stderr)
    with open(newfile) as f:
        newlines = f.readlines()
    print(f"// {newfile} has {len(newlines)} lines", file=sys.stderr)
    for line in difflib.unified_diff(oldlines, newlines, fromfile=oldfile, tofile=newfile):
        sys.stdout.write(line)


def main():
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} input output")
    with eopen(sys.argv[1]) as f:
        cases = read_cases(f)
    print("// Collected", len(cases), "cases", file=sys.stderr)
    with eopen(sys.argv[2], "w") as f:
        write_cases(f, cases)
    if sys.argv[1] != "-" and sys.argv[2] != "-" and sys.argv[1] != sys.argv[2]:
        compare(sys.argv[1], sys.argv[2])


main()

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


def extract_opcode(line):
    m = re.match(r"\A\s*TARGET\((\w+)\)\s*{\s*\Z", line)
    if m:
        return m.group(1)
    else:
        return None

START_MARKER = "/* Start instructions */"  # The '{' is on the preceding line.
END_MARKER = "} /* End instructions */"

def read_cases(f):
    cases = []
    case = None
    started = False
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
            case = line
            opc = extract_opcode(line)
            if not opc:
                se_str = f"error: no opcode in {line.strip()}"
            elif opc not in dis._all_opmap:
                se_str = f"error: unknown opcode {opc}"
            else:
                opc_int = dis._all_opmap[opc]
                try:
                    if opc_int in dis.hasarg:
                        se = dis.stack_effect(opc_int, 0)
                    else:
                        se = dis.stack_effect(opc_int)
                except ValueError:
                    se_str = "error: no stack effect"
                else:
                    se_str = f"stack_effect: {se}"
            indent = " " * leading_whitespace(line)
            case += f"{indent}    // {se_str}\n"
        else:
            if case:
                case += line
    if case:
        cases.append(case)
    return cases


def write_cases(f, cases):
    print("// <HEADER>\n", file=f)
    for case in cases:
        print("// <CASE>", file=f)  # This is just for debugging
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

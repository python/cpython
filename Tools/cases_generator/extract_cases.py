"""Extract the main interpreter switch cases."""

# Reads cases from ceval.c, writes to bytecodes.c.
# (This file is not meant to be compiled, but it has a .c extension
# so tooling like VS Code can be fooled into thinking it is C code.
# This helps editing and browsing the code.)
#
# The script generate_cases.py regenerates the cases.

import argparse
import difflib
import dis
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input", type=str, default="Python/ceval.c")
parser.add_argument("-o", "--output", type=str, default="Python/bytecodes.c")
parser.add_argument("-t", "--template", type=str, default="Tools/cases_generator/bytecodes_template.c")
parser.add_argument("-c", "--compare", action="store_true")
parser.add_argument("-q", "--quiet", action="store_true")


inverse_specializations = {
    specname: familyname
    for familyname, specnames in dis._specializations.items()
    for specname in specnames
}


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
    # Return (i, diff``) where i is the stack effect for oparg=0
    # and diff is the increment for oparg=1.
    # If it is irregular or unknown, raise ValueError.
    if m := re.match(f"^(\w+)__(\w+)$", opcode_name):
        # Super-instruction adds effect of both parts
        first, second = m.groups()
        se1, incr1 = figure_stack_effect(first)
        se2, incr2 = figure_stack_effect(second)
        if incr1 or incr2:
            raise ValueError(f"irregular stack effect for {opcode_name}")
        return se1 + se2, 0
    if opcode_name in inverse_specializations:
        # Specialized instruction maps to unspecialized instruction
        opcode_name = inverse_specializations[opcode_name]
    opcode = dis._all_opmap[opcode_name]
    if opcode in dis.hasarg:
        try:
            se = dis.stack_effect(opcode, 0)
        except ValueError as err:
            raise ValueError(f"{err} for {opcode_name}")
        if dis.stack_effect(opcode, 0, jump=True) != se:
            raise ValueError(f"{opcode_name} stack effect depends on jump flag")
        if dis.stack_effect(opcode, 0, jump=False) != se:
            raise ValueError(f"{opcode_name} stack effect depends on jump flag")
        for i in range(1, 257):
            if dis.stack_effect(opcode, i) != se:
                return figure_variable_stack_effect(opcode_name, opcode, se)
    else:
        try:
            se = dis.stack_effect(opcode)
        except ValueError as err:
            raise ValueError(f"{err} for {opcode_name}")
        if dis.stack_effect(opcode, jump=True) != se:
            raise ValueError(f"{opcode_name} stack effect depends on jump flag")
        if dis.stack_effect(opcode, jump=False) != se:
            raise ValueError(f"{opcode_name} stack effect depends on jump flag")
    return se, 0


def figure_variable_stack_effect(opcode_name, opcode, se0):
    # Is it a linear progression?
    se1 = dis.stack_effect(opcode, 1)
    diff = se1 - se0
    for i in range(2, 257):
        sei = dis.stack_effect(opcode, i)
        if sei - se0 != diff * i:
            raise ValueError(f"{opcode_name} has irregular stack effect")
    # Assume it's okay for larger oparg values too
    return se0, diff



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
                se, diff = figure_stack_effect(opcode_name)
            except ValueError as err:
                case += f"{indent}// error: {err}\n"
                case += f"{indent}inst({opcode_name}, (?? -- ??)) {{\n"
            else:
                inputs = []
                outputs = []
                if se > 0:
                    for i in range(se):
                        outputs.append(f"__{i}")
                elif se < 0:
                    for i in range(-se):
                        inputs.append(f"__{i}")
                if diff > 0:
                    if diff == 1:
                        outputs.append(f"__array[oparg]")
                    else:
                        outputs.append(f"__array[oparg*{diff}]")
                elif diff < 0:
                    if diff == -1:
                        inputs.append(f"__array[oparg]")
                    else:
                        inputs.append(f"__array[oparg*{-diff}]")
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
        caselines = case.splitlines()
        while caselines[-1].strip() == "":
            caselines.pop()
        if caselines[-1].strip() == "}":
            caselines.pop()
        else:
            raise ValueError("case does not end with '}'")
        if caselines[-1].strip() == "DISPATCH();":
            caselines.pop()
        caselines.append("        }")
        case = "\n".join(caselines)
        print(case + "\n", file=f)


def write_families(f):
    for opcode, specializations in dis._specializations.items():
        all = [opcode] + specializations
        if len(all) <= 3:
            members = ', '.join([opcode] + specializations)
            print(f"family({opcode.lower()}) = {members};", file=f)
        else:
            print(f"family({opcode.lower()}) =", file=f)
            for i in range(0, len(all), 3):
                members = ', '.join(all[i:i+3])
                if i+4 < len(all):
                    print(f"    {members},", file=f)
                else:
                    print(f"    {members};", file=f)


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
    with eopen(args.input) as f:
        cases = read_cases(f)
    with open(args.template) as f:
        prolog, epilog = f.read().split("// INSERT CASES HERE //", 1)
    if not args.quiet:
        print(f"// Read {len(cases)} cases from {args.input}", file=sys.stderr)
    with eopen(args.output, "w") as f:
        f.write(prolog)
        write_cases(f, cases)
        f.write(epilog)
        write_families(f)
    if not args.quiet:
        print(f"// Wrote {len(cases)} cases to {args.output}", file=sys.stderr)
    if args.compare:
        compare(args.input, args.output, args.quiet)


if __name__ == "__main__":
    main()

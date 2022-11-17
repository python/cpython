"""Generate the main interpreter switch."""

# Write the cases to generated_cases.c.h, which is #included in ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py?

import argparse
import os
import re
import sys
from typing import TextIO, cast

import parser
from parser import InstDef  # TODO: Use parser.InstDef

RE_PREDICTED = r"(?s)(?:PREDICT\(|GO_TO_INSTRUCTION\(|DEOPT_IF\(.*?,\s*)(\w+)\);"

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input", type=str, default="Python/bytecodes.c")
arg_parser.add_argument("-o", "--output", type=str, default="Python/generated_cases.c.h")
arg_parser.add_argument("-q", "--quiet", action="store_true")


def eopen(filename: str, mode: str = "r") -> TextIO:
    if filename == "-":
        if "r" in mode:
            return sys.stdin
        else:
            return sys.stdout
    return cast(TextIO, open(filename, mode))


def parse_cases(
    src: str, filename: str|None = None
) -> tuple[list[InstDef], list[parser.Super], list[parser.Family]]:
    psr = parser.Parser(src, filename=filename)
    instrs: list[InstDef] = []
    supers: list[parser.Super] = []
    families: list[parser.Family] = []
    while not psr.eof():
        if inst := psr.inst_def():
            instrs.append(inst)
        elif sup := psr.super_def():
            supers.append(sup)
        elif fam := psr.family_def():
            families.append(fam)
        else:
            raise psr.make_syntax_error(f"Unexpected token")
    return instrs, supers, families


def always_exits(block: parser.Block) -> bool:
    text = block.text
    lines = text.splitlines()
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines or lines[-1].strip() != "}":
        return False
    lines.pop()
    if not lines:
        return False
    line = lines.pop().rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " "*12:
        return False
    line = line[12:]
    return line.startswith(("goto ", "return ", "DISPATCH", "GO_TO_", "Py_UNREACHABLE()"))


def find_cache_size(instr: InstDef, families: list[parser.Family]) -> str | None:
    for family in families:
        if instr.name == family.members[0]:
            return family.size


def write_instr(
    instr: InstDef, predictions: set[str], indent: str, f: TextIO, dedent: int = 0, cache_size: str | None = None
) -> int:
    # Returns cache offset
    if dedent < 0:
        indent += " " * -dedent
    # Separate stack inputs from cache inputs
    input_names: set[str] = set()
    stack: list[parser.StackEffect] = []
    cache: list[parser.CacheEffect] = []
    for input in instr.inputs:
        if isinstance(input, parser.StackEffect):
            stack.append(input)
            input_names.add(input.name)
        else:
            assert isinstance(input, parser.CacheEffect), input
            cache.append(input)
    outputs = instr.outputs
    cache_offset = 0
    for ceffect in cache:
        if ceffect.name != "unused":
            bits = ceffect.size * 16
            f.write(f"{indent}    PyObject *{ceffect.name} = read{bits}(next_instr + {cache_offset});\n")
        cache_offset += ceffect.size
    if cache_size:
        f.write(f"{indent}    assert({cache_size} == {cache_offset});\n")
    # TODO: Is it better to count forward or backward?
    for i, effect in enumerate(reversed(stack), 1):
        if effect.name != "unused":
            f.write(f"{indent}    PyObject *{effect.name} = PEEK({i});\n")
    for output in instr.outputs:
        if output.name not in input_names and output.name != "unused":
            f.write(f"{indent}    PyObject *{output.name};\n")
    blocklines = instr.block.to_text(dedent=dedent).splitlines(True)
    # Remove blank lines from ends
    while blocklines and not blocklines[0].strip():
        blocklines.pop(0)
    while blocklines and not blocklines[-1].strip():
        blocklines.pop()
    # Remove leading '{' and trailing '}'
    assert blocklines and blocklines[0].strip() == "{"
    assert blocklines and blocklines[-1].strip() == "}"
    blocklines.pop()
    blocklines.pop(0)
    # Remove trailing blank lines
    while blocklines and not blocklines[-1].strip():
        blocklines.pop()
    # Write the body
    ninputs = len(stack)
    for line in blocklines:
        if m := re.match(r"(\s*)ERROR_IF\((.+), (\w+)\);\s*$", line):
            space, cond, label = m.groups()
            # ERROR_IF() must remove the inputs from the stack.
            # The code block is responsible for DECREF()ing them.
            if ninputs:
                f.write(f"{space}if ({cond}) goto pop_{ninputs}_{label};\n")
            else:
                f.write(f"{space}if ({cond}) goto {label};\n")
        else:
            f.write(line)
    if always_exits(instr.block):
        # None of the rest matters
        return cache_offset
    # Stack effect
    noutputs = len(outputs)
    diff = noutputs - ninputs
    if diff > 0:
        f.write(f"{indent}    STACK_GROW({diff});\n")
    elif diff < 0:
        f.write(f"{indent}    STACK_SHRINK({-diff});\n")
    for i, output in enumerate(reversed(outputs), 1):
        if output.name not in input_names and output.name != "unused":
            f.write(f"{indent}    POKE({i}, {output.name});\n")
    # Cache effect
    if cache_offset:
        f.write(f"{indent}    next_instr += {cache_offset};\n")
    return cache_offset


def write_cases(
    f: TextIO, instrs: list[InstDef], supers: list[parser.Super], families: list[parser.Family]
) -> dict[str, tuple[int, int, int]]:
    predictions: set[str] = set()
    for instr in instrs:
        for target in re.findall(RE_PREDICTED, instr.block.text):
            predictions.add(target)
    indent = "        "
    f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
    f.write(f"// Do not edit!\n")
    instr_index: dict[str, InstDef] = {}
    effects_table: dict[str, tuple[int, int, int]] = {}  # name -> (ninputs, noutputs, cache_offset)
    for instr in instrs:
        instr_index[instr.name] = instr
        f.write(f"\n{indent}TARGET({instr.name}) {{\n")
        if instr.name in predictions:
            f.write(f"{indent}    PREDICTED({instr.name});\n")
        cache_offset = write_instr(
            instr, predictions, indent, f,
            cache_size=find_cache_size(instr, families)
        )
        effects_table[instr.name] = len(instr.inputs), len(instr.outputs), cache_offset
        if not always_exits(instr.block):
            f.write(f"{indent}    DISPATCH();\n")
        # Write trailing '}'
        f.write(f"{indent}}}\n")

    for sup in supers:
        components = [instr_index[name] for name in sup.ops]
        f.write(f"\n{indent}TARGET({sup.name}) {{\n")
        for i, instr in enumerate(components):
            if i > 0:
                f.write(f"{indent}    NEXTOPARG();\n")
                f.write(f"{indent}    next_instr++;\n")
            f.write(f"{indent}    {{\n")
            write_instr(instr, predictions, indent, f, dedent=-4)
            f.write(f"    {indent}}}\n")
        f.write(f"{indent}    DISPATCH();\n")
        f.write(f"{indent}}}\n")

    return effects_table


def main():
    args = arg_parser.parse_args()
    with eopen(args.input) as f:
        srclines = f.read().splitlines()
    begin = srclines.index("// BEGIN BYTECODES //")
    end = srclines.index("// END BYTECODES //")
    src = "\n".join(srclines[begin+1 : end])
    instrs, supers, families = parse_cases(src, filename=args.input)
    ninstrs = nsupers = nfamilies = 0
    if not args.quiet:
        ninstrs = len(instrs)
        nsupers = len(supers)
        nfamilies = len(families)
        print(
            f"Read {ninstrs} instructions, {nsupers} supers, "
            f"and {nfamilies} families from {args.input}",
            file=sys.stderr,
        )
    with eopen(args.output, "w") as f:
        effects_table = write_cases(f, instrs, supers, families)
    if not args.quiet:
        print(
            f"Wrote {ninstrs + nsupers} instructions to {args.output}",
            file=sys.stderr,
        )
    # Check that families have consistent effects
    errors = 0
    for family in families:
        head = effects_table[family.members[0]]
        for member in family.members:
            if effects_table[member] != head:
                errors += 1
                print(
                    f"Family {family.name!r} has inconsistent effects (inputs, outputs, cache units):",
                    file=sys.stderr,
                )
                print(
                    f"  {family.members[0]} = {head}; {member} = {effects_table[member]}",
                )
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()

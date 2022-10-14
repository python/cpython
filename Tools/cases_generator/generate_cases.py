"""Generate the main interpreter switch."""

# Write the cases to cases.h, which can be #included in ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py?

import argparse
from dataclasses import dataclass
import dis
import re
import sys

import lexer as clx

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input", type=str, default="Python/bytecodes.inst")
arg_parser.add_argument("-o", "--output", type=str, default="Python/cases.h")
arg_parser.add_argument("-c", "--compare", action="store_true")
arg_parser.add_argument("-q", "--quiet", action="store_true")


class Lexer:
    def __init__(self, filename):
        with eopen(filename) as f:
            self.src = f.read()
        self.all_tokens = list(clx.tokenize(self.src))
        self.tokens = [tkn for tkn in self.all_tokens if tkn.kind != "COMMENT"]
        self.pos = 0

    def getpos(self):
        return self.pos
    
    def reset(self, pos):
        assert 0 <= pos < len(self.tokens), (pos, len(self.tokens))
        self.pos = pos

    def peek(self):
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return None

    def next(self):
        if self.pos < len(self.tokens):
            self.pos += 1
            return self.tokens[self.pos - 1]
        return None
    
    def eof(self):
        return self.pos >= len(self.tokens)

    def backup(self):
        assert self.pos > 0
        self.pos -= 1
    
    def expect(self, kind):
        tkn = self.next()
        if tkn is not None and tkn.kind == kind:
            return tkn
        self.backup()
        return None


@dataclass
class InstHeader:
    name: str
    inputs: list[str]
    outputs: list[str]

class Parser:
    def __init__(self, lexer):
        self.lexer = lexer
        self.getpos = lexer.getpos
        self.reset = lexer.reset
        self.peek = lexer.peek
        self.next = lexer.next
        self.eof = lexer.eof
        self.backup = lexer.backup
        self.expect = lexer.expect
    
    def c_blob(self):
        tokens = []
        level = 0
        while True:
            tkn = self.next()
            if tkn is None:
                break
            if tkn.kind in (clx.LBRACE, clx.LPAREN, clx.LBRACKET):
                level += 1
            elif tkn.kind in (clx.RBRACE, clx.RPAREN, clx.RBRACKET):
                level -= 1
                if level < 0:
                    self.backup()
                    break
            tokens.append(tkn)
        return tokens

    def inst_header(self):
        # inst(NAME, (inputs -- outputs)) {
        # TODO: Error out when there is something unexpected.
        here = self.getpos()
        if (tkn := self.expect(clx.IDENTIFIER)) and tkn.text == "inst":
            if (self.expect(clx.LPAREN)
                    and (tkn := self.expect(clx.IDENTIFIER))):
                name = tkn.text
                if self.expect(clx.COMMA) and self.expect(clx.LPAREN):
                    inp = self.inputs() or []
                    if self.expect(clx.MINUSMINUS):
                        outp = self.outputs() or []
                        if (self.expect(clx.RPAREN)
                                and self.expect(clx.RPAREN)
                                and self.expect(clx.LBRACE)):
                            return InstHeader(name, inp, outp)
        self.reset(here)
        return None

    def inputs(self):
        # input (, input)*
        here = self.getpos()
        if inp := self.input():
            near = self.getpos()
            if self.expect(clx.COMMA):
                if rest := self.input_list():
                    return [inp] + rest
                self.reset(near)
                return [inp]
        self.reset(here)
        return None

    def input(self):
        # IDENTIFIER
        if (tkn := self.expect(clx.IDENTIFIER)):
            return tkn.text
        return None

    def outputs(self):
        # output (, output)*
        here = self.getpos()
        if outp := self.output():
            near = self.getpos()
            if self.expect(clx.COMMA):
                if rest := self.output_list():
                    return [outp] + rest
                self.reset(near)
                return [outp]
        self.reset(here)
        return None

    def output(self):
        # IDENTIFIER
        if (tkn := self.expect(clx.IDENTIFIER)):
            return tkn.text
        return None


def eopen(filename, mode="r"):
    if filename == "-":
        if "r" in mode:
            return sys.stdin
        else:
            return sys.stdout
    return open(filename, mode)


def leading_whitespace(line):
    return len(line) - len(line.lstrip())


@dataclass
class Instruction:
    opcode_name: str
    inputs: list[str]
    outputs: list[str]
    c_code: list[str]  # Excludes outer {}, no trailing \n


@dataclass
class Family:
    family_name: str
    members: list[str]


INST_REGEX = r"""(?x)
^
inst
\s*
\(
    \s*
    (\w+)
    \s*
    ,
    \s*
    \(
        (.*)
        --
        (.*)
    \)
\)
\s*
{
\s*
$
"""

FAMILY_REGEX = r"""(?x)
^
family
\s*
\(
    \s*
    (\w+)
    \s*
\)
\s*
=
(.*)
;
\s*
$
"""

def parse_instrs(f):
    """Parse the DSL file and return a list of (opcode_name, stack_effect) pairs."""
    instrs = []
    # TODO: Count line numbers
    while raw := f.readline():
        indent = leading_whitespace(raw)
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if m := re.match(INST_REGEX, line):
            name, input, output = m.groups()
            inputs = [s.strip() for s in input.split(",")]
            outputs = [s.strip() for s in output.split(",")]
            c_code = []
            while raw := f.readline():
                line = raw.rstrip()
                indent2 = leading_whitespace(line)
                line = line.strip()
                if not indent2 and not line:
                    # Totally blank line
                    c_code.append("")
                    continue
                if indent2 == indent and line == "}":
                    # Correctly placed closing brace
                    break
                if indent2 < indent:
                    raise ValueError(f"Unexpected dedent: {line!r}")
                if raw[:indent] != " "*indent:
                    raise ValueError(f"Bad indent: {line!r}")
                c_code.append(raw[indent:].rstrip())
            instr = Instruction(name, inputs, outputs, c_code)
            instrs.append(instr)
        elif m := re.match(FAMILY_REGEX, line):
            name, memberlist = m.groups()
            members = [mbr.strip() for mbr in memberlist.split("+")]
            family = Family(name, members)
            instrs.append(family)
        else:
            raise ValueError(f"Unexpected line: {line!r}")
    return instrs


def write_cases(f, instrs):
    indent = "        "
    f.write("// This file is generated by Tools/scripts/generate_cases.py\n")
    f.write("// Do not edit!\n")
    for instr in instrs:
        if isinstance(instr, Family):
            continue  # Families are not written
        assert isinstance(instr, Instruction)
        f.write(f"\n{indent}TARGET({instr.opcode_name}) {{\n")
        input = ", ".join(instr.inputs)
        output = ", ".join(instr.outputs)
        # f.write(f"{indent}    // {input} -- {output}\n")
        for line in instr.c_code:
            if line:
                f.write(f"{indent}{line}\n")
            else:
                f.write("\n")
        f.write(f"{indent}}}\n")


def main():
    args = arg_parser.parse_args()
    if 0:
        lxr = Lexer(args.input)
        psr = Parser(lxr)
        inst = psr.inst_header()
        print(inst)
        blob = psr.c_blob()
        print(clx.to_text(blob))
        assert psr.expect(clx.RBRACE)
        return
    with eopen(args.input) as f:
        instrs = parse_instrs(f)
    if not args.quiet:
        ninstrs = sum(isinstance(instr, Instruction) for instr in instrs)
        nfamilies = sum(isinstance(instr, Family) for instr in instrs)
        print(
            f"Read {ninstrs} instructions "
            f"and {nfamilies} families from {args.input}",
            file=sys.stderr,
        )
    with eopen(args.output, "w") as f:
        write_cases(f, instrs)
    if not args.quiet:
        print(
            f"Wrote {ninstrs} instructions to {args.output}",
            file=sys.stderr,
        )


if __name__ == "__main__":
    main()

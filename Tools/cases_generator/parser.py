"""Parser for bytecodes.inst."""

from dataclasses import dataclass, field
from typing import NamedTuple, Callable, TypeVar

import lexer as lx
from plexer import PLexer


P = TypeVar("P", bound="Parser")
N = TypeVar("N", bound="Node")
def contextual(func: Callable[[P], N|None]) -> Callable[[P], N|None]:
    # Decorator to wrap grammar methods.
    # Resets position if `func` returns None.
    def contextual_wrapper(self: P) -> N|None:
        begin = self.getpos()
        res = func(self)
        if res is None:
            self.setpos(begin)
            return
        end = self.getpos()
        res.context = Context(begin, end, self)
        return res
    return contextual_wrapper


class Context(NamedTuple):
    begin: int
    end: int
    owner: PLexer

    def __repr__(self):
        return f"<{self.begin}-{self.end}>"


@dataclass
class Node:
    context: Context|None = field(init=False, default=None)

    @property
    def text(self) -> str:
        return self.to_text()

    def to_text(self, dedent: int = 0) -> str:
        context = self.context
        if not context:
            return ""
        tokens = context.owner.tokens
        begin = context.begin
        end = context.end
        return lx.to_text(tokens[begin:end], dedent)


@dataclass
class Block(Node):
    tokens: list[lx.Token]


@dataclass
class InstHeader(Node):
    name: str
    inputs: list[str]
    outputs: list[str]


@dataclass
class InstDef(Node):
    header: InstHeader
    block: Block

    @property
    def name(self):
        return self.header.name

    @property
    def inputs(self):
        return self.header.inputs

    @property
    def outputs(self):
        return self.header.outputs


@dataclass
class Super(Node):
    name: str
    ops: list[str]


@dataclass
class Family(Node):
    name: str
    members: list[str]


class Parser(PLexer):

    @contextual
    def inst_def(self) -> InstDef | None:
        if header := self.inst_header():
            if block := self.block():
                return InstDef(header, block)
            raise self.make_syntax_error("Expected block")
        return None

    @contextual
    def inst_header(self) -> InstHeader | None:
        # inst(NAME) | inst(NAME, (inputs -- outputs))
        # TODO: Error out when there is something unexpected.
        # TODO: Make INST a keyword in the lexer.``
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "inst":
            if (self.expect(lx.LPAREN)
                    and (tkn := self.expect(lx.IDENTIFIER))):
                name = tkn.text
                if self.expect(lx.COMMA):
                    inp, outp = self.stack_effect()
                    if self.expect(lx.RPAREN):
                        if ((tkn := self.peek())
                                and tkn.kind == lx.LBRACE):
                            self.check_overlaps(inp, outp)
                            return InstHeader(name, inp, outp)
                elif self.expect(lx.RPAREN):
                    return InstHeader(name, [], [])
        return None

    def check_overlaps(self, inp: list[str], outp: list[str]):
        for i, name in enumerate(inp):
            try:
                j = outp.index(name)
            except ValueError:
                continue
            else:
                if i != j:
                    raise self.make_syntax_error(
                        f"Input {name!r} at pos {i} repeated in output at different pos {j}")

    def stack_effect(self) -> tuple[list[str], list[str]]:
        # '(' [inputs] '--' [outputs] ')'
        if self.expect(lx.LPAREN):
            inp = self.inputs() or []
            if self.expect(lx.MINUSMINUS):
                outp = self.outputs() or []
                if self.expect(lx.RPAREN):
                    return inp, outp
        raise self.make_syntax_error("Expected stack effect")

    def inputs(self) -> list[str] | None:
        # input (, input)*
        here = self.getpos()
        if inp := self.input():
            near = self.getpos()
            if self.expect(lx.COMMA):
                if rest := self.inputs():
                    return [inp] + rest
            self.setpos(near)
            return [inp]
        self.setpos(here)
        return None

    def input(self) -> str | None:
        # IDENTIFIER
        if (tkn := self.expect(lx.IDENTIFIER)):
            if self.expect(lx.LBRACKET):
                if arg := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RBRACKET):
                        return f"{tkn.text}[{arg.text}]"
                    if self.expect(lx.TIMES):
                        if num := self.expect(lx.NUMBER):
                            if self.expect(lx.RBRACKET):
                                return f"{tkn.text}[{arg.text}*{num.text}]"
                raise self.make_syntax_error("Expected argument in brackets", tkn)

            return tkn.text
        if self.expect(lx.CONDOP):
            while self.expect(lx.CONDOP):
                pass
            return "??"
        return None

    def outputs(self) -> list[str] | None:
        # output (, output)*
        here = self.getpos()
        if outp := self.output():
            near = self.getpos()
            if self.expect(lx.COMMA):
                if rest := self.outputs():
                    return [outp] + rest
            self.setpos(near)
            return [outp]
        self.setpos(here)
        return None

    def output(self) -> str | None:
        return self.input()  # TODO: They're not quite the same.

    @contextual
    def super_def(self) -> Super | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "super":
            if self.expect(lx.LPAREN):
                if (tkn := self.expect(lx.IDENTIFIER)):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if ops := self.ops():
                                res = Super(tkn.text, ops)
                                return res

    def ops(self) -> list[str] | None:
        if tkn := self.expect(lx.IDENTIFIER):
            ops = [tkn.text]
            while self.expect(lx.PLUS):
                if tkn := self.require(lx.IDENTIFIER):
                    ops.append(tkn.text)
            self.require(lx.SEMI)
            return ops

    @contextual
    def family_def(self) -> Family | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "family":
            if self.expect(lx.LPAREN):
                if (tkn := self.expect(lx.IDENTIFIER)):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if members := self.members():
                                if self.expect(lx.SEMI):
                                    return Family(tkn.text, members)
        return None

    def members(self) -> list[str] | None:
        here = self.getpos()
        if tkn := self.expect(lx.IDENTIFIER):
            near = self.getpos()
            if self.expect(lx.COMMA):
                if rest := self.members():
                    return [tkn.text] + rest
            self.setpos(near)
            return [tkn.text]
        self.setpos(here)
        return None

    @contextual
    def block(self) -> Block:
        tokens = self.c_blob()
        return Block(tokens)

    def c_blob(self) -> list[lx.Token]:
        tokens: list[lx.Token] = []
        level = 0
        while tkn := self.next(raw=True):
            if tkn.kind in (lx.LBRACE, lx.LPAREN, lx.LBRACKET):
                level += 1
            elif tkn.kind in (lx.RBRACE, lx.RPAREN, lx.RBRACKET):
                level -= 1
                if level <= 0:
                    break
            tokens.append(tkn)
        return tokens


if __name__ == "__main__":
    import sys
    if sys.argv[1:]:
        filename = sys.argv[1]
        if filename == "-c" and sys.argv[2:]:
            src = sys.argv[2]
            filename = None
        else:
            with open(filename) as f:
                src = f.read()
            srclines = src.splitlines()
            begin = srclines.index("// BEGIN BYTECODES //")
            end = srclines.index("// END BYTECODES //")
            src = "\n".join(srclines[begin+1 : end])
    else:
        filename = None
        src = "if (x) { x.foo; // comment\n}"
    parser = Parser(src, filename)
    x = parser.inst_def()
    print(x)

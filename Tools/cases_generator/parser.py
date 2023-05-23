"""Parser for bytecodes.inst."""

from dataclasses import dataclass, field
from typing import NamedTuple, Callable, TypeVar, Literal

import lexer as lx
from plexer import PLexer


P = TypeVar("P", bound="Parser")
N = TypeVar("N", bound="Node")


def contextual(func: Callable[[P], N | None]) -> Callable[[P], N | None]:
    # Decorator to wrap grammar methods.
    # Resets position if `func` returns None.
    def contextual_wrapper(self: P) -> N | None:
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
        return f"<{self.owner.filename}: {self.begin}-{self.end}>"


@dataclass
class Node:
    context: Context | None = field(init=False, compare=False, default=None)

    @property
    def text(self) -> str:
        return self.to_text()

    def to_text(self, dedent: int = 0) -> str:
        context = self.context
        if not context:
            return ""
        return lx.to_text(self.tokens, dedent)

    @property
    def tokens(self) -> list[lx.Token]:
        context = self.context
        if not context:
            return []
        tokens = context.owner.tokens
        begin = context.begin
        end = context.end
        return tokens[begin:end]


@dataclass
class Block(Node):
    # This just holds a context which has the list of tokens.
    pass


@dataclass
class StackEffect(Node):
    name: str
    type: str = ""  # Optional `:type`
    cond: str = ""  # Optional `if (cond)`
    size: str = ""  # Optional `[size]`
    # Note: size cannot be combined with type or cond


@dataclass
class Expression(Node):
    size: str


@dataclass
class CacheEffect(Node):
    name: str
    size: int


@dataclass
class OpName(Node):
    name: str


InputEffect = StackEffect | CacheEffect
OutputEffect = StackEffect
UOp = OpName | CacheEffect


@dataclass
class InstHeader(Node):
    override: bool
    register: bool
    kind: Literal["inst", "op", "legacy"]  # Legacy means no (inputs -- outputs)
    name: str
    inputs: list[InputEffect]
    outputs: list[OutputEffect]


@dataclass
class InstDef(Node):
    override: bool
    register: bool
    kind: Literal["inst", "op", "legacy"]
    name: str
    inputs: list[InputEffect]
    outputs: list[OutputEffect]
    block: Block


@dataclass
class Super(Node):
    name: str
    ops: list[OpName]


@dataclass
class Macro(Node):
    name: str
    uops: list[UOp]


@dataclass
class Family(Node):
    name: str
    size: str  # Variable giving the cache size in code units
    members: list[str]


class Parser(PLexer):
    @contextual
    def definition(self) -> InstDef | Super | Macro | Family | None:
        if inst := self.inst_def():
            return inst
        if super := self.super_def():
            return super
        if macro := self.macro_def():
            return macro
        if family := self.family_def():
            return family

    @contextual
    def inst_def(self) -> InstDef | None:
        if hdr := self.inst_header():
            if block := self.block():
                return InstDef(
                    hdr.override, hdr.register, hdr.kind, hdr.name, hdr.inputs, hdr.outputs, block
                )
            raise self.make_syntax_error("Expected block")
        return None

    @contextual
    def inst_header(self) -> InstHeader | None:
        # [override] inst(NAME)
        #   | [override] [register] inst(NAME, (inputs -- outputs))
        #   | [override] [register] op(NAME, (inputs -- outputs))
        # TODO: Make INST a keyword in the lexer.
        override = bool(self.expect(lx.OVERRIDE))
        register = bool(self.expect(lx.REGISTER))
        if (tkn := self.expect(lx.IDENTIFIER)) and (kind := tkn.text) in ("inst", "op"):
            if self.expect(lx.LPAREN) and (tkn := self.expect(lx.IDENTIFIER)):
                name = tkn.text
                if self.expect(lx.COMMA):
                    inp, outp = self.io_effect()
                    if self.expect(lx.RPAREN):
                        if (tkn := self.peek()) and tkn.kind == lx.LBRACE:
                            return InstHeader(override, register, kind, name, inp, outp)
                elif self.expect(lx.RPAREN) and kind == "inst":
                    # No legacy stack effect if kind is "op".
                    return InstHeader(override, register, "legacy", name, [], [])
        return None

    def io_effect(self) -> tuple[list[InputEffect], list[OutputEffect]]:
        # '(' [inputs] '--' [outputs] ')'
        if self.expect(lx.LPAREN):
            inputs = self.inputs() or []
            if self.expect(lx.MINUSMINUS):
                outputs = self.outputs() or []
                if self.expect(lx.RPAREN):
                    return inputs, outputs
        raise self.make_syntax_error("Expected stack effect")

    def inputs(self) -> list[InputEffect] | None:
        # input (',' input)*
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

    @contextual
    def input(self) -> InputEffect | None:
        return self.cache_effect() or self.stack_effect()

    def outputs(self) -> list[OutputEffect] | None:
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

    @contextual
    def output(self) -> OutputEffect | None:
        return self.stack_effect()

    @contextual
    def cache_effect(self) -> CacheEffect | None:
        # IDENTIFIER '/' NUMBER
        if tkn := self.expect(lx.IDENTIFIER):
            if self.expect(lx.DIVIDE):
                num = self.require(lx.NUMBER).text
                try:
                    size = int(num)
                except ValueError:
                    raise self.make_syntax_error(f"Expected integer, got {num!r}")
                else:
                    return CacheEffect(tkn.text, size)

    @contextual
    def stack_effect(self) -> StackEffect | None:
        #   IDENTIFIER [':' IDENTIFIER] ['if' '(' expression ')']
        # | IDENTIFIER '[' expression ']'
        if tkn := self.expect(lx.IDENTIFIER):
            type_text = ""
            if self.expect(lx.COLON):
                type_text = self.require(lx.IDENTIFIER).text.strip()
            cond_text = ""
            if self.expect(lx.IF):
                self.require(lx.LPAREN)
                if not (cond := self.expression()):
                    raise self.make_syntax_error("Expected condition")
                self.require(lx.RPAREN)
                cond_text = cond.text.strip()
            size_text = ""
            if self.expect(lx.LBRACKET):
                if type_text or cond_text:
                    raise self.make_syntax_error("Unexpected [")
                if not (size := self.expression()):
                    raise self.make_syntax_error("Expected expression")
                self.require(lx.RBRACKET)
                type_text = "PyObject **"
                size_text = size.text.strip()
            return StackEffect(tkn.text, type_text, cond_text, size_text)

    @contextual
    def expression(self) -> Expression | None:
        tokens: list[lx.Token] = []
        level = 1
        while tkn := self.peek():
            if tkn.kind in (lx.LBRACKET, lx.LPAREN):
                level += 1
            elif tkn.kind in (lx.RBRACKET, lx.RPAREN):
                level -= 1
                if level == 0:
                    break
            tokens.append(tkn)
            self.next()
        if not tokens:
            return None
        return Expression(lx.to_text(tokens).strip())

    @contextual
    def super_def(self) -> Super | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "super":
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if ops := self.ops():
                                self.require(lx.SEMI)
                                res = Super(tkn.text, ops)
                                return res

    def ops(self) -> list[OpName] | None:
        if op := self.op():
            ops = [op]
            while self.expect(lx.PLUS):
                if op := self.op():
                    ops.append(op)
            return ops

    @contextual
    def op(self) -> OpName | None:
        if tkn := self.expect(lx.IDENTIFIER):
            return OpName(tkn.text)

    @contextual
    def macro_def(self) -> Macro | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "macro":
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if uops := self.uops():
                                self.require(lx.SEMI)
                                res = Macro(tkn.text, uops)
                                return res

    def uops(self) -> list[UOp] | None:
        if uop := self.uop():
            uops = [uop]
            while self.expect(lx.PLUS):
                if uop := self.uop():
                    uops.append(uop)
                else:
                    raise self.make_syntax_error("Expected op name or cache effect")
            return uops

    @contextual
    def uop(self) -> UOp | None:
        if tkn := self.expect(lx.IDENTIFIER):
            if self.expect(lx.DIVIDE):
                if num := self.expect(lx.NUMBER):
                    try:
                        size = int(num.text)
                    except ValueError:
                        raise self.make_syntax_error(
                            f"Expected integer, got {num.text!r}"
                        )
                    else:
                        return CacheEffect(tkn.text, size)
                raise self.make_syntax_error("Expected integer")
            else:
                return OpName(tkn.text)

    @contextual
    def family_def(self) -> Family | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "family":
            size = None
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.COMMA):
                        if not (size := self.expect(lx.IDENTIFIER)):
                            raise self.make_syntax_error("Expected identifier")
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if not self.expect(lx.LBRACE):
                                raise self.make_syntax_error("Expected {")
                            if members := self.members():
                                if self.expect(lx.RBRACE) and self.expect(lx.SEMI):
                                    return Family(
                                        tkn.text, size.text if size else "", members
                                    )
        return None

    def members(self) -> list[str] | None:
        here = self.getpos()
        if tkn := self.expect(lx.IDENTIFIER):
            members = [tkn.text]
            while self.expect(lx.COMMA):
                if tkn := self.expect(lx.IDENTIFIER):
                    members.append(tkn.text)
                else:
                    break
            peek = self.peek()
            if not peek or peek.kind != lx.RBRACE:
                raise self.make_syntax_error("Expected comma or right paren")
            return members
        self.setpos(here)
        return None

    @contextual
    def block(self) -> Block | None:
        if self.c_blob():
            return Block()

    def c_blob(self) -> list[lx.Token]:
        tokens: list[lx.Token] = []
        level = 0
        while tkn := self.next(raw=True):
            tokens.append(tkn)
            if tkn.kind in (lx.LBRACE, lx.LPAREN, lx.LBRACKET):
                level += 1
            elif tkn.kind in (lx.RBRACE, lx.RPAREN, lx.RBRACKET):
                level -= 1
                if level <= 0:
                    break
        return tokens


if __name__ == "__main__":
    import sys

    if sys.argv[1:]:
        filename = sys.argv[1]
        if filename == "-c" and sys.argv[2:]:
            src = sys.argv[2]
            filename = "<string>"
        else:
            with open(filename) as f:
                src = f.read()
            srclines = src.splitlines()
            begin = srclines.index("// BEGIN BYTECODES //")
            end = srclines.index("// END BYTECODES //")
            src = "\n".join(srclines[begin + 1 : end])
    else:
        filename = "<default>"
        src = "if (x) { x.foo; // comment\n}"
    parser = Parser(src, filename)
    x = parser.definition()
    print(x)

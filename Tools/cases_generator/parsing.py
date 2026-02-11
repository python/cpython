"""Parser for bytecodes.inst."""

from dataclasses import dataclass, field
from typing import NamedTuple, Callable, TypeVar, Literal, cast, Iterator
from io import StringIO

import lexer as lx
from plexer import PLexer
from cwriter import CWriter


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
            return None
        end = self.getpos()
        res.context = Context(begin, end, self)
        return res

    return contextual_wrapper


class Context(NamedTuple):
    begin: int
    end: int
    owner: PLexer

    def __repr__(self) -> str:
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

    @property
    def first_token(self) -> lx.Token:
        context = self.context
        assert context is not None
        return context.owner.tokens[context.begin]

# Statements

Visitor = Callable[["Stmt"], None]

class Stmt:

    def __repr__(self) -> str:
        io = StringIO()
        out = CWriter(io, 0, False)
        self.print(out)
        return io.getvalue()

    def print(self, out:CWriter) -> None:
        raise NotImplementedError

    def accept(self, visitor: Visitor) -> None:
        raise NotImplementedError

    def tokens(self) -> Iterator[lx.Token]:
        raise NotImplementedError


@dataclass
class IfStmt(Stmt):
    if_: lx.Token
    condition: list[lx.Token]
    body: Stmt
    else_: lx.Token | None
    else_body: Stmt | None

    def print(self, out:CWriter) -> None:
        out.emit(self.if_)
        for tkn in self.condition:
            out.emit(tkn)
        self.body.print(out)
        if self.else_ is not None:
            out.emit(self.else_)
        self.body.print(out)
        if self.else_body is not None:
            self.else_body.print(out)

    def accept(self, visitor: Visitor) -> None:
        visitor(self)
        self.body.accept(visitor)
        if self.else_body is not None:
            self.else_body.accept(visitor)

    def tokens(self) -> Iterator[lx.Token]:
        yield self.if_
        yield from self.condition
        yield from self.body.tokens()
        if self.else_ is not None:
            yield self.else_
        if self.else_body is not None:
            yield from self.else_body.tokens()


@dataclass
class ForStmt(Stmt):
    for_: lx.Token
    header: list[lx.Token]
    body: Stmt

    def print(self, out:CWriter) -> None:
        out.emit(self.for_)
        for tkn in self.header:
            out.emit(tkn)
        self.body.print(out)

    def accept(self, visitor: Visitor) -> None:
        visitor(self)
        self.body.accept(visitor)

    def tokens(self) -> Iterator[lx.Token]:
        yield self.for_
        yield from self.header
        yield from self.body.tokens()


@dataclass
class WhileStmt(Stmt):
    while_: lx.Token
    condition: list[lx.Token]
    body: Stmt

    def print(self, out:CWriter) -> None:
        out.emit(self.while_)
        for tkn in self.condition:
            out.emit(tkn)
        self.body.print(out)

    def accept(self, visitor: Visitor) -> None:
        visitor(self)
        self.body.accept(visitor)

    def tokens(self) -> Iterator[lx.Token]:
        yield self.while_
        yield from self.condition
        yield from self.body.tokens()


@dataclass
class MacroIfStmt(Stmt):
    condition: lx.Token
    body: list[Stmt]
    else_: lx.Token | None
    else_body: list[Stmt] | None
    endif: lx.Token

    def print(self, out:CWriter) -> None:
        out.emit(self.condition)
        for stmt in self.body:
            stmt.print(out)
        if self.else_body is not None:
            out.emit("#else\n")
            for stmt in self.else_body:
                stmt.print(out)

    def accept(self, visitor: Visitor) -> None:
        visitor(self)
        for stmt in self.body:
            stmt.accept(visitor)
        if self.else_body is not None:
            for stmt in self.else_body:
                stmt.accept(visitor)

    def tokens(self) -> Iterator[lx.Token]:
        yield self.condition
        for stmt in self.body:
            yield from stmt.tokens()
        if self.else_body is not None:
            for stmt in self.else_body:
                yield from stmt.tokens()


@dataclass
class BlockStmt(Stmt):
    open: lx.Token
    body: list[Stmt]
    close: lx.Token

    def print(self, out:CWriter) -> None:
        out.emit(self.open)
        for stmt in self.body:
            stmt.print(out)
        out.start_line()
        out.emit(self.close)

    def accept(self, visitor: Visitor) -> None:
        visitor(self)
        for stmt in self.body:
            stmt.accept(visitor)

    def tokens(self) -> Iterator[lx.Token]:
        yield self.open
        for stmt in self.body:
            yield from stmt.tokens()
        yield self.close


@dataclass
class SimpleStmt(Stmt):
    contents: list[lx.Token]

    def print(self, out:CWriter) -> None:
        for tkn in self.contents:
            out.emit(tkn)

    def tokens(self) -> Iterator[lx.Token]:
        yield from self.contents

    def accept(self, visitor: Visitor) -> None:
        visitor(self)

    __hash__ = object.__hash__

@dataclass
class StackEffect(Node):
    name: str = field(compare=False)  # __eq__ only uses type, cond, size
    size: str = ""  # Optional `[size]`
    # Note: size cannot be combined with type or cond

    def __repr__(self) -> str:
        items = [self.name, self.size]
        while items and items[-1] == "":
            del items[-1]
        return f"StackEffect({', '.join(repr(item) for item in items)})"


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
    annotations: list[str]
    kind: Literal["inst", "op"]
    name: str
    inputs: list[InputEffect]
    outputs: list[OutputEffect]


@dataclass
class InstDef(Node):
    annotations: list[str]
    kind: Literal["inst", "op"]
    name: str
    inputs: list[InputEffect]
    outputs: list[OutputEffect]
    block: BlockStmt


@dataclass
class Macro(Node):
    name: str
    uops: list[UOp]


@dataclass
class Family(Node):
    name: str
    size: str  # Variable giving the cache size in code units
    members: list[str]


@dataclass
class Pseudo(Node):
    name: str
    inputs: list[InputEffect]
    outputs: list[OutputEffect]
    flags: list[str]  # instr flags to set on the pseudo instruction
    targets: list[str]  # opcodes this can be replaced by
    as_sequence: bool

@dataclass
class LabelDef(Node):
    name: str
    spilled: bool
    block: BlockStmt


AstNode = InstDef | Macro | Pseudo | Family | LabelDef


class Parser(PLexer):
    @contextual
    def definition(self) -> AstNode | None:
        if macro := self.macro_def():
            return macro
        if family := self.family_def():
            return family
        if pseudo := self.pseudo_def():
            return pseudo
        if inst := self.inst_def():
            return inst
        if label := self.label_def():
            return label
        return None

    @contextual
    def label_def(self) -> LabelDef | None:
        spilled = False
        if self.expect(lx.SPILLED):
            spilled = True
        if self.expect(lx.LABEL):
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RPAREN):
                        block = self.block()
                        return LabelDef(tkn.text, spilled, block)
        return None

    @contextual
    def inst_def(self) -> InstDef | None:
        if hdr := self.inst_header():
            block = self.block()
            return InstDef(
                hdr.annotations,
                hdr.kind,
                hdr.name,
                hdr.inputs,
                hdr.outputs,
                block,
            )
        return None

    @contextual
    def inst_header(self) -> InstHeader | None:
        # annotation* inst(NAME, (inputs -- outputs))
        # | annotation* op(NAME, (inputs -- outputs))
        annotations = []
        while anno := self.expect(lx.ANNOTATION):
            if anno.text == "replicate":
                self.require(lx.LPAREN)
                stop = self.require(lx.NUMBER)
                start_text = "0"
                if self.expect(lx.COLON):
                    start_text = stop.text
                    stop = self.require(lx.NUMBER)
                self.require(lx.RPAREN)
                annotations.append(f"replicate({start_text}:{stop.text})")
            else:
                annotations.append(anno.text)
        tkn = self.expect(lx.INST)
        if not tkn:
            tkn = self.expect(lx.OP)
        if tkn:
            kind = cast(Literal["inst", "op"], tkn.text)
            if self.expect(lx.LPAREN) and (tkn := self.expect(lx.IDENTIFIER)):
                name = tkn.text
                if self.expect(lx.COMMA):
                    inp, outp = self.io_effect()
                    if self.expect(lx.RPAREN):
                        if (tkn := self.peek()) and tkn.kind == lx.LBRACE:
                            return InstHeader(annotations, kind, name, inp, outp)
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
            inp = cast(InputEffect, inp)
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
        return None

    @contextual
    def stack_effect(self) -> StackEffect | None:
        # IDENTIFIER [':' IDENTIFIER [TIMES]] ['if' '(' expression ')']
        # | IDENTIFIER '[' expression ']'
        if tkn := self.expect(lx.IDENTIFIER):
            size_text = ""
            if self.expect(lx.LBRACKET):
                if not (size := self.expression()):
                    raise self.make_syntax_error("Expected expression")
                self.require(lx.RBRACKET)
                size_text = size.text.strip()
            return StackEffect(tkn.text, size_text)
        return None

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

    # def ops(self) -> list[OpName] | None:
    #     if op := self.op():
    #         ops = [op]
    #         while self.expect(lx.PLUS):
    #             if op := self.op():
    #                 ops.append(op)
    #         return ops

    @contextual
    def op(self) -> OpName | None:
        if tkn := self.expect(lx.IDENTIFIER):
            return OpName(tkn.text)
        return None

    @contextual
    def macro_def(self) -> Macro | None:
        if tkn := self.expect(lx.MACRO):
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if uops := self.uops():
                                self.require(lx.SEMI)
                                res = Macro(tkn.text, uops)
                                return res
        return None

    def uops(self) -> list[UOp] | None:
        if uop := self.uop():
            uop = cast(UOp, uop)
            uops = [uop]
            while self.expect(lx.PLUS):
                if uop := self.uop():
                    uop = cast(UOp, uop)
                    uops.append(uop)
                else:
                    raise self.make_syntax_error("Expected op name or cache effect")
            return uops
        return None

    @contextual
    def uop(self) -> UOp | None:
        if tkn := self.expect(lx.IDENTIFIER):
            if self.expect(lx.DIVIDE):
                sign = 1
                if negate := self.expect(lx.MINUS):
                    sign = -1
                if num := self.expect(lx.NUMBER):
                    try:
                        size = sign * int(num.text)
                    except ValueError:
                        raise self.make_syntax_error(
                            f"Expected integer, got {num.text!r}"
                        )
                    else:
                        return CacheEffect(tkn.text, size)
                raise self.make_syntax_error("Expected integer")
            else:
                return OpName(tkn.text)
        return None

    @contextual
    def family_def(self) -> Family | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "family":
            size = None
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.COMMA):
                        if not (size := self.expect(lx.IDENTIFIER)):
                            if not (size := self.expect(lx.NUMBER)):
                                raise self.make_syntax_error(
                                    "Expected identifier or number"
                                )
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

    def flags(self) -> list[str]:
        here = self.getpos()
        if self.expect(lx.LPAREN):
            if tkn := self.expect(lx.IDENTIFIER):
                flags = [tkn.text]
                while self.expect(lx.COMMA):
                    if tkn := self.expect(lx.IDENTIFIER):
                        flags.append(tkn.text)
                    else:
                        break
                if not self.expect(lx.RPAREN):
                    raise self.make_syntax_error("Expected comma or right paren")
                return flags
        self.setpos(here)
        return []

    @contextual
    def pseudo_def(self) -> Pseudo | None:
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "pseudo":
            size = None
            if self.expect(lx.LPAREN):
                if tkn := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.COMMA):
                        inp, outp = self.io_effect()
                        if self.expect(lx.COMMA):
                            flags = self.flags()
                        else:
                            flags = []
                        if self.expect(lx.RPAREN):
                            if self.expect(lx.EQUALS):
                                if self.expect(lx.LBRACE):
                                    as_sequence = False
                                    closing = lx.RBRACE
                                elif self.expect(lx.LBRACKET):
                                    as_sequence = True
                                    closing = lx.RBRACKET
                                else:
                                    raise self.make_syntax_error("Expected { or [")
                                if members := self.members(allow_sequence=True):
                                    if self.expect(closing) and self.expect(lx.SEMI):
                                        return Pseudo(
                                            tkn.text, inp, outp, flags, members, as_sequence
                                        )
        return None

    def members(self, allow_sequence : bool=False) -> list[str] | None:
        here = self.getpos()
        if tkn := self.expect(lx.IDENTIFIER):
            members = [tkn.text]
            while self.expect(lx.COMMA):
                if tkn := self.expect(lx.IDENTIFIER):
                    members.append(tkn.text)
                else:
                    break
            peek = self.peek()
            kinds = [lx.RBRACE, lx.RBRACKET] if allow_sequence else [lx.RBRACE]
            if not peek or peek.kind not in kinds:
                raise self.make_syntax_error(
                    f"Expected comma or right paren{'/bracket' if allow_sequence else ''}")
            return members
        self.setpos(here)
        return None

    def block(self) -> BlockStmt:
        open = self.require(lx.LBRACE)
        stmts: list[Stmt] = []
        while not (close := self.expect(lx.RBRACE)):
            stmts.append(self.stmt())
        return BlockStmt(open, stmts, close)

    def stmt(self) -> Stmt:
        if tkn := self.expect(lx.IF):
            return self.if_stmt(tkn)
        elif self.expect(lx.LBRACE):
            self.backup()
            return self.block()
        elif tkn := self.expect(lx.FOR):
            return self.for_stmt(tkn)
        elif tkn := self.expect(lx.WHILE):
            return self.while_stmt(tkn)
        elif tkn := self.expect(lx.CMACRO_IF):
            return self.macro_if(tkn)
        elif tkn := self.expect(lx.CMACRO_ELSE):
            msg = "Unexpected #else"
            raise self.make_syntax_error(msg)
        elif tkn := self.expect(lx.CMACRO_ENDIF):
            msg = "Unexpected #endif"
            raise self.make_syntax_error(msg)
        elif tkn := self.expect(lx.CMACRO_OTHER):
            return SimpleStmt([tkn])
        elif tkn := self.expect(lx.SWITCH):
            msg = "switch statements are not supported due to their complex flow control. Sorry."
            raise self.make_syntax_error(msg)
        tokens = self.consume_to(lx.SEMI)
        return SimpleStmt(tokens)

    def if_stmt(self, if_: lx.Token) -> IfStmt:
        lparen = self.require(lx.LPAREN)
        condition = [lparen] + self.consume_to(lx.RPAREN)
        body = self.block()
        else_body: Stmt | None = None
        else_: lx.Token | None = None
        if else_ := self.expect(lx.ELSE):
            if inner := self.expect(lx.IF):
                else_body = self.if_stmt(inner)
            else:
                else_body = self.block()
        return IfStmt(if_, condition, body, else_, else_body)

    def macro_if(self, cond: lx.Token) -> MacroIfStmt:
        else_ = None
        body: list[Stmt] = []
        else_body: list[Stmt] | None = None
        part = body
        while True:
            if tkn := self.expect(lx.CMACRO_ENDIF):
                return MacroIfStmt(cond, body, else_, else_body, tkn)
            elif tkn := self.expect(lx.CMACRO_ELSE):
                if part is else_body:
                    raise self.make_syntax_error("Multiple #else")
                else_ = tkn
                else_body = []
                part = else_body
            else:
                part.append(self.stmt())

    def for_stmt(self, for_: lx.Token) -> ForStmt:
        lparen = self.require(lx.LPAREN)
        header = [lparen] + self.consume_to(lx.RPAREN)
        body = self.block()
        return ForStmt(for_, header, body)

    def while_stmt(self, while_: lx.Token) -> WhileStmt:
        lparen = self.require(lx.LPAREN)
        cond = [lparen] + self.consume_to(lx.RPAREN)
        body = self.block()
        return WhileStmt(while_, cond, body)


if __name__ == "__main__":
    import sys
    import pprint

    if sys.argv[1:]:
        filename = sys.argv[1]
        if filename == "-c" and sys.argv[2:]:
            src = sys.argv[2]
            filename = "<string>"
        else:
            with open(filename, "r") as f:
                src = f.read()
            srclines = src.splitlines()
            begin = srclines.index("// BEGIN BYTECODES //")
            end = srclines.index("// END BYTECODES //")
            src = "\n".join(srclines[begin + 1 : end])
    else:
        filename = "<default>"
        src = "if (x) { x.foo; // comment\n}"
    parser = Parser(src, filename)
    while node := parser.definition():
        pprint.pprint(node)

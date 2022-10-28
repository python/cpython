# C expression parser

from __future__ import annotations

from ast import literal_eval
from dataclasses import dataclass, field
from typing import NamedTuple, Callable, TypeVar

import lexer as lx
from plexer import PLexer

Token = lx.Token


T = TypeVar("T", bound="EParser")
def contextual(func: Callable[[T], Node|None]) -> Callable[[T], Node|None]:
    # Decorator to wrap grammar methods.
    # Resets position if `func` returns None.
    def contextual_wrapper(self: T) -> Node|None:
        begin = self.getpos()
        res = func(self)
        if res is None:
            self.setpos(begin)
            return
        end = self.getpos()
        res.context = Context(self, begin, end)
        return res
    return contextual_wrapper


class Context(NamedTuple):
    owner: PLexer
    begin: int
    end: int


@dataclass
class Node:
    context: Context|None = field(init=False, default=None)

    @property
    def text(self) -> str:
        context = self.context
        if not context:
            return ""
        tokens = context.owner.tokens
        begin = context.begin
        end = context.end
        return lx.to_text(tokens[begin:end])


@dataclass
class InfixOp(Node):
    left: Node
    op: Token
    right: Node


@dataclass
class ConditionalMiddle(Node):
    # Syntactically, '?expr:' is a binary operator
    middle: Node

@dataclass
class PrefixOp(Node):
    op: Token | ConditionalMiddle
    expr: Node


@dataclass
class Cast(Node):
    type: Node
    expr: Node


@dataclass
class PointerType(Node):
    # *type; **type is represented as PointerType(PointerType(type))
    type: Node


@dataclass
class NumericType(Node):
    number_type: list[Token]  # int, register unsigned long, char, float, etc.


@dataclass
class NamedType(Node):
    name: Token


@dataclass
class Call(Node):
    term: Node
    args: list[Node]


@dataclass
class Index(Node):
    term: Node
    index: Node


@dataclass
class Period(Node):
    term: Node
    name: Token


@dataclass
class Arrow(Node):
    term: Node
    name: Token

@dataclass
class PostfixOp(Node):
    expr: Node
    op: Token


@dataclass
class Number(Node):
    tok: Token

    @property
    def value(self):
        text = self.tok.text
        if text.startswith("'"):
            return ord(literal_eval(text))
        try:
            return int(text)
        except ValueError:
            return float(text)


@dataclass
class String(Node):
    tokens: list[Token]


@dataclass
class Name(Node):
    tok: Token

    @property
    def name(self):
        return self.tok.text


PREFIX_OPS = {
    lx.PLUSPLUS,
    lx.MINUSMINUS,
    lx.AND,
    lx.TIMES,
    lx.PLUS,
    lx.MINUS,
    lx.NOT,
    lx.LNOT,
    }

INFIX_OPS = {
    lx.TIMES,
    lx.DIVIDE,
    lx.MOD,

    lx.PLUS,
    lx.MINUS,

    lx.LSHIFT,
    lx.RSHIFT,

    lx.LT,
    lx.LE,
    lx.GT,
    lx.GE,

    lx.NE,
    lx.EQ,

    lx.AND,

    lx.XOR,

    lx.OR,

    lx.LAND,

    lx.LOR,

    lx.EQUALS,
    lx.TIMESEQUAL,
    lx.DIVEQUAL,
    lx.MODEQUAL,
    lx.PLUSEQUAL,
    lx.MINUSEQUAL,
    lx.LSHIFTEQUAL,
    lx.RSHIFTEQUAL,
    lx.ANDEQUAL,
    lx.XOREQUAL,
    lx.OREQUAL,
}


class EParser(PLexer):

    @contextual
    def expr(self) -> Node | None:
        things: list[Node|Token] = []  # TODO: list[tuple[Token|None, Node]]
        if not (term := self.full_term()):
            return None
        things.append(term)
        while (q := self.expect(lx.CONDOP)) or (op := self.infix_op()):
            if q:
                middle = self.full_term()
                if not middle:
                    raise self.make_syntax_error("Expected expression")
                self.require(lx.COLON)
                op = ConditionalMiddle(middle)
            things.append(op)
            if not (term := self.full_term()):
                raise self.make_syntax_error(f"Expected term following {op}")
            things.append(term)
        assert len(things) % 2 == 1, things
        # TODO: Infix operator precedence
        while len(things) >= 3:
            right = things.pop()
            op = things.pop()
            left = things.pop()
            things.append(InfixOp(left, op, right))
        return things[0]

    @contextual
    def full_term(self) -> Node | None:
        tok = self.peek()
        if tok and tok.kind in PREFIX_OPS:
            self.next()
            term = self.full_term()
            if not term:
                raise self.make_syntax_error(f"Expected term following {tok}")
            return PrefixOp(tok, term)
        # TODO: SIZEOF
        if cast := self.cast():
            return cast
        term = self.term()
        if not term:
            return None
        while True:
            if self.expect(lx.LPAREN):
                args: list[Node] = []
                while arg := self.expr():
                    args.append(arg)
                    if not self.expect(lx.COMMA):
                        break
                self.require(lx.RPAREN)
                term = Call(term, args)
            elif self.expect(lx.LBRACKET):
                index = self.expr()
                if not index:
                    raise self.make_syntax_error("Expected index expression")
                self.require(lx.RBRACKET)
                term = Index(term, index)
            elif self.expect(lx.PERIOD):
                name = self.require(lx.IDENTIFIER)
                term = Period(term, name)
            elif self.expect(lx.ARROW):
                name = self.require(lx.IDENTIFIER)
                term = Arrow(term, name)
            elif (tok := self.expect(lx.PLUSPLUS)) or (tok := self.expect(lx.MINUSMINUS)):
                term = PostfixOp(term, tok)
            else:
                # TODO: Others
                break
        return term

    @contextual
    def cast(self):
        if self.expect(lx.LPAREN):
            if type := self.type():
                if self.expect(lx.RPAREN):
                    if term := self.full_term():
                        return Cast(type, term)

    @contextual
    def type(self):
        if not (type := self.type_name()):
            return None
        while self.expect(lx.TIMES):
            type = PointerType(type)
        # TODO: [] and ()
        return type

    @contextual
    def type_name(self):
        if not (token := self.peek()):
            return None
        # TODO: unsigned, short, long
        # TODO: const, volatile, extern, register, etc.
        if token.kind in (lx.INT, lx.CHAR, lx.FLOAT, lx.DOUBLE):
            self.next()
            return NumericType([token])
        if token.kind == lx.IDENTIFIER:
            # TODO: Check the list of known typedefs
            self.next()
            return NamedType(token)

    @contextual
    def term(self) -> Node | None:
        token = self.next()
        if not token:
            return None
        if token.kind == lx.NUMBER:
            return Number(token)
        if token.kind == lx.STRING:
            strings: list[Token] = [token]
            while token := self.expect(lx.STRING):
                strings.append(token)
            assert strings
            return String(strings)
        if token.kind == lx.CHARACTER:
            return Number(token)  # NB!
        if token.kind == lx.IDENTIFIER:
            return Name(token)
        if token.kind == lx.LPAREN:
            expr = self.expr()
            self.require(lx.RPAREN)
            return expr
        return None

    def infix_op(self) -> Token | None:
        token = self.next()
        if token is None:
            return None
        # TODO: All infix operators
        if token.kind in INFIX_OPS:
            return token
        self.backup()
        return None


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
    else:
        filename = None
        src = "if (x) { x.foo; // comment\n}"
    p = EParser(src, filename)
    x = p.expr()
    assert x, p.getpos()
    if x.text.rstrip() != src.rstrip():
        print("=== src ===")
        print(src)
        print("=== text ===")
        print(x.text)
        print("=== data ===")
        print(x)
        print("=== === ===")
        print("FAIL")
    else:
        print(x.text)
        print("OK")

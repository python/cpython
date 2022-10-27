# C expression parser

from __future__ import annotations

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
class PrefixOp(Node):
    op: Token
    expr: Node


@dataclass
class Cast(Node):
    type: Node
    expr: Node


@dataclass
class Type(Node):
    name: Token
    stars: int


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
        try:
            return int(text)
        except ValueError:
            return float(text)


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
        # TODO: All the other forms of expressions
        things: list[Node|Token] = []  # TODO: list[tuple[Token|None, Node]]
        if not (term := self.full_term()):
            return None
        things.append(term)
        while (op := self.infix_op()):
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
        if self.expect(lx.LPAREN):
            args: list[Node] = []
            while arg := self.expr():
                args.append(arg)
                if not self.expect(lx.COMMA):
                    break
            self.require(lx.RPAREN)
            return Call(term, args)
        if self.expect(lx.LBRACKET):
            index = self.expr()
            if not index:
                raise self.make_syntax_error("Expected index expression")
            self.require(lx.RBRACKET)
            return Index(term, index)
        if self.expect(lx.PERIOD):
            name = self.require(lx.IDENTIFIER)
            return Period(term, name)
        if self.expect(lx.ARROW):
            name = self.require(lx.IDENTIFIER)
            return Arrow(term, name)
        if (tok := self.expect(lx.PLUSPLUS)) or (tok := self.expect(lx.MINUSMINUS)):
            return PostfixOp(term, tok)
        # TODO: Others
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
        token = self.peek()
        if token and token.kind in (lx.INT, lx.CHAR, lx.FLOAT, lx.DOUBLE, lx.IDENTIFIER):
            type = self.next()
            assert type
            stars = 0
            while self.expect(lx.TIMES):
                stars += 1
            return Type(type, stars)            

    @contextual
    def term(self) -> Node | None:
        token = self.next()
        if not token:
            return None
        if token.kind == lx.NUMBER:
            return Number(token)
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

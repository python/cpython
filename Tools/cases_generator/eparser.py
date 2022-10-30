# C expression parser

from __future__ import annotations

from ast import literal_eval
from dataclasses import dataclass, field
from typing import NamedTuple, Callable, TypeVar

import lexer as lx
from plexer import PLexer

Token = lx.Token


T = TypeVar("T", bound="EParser")
S = TypeVar("S", bound="Node")
def contextual(func: Callable[[T], S|None]) -> Callable[[T], S|None]:
    # Decorator to wrap grammar methods.
    # Resets position if `func` returns None.
    def contextual_wrapper(self: T) -> S|None:
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
    # Syntactically, '?expr:' is an infix operator
    middle: Node


@dataclass
class ConditionalOp(Node):
    cond: Node
    then: Node
    orelse: Node


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
class ArrayType(Node):
    type: Node
    size: Node | None


@dataclass
class FunctionType(Node):
    type: Node
    args: list[Node]


@dataclass
class NumericType(Node):
    number_type: list[Token]  # int, unsigned long, char, float, void, etc.


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

    def __repr__(self):
        tr = repr(self.tok)
        if self.tok.kind == lx.IDENTIFIER:
            trs = tr.split()
            tr = trs[-1].rstrip(")")
        return f"{self.name!r}({self.context}, {tr})"


NUMERIC_TYPES = {
    lx.VOID,
    lx.UNSIGNED,
    lx.SIGNED,
    lx.CHAR,
    lx.SHORT,
    lx.INT,
    lx.LONG,
    lx.FLOAT,
    lx.DOUBLE,
}


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

INFIX_OPS_BY_PRIO = [
    {
        lx.TIMES,
        lx.DIVIDE,
        lx.MOD,
    },
    {
        lx.PLUS,
        lx.MINUS,
    },
    {
        lx.LSHIFT,
        lx.RSHIFT,
    },
    {
        lx.LT,
        lx.LE,
        lx.GT,
        lx.GE,
    },
    {
        lx.NE,
        lx.EQ,
    },
    {
        lx.AND,
    },
    {
        lx.XOR,
    },
    {
        lx.OR,
    },
    {
        lx.LAND,
    },
    {
        lx.LOR,
    },
    {
        lx.CONDOP,
    },
    {
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
    },
    {
        lx.COMMA,
    },
]

INFIX_OPS = set.union(*INFIX_OPS_BY_PRIO)

PRIO = {op: 12 - prio for prio, ops in enumerate(INFIX_OPS_BY_PRIO) for op in ops}
# Sanity checks
assert PRIO[lx.TIMES] == 12, PRIO
assert PRIO[lx.COMMA] == 0, PRIO


def op_from_arg(arg: ConditionalMiddle | Token) -> str:
    if isinstance(arg, Token):
        return arg.kind
    assert isinstance(arg, ConditionalMiddle)
    return lx.CONDOP


def must_reduce(arg1: ConditionalMiddle | Token, arg2: ConditionalMiddle | Token) -> bool:
    # Examples:
    # '*', '+' -> True
    # '+', '*' -> False
    # '+', '+' -> True
    # '?', '?' -> False  # conditional is right-associative
    # '=', '=' -> False  # assignment is right-associative
    op1 = op_from_arg(arg1)
    op2 = op_from_arg(arg2)
    return PRIO[op1] > PRIO[op2] or (PRIO[op1] == PRIO[op2] and PRIO[op1] > PRIO[lx.CONDOP])


def make_infix_op(left: Node, op: ConditionalMiddle | Token, right: Node) -> Node:
    if isinstance(op, ConditionalMiddle):
        return ConditionalOp(left, op.middle, right)
    return InfixOp(left, op, right)


class EParser(PLexer):

    @contextual
    def expr(self) -> Node | None:
        return self._expr(True)

    @contextual
    def expr1(self) -> Node | None:
        return self._expr(False)

    def _expr(self, allow_comma: bool) -> Node | None:
        things: list[tuple[ConditionalMiddle|Token|None, Node]] = []
        if not (term := self.full_term()):
            return None
        things.append((None, term))
        while op := self.infix_op(allow_comma):
            if op.kind == lx.CONDOP:
                middle = self.full_term()
                if not middle:
                    raise self.make_syntax_error("Expected expression")
                self.require(lx.COLON)
                op = ConditionalMiddle(middle)
            lastop, lastterm = things[-1]
            if lastop is not None and must_reduce(lastop, op):
                things.pop()
                prevop, prevterm = things.pop()
                things.append((prevop, make_infix_op(prevterm, lastop, lastterm)))
            if not (term := self.full_term()):
                raise self.make_syntax_error(f"Expected term following {op}")
            things.append((op, term))
        while len(things) >= 2:
            lastop, lastterm = things.pop()
            assert lastop is not None
            prevop, prevterm = things.pop()
            things.append((prevop, make_infix_op(prevterm, lastop, lastterm)))
        return things[0][1]

    @contextual
    def full_term(self) -> Node | None:
        tok = self.peek()
        if tok and tok.kind in PREFIX_OPS:
            self.next()
            term = self.full_term()
            if not term:
                raise self.make_syntax_error(f"Expected term following {tok}")
            return PrefixOp(tok, term)
        # TODO: SIZEOF -- it's "sizeof(type)" or "sizeof term"
        if cast := self.cast():
            return cast
        term = self.term()
        if not term:
            return None
        while True:
            if self.expect(lx.LPAREN):
                args: list[Node] = []
                while arg := self.expr1():
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
        # TODO: []
        while self.expect(lx.LPAREN):
            if self.expect(lx.TIMES):
                self.require(lx.RPAREN)
                type = PointerType(type)
                continue
            args: list[Node] = []
            while arg := self.type():
                args.append(arg)
                if not self.expect(lx.COMMA):
                    break
            self.require(lx.RPAREN)
            type = FunctionType(type, args)
        return type

    @contextual
    def type_name(self):
        if not (token := self.peek()):
            return None
        # TODO: const, volatile, register, etc.
        tokens = []
        while token and token.kind in NUMERIC_TYPES:
            tokens.append(token)
            self.next()
            token = self.peek()
        if tokens:
            return NumericType(tokens)
        if token and token.kind == lx.IDENTIFIER:
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

    def infix_op(self, allow_comma: bool) -> Token | None:
        token = self.next()
        if token is None:
            return None
        if token.kind in INFIX_OPS:
            if allow_comma or token.kind != lx.COMMA:
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
        print("=== text ===")
        print(x.text)
        print("=== data ===")
        print(x)
        print("=== === ===")
        print("OK")

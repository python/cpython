from __future__ import annotations

from dataclasses import dataclass, field
from typing import NamedTuple, Callable

import lexer as lx
from plexer import Lexer
Token = lx.Token


def contextual(func: Callable[[Parser], Node|None]):
    # Decorator to wrap grammar methods.
    # Resets position if `func` returns None.
    def contextual_wrapper(self: Parser) -> Node|None:
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
    owner: Lexer
    begin: int
    end: int


@dataclass
class Node:
    context: Context|None = field(init=False, default=None)

    @property
    def text(self) -> str:
        context = self.context
        tokens = context.owner.tokens
        begin = context.begin
        end = context.end
        return lx.to_text(tokens[begin:end])


@dataclass
class Block(Node):
    stmts: list[Node]


@dataclass
class ForStmt(Node):
    init: Node | None
    cond: Node | None
    next: Node | None
    body: Node


@dataclass
class IfStmt(Node):
    cond: Node
    body: Node
    orelse: Node | None


@dataclass
class WhileStmt(Node):
    cond: Node
    body: Node


@dataclass
class BreakStmt():
    pass


@dataclass
class ContinueStmt():
    pass


@dataclass
class ReturnStmt(Node):
    expr: Node | None


@dataclass
class GotoStmt(Node):
    label: Token


@dataclass
class NullStmt(Node):
    pass


@dataclass
class VarDecl(Node):
    type: Token
    name: Token
    init: Node | None


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
        v = self.tok.value
        try:
            return int(v)
        except ValueError:
            return float(v)


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


class Parser(Lexer):

    @contextual
    def stmt(self) -> Node | None:
        if self.eof():
            return None
        kind = self.peek().kind
        if kind == lx.LBRACE:
            return self.block()
        if kind == lx.FOR:
            return self.for_stmt()
        if kind == lx.IF:
            return self.if_stmt()
        if kind == lx.WHILE:
            return self.while_stmt()
        # TODO: switch
        if kind == lx.BREAK:
            self.next()
            self.require(lx.SEMI)
            return BreakStmt()
        if kind == lx.CONTINUE:
            self.next()
            self.require(lx.SEMI)
            return ContinueStmt()
        if kind == lx.RETURN:
            self.next()
            expr = self.expr()  # May be None
            self.require(lx.SEMI)
            return ReturnStmt(expr)
        if kind == lx.GOTO:
            self.next()
            label = self.require(lx.IDENTIFIER)
            self.require(lx.SEMI)
            return GotoStmt(label)
        # TODO: switch, case, default, label
        if kind == lx.SEMI:
            return self.empty_stmt()
        if decl := self.declaration():
            return decl
        return self.expr_stmt()

    @contextual
    def block(self):
        if self.expect(lx.LBRACE):
            stmts = []
            while s := self.stmt():
                stmts.append(s)
            if not self.expect(lx.RBRACE):
                raise self.make_syntax_error("Expected '}'")
            return Block(stmts)

    @contextual
    def for_stmt(self):
        if self.expect(lx.FOR):
            self.require(lx.LPAREN)
            init = self.expr()
            self.require(lx.SEMI)
            cond = self.expr()
            self.require(lx.SEMI)
            next = self.expr()
            self.require(lx.RPAREN)
            body = self.stmt()
            if not body:
                raise self.make_syntax_error("Expected statement")
            return ForStmt(init, cond, next, body)

    @contextual
    def if_stmt(self):
        if self.expect(lx.IF):
            self.require(lx.LPAREN)
            cond = self.expr()
            if not cond:
                raise self.make_syntax_error("Expected expression")
            self.require(lx.RPAREN)
            body = self.stmt()
            if not body:
                raise self.make_syntax_error("Expected statement")
            orelse = None
            if self.expect(lx.ELSE):
                orelse = self.stmt()
                if not orelse:
                    raise self.make_syntax_error("Expected statement")
            return IfStmt(cond, body, orelse)

    @contextual
    def while_stmt(self):
        if self.expect(lx.WHILE):
            self.require(lx.LPAREN)
            cond = self.expr()
            if not cond:
                raise self.make_syntax_error("Expected expression")
            self.require(lx.RPAREN)
            body = self.stmt()
            if not body:
                raise self.make_syntax_error("Expected statement")
            return WhileStmt(cond, body)

    @contextual
    def empty_stmt(self):
        if self.expect(lx.SEMI):
            return NullStmt()

    @contextual
    def expr_stmt(self):
        if expr := self.expr():
            self.require(lx.SEMI)
            return expr

    def declaration(self):
        tok = self.peek()
        if not tok:
            return None
        # TODO: Do it for real
        if tok.kind in (lx.INT, lx.CHAR, lx.FLOAT, lx.DOUBLE):
            type = self.next()
            name = self.require(lx.IDENTIFIER)
            if self.expect(lx.EQUALS):
                init = self.expr()
                if not init:
                    raise self.make_syntax_error("Expected initialization expression")
            else:
                init = None
            self.require(lx.SEMI)
            return VarDecl(type, name, init)


    @contextual
    def expr(self) -> Node | None:
        # TODO: All the other forms of expressions
        things = []
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
            return PrefixOp(self.next(), self.full_term())
        # TODO: SIZEOF
        if cast := self.cast():
            return cast
        term = self.term()
        if not term:
            return None
        if self.expect(lx.LPAREN):
            args = []
            while arg := self.expr():
                args.append(arg)
                if not self.expect(lx.COMMA):
                    break
            self.require(lx.RPAREN)
            return Call(term, args)
        if self.expect(lx.LBRACKET):
            index = self.expr()
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
        tok = self.peek()
        if tok.kind in (lx.INT, lx.CHAR, lx.FLOAT, lx.DOUBLE):
            type = self.next()
            stars = 0
            while self.expect(lx.TIMES):
                stars += 1
            return Type(type, stars)            

    @contextual
    def term(self) -> Node | None:
        token = self.next()
        if token.kind == lx.NUMBER:
            return Number(token)
        if token.kind == lx.IDENTIFIER:
            return Name(token)
        if token.kind == lx.LPAREN:
            expr = self.expr()
            self.require(lx.RPAREN)
            return expr
        self.backup()
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
    p = Parser(src, filename)
    x = p.stmt()
    assert x, p.getpos()
    if x.text.rstrip() != src.rstrip():
        print("=== src ===")
        print(src)
        print("=== text ===")
        print(x.text)
        print("=== === ===")
        print("FAIL")
    else:
        print(x.text)
        print("OK")

# C statement parser

from __future__ import annotations

from dataclasses import dataclass

import lexer as lx
from eparser import EParser, contextual, Node, PointerType, ArrayType, FunctionType

Token = lx.Token


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
class SwitchStmt(Node):
    expr: Node
    block: Node  # Cases are not syntactically part of this


@dataclass
class CaseStmt(Node):
    expr: Node | None  # None for default
    block: Node


@dataclass
class BreakStmt(Node):
    pass


@dataclass
class ContinueStmt(Node):
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
class DeclStmt(Node):
    decls: list[Node]


@dataclass
class VarDecl(Node):
    type: Node
    name: Token
    init: Node | None


@dataclass
class InitializerList(Node):
    exprs: list[Node]


class SParser(EParser):

    @contextual
    def stmt(self) -> Node | None:
        if self.eof():
            return None
        token = self.peek()
        if token is None:
            return None
        kind = token.kind
        if kind == lx.LBRACE:
            return self.block()
        if kind == lx.FOR:
            return self.for_stmt()
        if kind == lx.IF:
            return self.if_stmt()
        if kind == lx.WHILE:
            return self.while_stmt()
        if kind == lx.SWITCH:
            return self.switch_stmt()
        if kind == lx.CASE:
            return self.case_stmt()
        if kind == lx.DEFAULT:
            return self.default_stmt()
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
            return self.null_stmt()
        if decl := self.decl_stmt():
            return decl
        return self.expr_stmt()

    @contextual
    def block(self):
        if self.expect(lx.LBRACE):
            stmts: list[Node] = []
            while s := self.stmt():
                stmts.append(s)
            if not self.expect(lx.RBRACE):
                raise self.make_syntax_error("Expected '}'")
            return Block(stmts)

    @contextual
    def for_stmt(self):
        if self.expect(lx.FOR):
            self.require(lx.LPAREN)
            init = self.decl_stmt() or self.expr_stmt() or self.null_stmt()
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
    def switch_stmt(self):
        if self.expect(lx.SWITCH):
            if self.expect(lx.LPAREN):
                expr = self.expr()
                if not expr:
                    raise self.make_syntax_error("Expected expression")
                self.require(lx.RPAREN)
                block = self.block()
                return SwitchStmt(expr, block)

    @contextual
    def case_stmt(self):
        if self.expect(lx.CASE):
            expr = self.expr()
            if expr is None:
                raise self.make_syntax_error("Expected expression")
            self.require(lx.COLON)
            stmt = self.stmt()
            if stmt is None:
                raise self.make_syntax_error("Expected statement")
            return CaseStmt(expr, stmt)

    @contextual
    def default_stmt(self):
        if self.expect(lx.DEFAULT):
            self.require(lx.COLON)
            stmt = self.stmt()
            if stmt is None:
                raise self.make_syntax_error("Expected statement")
            return CaseStmt(None, stmt)

    @contextual
    def null_stmt(self):
        if self.expect(lx.SEMI):
            return NullStmt()

    @contextual
    def expr_stmt(self):
        if expr := self.expr():
            self.require(lx.SEMI)
            return expr

    @contextual
    def decl_stmt(self):
        if not (type := self.type_name()):
            return None
        decl = self.decl(type)
        if not decl:
            return None
        decls = [decl]
        while self.expect(lx.COMMA):
            decl = self.decl(type)
            if decl is None:
                raise self.make_syntax_error("Expected declaration")
            decls.append(decl)
        self.require(lx.SEMI)
        return DeclStmt(decls)

    def decl(self, type: Node):
        stars = 0
        while self.expect(lx.TIMES):
            stars += 1
        if name := self.expect(lx.IDENTIFIER):
            while self.expect(lx.LBRACKET):
                expr = self.expr()
                if not expr:
                    raise self.make_syntax_error("Expected dimension expression")
                self.require(lx.RBRACKET)
                type = ArrayType(type, expr)
            for _ in range(stars):
                type = PointerType(type)
            if self.expect(lx.EQUALS):
                if self.expect(lx.LBRACE):
                    exprs = []
                    while expr := self.expr():
                        exprs.append(expr)
                        if not self.expect(lx.COMMA):
                            break
                    self.require(lx.RBRACE)
                    init = InitializerList(exprs)
                else:
                    init = self.expr()
                if not init:
                    raise self.make_syntax_error("Expected initialization expression")
            else:
                init = None
            return VarDecl(type, name, init)
        # int **(*func)(int, int)
        if self.expect(lx.LPAREN):
            if self.expect(lx.TIMES):
                if name := self.expect(lx.IDENTIFIER):
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.LPAREN):
                            args: list[Node] = []
                            while arg := self.type():
                                args.append(arg)
                                if not self.expect(lx.COMMA):
                                    break
                            if self.expect(lx.RPAREN):
                                for _ in range(stars):
                                    type = PointerType(type)
                                type = FunctionType(type, args)
                                # TODO: Initializer
                                return VarDecl(type, name, None)


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
    p = SParser(src, filename)
    x = p.stmt()
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

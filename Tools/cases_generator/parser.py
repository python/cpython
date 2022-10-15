"""Parser for bytecodes.inst."""

from dataclasses import dataclass

import lexer as lx


class Lexer:
    def __init__(self, src):
        self.src = src
        self.all_tokens = list(lx.tokenize(self.src))
        self.tokens = [tkn for tkn in self.all_tokens if tkn.kind != "COMMENT"]
        self.pos = 0

    def getpos(self):
        return self.pos

    def reset(self, pos):
        assert 0 <= pos <= len(self.tokens), (pos, len(self.tokens))
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


@dataclass
class Family:
    name: str
    members: list[str]


class Parser(Lexer):
    def __init__(self, src):
        super().__init__(src)

    def c_blob(self):
        tokens = []
        level = 0
        while True:
            tkn = self.next()
            if tkn is None:
                break
            if tkn.kind in (lx.LBRACE, lx.LPAREN, lx.LBRACKET):
                level += 1
            elif tkn.kind in (lx.RBRACE, lx.RPAREN, lx.RBRACKET):
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
        # TODO: Make INST a keyword in the lexer.
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "inst":
            if (self.expect(lx.LPAREN)
                    and (tkn := self.expect(lx.IDENTIFIER))):
                name = tkn.text
                if self.expect(lx.COMMA) and self.expect(lx.LPAREN):
                    inp = self.inputs() or []
                    if self.expect(lx.MINUSMINUS):
                        outp = self.outputs() or []
                        if (self.expect(lx.RPAREN)
                                and self.expect(lx.RPAREN)
                                and self.expect(lx.LBRACE)):
                            return InstHeader(name, inp, outp)
        self.reset(here)
        return None

    def inputs(self):
        # input (, input)*
        here = self.getpos()
        if inp := self.input():
            near = self.getpos()
            if self.expect(lx.COMMA):
                if rest := self.inputs():
                    return [inp] + rest
            self.reset(near)
            return [inp]
        self.reset(here)
        return None

    def input(self):
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
                raise SyntaxError("Expected argument in brackets")
            return tkn.text
        if self.expect(lx.CONDOP):
            while self.expect(lx.CONDOP):
                pass
            return "??"
        return None

    def outputs(self):
        # output (, output)*
        here = self.getpos()
        if outp := self.output():
            print("OUTPUT", outp)
            near = self.getpos()
            if self.expect(lx.COMMA):
                if rest := self.outputs():
                    return [outp] + rest
            self.reset(near)
            return [outp]
        self.reset(here)
        return None

    def output(self):
        return self.input()  # TODO: They're not quite the same.

    def family_def(self):
        here = self.getpos()
        if (tkn := self.expect(lx.IDENTIFIER)) and tkn.text == "family":
            if self.expect(lx.LPAREN):
                if (tkn := self.expect(lx.IDENTIFIER)):
                    name = tkn.text
                    if self.expect(lx.RPAREN):
                        if self.expect(lx.EQUALS):
                            if members := self.members():
                                if self.expect(lx.SEMI):
                                    return Family(name, members)
        self.reset(here)
        return None

    def members(self):
        here = self.getpos()
        if tkn := self.expect(lx.IDENTIFIER):
            near = self.getpos()
            if self.expect(lx.PLUS):
                if rest := self.members():
                    return [tkn.text] + rest
            self.reset(near)
            return [tkn.text]
        self.reset(here)
        return None

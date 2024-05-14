import lexer as lx

Token = lx.Token


class PLexer:
    def __init__(self, src: str, filename: str):
        self.src = src
        self.filename = filename
        self.tokens = list(lx.tokenize(self.src, filename=filename))
        self.pos = 0

    def getpos(self) -> int:
        # Current position
        return self.pos

    def eof(self) -> bool:
        # Are we at EOF?
        return self.pos >= len(self.tokens)

    def setpos(self, pos: int) -> None:
        # Reset position
        assert 0 <= pos <= len(self.tokens), (pos, len(self.tokens))
        self.pos = pos

    def backup(self) -> None:
        # Back up position by 1
        assert self.pos > 0
        self.pos -= 1

    def next(self, raw: bool = False) -> Token | None:
        # Return next token and advance position; None if at EOF
        # TODO: Return synthetic EOF token instead of None?
        while self.pos < len(self.tokens):
            tok = self.tokens[self.pos]
            self.pos += 1
            if raw or tok.kind != "COMMENT":
                return tok
        return None

    def peek(self, raw: bool = False) -> Token | None:
        # Return next token without advancing position
        tok = self.next(raw=raw)
        self.backup()
        return tok

    def maybe(self, kind: str, raw: bool = False) -> Token | None:
        # Return next token without advancing position if kind matches
        tok = self.peek(raw=raw)
        if tok and tok.kind == kind:
            return tok
        return None

    def expect(self, kind: str) -> Token | None:
        # Return next token and advance position if kind matches
        tkn = self.next()
        if tkn is not None:
            if tkn.kind == kind:
                return tkn
            self.backup()
        return None

    def require(self, kind: str) -> Token:
        # Return next token and advance position, requiring kind to match
        tkn = self.next()
        if tkn is not None and tkn.kind == kind:
            return tkn
        raise self.make_syntax_error(
            f"Expected {kind!r} but got {tkn and tkn.text!r}", tkn
        )

    def extract_line(self, lineno: int) -> str:
        # Return source line `lineno` (1-based)
        lines = self.src.splitlines()
        if lineno > len(lines):
            return ""
        return lines[lineno - 1]

    def make_syntax_error(self, message: str, tkn: Token | None = None) -> SyntaxError:
        # Construct a SyntaxError instance from message and token
        if tkn is None:
            tkn = self.peek()
        if tkn is None:
            tkn = self.tokens[-1]
        return lx.make_syntax_error(
            message, self.filename, tkn.line, tkn.column, self.extract_line(tkn.line)
        )


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
    else:
        filename = "<default>"
        src = "if (x) { x.foo; // comment\n}"
    p = PLexer(src, filename)
    while not p.eof():
        tok = p.next(raw=True)
        assert tok
        left = repr(tok)
        right = lx.to_text([tok]).rstrip()
        print(f"{left:40.40} {right}")

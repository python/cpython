
from lexer import Token
from typing import TextIO

class CWriter:
    'A writer that understands how to format C code.'

    def __init__(self, out: TextIO, indent: int = 0):
        self.out = out
        self.indent = indent
        self.line = -1
        self.column = 0
        self.newline = False

    def set_position(self, tkn: Token):
        if self.line < tkn.line and not self.newline:
            self.newline = True
            self.out.write("\n")
        self.line = tkn.end_line
        self.column = tkn.end_column

    def emit_token(self, tkn: Token):
        if self.line < tkn.line:
            self.line = tkn.line
            self.newline = True
            self.out.write("\n")
        if self.newline:
            self.column = 0
        elif self.column > 0 and self.column < tkn.column:
            self.out.write(" ") * (tkn.column-self.column)
        self.line = tkn.end_line
        if tkn.kind == "COMMENT" and "\n" in tkn.text:
            # Try to format multi-line comments sensibly
            first = True
            for line in tkn.text.splitlines(True):
                line = line.lstrip(" ")
                if not first:
                    adjust = 1 if line[0] == "*" else 3
                    self.out.write(" " * (self.column + adjust))
                self.emit_text(line)
                first = False
            self.column = tkn.end_column
            self.newline = False
            return
        self.column = tkn.end_column
        if tkn.kind == "CMACRO":
            self.out.write(tkn.text)
        else:
            self.emit_text(tkn.text)

    def emit_text(self, txt: Token):
        parens = txt.count("(") - txt.count(")")
        if parens < 0:
            self.indent += parens
        if "}" in txt or txt.endswith(":"):
            self.indent -= 1
            assert self.indent >= 0
        if self.newline:
            self.out.write("    "*self.indent)
            self.newline = False
            txt = txt.lstrip(" ")
        self.out.write(txt)
        if txt.endswith("\n"):
            self.line += 1
            self.newline = True
        elif txt.endswith("{") or txt.startswith("}"):
            if not self.newline:
                self.out.write("\n")
                self.line += 1
                self.newline = True
        if "{" in txt or txt.endswith(":"):
            self.indent += 1
        if parens > 0:
            self.indent += parens

    def emit(self, txt: str | Token):
        if isinstance(txt, Token):
            self.emit_token(txt)
        elif isinstance(txt, str):
            if txt.count("\n") > 1:
                for line in txt.splitlines(True):
                    self.emit_text(line)
                return
            self.emit_text(txt)
        else:
            assert False

    def start_line(self):
        if not self.newline:
            self.out.write("\n")
            self.newline = True

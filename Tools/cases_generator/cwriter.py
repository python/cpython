
from lexer import Token

class CWriter:
    'A writer that understands how to format C code.'

    def __init__(self, out, indent = 0):
        self.out = out
        self.indent = indent
        self.line = 1
        self.column = 0
        self.newline = False

    def emit_token(self, tkn):
        if self.line < tkn.line:
            self.line = tkn.line
            self.newline = True
            self.out.write("\n")
        if self.newline:
            self.column = 0
        elif self.column > 0 and self.column < tkn.column:
            self.out.write(" ") * (tkn.column-self.column)
        self.column = tkn.end_column
        self.line = tkn.line
        self.emit_text(tkn.text)

    def emit_text(self, txt):
        if "}" in txt or txt.endswith(":"):
            self.indent -= 1
            assert self.indent >= 0
        if self.newline:
            self.out.write("    "*self.indent)
            self.newline = False
            txt = txt.lstrip(" ")
        self.out.write(txt)
        if txt.endswith("\n"):
            self.newline = True
        elif txt.endswith("{") or txt.endswith(":") or txt.startswith("}") or txt.endswith(";"):
            self.out.write("\n")
            self.line += 1
            self.newline = True
        if "{" in txt or txt.endswith(":"):
            self.indent += 1

    def emit(self, txt):
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
            assert not isinstance(txt, tuple)
            for item in txt:
                self.write(item)

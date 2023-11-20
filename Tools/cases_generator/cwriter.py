
from lexer import Token
from typing import TextIO

class CWriter:
    'A writer that understands tokens and how to format C code'

    def __init__(self, out: TextIO, indent: int, line_directives: bool):
        self.out = out
        self.base_column = indent * 4
        self.indents = [ i*4 for i in range(indent+1) ]
        self.line = -1
        self.column = 0
        self.line_directives = line_directives

    def set_position(self, tkn: Token, emit_lines=True):
        if self.line < tkn.line:
            self.column = 0
            if emit_lines:
                self.out.write("\n")
            if self.line_directives:
                self.out.write(f'#line {tkn.line} "{tkn.filename}"\n')
        if self.column == 0:
            self.out.write(" " * self.indents[-1])
        else:
            #Token colums are 1 based
            column = tkn.column - 1
            if self.column < column:
                self.out.write(" " * (column-self.column))
        self.line = tkn.end_line
        self.column = tkn.end_column - 1

    def maybe_dedent(self, txt: str):
        parens = txt.count("(") - txt.count(")")
        if parens < 0:
            self.indents.pop()
        elif "}" in txt or txt.endswith(":"):
            self.indents.pop()

    def maybe_indent(self, txt: str):
        parens = txt.count("(") - txt.count(")")
        if parens > 0:
            offset = self.column
            if offset <= self.indents[-1] or offset > 40:
                offset = self.indents[-1] + 4
            self.indents.append(offset)
        elif "{" in txt or txt.endswith(":"):
            self.indents.append(self.indents[-1] + 4)

    def emit_token(self, tkn: Token):
        self.maybe_dedent(tkn.text)
        self.set_position(tkn)
        self.emit_text(tkn.text)
        self.maybe_indent(tkn.text)

    def emit_text(self, txt: str):
        if self.column == 0 and txt.strip():
            self.out.write(" " * self.indents[-1])
            self.column = self.base_column
        self.out.write(txt)

    def emit_str(self, txt: str):
        self.maybe_dedent(txt)
        self.emit_text(txt)
        if txt.endswith("\n"):
            self.column = 0
            self.line += 1
        elif txt.endswith("{") or txt.startswith("}"):
            if self.column:
                self.column = 0
                self.line = 0
        else:
            self.column += len(txt)
        self.maybe_indent(txt)

    def emit(self, txt: str | Token):
        if isinstance(txt, Token):
            self.emit_token(txt)
        elif isinstance(txt, str):
            self.emit_str(txt)
        else:
            assert False

    def start_line(self):
        if self.column:
            self.out.write("\n")
            self.column = 0

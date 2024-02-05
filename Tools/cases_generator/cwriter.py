import contextlib
from lexer import Token
from typing import TextIO, Iterator


class CWriter:
    "A writer that understands tokens and how to format C code"

    last_token: Token | None

    def __init__(self, out: TextIO, indent: int, line_directives: bool):
        self.out = out
        self.base_column = indent * 4
        self.indents = [i * 4 for i in range(indent + 1)]
        self.line_directives = line_directives
        self.last_token = None
        self.newline = True

    def set_position(self, tkn: Token) -> None:
        if self.last_token is not None:
            if self.last_token.line < tkn.line:
                self.out.write("\n")
                if self.line_directives:
                    self.out.write(f'#line {tkn.line} "{tkn.filename}"\n')
                self.out.write(" " * self.indents[-1])
            else:
                gap = tkn.column - self.last_token.end_column
                self.out.write(" " * gap)
        elif self.newline:
            self.out.write(" " * self.indents[-1])
        self.last_token = tkn
        self.newline = False

    def emit_at(self, txt: str, where: Token) -> None:
        self.set_position(where)
        self.out.write(txt)

    def maybe_dedent(self, txt: str) -> None:
        parens = txt.count("(") - txt.count(")")
        if parens < 0:
            self.indents.pop()
        braces = txt.count("{") - txt.count("}")
        if braces < 0 or is_label(txt):
            self.indents.pop()

    def maybe_indent(self, txt: str) -> None:
        parens = txt.count("(") - txt.count(")")
        if parens > 0:
            if self.last_token:
                offset = self.last_token.end_column - 1
                if offset <= self.indents[-1] or offset > 40:
                    offset = self.indents[-1] + 4
            else:
                offset = self.indents[-1] + 4
            self.indents.append(offset)
        if is_label(txt):
            self.indents.append(self.indents[-1] + 4)
        else:
            braces = txt.count("{") - txt.count("}")
            if braces > 0:
                assert braces == 1
                if 'extern "C"' in txt:
                    self.indents.append(self.indents[-1])
                else:
                    self.indents.append(self.indents[-1] + 4)

    def emit_text(self, txt: str) -> None:
        self.out.write(txt)

    def emit_multiline_comment(self, tkn: Token) -> None:
        self.set_position(tkn)
        lines = tkn.text.splitlines(True)
        first = True
        for line in lines:
            text = line.lstrip()
            if first:
                spaces = 0
            else:
                spaces = self.indents[-1]
                if text.startswith("*"):
                    spaces += 1
                else:
                    spaces += 3
            first = False
            self.out.write(" " * spaces)
            self.out.write(text)

    def emit_token(self, tkn: Token) -> None:
        if tkn.kind == "COMMENT" and "\n" in tkn.text:
            return self.emit_multiline_comment(tkn)
        self.maybe_dedent(tkn.text)
        self.set_position(tkn)
        self.emit_text(tkn.text)
        self.maybe_indent(tkn.text)

    def emit_str(self, txt: str) -> None:
        self.maybe_dedent(txt)
        if self.newline and txt:
            if txt[0] != "\n":
                self.out.write(" " * self.indents[-1])
            self.newline = False
        self.emit_text(txt)
        if txt.endswith("\n"):
            self.newline = True
        self.maybe_indent(txt)
        self.last_token = None

    def emit(self, txt: str | Token) -> None:
        if isinstance(txt, Token):
            self.emit_token(txt)
        elif isinstance(txt, str):
            self.emit_str(txt)
        else:
            assert False

    def start_line(self) -> None:
        if not self.newline:
            self.out.write("\n")
        self.newline = True
        self.last_token = None

    @contextlib.contextmanager
    def header_guard(self, name: str) -> Iterator[None]:
        self.out.write(
            f"""
#ifndef {name}
#define {name}
#ifdef __cplusplus
extern "C" {{
#endif

"""
        )
        yield
        self.out.write(
            f"""
#ifdef __cplusplus
}}
#endif
#endif /* !{name} */
"""
        )


def is_label(txt: str) -> bool:
    return not txt.startswith("//") and txt.endswith(":")

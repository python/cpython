import io
import shlex
import dataclasses as dc
from typing import Any


@dc.dataclass
class Token:
    line: str
    lineno: int
    pos: int
    token: str


class Lexer(shlex.shlex):

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        super().__init__(*args, **kwargs)
        self.wordchars += "."


def generate_tokens(line: str, lineno: int = 0) -> list[Token]:
    buf = io.StringIO(line)
    lexer = Lexer(buf)
    tokens: list[Token] = []
    for idx, token in enumerate(lexer):
        if idx and tokens[-1].token == "-" and token == ">":
            tokens.pop()
            token = "->"
        pos = buf.tell() - len(token) - 1
        tokens.append(Token(line, lineno, pos, token))
    return tokens

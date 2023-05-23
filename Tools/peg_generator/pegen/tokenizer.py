import token
import tokenize
from typing import Dict, Iterator, List

Mark = int  # NewType('Mark', int)

exact_token_types = token.EXACT_TOKEN_TYPES


def shorttok(tok: tokenize.TokenInfo) -> str:
    return "%-25.25s" % f"{tok.start[0]}.{tok.start[1]}: {token.tok_name[tok.type]}:{tok.string!r}"


class Tokenizer:
    """Caching wrapper for the tokenize module.

    This is pretty tied to Python's syntax.
    """

    _tokens: List[tokenize.TokenInfo]

    def __init__(
        self, tokengen: Iterator[tokenize.TokenInfo], *, path: str = "", verbose: bool = False
    ):
        self._tokengen = tokengen
        self._tokens = []
        self._index = 0
        self._verbose = verbose
        self._lines: Dict[int, str] = {}
        self._path = path
        if verbose:
            self.report(False, False)

    def getnext(self) -> tokenize.TokenInfo:
        """Return the next token and updates the index."""
        cached = not self._index == len(self._tokens)
        tok = self.peek()
        self._index += 1
        if self._verbose:
            self.report(cached, False)
        return tok

    def peek(self) -> tokenize.TokenInfo:
        """Return the next token *without* updating the index."""
        while self._index == len(self._tokens):
            tok = next(self._tokengen)
            if tok.type in (tokenize.NL, tokenize.COMMENT):
                continue
            if tok.type == token.ERRORTOKEN and tok.string.isspace():
                continue
            if (
                tok.type == token.NEWLINE
                and self._tokens
                and self._tokens[-1].type == token.NEWLINE
            ):
                continue
            self._tokens.append(tok)
            if not self._path:
                self._lines[tok.start[0]] = tok.line
        return self._tokens[self._index]

    def diagnose(self) -> tokenize.TokenInfo:
        if not self._tokens:
            self.getnext()
        return self._tokens[-1]

    def get_last_non_whitespace_token(self) -> tokenize.TokenInfo:
        for tok in reversed(self._tokens[: self._index]):
            if tok.type != tokenize.ENDMARKER and (
                tok.type < tokenize.NEWLINE or tok.type > tokenize.DEDENT
            ):
                break
        return tok

    def get_lines(self, line_numbers: List[int]) -> List[str]:
        """Retrieve source lines corresponding to line numbers."""
        if self._lines:
            lines = self._lines
        else:
            n = len(line_numbers)
            lines = {}
            count = 0
            seen = 0
            with open(self._path) as f:
                for l in f:
                    count += 1
                    if count in line_numbers:
                        seen += 1
                        lines[count] = l
                        if seen == n:
                            break

        return [lines[n] for n in line_numbers]

    def mark(self) -> Mark:
        return self._index

    def reset(self, index: Mark) -> None:
        if index == self._index:
            return
        assert 0 <= index <= len(self._tokens), (index, len(self._tokens))
        old_index = self._index
        self._index = index
        if self._verbose:
            self.report(True, index < old_index)

    def report(self, cached: bool, back: bool) -> None:
        if back:
            fill = "-" * self._index + "-"
        elif cached:
            fill = "-" * self._index + ">"
        else:
            fill = "-" * self._index + "*"
        if self._index == 0:
            print(f"{fill} (Bof)")
        else:
            tok = self._tokens[self._index - 1]
            print(f"{fill} {shorttok(tok)}")

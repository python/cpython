# Parser for C code
# Originally by Mark Shannon (mark@hotpy.org)
# https://gist.github.com/markshannon/db7ab649440b5af765451bb77c7dba34

__all__: list[str] = []

import re
from dataclasses import dataclass
from collections.abc import Iterator


def choice(*opts: str) -> str:
    return "|".join("(%s)" % opt for opt in opts)


# Regexes

# Mapping from operator names to their regular expressions.
operators = {
    # Longer operators must go before shorter ones.
    (PLUSPLUS := "PLUSPLUS"): r'\+\+',
    (MINUSMINUS := "MINUSMINUS"): r"--",
    # ->
    (ARROW := "ARROW"): r"->",
    (ELLIPSIS := "ELLIPSIS"): r"\.\.\.",
    # Assignment operators
    (TIMESEQUAL := "TIMESEQUAL"): r"\*=",
    (DIVEQUAL := "DIVEQUAL"): r"/=",
    (MODEQUAL := "MODEQUAL"): r"%=",
    (PLUSEQUAL := "PLUSEQUAL"): r"\+=",
    (MINUSEQUAL := "MINUSEQUAL"): r"-=",
    (LSHIFTEQUAL := "LSHIFTEQUAL"): r"<<=",
    (RSHIFTEQUAL := "RSHIFTEQUAL"): r">>=",
    (ANDEQUAL := "ANDEQUAL"): r"&=",
    (OREQUAL := "OREQUAL"): r"\|=",
    (XOREQUAL := "XOREQUAL"): r"\^=",
    # Operators
    (PLUS := "PLUS"): r"\+",
    (MINUS := "MINUS"): r"-",
    (TIMES := "TIMES"): r"\*",
    (DIVIDE := "DIVIDE"): r"/",
    (MOD := "MOD"): r"%",
    (NOT := "NOT"): r"~",
    (XOR := "XOR"): r"\^",
    (LOR := "LOR"): r"\|\|",
    (LAND := "LAND"): r"&&",
    (LSHIFT := "LSHIFT"): r"<<",
    (RSHIFT := "RSHIFT"): r">>",
    (LE := "LE"): r"<=",
    (GE := "GE"): r">=",
    (EQ := "EQ"): r"==",
    (NE := "NE"): r"!=",
    (LT := "LT"): r"<",
    (GT := "GT"): r">",
    (LNOT := "LNOT"): r"!",
    (OR := "OR"): r"\|",
    (AND := "AND"): r"&",
    (EQUALS := "EQUALS"): r"=",
    # ?
    (CONDOP := "CONDOP"): r"\?",
    # Delimiters
    (LPAREN := "LPAREN"): r"\(",
    (RPAREN := "RPAREN"): r"\)",
    (LBRACKET := "LBRACKET"): r"\[",
    (RBRACKET := "RBRACKET"): r"\]",
    (LBRACE := "LBRACE"): r"\{",
    (RBRACE := "RBRACE"): r"\}",
    (COMMA := "COMMA"): r",",
    (PERIOD := "PERIOD"): r"\.",
    (SEMI := "SEMI"): r";",
    (COLON := "COLON"): r":",
    (BACKSLASH := "BACKSLASH"): r"\\",
}
__all__.extend(operators.keys())
opmap = {pattern.replace("\\", "") or "\\": opname
         for opname, pattern in operators.items()}
del opname, pattern

# Macros
macro = r"#.*\n"
CMACRO = "CMACRO"
__all__.append(CMACRO)

id_re = r"[a-zA-Z_][0-9a-zA-Z_]*"
IDENTIFIER = "IDENTIFIER"
__all__.append(IDENTIFIER)

suffix = r"([uU]?[lL]?[lL]?)"
octal = r"0[0-7]+" + suffix
hex = r"0[xX][0-9a-fA-F]+"
decimal_digits = r"(0|[1-9][0-9]*)"
decimal = decimal_digits + suffix

exponent = r"""([eE][-+]?[0-9]+)"""
fraction = r"""([0-9]*\.[0-9]+)|([0-9]+\.)"""
float = "((((" + fraction + ")" + exponent + "?)|([0-9]+" + exponent + "))[FfLl]?)"

number_re = choice(octal, hex, float, decimal)
NUMBER = "NUMBER"
__all__.append(NUMBER)

simple_escape = r"""([a-zA-Z._~!=&\^\-\\?'"])"""
decimal_escape = r"""(\d+)"""
hex_escape = r"""(x[0-9a-fA-F]+)"""
escape_sequence = (
    r"""(\\(""" + simple_escape + "|" + decimal_escape + "|" + hex_escape + "))"
)
string_char = r"""([^"\\\n]|""" + escape_sequence + ")"
str_re = '"' + string_char + '*"'
STRING = "STRING"
char = r"\'.\'"  # TODO: escape sequence
CHARACTER = "CHARACTER"
__all__.extend([STRING, CHARACTER])

comment_re = r"(//.*)|/\*([^*]|\*[^/])*\*/"
COMMENT = "COMMENT"
__all__.append(COMMENT)

newline = r"\n"
NEWLINE = "NEWLINE"
__all__.append(NEWLINE)

invalid = (
    r"\S"  # A single non-space character that's not caught by any of the other patterns
)
matcher = re.compile(
    choice(
        id_re,
        number_re,
        str_re,
        char,
        newline,
        macro,
        comment_re,
        *operators.values(),
        invalid,
    )
)
letter = re.compile(r"[a-zA-Z_]")

# Mapping from keyword to their token kind.
keywords = {
    'auto': (AUTO := "AUTO"),
    'break': (BREAK := "BREAK"),
    'case': (CASE := "CASE"),
    'char': (CHAR := "CHAR"),
    'const': (CONST := "CONST"),
    'continue': (CONTINUE := "CONTINUE"),
    'default': (DEFAULT := "DEFAULT"),
    'do': (DO := "DO"),
    'double': (DOUBLE := "DOUBLE"),
    'else': (ELSE := "ELSE"),
    'enum': (ENUM := "ENUM"),
    'extern': (EXTERN := "EXTERN"),
    'float': (FLOAT := "FLOAT"),
    'for': (FOR := "FOR"),
    'goto': (GOTO := "GOTO"),
    'if': (IF := "IF"),
    'inline': (INLINE := "INLINE"),
    'int': (INT := "INT"),
    'long': (LONG := "LONG"),
    'offsetof': (OFFSETOF := "OFFSETOF"),
    'return': (RETURN := "RETURN"),
    'short': (SHORT := "SHORT"),
    'signed': (SIGNED := "SIGNED"),
    'sizeof': (SIZEOF := "SIZEOF"),
    'static': (STATIC := "STATIC"),
    'struct': (STRUCT := "STRUCT"),
    'switch': (SWITCH := "SWITCH"),
    'typedef': (TYPEDEF := "TYPEDEF"),
    'union': (UNION := "UNION"),
    'unsigned': (UNSIGNED := "UNSIGNED"),
    'void': (VOID := "VOID"),
    'volatile': (VOLATILE := "VOLATILE"),
    'while': (WHILE := "WHILE"),
    # An instruction in the DSL.
    'inst': (INST := "INST"),
    # A micro-op in the DSL.
    'op': (OP := "OP"),
    # A macro in the DSL.
    'macro': (MACRO := "MACRO"),
}
__all__.extend(keywords.values())
KEYWORD = 'KEYWORD'

ANNOTATION = "ANNOTATION"
annotations = {
    ANN_SPECIALIZING := "specializing",
    ANN_OVERRIDE := "override",
    ANN_REGISTER := "register",
    ANN_REPLACED := "replaced",
    ANN_PURE := "pure",
    ANN_SPLIT := "split",
    ANN_REPLICATE := "replicate",
    ANN_TIER_1 := "tier1",
    ANN_TIER_2 := "tier2",
}


def make_syntax_error(
    message: str,
    filename: str | None,
    line: int,
    column: int,
    line_text: str,
) -> SyntaxError:
    return SyntaxError(message, (filename, line, column, line_text))


@dataclass(slots=True, frozen=True)
class Token:
    filename: str
    kind: str
    text: str
    begin: tuple[int, int]
    end: tuple[int, int]

    @property
    def line(self) -> int:
        return self.begin[0]

    @property
    def column(self) -> int:
        return self.begin[1]

    @property
    def end_line(self) -> int:
        return self.end[0]

    @property
    def end_column(self) -> int:
        return self.end[1]

    @property
    def width(self) -> int:
        return self.end[1] - self.begin[1]

    def replaceText(self, txt: str) -> "Token":
        assert isinstance(txt, str)
        return Token(self.filename, self.kind, txt, self.begin, self.end)

    def __repr__(self) -> str:
        b0, b1 = self.begin
        e0, e1 = self.end
        if b0 == e0:
            return f"{self.kind}({self.text!r}, {b0}:{b1}:{e1})"
        else:
            return f"{self.kind}({self.text!r}, {b0}:{b1}, {e0}:{e1})"


def tokenize(src: str, line: int = 1, filename: str = "") -> Iterator[Token]:
    linestart = -1
    for m in matcher.finditer(src):
        start, end = m.span()
        text = m.group(0)
        if text in keywords:
            kind = keywords[text]
        elif text in annotations:
            kind = ANNOTATION
        elif letter.match(text):
            kind = IDENTIFIER
        elif text == "...":
            kind = ELLIPSIS
        elif text == ".":
            kind = PERIOD
        elif text[0] in "0123456789.":
            kind = NUMBER
        elif text[0] == '"':
            kind = STRING
        elif text in opmap:
            kind = opmap[text]
        elif text == "\n":
            linestart = start
            line += 1
            kind = NEWLINE
        elif text[0] == "'":
            kind = CHARACTER
        elif text[0] == "#":
            kind = CMACRO
        elif text[0] == "/" and text[1] in "/*":
            kind = COMMENT
        else:
            lineend = src.find("\n", start)
            if lineend == -1:
                lineend = len(src)
            raise make_syntax_error(
                f"Bad token: {text}",
                filename,
                line,
                start - linestart + 1,
                src[linestart:lineend],
            )
        if kind == COMMENT:
            begin = line, start - linestart
            newlines = text.count("\n")
            if newlines:
                linestart = start + text.rfind("\n")
                line += newlines
        else:
            begin = line, start - linestart
            if kind == CMACRO:
                linestart = end
                line += 1
        if kind != NEWLINE:
            yield Token(
                filename, kind, text, begin, (line, start - linestart + len(text))
            )


def to_text(tkns: list[Token], dedent: int = 0) -> str:
    res: list[str] = []
    line, col = -1, 1 + dedent
    for tkn in tkns:
        if line == -1:
            line, _ = tkn.begin
        l, c = tkn.begin
        # assert(l >= line), (line, txt, start, end)
        while l > line:
            line += 1
            res.append("\n")
            col = 1 + dedent
        res.append(" " * (c - col))
        text = tkn.text
        if dedent != 0 and tkn.kind == COMMENT and "\n" in text:
            if dedent < 0:
                text = text.replace("\n", "\n" + " " * -dedent)
            # TODO: dedent > 0
        res.append(text)
        line, col = tkn.end
    return "".join(res)


if __name__ == "__main__":
    import sys

    filename = sys.argv[1]
    if filename == "-c":
        src = sys.argv[2]
    else:
        src = open(filename).read()
    # print(to_text(tokenize(src)))
    for tkn in tokenize(src, filename=filename):
        print(tkn)

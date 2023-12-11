# Parser for C code
# Originally by Mark Shannon (mark@hotpy.org)
# https://gist.github.com/markshannon/db7ab649440b5af765451bb77c7dba34

import re
from dataclasses import dataclass
from collections.abc import Iterator


def choice(*opts: str) -> str:
    return "|".join("(%s)" % opt for opt in opts)


# Regexes

# Longer operators must go before shorter ones.

PLUSPLUS = r"\+\+"
MINUSMINUS = r"--"

# ->
ARROW = r"->"
ELLIPSIS = r"\.\.\."

# Assignment operators
TIMESEQUAL = r"\*="
DIVEQUAL = r"/="
MODEQUAL = r"%="
PLUSEQUAL = r"\+="
MINUSEQUAL = r"-="
LSHIFTEQUAL = r"<<="
RSHIFTEQUAL = r">>="
ANDEQUAL = r"&="
OREQUAL = r"\|="
XOREQUAL = r"\^="

# Operators
PLUS = r"\+"
MINUS = r"-"
TIMES = r"\*"
DIVIDE = r"/"
MOD = r"%"
NOT = r"~"
XOR = r"\^"
LOR = r"\|\|"
LAND = r"&&"
LSHIFT = r"<<"
RSHIFT = r">>"
LE = r"<="
GE = r">="
EQ = r"=="
NE = r"!="
LT = r"<"
GT = r">"
LNOT = r"!"
OR = r"\|"
AND = r"&"
EQUALS = r"="

# ?
CONDOP = r"\?"

# Delimiters
LPAREN = r"\("
RPAREN = r"\)"
LBRACKET = r"\["
RBRACKET = r"\]"
LBRACE = r"\{"
RBRACE = r"\}"
COMMA = r","
PERIOD = r"\."
SEMI = r";"
COLON = r":"
BACKSLASH = r"\\"

operators = {op: pattern for op, pattern in globals().items() if op == op.upper()}
for op in operators:
    globals()[op] = op
opmap = {pattern.replace("\\", "") or "\\": op for op, pattern in operators.items()}

# Macros
macro = r"# *(ifdef|ifndef|undef|define|error|endif|if|else|include|#)"
CMACRO = "CMACRO"

id_re = r"[a-zA-Z_][0-9a-zA-Z_]*"
IDENTIFIER = "IDENTIFIER"


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

comment_re = r"(//.*)|/\*([^*]|\*[^/])*\*/"
COMMENT = "COMMENT"

newline = r"\n"
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


kwds = []
AUTO = "AUTO"
kwds.append(AUTO)
BREAK = "BREAK"
kwds.append(BREAK)
CASE = "CASE"
kwds.append(CASE)
CHAR = "CHAR"
kwds.append(CHAR)
CONST = "CONST"
kwds.append(CONST)
CONTINUE = "CONTINUE"
kwds.append(CONTINUE)
DEFAULT = "DEFAULT"
kwds.append(DEFAULT)
DO = "DO"
kwds.append(DO)
DOUBLE = "DOUBLE"
kwds.append(DOUBLE)
ELSE = "ELSE"
kwds.append(ELSE)
ENUM = "ENUM"
kwds.append(ENUM)
EXTERN = "EXTERN"
kwds.append(EXTERN)
FLOAT = "FLOAT"
kwds.append(FLOAT)
FOR = "FOR"
kwds.append(FOR)
GOTO = "GOTO"
kwds.append(GOTO)
IF = "IF"
kwds.append(IF)
INLINE = "INLINE"
kwds.append(INLINE)
INT = "INT"
kwds.append(INT)
LONG = "LONG"
kwds.append(LONG)
OFFSETOF = "OFFSETOF"
kwds.append(OFFSETOF)
RESTRICT = "RESTRICT"
kwds.append(RESTRICT)
RETURN = "RETURN"
kwds.append(RETURN)
SHORT = "SHORT"
kwds.append(SHORT)
SIGNED = "SIGNED"
kwds.append(SIGNED)
SIZEOF = "SIZEOF"
kwds.append(SIZEOF)
STATIC = "STATIC"
kwds.append(STATIC)
STRUCT = "STRUCT"
kwds.append(STRUCT)
SWITCH = "SWITCH"
kwds.append(SWITCH)
TYPEDEF = "TYPEDEF"
kwds.append(TYPEDEF)
UNION = "UNION"
kwds.append(UNION)
UNSIGNED = "UNSIGNED"
kwds.append(UNSIGNED)
VOID = "VOID"
kwds.append(VOID)
VOLATILE = "VOLATILE"
kwds.append(VOLATILE)
WHILE = "WHILE"
kwds.append(WHILE)
# An instruction in the DSL
INST = "INST"
kwds.append(INST)
# A micro-op in the DSL
OP = "OP"
kwds.append(OP)
# A macro in the DSL
MACRO = "MACRO"
kwds.append(MACRO)
keywords = {name.lower(): name for name in kwds}

ANNOTATION = "ANNOTATION"
annotations = {"specializing", "guard", "override", "register", "replaced"}

__all__ = []
__all__.extend(kwds)


def make_syntax_error(
    message: str,
    filename: str | None,
    line: int,
    column: int,
    line_text: str,
) -> SyntaxError:
    return SyntaxError(message, (filename, line, column, line_text))


@dataclass(slots=True)
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
            kind = "\n"
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
        if kind != "\n":
            yield Token(filename, kind, text, begin, (line, start - linestart + len(text)))


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
        if dedent != 0 and tkn.kind == "COMMENT" and "\n" in text:
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

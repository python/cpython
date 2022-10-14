
import re
import sys
import collections

def choice(*opts):
    return "|".join("(%s)" % opt for opt in opts)

# Regexes

#Longer operators must go before shorter ones.

PLUSPLUS = r'\+\+'
MINUSMINUS = r'--'

# ->
ARROW = r'->'
ELLIPSIS = r'\.\.\.'

# Operators
PLUS = r'\+'
MINUS = r'-'
TIMES = r'\*'
DIVIDE = r'/'
MOD = r'%'
OR = r'\|'
AND = r'&'
NOT = r'~'
XOR = r'\^'
LSHIFT = r'<<'
RSHIFT = r'>>'
LOR = r'\|\|'
LAND = r'&&'
LNOT = r'!'
LT = r'<'
GT = r'>'
LE = r'<='
GE = r'>='
EQ = r'=='
NE = r'!='

# Assignment operators
EQUALS = r'='
TIMESEQUAL = r'\*='
DIVEQUAL = r'/='
MODEQUAL = r'%='
PLUSEQUAL = r'\+='
MINUSEQUAL = r'-='
LSHIFTEQUAL = r'<<='
RSHIFTEQUAL = r'>>='
ANDEQUAL = r'&='
OREQUAL = r'\|='
XOREQUAL = r'\^='

# ?
CONDOP = r'\?'

# Delimeters
LPAREN = r'\('
RPAREN = r'\)'
LBRACKET = r'\['
RBRACKET = r'\]'
LBRACE = r'\{'
RBRACE = r'\}'
COMMA = r','
PERIOD = r'\.'
SEMI = r';'
COLON = r':'
BACKSLASH = r'\\'

operators = { op: pattern for op, pattern in globals().items() if op == op.upper() }
for op in operators:
    globals()[op] = op
opmap = { pattern.replace("\\", "") or '\\' : op for op, pattern in operators.items() }
    
#Macros
macro = r'# *(ifdef|ifndef|undef|define|error|endif|if|else|include|#)'
MACRO = 'MACRO'

id_re = r'[a-zA-Z_][0-9a-zA-Z_]*'
IDENTIFIER = 'IDENTIFIER'

suffix = r'([uU]?[lL]?[lL]?)'
octal = '0[0-7]*'+suffix
hex = r'0[xX][0-9a-fA-F]+'
decimal_digits = r'(0|[1-9][0-9]*)'
decimal = decimal_digits + suffix


exponent = r"""([eE][-+]?[0-9]+)"""
fraction = r"""([0-9]*\.[0-9]+)|([0-9]+\.)"""
float = '(((('+fraction+')'+exponent+'?)|([0-9]+'+exponent+'))[FfLl]?)'

number_re = choice(float, octal, hex, decimal)
NUMBER = 'NUMBER'

simple_escape = r"""([a-zA-Z._~!=&\^\-\\?'"])"""
decimal_escape = r"""(\d+)"""
hex_escape = r"""(x[0-9a-fA-F]+)"""
escape_sequence = r"""(\\("""+simple_escape+'|'+decimal_escape+'|'+hex_escape+'))'
string_char = r"""([^"\\\n]|"""+escape_sequence+')'
str_re = '"'+string_char+'*"'
STRING = 'STRING'
char = r'\'.\''
CHAR = 'CHAR'

comment_re = r'//.*|/\*([^*]|\*[^/])*\*/'
COMMENT = 'COMMENT'

newline = r"\n"
matcher = re.compile(choice(id_re, number_re, str_re, char, newline, macro, comment_re, *operators.values()))
letter = re.compile(r'[a-zA-Z_]')

keywords = (
    'AUTO', 'BREAK', 'CASE', 'CHAR', 'CONST',
    'CONTINUE', 'DEFAULT', 'DO', 'DOUBLE', 'ELSE', 'ENUM', 'EXTERN',
    'FLOAT', 'FOR', 'GOTO', 'IF', 'INLINE', 'INT', 'LONG',
    'REGISTER', 'OFFSETOF',
    'RESTRICT', 'RETURN', 'SHORT', 'SIGNED', 'SIZEOF', 'STATIC', 'STRUCT',
    'SWITCH', 'TYPEDEF', 'UNION', 'UNSIGNED', 'VOID',
    'VOLATILE', 'WHILE'
)
for name in keywords:
    globals()[name] = name
keywords = { name.lower() : name for name in keywords }

_Token = collections.namedtuple("Token", ("kind", "text", "begin", "end"))

class Token:

    __slots__ = "kind", "text", "begin", "end"

    def __init__(self, kind, text, begin, end):
        assert isinstance(text, str)
        self.kind = kind
        self.text = text
        self.begin = begin
        self.end = end

    @property
    def line(self):
        return self.begin[0]

    @property
    def column(self):
        return self.begin[1]

    @property
    def end_line(self):
        return self.end[0]
 
    @property
    def end_column(self):
        return self.end[1]
 
    @property
    def width(self):
        return self.end[1] - self.begin[1]

    def replaceText(self, txt):
        assert isinstance(txt, str)
        return Token(self.kind, txt, self.begin, self.end)

    def __str__(self):
        return f"Token({self.kind}, {self.text}, {self.begin}, {self.end})"

def tokenize(src, line=1):
    linestart = 0
    for m in matcher.finditer(src):
        start, end = m.span()
        text = m.group(0)
        if text in keywords:
            kind = keywords[text]
        elif letter.match(text):
            kind = IDENTIFIER
        elif text[0] in '0123456789.':
            kind = NUMBER
        elif text[0] == '"':
            kind = STRING
        elif text in opmap:
            kind = opmap[text]
        elif text == '\n':
            linestart = start
            line += 1
            kind = '\n'
        elif text[0] == "'":
            kind = CHAR
        elif text[0] == '#':
            kind = MACRO
        elif text[0] == '/' and text[1] in '/*':
            kind = COMMENT
        else:
            raise SyntaxError(text)
        if kind == COMMENT:
            begin = line, start-linestart
            newlines = text.count('\n')
            if newlines:
                linestart = start + text.rfind('\n')
                line += newlines
        else:
            begin = line, start-linestart
        if kind != "\n":
            yield Token(kind, text, begin, (line, start-linestart+len(text)))

__all__ = []
__all__.extend([kind for kind in globals() if kind.upper() == kind])

def to_text(tkns, dedent=0):
    res = []
    line, col = -1, 1+dedent
    for tkn in tkns:
        if line == -1:
            line, _ = tkn.begin
        l, c = tkn.begin
        #assert(l >= line), (line, txt, start, end)
        while l > line:
            line += 1
            res.append('\n')
            col = 1+dedent
        res.append(' '*(c-col))
        res.append(tkn.text)
        line, col = tkn.end
    return ''.join(res)

if __name__ == "__main__":
    import sys
    src = open(sys.argv[1]).read()
    #print(to_text(tokenize(src)))
    for tkn in tokenize(src):
        print(tkn)


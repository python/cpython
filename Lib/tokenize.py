"""Tokenization help for Python programs.

tokenize(readline) is a generator that breaks a stream of
bytes into Python tokens. It decodes the bytes according to
PEP-0263 for determining source file encoding.

It accepts a readline-like method which is called
repeatedly to get the next line of input (or b"" for EOF).  It generates
5-tuples with these members:

    the token type (see token.py)
    the token (a string)
    the starting (row, column) indices of the token (a 2-tuple of ints)
    the ending (row, column) indices of the token (a 2-tuple of ints)
    the original line (string)

It is designed to match the working of the Python tokenizer exactly, except
that it produces COMMENT tokens for comments and gives type OP for all
operators. Aditionally, all token lists start with an ENCODING token
which tells you which encoding was used to decode the bytes stream."""

__author__ = 'Ka-Ping Yee <ping@lfw.org>'
__credits__ = ('GvR, ESR, Tim Peters, Thomas Wouters, Fred Drake, '
               'Skip Montanaro, Raymond Hettinger, Trent Nelson, '
               'Michael Foord')

import re, string, sys
from token import *
from codecs import lookup
from itertools import chain, repeat
cookie_re = re.compile("coding[:=]\s*([-\w.]+)")

import token
__all__ = [x for x in dir(token) if x[0] != '_'] + ["COMMENT", "tokenize",
           "detect_encoding", "NL", "untokenize", "ENCODING"]
del token

COMMENT = N_TOKENS
tok_name[COMMENT] = 'COMMENT'
NL = N_TOKENS + 1
tok_name[NL] = 'NL'
ENCODING = N_TOKENS + 2
tok_name[ENCODING] = 'ENCODING'
N_TOKENS += 3

def group(*choices): return '(' + '|'.join(choices) + ')'
def any(*choices): return group(*choices) + '*'
def maybe(*choices): return group(*choices) + '?'

Whitespace = r'[ \f\t]*'
Comment = r'#[^\r\n]*'
Ignore = Whitespace + any(r'\\\r?\n' + Whitespace) + maybe(Comment)
Name = r'[a-zA-Z_]\w*'

Hexnumber = r'0[xX][\da-fA-F]+'
Binnumber = r'0[bB][01]+'
Octnumber = r'0[oO][0-7]+'
Decnumber = r'(?:0+|[1-9]\d*)'
Intnumber = group(Hexnumber, Binnumber, Octnumber, Decnumber)
Exponent = r'[eE][-+]?\d+'
Pointfloat = group(r'\d+\.\d*', r'\.\d+') + maybe(Exponent)
Expfloat = r'\d+' + Exponent
Floatnumber = group(Pointfloat, Expfloat)
Imagnumber = group(r'\d+[jJ]', Floatnumber + r'[jJ]')
Number = group(Imagnumber, Floatnumber, Intnumber)

# Tail end of ' string.
Single = r"[^'\\]*(?:\\.[^'\\]*)*'"
# Tail end of " string.
Double = r'[^"\\]*(?:\\.[^"\\]*)*"'
# Tail end of ''' string.
Single3 = r"[^'\\]*(?:(?:\\.|'(?!''))[^'\\]*)*'''"
# Tail end of """ string.
Double3 = r'[^"\\]*(?:(?:\\.|"(?!""))[^"\\]*)*"""'
Triple = group("[bB]?[rR]?'''", '[bB]?[rR]?"""')
# Single-line ' or " string.
String = group(r"[bB]?[rR]?'[^\n'\\]*(?:\\.[^\n'\\]*)*'",
               r'[bB]?[rR]?"[^\n"\\]*(?:\\.[^\n"\\]*)*"')

# Because of leftmost-then-longest match semantics, be sure to put the
# longest operators first (e.g., if = came before ==, == would get
# recognized as two instances of =).
Operator = group(r"\*\*=?", r">>=?", r"<<=?", r"!=",
                 r"//=?", r"->",
                 r"[+\-*/%&|^=<>]=?",
                 r"~")

Bracket = '[][(){}]'
Special = group(r'\r?\n', r'\.\.\.', r'[:;.,@]')
Funny = group(Operator, Bracket, Special)

PlainToken = group(Number, Funny, String, Name)
Token = Ignore + PlainToken

# First (or only) line of ' or " string.
ContStr = group(r"[bB]?[rR]?'[^\n'\\]*(?:\\.[^\n'\\]*)*" +
                group("'", r'\\\r?\n'),
                r'[bB]?[rR]?"[^\n"\\]*(?:\\.[^\n"\\]*)*' +
                group('"', r'\\\r?\n'))
PseudoExtras = group(r'\\\r?\n', Comment, Triple)
PseudoToken = Whitespace + group(PseudoExtras, Number, Funny, ContStr, Name)

tokenprog, pseudoprog, single3prog, double3prog = map(
    re.compile, (Token, PseudoToken, Single3, Double3))
endprogs = {"'": re.compile(Single), '"': re.compile(Double),
            "'''": single3prog, '"""': double3prog,
            "r'''": single3prog, 'r"""': double3prog,
            "b'''": single3prog, 'b"""': double3prog,
            "br'''": single3prog, 'br"""': double3prog,
            "R'''": single3prog, 'R"""': double3prog,
            "B'''": single3prog, 'B"""': double3prog,
            "bR'''": single3prog, 'bR"""': double3prog,
            "Br'''": single3prog, 'Br"""': double3prog,
            "BR'''": single3prog, 'BR"""': double3prog,
            'r': None, 'R': None, 'b': None, 'B': None}

triple_quoted = {}
for t in ("'''", '"""',
          "r'''", 'r"""', "R'''", 'R"""',
          "b'''", 'b"""', "B'''", 'B"""',
          "br'''", 'br"""', "Br'''", 'Br"""',
          "bR'''", 'bR"""', "BR'''", 'BR"""'):
    triple_quoted[t] = t
single_quoted = {}
for t in ("'", '"',
          "r'", 'r"', "R'", 'R"',
          "b'", 'b"', "B'", 'B"',
          "br'", 'br"', "Br'", 'Br"',
          "bR'", 'bR"', "BR'", 'BR"' ):
    single_quoted[t] = t

tabsize = 8

class TokenError(Exception): pass

class StopTokenizing(Exception): pass


class Untokenizer:

    def __init__(self):
        self.tokens = []
        self.prev_row = 1
        self.prev_col = 0
        self.encoding = None

    def add_whitespace(self, start):
        row, col = start
        assert row <= self.prev_row
        col_offset = col - self.prev_col
        if col_offset:
            self.tokens.append(" " * col_offset)

    def untokenize(self, iterable):
        for t in iterable:
            if len(t) == 2:
                self.compat(t, iterable)
                break
            tok_type, token, start, end, line = t
            if tok_type == ENCODING:
                self.encoding = token
                continue
            self.add_whitespace(start)
            self.tokens.append(token)
            self.prev_row, self.prev_col = end
            if tok_type in (NEWLINE, NL):
                self.prev_row += 1
                self.prev_col = 0
        return "".join(self.tokens)

    def compat(self, token, iterable):
        startline = False
        indents = []
        toks_append = self.tokens.append
        toknum, tokval = token

        if toknum in (NAME, NUMBER):
            tokval += ' '
        if toknum in (NEWLINE, NL):
            startline = True
        prevstring = False
        for tok in iterable:
            toknum, tokval = tok[:2]
            if toknum == ENCODING:
                self.encoding = tokval
                continue

            if toknum in (NAME, NUMBER):
                tokval += ' '

            # Insert a space between two consecutive strings
            if toknum == STRING:
                if prevstring:
                    tokval = ' ' + tokval
                prevstring = True
            else:
                prevstring = False

            if toknum == INDENT:
                indents.append(tokval)
                continue
            elif toknum == DEDENT:
                indents.pop()
                continue
            elif toknum in (NEWLINE, NL):
                startline = True
            elif startline and indents:
                toks_append(indents[-1])
                startline = False
            toks_append(tokval)


def untokenize(iterable):
    """Transform tokens back into Python source code.
    It returns a bytes object, encoded using the ENCODING
    token, which is the first token sequence output by tokenize.

    Each element returned by the iterable must be a token sequence
    with at least two elements, a token number and token value.  If
    only two tokens are passed, the resulting output is poor.

    Round-trip invariant for full input:
        Untokenized source will match input source exactly

    Round-trip invariant for limited intput:
        # Output bytes will tokenize the back to the input
        t1 = [tok[:2] for tok in tokenize(f.readline)]
        newcode = untokenize(t1)
        readline = BytesIO(newcode).readline
        t2 = [tok[:2] for tok in tokenize(readline)]
        assert t1 == t2
    """
    ut = Untokenizer()
    out = ut.untokenize(iterable)
    if ut.encoding is not None:
        out = out.encode(ut.encoding)
    return out


def detect_encoding(readline):
    """
    The detect_encoding() function is used to detect the encoding that should
    be used to decode a Python source file. It requires one argment, readline,
    in the same way as the tokenize() generator.

    It will call readline a maximum of twice, and return the encoding used
    (as a string) and a list of any lines (left as bytes) it has read
    in.

    It detects the encoding from the presence of a utf-8 bom or an encoding
    cookie as specified in pep-0263. If both a bom and a cookie are present,
    but disagree, a SyntaxError will be raised.

    If no encoding is specified, then the default of 'utf-8' will be returned.
    """
    utf8_bom = b'\xef\xbb\xbf'
    bom_found = False
    encoding = None
    def read_or_stop():
        try:
            return readline()
        except StopIteration:
            return b''

    def find_cookie(line):
        try:
            line_string = line.decode('ascii')
        except UnicodeDecodeError:
            pass
        else:
            matches = cookie_re.findall(line_string)
            if matches:
                encoding = matches[0]
                if bom_found and lookup(encoding).name != 'utf-8':
                    # This behaviour mimics the Python interpreter
                    raise SyntaxError('encoding problem: utf-8')
                return encoding

    first = read_or_stop()
    if first.startswith(utf8_bom):
        bom_found = True
        first = first[3:]
    if not first:
        return 'utf-8', []

    encoding = find_cookie(first)
    if encoding:
        return encoding, [first]

    second = read_or_stop()
    if not second:
        return 'utf-8', [first]

    encoding = find_cookie(second)
    if encoding:
        return encoding, [first, second]

    return 'utf-8', [first, second]


def tokenize(readline):
    """
    The tokenize() generator requires one argment, readline, which
    must be a callable object which provides the same interface as the
    readline() method of built-in file objects. Each call to the function
    should return one line of input as bytes.  Alternately, readline
    can be a callable function terminating with StopIteration:
        readline = open(myfile, 'rb').__next__  # Example of alternate readline

    The generator produces 5-tuples with these members: the token type; the
    token string; a 2-tuple (srow, scol) of ints specifying the row and
    column where the token begins in the source; a 2-tuple (erow, ecol) of
    ints specifying the row and column where the token ends in the source;
    and the line on which the token was found. The line passed is the
    logical line; continuation lines are included.

    The first token sequence will always be an ENCODING token
    which tells you which encoding was used to decode the bytes stream.
    """
    encoding, consumed = detect_encoding(readline)
    def readline_generator():
        while True:
            try:
                yield readline()
            except StopIteration:
                return
    chained = chain(consumed, readline_generator())
    return _tokenize(chained.__next__, encoding)


def _tokenize(readline, encoding):
    lnum = parenlev = continued = 0
    namechars, numchars = string.ascii_letters + '_', '0123456789'
    contstr, needcont = '', 0
    contline = None
    indents = [0]

    if encoding is not None:
        yield (ENCODING, encoding, (0, 0), (0, 0), '')
    while 1:                                   # loop over lines in stream
        try:
            line = readline()
        except StopIteration:
            line = b''

        if encoding is not None:
            line = line.decode(encoding)
        lnum = lnum + 1
        pos, max = 0, len(line)

        if contstr:                            # continued string
            if not line:
                raise TokenError("EOF in multi-line string", strstart)
            endmatch = endprog.match(line)
            if endmatch:
                pos = end = endmatch.end(0)
                yield (STRING, contstr + line[:end],
                       strstart, (lnum, end), contline + line)
                contstr, needcont = '', 0
                contline = None
            elif needcont and line[-2:] != '\\\n' and line[-3:] != '\\\r\n':
                yield (ERRORTOKEN, contstr + line,
                           strstart, (lnum, len(line)), contline)
                contstr = ''
                contline = None
                continue
            else:
                contstr = contstr + line
                contline = contline + line
                continue

        elif parenlev == 0 and not continued:  # new statement
            if not line: break
            column = 0
            while pos < max:                   # measure leading whitespace
                if line[pos] == ' ': column = column + 1
                elif line[pos] == '\t': column = (column/tabsize + 1)*tabsize
                elif line[pos] == '\f': column = 0
                else: break
                pos = pos + 1
            if pos == max: break

            if line[pos] in '#\r\n':           # skip comments or blank lines
                if line[pos] == '#':
                    comment_token = line[pos:].rstrip('\r\n')
                    nl_pos = pos + len(comment_token)
                    yield (COMMENT, comment_token,
                           (lnum, pos), (lnum, pos + len(comment_token)), line)
                    yield (NL, line[nl_pos:],
                           (lnum, nl_pos), (lnum, len(line)), line)
                else:
                    yield ((NL, COMMENT)[line[pos] == '#'], line[pos:],
                           (lnum, pos), (lnum, len(line)), line)
                continue

            if column > indents[-1]:           # count indents or dedents
                indents.append(column)
                yield (INDENT, line[:pos], (lnum, 0), (lnum, pos), line)
            while column < indents[-1]:
                if column not in indents:
                    raise IndentationError(
                        "unindent does not match any outer indentation level",
                        ("<tokenize>", lnum, pos, line))
                indents = indents[:-1]
                yield (DEDENT, '', (lnum, pos), (lnum, pos), line)

        else:                                  # continued statement
            if not line:
                raise TokenError("EOF in multi-line statement", (lnum, 0))
            continued = 0

        while pos < max:
            pseudomatch = pseudoprog.match(line, pos)
            if pseudomatch:                                # scan for tokens
                start, end = pseudomatch.span(1)
                spos, epos, pos = (lnum, start), (lnum, end), end
                token, initial = line[start:end], line[start]

                if (initial in numchars or                  # ordinary number
                    (initial == '.' and token != '.' and token != '...')):
                    yield (NUMBER, token, spos, epos, line)
                elif initial in '\r\n':
                    yield (NL if parenlev > 0 else NEWLINE,
                           token, spos, epos, line)
                elif initial == '#':
                    assert not token.endswith("\n")
                    yield (COMMENT, token, spos, epos, line)
                elif token in triple_quoted:
                    endprog = endprogs[token]
                    endmatch = endprog.match(line, pos)
                    if endmatch:                           # all on one line
                        pos = endmatch.end(0)
                        token = line[start:pos]
                        yield (STRING, token, spos, (lnum, pos), line)
                    else:
                        strstart = (lnum, start)           # multiple lines
                        contstr = line[start:]
                        contline = line
                        break
                elif initial in single_quoted or \
                    token[:2] in single_quoted or \
                    token[:3] in single_quoted:
                    if token[-1] == '\n':                  # continued string
                        strstart = (lnum, start)
                        endprog = (endprogs[initial] or endprogs[token[1]] or
                                   endprogs[token[2]])
                        contstr, needcont = line[start:], 1
                        contline = line
                        break
                    else:                                  # ordinary string
                        yield (STRING, token, spos, epos, line)
                elif initial in namechars:                 # ordinary name
                    yield (NAME, token, spos, epos, line)
                elif initial == '\\':                      # continued stmt
                    continued = 1
                else:
                    if initial in '([{': parenlev = parenlev + 1
                    elif initial in ')]}': parenlev = parenlev - 1
                    yield (OP, token, spos, epos, line)
            else:
                yield (ERRORTOKEN, line[pos],
                           (lnum, pos), (lnum, pos+1), line)
                pos = pos + 1

    for indent in indents[1:]:                 # pop remaining indent levels
        yield (DEDENT, '', (lnum, 0), (lnum, 0), '')
    yield (ENDMARKER, '', (lnum, 0), (lnum, 0), '')


# An undocumented, backwards compatible, API for all the places in the standard
# library that expect to be able to use tokenize with strings
def generate_tokens(readline):
    return _tokenize(readline, None)

"""Tokenization help for Python programs.

This module exports a function called 'tokenize()' that breaks a stream of
text into Python tokens.  It accepts a readline-like method which is called
repeatedly to get the next line of input (or "" for EOF) and a "token-eater"
function which is called once for each token found.  The latter function is
passed the token type, a string containing the token, the starting and
ending (row, column) coordinates of the token, and the original line.  It is
designed to match the working of the Python tokenizer exactly, except that
it produces COMMENT tokens for comments and gives type OP for all operators.

For compatibility with the older 'tokenize' module, this also compiles a
regular expression into 'tokenprog' that matches Python tokens in individual
lines of text, leaving the token in 'tokenprog.group(3)', but does not
handle indentation, continuations, or multi-line strings."""

__version__ = "Ka-Ping Yee, 26 March 1997"

import string, regex
from token import *

COMMENT = N_TOKENS
tok_name[COMMENT] = 'COMMENT'

# Changes from 1.3:
#     Ignore now accepts \f as whitespace.  Operator now includes '**'.
#     Ignore and Special now accept \n or \r\n at the end of a line.
#     Imagnumber is new.  Expfloat is corrected to reject '0e4'.
# Note: to get a quoted backslash in a regex, it must be enclosed in brackets.

def group(*choices): return '\(' + string.join(choices, '\|') + '\)'

Whitespace = '[ \f\t]*'
Comment = '\(#[^\r\n]*\)'
Ignore = Whitespace + group('[\]\r?\n' + Whitespace)+'*' + Comment+'?'
Name = '[a-zA-Z_][a-zA-Z0-9_]*'

Hexnumber = '0[xX][0-9a-fA-F]*[lL]?'
Octnumber = '0[0-7]*[lL]?'
Decnumber = '[1-9][0-9]*[lL]?'
Intnumber = group(Hexnumber, Octnumber, Decnumber)
Exponent = '[eE][-+]?[0-9]+'
Pointfloat = group('[0-9]+\.[0-9]*', '\.[0-9]+') + group(Exponent) + '?'
Expfloat = '[1-9][0-9]*' + Exponent
Floatnumber = group(Pointfloat, Expfloat)
Imagnumber = group('0[jJ]', '[1-9][0-9]*[jJ]', Floatnumber + '[jJ]')
Number = group(Imagnumber, Floatnumber, Intnumber)

Single = group("^'", "[^\]'")
Double = group('^"', '[^\]"')
Single3 = group("^'''", "[^\]'''")
Double3 = group('^"""', '[^\]"""')
Triple = group("'''", '"""')
String = group("'" + group('[\].', "[^\n'\]") + "*'",
               '"' + group('[\].', '[^\n"\]') + '*"')

Operator = group('\+', '\-', '\*\*', '\*', '\^', '~', '/', '%', '&', '|',
                 '<<', '>>', '==', '<=', '<>', '!=', '>=', '=', '<', '>')
Bracket = '[][(){}]'
Special = group('\r?\n', '[:;.,`]')
Funny = group(Operator, Bracket, Special)

PlainToken = group(Name, Number, String, Funny)
Token = Ignore + PlainToken

ContStr = group("'" + group('[\].', "[^\n'\]")+'*' + group("'", '[\]\r?\n'),
                '"' + group('[\].', '[^\n"\]')+'*' + group('"', '[\]\r?\n'))
PseudoExtras = group('[\]\r?\n', Comment, Triple)
PseudoToken = Whitespace + group(PseudoExtras, Name, Number, ContStr, Funny)

try:
    saved_syntax = regex.set_syntax(0)         # use default syntax
    tokenprog = regex.compile(Token)
    pseudoprog = regex.compile(PseudoToken)
    endprogs = { '\'': regex.compile(Single), '"': regex.compile(Double),
        '\'\'\'': regex.compile(Single3), '"""': regex.compile(Double3) }
finally:
    regex.set_syntax(saved_syntax)             # restore original syntax

tabsize = 8
TokenError = 'TokenError'
def printtoken(type, token, (srow, scol), (erow, ecol), line): # for testing
    print "%d,%d-%d,%d:\t%s\t%s" % \
        (srow, scol, erow, ecol, tok_name[type], repr(token))

def tokenize(readline, tokeneater=printtoken):
    lnum = parenlev = continued = 0
    namechars, numchars = string.letters + '_', string.digits
    contstr = ''
    indents = [0]

    while 1:                                   # loop over lines in stream
        line = readline()
        lnum = lnum + 1
        pos, max = 0, len(line)

        if contstr:                            # continued string
            if not line: raise TokenError, "EOF within multi-line string"
            if endprog.search(line) >= 0:
                pos = end = endprog.regs[0][1]
                tokeneater(STRING, contstr + line[:end],
                           strstart, (lnum, end), line)
                contstr = ''
            else:
                contstr = contstr + line
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
                tokeneater((NEWLINE, COMMENT)[line[pos] == '#'], line[pos:],
                           (lnum, pos), (lnum, len(line)), line)
                continue

            if column > indents[-1]:           # count indents or dedents
                indents.append(column)
                tokeneater(INDENT, line[:pos], (lnum, 0), (lnum, pos), line)
            while column < indents[-1]:
                indents = indents[:-1]
                tokeneater(DEDENT, line[:pos], (lnum, 0), (lnum, pos), line)

        else:                                  # continued statement
            if not line: raise TokenError, "EOF within multi-line statement"
            continued = 0

        while pos < max:
            if pseudoprog.match(line, pos) > 0:            # scan for tokens
                start, end = pseudoprog.regs[1]
                spos, epos = (lnum, start), (lnum, end)
                token, initial = line[start:end], line[start]
                pos = end

                if initial in namechars:                   # ordinary name
                    tokeneater(NAME, token, spos, epos, line)
                elif initial in numchars:                  # ordinary number
                    tokeneater(NUMBER, token, spos, epos, line)
                elif initial in '\r\n':
                    tokeneater(NEWLINE, token, spos, epos, line)
                elif initial == '#':
                    tokeneater(COMMENT, token, spos, epos, line)
                elif initial == '\\':                      # continued stmt
                    continued = 1
                elif token in ('\'\'\'', '"""'):           # triple-quoted
                    endprog = endprogs[token]
                    if endprog.search(line, pos) >= 0:     # all on one line
                        pos = endprog.regs[0][1]
                        token = line[start:pos]
                        tokeneater(STRING, token, spos, (lnum, pos), line)
                    else:
                        strstart = (lnum, start)           # multiple lines
                        contstr = line[start:]
                        break
                elif initial in '\'"':
                    if token[-1] == '\n':                  # continued string
                        strstart = (lnum, start)
                        endprog, contstr = endprogs[initial], line[start:]
                        break
                    else:                                  # ordinary string
                        tokeneater(STRING, token, spos, epos, line)
                else:
                    if initial in '([{': parenlev = parenlev + 1
                    elif initial in ')]}': parenlev = parenlev - 1
                    tokeneater(OP, token, spos, epos, line)
            else:
                tokeneater(ERRORTOKEN, line[pos], spos, (lnum, pos+1), line)
                pos = pos + 1

    for indent in indents[1:]:                 # pop remaining indent levels
        tokeneater(DEDENT, '', (lnum, 0), (lnum, 0), '')

if __name__ == '__main__':                     # testing
    import sys
    tokenize(open(sys.argv[-1]).readline)

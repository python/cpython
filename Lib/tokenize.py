"""tokenize.py (Ka-Ping Yee, 4 March 1997)

This module compiles a regular expression that recognizes Python tokens
in individual lines of text.  The regular expression handles everything
except indentation, continuations, and triple-quoted strings.  The function
'tokenize.tokenize()' takes care of these things for streams of text.  It
accepts a file-like object and a function, uses the readline() method to
scan the file, and calls the function called once for each token found
passing its type, a string containing the token, the line number, the line,
and the starting and ending positions of the token within the line.
It is designed to match the working of the Python tokenizer exactly."""

import string, regex
from token import *

def group(*choices): return '\(' + string.join(choices, '\|') + '\)'

Ignore = '[ \f\t]*\([\]\r?\n[ \t]*\)*\(#.*\)?'
Name = '[a-zA-Z_][a-zA-Z0-9_]*'

Hexnumber = '0[xX][0-9a-fA-F]*[lL]?'
Octnumber = '0[0-7]*[lL]?'
Decnumber = '[1-9][0-9]*[lL]?'
Intnumber = group(Hexnumber, Octnumber, Decnumber)
Exponent = '[eE][-+]?[0-9]+'
Pointfloat = group('[0-9]+\.[0-9]*', '\.[0-9]+') + group(Exponent) + '?'
Expfloat = '[0-9]+' + Exponent
Floatnumber = group(Pointfloat, Expfloat)
Number = group(Floatnumber, Intnumber)

Single = group('^\'', '[^\]\'')
Double = group('^"', '[^\]"')
Tsingle = group('^\'\'\'', '[^\]\'\'\'')
Tdouble = group('^"""', '[^\]"""')
Triple = group('\'\'\'', '"""')
String = group('\'' + group('[\].', '[^\'\]')+ '*' + group('\'', '[\]\n'),
               '"'  + group('[\].', '[^"\]') + '*' + group('"', '[\]\n'))

Operator = group('\+', '\-', '\*\*', '\*', '\^', '~', '/', '%', '&', '|',
                 '<<', '>>', '==', '<=', '<>', '!=', '>=', '=', '<', '>')
Bracket = '[][(){}]'
Special = group('[\]?\r?\n', '[:;.,`\f]')
Funny = group(Operator, Bracket, Special)

PlainToken = group(Name, Number, Triple, String, Funny)
Token = Ignore + PlainToken

try:
    save_syntax = regex.set_syntax(0)          # use default syntax
    tokenprog = regex.compile(Token)
    endprogs = { '\'': regex.compile(Single), '"': regex.compile(Double),
        '\'\'\'': regex.compile(Tsingle), '"""': regex.compile(Tdouble) }
finally:
    regex.set_syntax(save_syntax)              # restore original syntax

tabsize = 8
TokenError = 'TokenError'
def printtoken(type, string, linenum, line, start, end):   # for testing
    print `linenum` + ':', tok_name[type], repr(string)

def tokenize(readline, tokeneater = printtoken):
    linenum = parenlev = continued = 0
    namechars, numchars = string.letters + '_', string.digits
    contstr = ''
    indents = [0]
    while 1:                                   # loop over lines in stream
        line = readline()
        linenum = linenum + 1
        if line[-2:] == '\r\n': line = line[:-2] + '\n'
        pos, max = 0, len(line)

        if contstr:                            # continued string
            if not line: raise TokenError, "EOF within multi-line string"
            if contstr[-2:] == '\\\n': contstr = contstr[:-2] + '\n'
            if endprog.search(line) >= 0:
                pos = end = endprog.regs[0][1]
                tokeneater(STRING, contstr + line[:end], linenum, line, 0, 0)
                contstr = ''
            else:
                contstr = contstr + line
                continue

        elif parenlev == 0 and not continued:  # this is a new statement
            if not line: break
            column = 0
            while 1:                           # measure leading whitespace
                if line[pos] == ' ': column = column + 1
                elif line[pos] == '\t': column = (column/tabsize + 1) * tabsize
                elif line[pos] == '\f': column = 0
                else: break
                pos = pos + 1
            if line[pos] in '#\n': continue    # skip comments or blank lines

            if column > indents[-1]:           # count indents or dedents
                indents.append(column)
                tokeneater(INDENT, '\t', linenum, line, 0, 0)
            while column < indents[-1]:
                indents = indents[:-1]
                tokeneater(DEDENT, '\t', linenum, line, 0, 0)

        else:                                  # continued statement
            if not line: raise TokenError, "EOF within multi-line statement"
            continued = 0

        while pos < max:
            if tokenprog.match(line, pos) > 0:             # scan for tokens
                start, end = tokenprog.regs[3]
                token = line[start:end]
                pos = end

                if token[0] in namechars:                  # ordinary name
                    tokeneater(NAME, token, linenum, line, start, end)
                elif token[0] in numchars:                 # ordinary number
                    tokeneater(NUMBER, token, linenum, line, start, end)

                elif token in ('\'\'\'', '"""'):           # triple-quoted
                    endprog = endprogs[token]
                    if endprog.search(line, pos) >= 0:     # all on one line
                        pos = endprog.regs[0][1]
                        tokeneater(STRING, token, linenum, line, start, pos)
                    else:
                        contstr = line[start:]             # multiple lines
                        break
                elif token[0] in '\'"':
                    if token[-1] == '\n':                  # continued string
                        endprog, contstr = endprogs[token[0]], line[start:]
                        break
                    else:                                  # ordinary string
                        tokeneater(STRING, token, linenum, line, start, end)

                elif token[0] == '\n':
                    tokeneater(NEWLINE, token, linenum, line, start, end)
                elif token[0] == '\\':                     # continued stmt
                    continued = 1

                else:
                    if token[0] in '([{': parenlev = parenlev + 1
                    if token[0] in ')]}': parenlev = parenlev - 1
                    tokeneater(OP, token, linenum, line, start, end)
            else:
                tokeneater(ERRORTOKEN, line[pos], linenum, line, pos, pos + 1)
                pos = pos + 1

    for indent in indents[1:]:                 # pop remaining indent levels
        tokeneater(DEDENT, '\t', linenum, line, 0, 0)

if __name__ == '__main__':                     # testing
    import sys
    file = open(sys.argv[-1])
    tokenize(file.readline)

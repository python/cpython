# Module 'parser'
#
# Parse S-expressions output by the Panel Editor
# (which is written in Scheme so it can't help writing S-expressions).
#
# See notes at end of file.


whitespace = ' \t\n'
operators = '()\''
separators = operators + whitespace + ';' + '"'


# Tokenize a string.
# Return a list of tokens (strings).
#
def tokenize_string(s):
    tokens = []
    while s:
        c = s[:1]
        if c in whitespace:
            s = s[1:]
        elif c == ';':
            s = ''
        elif c == '"':
            n = len(s)
            i = 1
            while i < n:
                c = s[i]
                i = i+1
                if c == '"': break
                if c == '\\': i = i+1
            tokens.append(s[:i])
            s = s[i:]
        elif c in operators:
            tokens.append(c)
            s = s[1:]
        else:
            n = len(s)
            i = 1
            while i < n:
                if s[i] in separators: break
                i = i+1
            tokens.append(s[:i])
            s = s[i:]
    return tokens


# Tokenize a whole file (given as file object, not as file name).
# Return a list of tokens (strings).
#
def tokenize_file(fp):
    tokens = []
    while 1:
        line = fp.readline()
        if not line: break
        tokens = tokens + tokenize_string(line)
    return tokens


# Exception raised by parse_exr.
#
syntax_error = 'syntax error'


# Parse an S-expression.
# Input is a list of tokens as returned by tokenize_*().
# Return a pair (expr, tokens)
# where expr is a list representing the s-expression,
# and tokens contains the remaining tokens.
# May raise syntax_error.
#
def parse_expr(tokens):
    if (not tokens) or tokens[0] != '(':
        raise syntax_error, 'expected "("'
    tokens = tokens[1:]
    expr = []
    while 1:
        if not tokens:
            raise syntax_error, 'missing ")"'
        if tokens[0] == ')':
            return expr, tokens[1:]
        elif tokens[0] == '(':
            subexpr, tokens = parse_expr(tokens)
            expr.append(subexpr)
        else:
            expr.append(tokens[0])
            tokens = tokens[1:]


# Parse a file (given as file object, not as file name).
# Return a list of parsed S-expressions found at the top level.
#
def parse_file(fp):
    tokens = tokenize_file(fp)
    exprlist = []
    while tokens:
        expr, tokens = parse_expr(tokens)
        exprlist.append(expr)
    return exprlist


# EXAMPLE:
#
# The input
#       '(hip (hop hur-ray))'
#
# passed to tokenize_string() returns the token list
#       ['(', 'hip', '(', 'hop', 'hur-ray', ')', ')']
#
# When this is passed to parse_expr() it returns the expression
#       ['hip', ['hop', 'hur-ray']]
# plus an empty token list (because there are no tokens left.
#
# When a file containing the example is passed to parse_file() it returns
# a list whose only element is the output of parse_expr() above:
#       [['hip', ['hop', 'hur-ray']]]


# TOKENIZING:
#
# Comments start with semicolon (;) and continue till the end of the line.
#
# Tokens are separated by whitespace, except the following characters
# always form a separate token (outside strings):
#       ( ) '
# Strings are enclosed in double quotes (") and backslash (\) is used
# as escape character in strings.

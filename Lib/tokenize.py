# This module compiles a regular expression that recognizes Python tokens.
# It is designed to match the working of the Python tokenizer exactly.
# It takes care of everything except indentation;
# note that un-escaped newlines are tokens, too.
# tokenprog.regs[3] gives the location of the token without whitespace
# It also defines various subexpressions, but doesn't compile them.
# See the function test() below for an example of how to use.

import regex

# Note: to get a quoted backslash in a regexp, it must be quadrupled.

Ignore = '[ \t]*\(\\\\\n[ \t]*\)*\(#.*\)?'

Name = '[a-zA-Z_][a-zA-Z0-9_]*'

Hexnumber = '0[xX][0-9a-fA-F]*[lL]?'
Octnumber = '0[0-7]*[lL]?'
Decnumber = '[1-9][0-9]*[lL]?'
Intnumber = Hexnumber + '\|' + Octnumber + '\|' + Decnumber
Exponent = '[eE][-+]?[0-9]+'
Pointfloat = '\([0-9]+\.[0-9]*\|\.[0-9]+\)\(' + Exponent + '\)?'
Expfloat = '[0-9]+' + Exponent
Floatnumber = Pointfloat + '\|' + Expfloat
Number = Intnumber + '\|' + Floatnumber

String = '\'\(\\\\.\|[^\\\n\']\)*\''

Operator = '~\|\+\|-\|\*\|/\|%\|\^\|&\||\|<<\|>>\|==\|<=\|<>\|!=\|>=\|=\|<\|>'
Bracket = '[][(){}]'
Special = '[:;.,`\n]'
Funny = Operator + '\|' + Bracket + '\|' + Special

PlainToken = Name + '\|' + Number + '\|' + String + '\|' + Funny

Token = Ignore + '\(' + PlainToken + '\)'

try:
	save_syntax = regex.set_syntax(0) # Use default syntax
	tokenprog = regex.compile(Token)
finally:
	dummy = regex.set_syntax(save_syntax) # Restore original syntax


def test(file):
	f = open(file, 'r')
	while 1:
		line = f.readline()
		if not line: break
		i, n = 0, len(line)
		while i < n:
			j = tokenprog.match(line, i)
			if j < 0:
				print 'No token at', `line[i:i+20]` + '...'
				i = i+1
			else:
				i = i+j
				a, b = tokenprog.regs[3]
				if a < b:
					print 'Token:', `line[a:b]`

import os
import sys
import regex
import string
import getopt

def main():
	process(sys.stdin, sys.stdout)

dashes = regex.compile('^-+[ \t]*$')
equals = regex.compile('^=+[ \t]*$')
stars = regex.compile('^\*+[ \t]*$')
blank = regex.compile('^[ \t]*$')
indented = regex.compile('^\( *\t\|        \)[ \t]*[^ \t]')

def process(fi, fo):
	inverbatim = 0
	line = '\n'
	nextline = fi.readline()
	while nextline:
		prevline = line
		line = nextline
		nextline = fi.readline()
		fmt = None
		if dashes.match(nextline) >= 0:
			fmt = '\\subsection{%s}\n'
		elif equals.match(nextline) >= 0:
			fmt = '\\section{%s}\n'
		elif stars.match(nextline) >= 0:
			fmt = '\\chapter{%s}\n'
		if fmt:
			nextline = '\n'
			line =  fmt % string.strip(line)
		elif inverbatim:
			if blank.match(line) >= 0 and \
				  indented.match(nextline) < 0:
				inverbatim = 0
				fo.write('\\end{verbatim}\n')
		else:
			if indented.match(line) >= 0 and \
				  blank.match(prevline) >= 0:
				inverbatim = 1
				fo.write('\\begin{verbatim}\n')
		if inverbatim:
			line = string.expandtabs(line, 4)
		fo.write(line)

#main()
process(open('ext.tex', 'r'), sys.stdout)

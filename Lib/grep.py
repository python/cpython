# 'grep'

import regex
from regex_syntax import *
import string

def grep(pat, filename):
	return ggrep(RE_SYNTAX_GREP, pat, filename)

def egrep(pat, filename):
	return ggrep(RE_SYNTAX_EGREP, pat, filename)

def emgrep(pat, filename):
	return ggrep(RE_SYNTAX_EMACS, pat, filename)

def ggrep(syntax, pat, filename):
	syntax = regex.set_syntax(syntax)
	try:
		prog = regex.compile(pat)
	finally:
		syntax = regex.set_syntax(syntax)
	fp = open(filename, 'r')
	lineno = 0
	while 1:
		line = fp.readline()
		if not line: break
		lineno = lineno + 1
		if prog.search(line) >= 0:
			if line[-1:] == '\n': line = line[:-1]
			prefix = string.rjust(`lineno`, 3) + ': '
			print prefix + line
			if 0: # XXX
				start, end = prog.regs[0]
				line = line[:start]
				if '\t' not in line:
					prefix = ' ' * (len(prefix) + start)
				else:
					prefix = ' ' * len(prefix)
					for c in line:
						if c <> '\t': c = ' '
						prefix = prefix + c
				if start == end: prefix = prefix + '\\'
				else: prefix = prefix + '^'*(end-start)
				print prefix

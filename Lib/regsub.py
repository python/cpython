"""Regexp-based split and replace using the obsolete regex module.

This module is only for backward compatibility.  These operations
are now provided by the new regular expression module, "re".

sub(pat, repl, str):        replace first occurrence of pattern in string
gsub(pat, repl, str):       replace all occurrences of pattern in string
split(str, pat, maxsplit):  split string using pattern as delimiter
splitx(str, pat, maxsplit): split string using pattern as delimiter plus
                            return delimiters
"""

import regex


# Replace first occurrence of pattern pat in string str by replacement
# repl.  If the pattern isn't found, the string is returned unchanged.
# The replacement may contain references \digit to subpatterns and
# escaped backslashes.  The pattern may be a string or an already
# compiled pattern.

def sub(pat, repl, str):
	prog = compile(pat)
	if prog.search(str) >= 0:
		regs = prog.regs
		a, b = regs[0]
		str = str[:a] + expand(repl, regs, str) + str[b:]
	return str


# Replace all (non-overlapping) occurrences of pattern pat in string
# str by replacement repl.  The same rules as for sub() apply.
# Empty matches for the pattern are replaced only when not adjacent to
# a previous match, so e.g. gsub('', '-', 'abc') returns '-a-b-c-'.

def gsub(pat, repl, str):
	prog = compile(pat)
	new = ''
	start = 0
	first = 1
	while prog.search(str, start) >= 0:
		regs = prog.regs
		a, b = regs[0]
		if a == b == start and not first:
			if start >= len(str) or prog.search(str, start+1) < 0:
				break
			regs = prog.regs
			a, b = regs[0]
		new = new + str[start:a] + expand(repl, regs, str)
		start = b
		first = 0
	new = new + str[start:]
	return new


# Split string str in fields separated by delimiters matching pattern
# pat.  Only non-empty matches for the pattern are considered, so e.g.
# split('abc', '') returns ['abc'].
# The optional 3rd argument sets the number of splits that are performed.

def split(str, pat, maxsplit = 0):
	return intsplit(str, pat, maxsplit, 0)

# Split string str in fields separated by delimiters matching pattern
# pat.  Only non-empty matches for the pattern are considered, so e.g.
# split('abc', '') returns ['abc']. The delimiters are also included
# in the list.
# The optional 3rd argument sets the number of splits that are performed.


def splitx(str, pat, maxsplit = 0):
	return intsplit(str, pat, maxsplit, 1)
	
# Internal function used to implement split() and splitx().

def intsplit(str, pat, maxsplit, retain):
	prog = compile(pat)
	res = []
	start = next = 0
	splitcount = 0
	while prog.search(str, next) >= 0:
		regs = prog.regs
		a, b = regs[0]
		if a == b:
			next = next + 1
			if next >= len(str):
				break
		else:
			res.append(str[start:a])
			if retain:
				res.append(str[a:b])
			start = next = b
			splitcount = splitcount + 1
			if (maxsplit and (splitcount >= maxsplit)):
			    break
	res.append(str[start:])
	return res


# Capitalize words split using a pattern

def capwords(str, pat='[^a-zA-Z0-9_]+'):
	import string
	words = splitx(str, pat)
	for i in range(0, len(words), 2):
		words[i] = string.capitalize(words[i])
	return string.joinfields(words, "")


# Internal subroutines:
# compile(pat): compile a pattern, caching already compiled patterns
# expand(repl, regs, str): expand \digit escapes in replacement string


# Manage a cache of compiled regular expressions.
#
# If the pattern is a string a compiled version of it is returned.  If
# the pattern has been used before we return an already compiled
# version from the cache; otherwise we compile it now and save the
# compiled version in the cache, along with the syntax it was compiled
# with.  Instead of a string, a compiled regular expression can also
# be passed.

cache = {}

def compile(pat):
	if type(pat) <> type(''):
		return pat		# Assume it is a compiled regex
	key = (pat, regex.get_syntax())
	if cache.has_key(key):
		prog = cache[key]	# Get it from the cache
	else:
		prog = cache[key] = regex.compile(pat)
	return prog


def clear_cache():
	global cache
	cache = {}


# Expand \digit in the replacement.
# Each occurrence of \digit is replaced by the substring of str
# indicated by regs[digit].  To include a literal \ in the
# replacement, double it; other \ escapes are left unchanged (i.e.
# the \ and the following character are both copied).

def expand(repl, regs, str):
	if '\\' not in repl:
		return repl
	new = ''
	i = 0
	ord0 = ord('0')
	while i < len(repl):
		c = repl[i]; i = i+1
		if c <> '\\' or i >= len(repl):
			new = new + c
		else:
			c = repl[i]; i = i+1
			if '0' <= c <= '9':
				a, b = regs[ord(c)-ord0]
				new = new + str[a:b]
			elif c == '\\':
				new = new + c
			else:
				new = new + '\\' + c
	return new


# Test program, reads sequences "pat repl str" from stdin.
# Optional argument specifies pattern used to split lines.

def test():
	import sys
	if sys.argv[1:]:
		delpat = sys.argv[1]
	else:
		delpat = '[ \t\n]+'
	while 1:
		if sys.stdin.isatty(): sys.stderr.write('--> ')
		line = sys.stdin.readline()
		if not line: break
		if line[-1] == '\n': line = line[:-1]
		fields = split(line, delpat)
		if len(fields) <> 3:
			print 'Sorry, not three fields'
			print 'split:', `fields`
			continue
		[pat, repl, str] = split(line, delpat)
		print 'sub :', `sub(pat, repl, str)`
		print 'gsub:', `gsub(pat, repl, str)`

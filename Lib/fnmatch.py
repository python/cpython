# module 'fnmatch' -- filename matching with shell patterns
# This version translates the pattern to a regular expression
# and moreover caches the expressions.

import os
import regex

cache = {}

def fnmatch(name, pat):
	name = os.path.normcase(name)
	pat = os.path.normcase(pat)
	if not cache.has_key(pat):
		res = translate(pat)
		save_syntax = regex.set_syntax(0)
		cache[pat] = regex.compile(res)
		save_syntax = regex.set_syntax(save_syntax)
	return cache[pat].match(name) == len(name)

def translate(pat):
	i, n = 0, len(pat)
	res = ''
	while i < n:
		c = pat[i]
		i = i+1
		if c == '*':
			res = res + '.*'
		elif c == '?':
			res = res + '.'
		elif c == '[':
			j = i
			if j < n and pat[j] == '!':
				j = j+1
			if j < n and pat[j] == ']':
				j = j+1
			while j < n and pat[j] != ']':
				j = j+1
			if j >= n:
				res = res + '\\['
			else:
				stuff = pat[i:j]
				i = j+1
				if stuff[0] == '!':
					stuff = '[^' + stuff[1:] + ']'
				elif stuff == '^'*len(stuff):
					stuff = '\\^'
				else:
					while stuff[0] == '^':
						stuff = stuff[1:] + stuff[0]
					stuff = '[' + stuff + ']'
				res = res + stuff
		elif c in '\\.+^$':
			res = res + ('\\' + c)
		else:
			res = res + c
	return res

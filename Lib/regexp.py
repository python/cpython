# Provide backward compatibility for module "regexp" using "regex".

import regex
from regex_syntax import *

class Prog:
	def __init__(self, pat):
		save_syntax = regex.set_syntax(RE_SYNTAX_AWK)
		try:
			self.prog = regex.compile(pat)
		finally:
			xxx = regex.set_syntax(save_syntax)
	def match(self, str, offset = 0):
		if self.prog.search(str, offset) < 0:
			return ()
		regs = self.prog.regs
		i = len(regs)
		while i > 0 and regs[i-1] == (-1, -1):
			i = i-1
		return regs[:i]

def compile(pat):
	return Prog(pat)

cache_pat = None
cache_prog = None

def match(pat, str):
	global cache_pat, cache_prog
	if pat <> cache_pat:
		cache_pat, cache_prog = pat, compile(pat)
	return cache_prog.match(str)

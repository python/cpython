#!/usr/bin/env python
# -*- mode: python -*-
# $Id$


import sys
import string
from pcre import *

#
# First, the public part of the interface:
#

# pcre.error and re.error should be the same, since exceptions can be
# raised from  either module.

# compilation flags

I = IGNORECASE
M = MULTILINE
S = DOTALL 
X = VERBOSE 

#
#
#

_cache = {}
_MAXCACHE = 20

def _cachecompile(pattern, flags=0):
    key = (pattern, flags)
    try:
	return _cache[key]
    except KeyError:
	pass
    value = compile(pattern, flags)
    if len(_cache) >= _MAXCACHE:
	_cache.clear()
    _cache[key] = value
    return value

def match(pattern, string, flags=0):
    return _cachecompile(pattern, flags).match(string)
  
def search(pattern, string, flags=0):
    return _cachecompile(pattern, flags).search(string)
  
def sub(pattern, repl, string, count=0):
    if type(pattern) == type(''):
	pattern = _cachecompile(pattern)
    return pattern.sub(repl, string, count)

def subn(pattern, repl, string, count=0):
    if type(pattern) == type(''):
	pattern = _cachecompile(pattern)
    return pattern.subn(repl, string, count)
  
def split(pattern, string, maxsplit=0):
    if type(pattern) == type(''):
	pattern = _cachecompile(pattern)
    return pattern.split(string, maxsplit)

#
#
#

class RegexObject:
    def __init__(self, pattern, flags, code, groupindex):
	self.code = code 
	self.flags = flags
	self.pattern = pattern
	self.groupindex = groupindex
    def search(self, string, pos=0):
	regs = self.code.match(string, pos, 0)
	if regs is None:
	    return None
	self.num_regs=len(regs)
	
	return MatchObject(self,
			   string,
			   pos,
			   regs)
    
    def match(self, string, pos=0):
	regs = self.code.match(string, pos, ANCHORED)
	if regs is None:
	    return None
	self.num_regs=len(regs)
	return MatchObject(self,
			   string,
			   pos,
			   regs)
    
    def sub(self, repl, string, count=0):
        return self.subn(repl, string, count)[0]
    
    def subn(self, repl, source, count=0):
	if count < 0:
	    raise error, "negative substitution count"
	if count == 0:
	    import sys
	    count = sys.maxint
	if type(repl) == type(''):
	    if '\\' in repl:
		repl = lambda m, r=repl: pcre_expand(m, r)
	    else:
		repl = lambda m, r=repl: r
	n = 0           # Number of matches
	pos = 0         # Where to start searching
	lastmatch = -1  # End of last match
	results = []    # Substrings making up the result
	end = len(source)
	while n < count and pos <= end:
	    m = self.search(source, pos)
	    if not m:
		break
	    i, j = m.span(0)
	    if i == j == lastmatch:
		# Empty match adjacent to previous match
		pos = pos + 1
		results.append(source[lastmatch:pos])
		continue
	    if pos < i:
		results.append(source[pos:i])
	    results.append(repl(m))
	    pos = lastmatch = j
	    if i == j:
		# Last match was empty; don't try here again
		pos = pos + 1
		results.append(source[lastmatch:pos])
	    n = n + 1
	results.append(source[pos:])
	return (string.join(results, ''), n)
									    
    def split(self, source, maxsplit=0):
	if maxsplit < 0:
	    raise error, "negative split count"
	if maxsplit == 0:
	    import sys
	    maxsplit = sys.maxint
	n = 0
	pos = 0
	lastmatch = 0
	results = []
	end = len(source)
	while n < maxsplit:
	    m = self.search(source, pos)
	    if not m:
		break
	    i, j = m.span(0)
	    if i == j:
		# Empty match
		if pos >= end:
		    break
		pos = pos+1
		continue
	    results.append(source[lastmatch:i])
	    g = m.group()
	    if g:
		results[len(results):] = list(g)
	    pos = lastmatch = j
	results.append(source[lastmatch:])
	return results

class MatchObject:
    def __init__(self, re, string, pos, regs):
	self.re = re
	self.string = string
	self.pos = pos
	self.regs = regs
	
    def start(self, g):
	if type(g) == type(''):
	    try:
		g = self.re.groupindex[g]
	    except (KeyError, TypeError):
		raise IndexError, ('group "' + g + '" is undefined')
	return self.regs[g][0]
    
    def end(self, g):
	if type(g) == type(''):
	    try:
		g = self.re.groupindex[g]
	    except (KeyError, TypeError):
		raise IndexError, ('group "' + g + '" is undefined')
	return self.regs[g][1]
    
    def span(self, g):
	if type(g) == type(''):
	    try:
		g = self.re.groupindex[g]
	    except (KeyError, TypeError):
		raise IndexError, ('group "' + g + '" is undefined')
	return self.regs[g]
    
    def group(self, *groups):
	if len(groups) == 0:
	    groups = range(1, self.re.num_regs)
	    use_all = 1
	else:
	    use_all = 0
	result = []
	for g in groups:
	    if type(g) == type(''):
		try:
		    g = self.re.groupindex[g]
		except (KeyError, TypeError):
		    raise IndexError, ('group "' + g + '" is undefined')
	    if len(self.regs)<=g: raise IndexError, ('group "' + str(g) + '" is undefined')
	    elif (self.regs[g][0] == -1) or (self.regs[g][1] == -1):
		result.append(None)
	    else:
		result.append(self.string[self.regs[g][0]:self.regs[g][1]])
	if use_all or len(result) > 1:
	    return tuple(result)
	elif len(result) == 1:
	    return result[0]
	else:
	    return ()

def escape(pattern):
    result = []
    alphanum=string.letters+'_'+string.digits
    for char in pattern:
	if char not in alphanum:
	    result.append('\\')
	result.append(char)
    return string.join(result, '')

def compile(pattern, flags=0):
    groupindex={}
    code=pcre_compile(pattern, flags, groupindex)
    return RegexObject(pattern, flags, code, groupindex)
    


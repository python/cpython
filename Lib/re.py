#!/usr/bin/env python
# -*- mode: python -*-
# $Id$


import sys
import string
from pcre import *

[ NORMAL, CHARCLASS, REPLACEMENT ] = range(3)
[ CHAR, MEMORY_REFERENCE, SYNTAX, NOT_SYNTAX, SET, WORD_BOUNDARY, NOT_WORD_BOUNDARY, BEGINNING_OF_BUFFER, END_OF_BUFFER ] = range(9)

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
	self.num_regs=len(regs)/2
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

def valid_identifier(id):
    import string
    if len(id) == 0:
	return 0
    if id[0] not in string.letters+'_':
	return 0
    for char in id[1:]:
	if not syntax_table[char] & word:
	    return 0
    return 1

def compile(pattern, flags=0):
    groupindex={}
    code=pcre_compile(pattern, flags, groupindex)
    return RegexObject(pattern, flags, code, groupindex)
    
def _expand(m, repl):
    results = []
    index = 0
    size = len(repl)
    while index < size:
	found = string.find(repl, '\\', index)
	if found < 0:
	    results.append(repl[index:])
	    break
	if found > index:
	    results.append(repl[index:found])
	escape_type, value, index = _expand_escape(repl, found+1, REPLACEMENT)
	if escape_type == CHAR:
	    results.append(value)
	elif escape_type == MEMORY_REFERENCE:
	    r = m.group(value)
	    if r is None:
		raise error, ('group "' + str(value) + '" did not contribute '
			      'to the match')
	    results.append(m.group(value))
	else:
	    raise error, "bad escape in replacement"
    return string.join(results, '')

def _expand_escape(pattern, index, context=NORMAL):
    if index >= len(pattern):
	raise error, 'escape ends too soon'

    elif pattern[index] == 't':
	return CHAR, chr(9), index + 1
    
    elif pattern[index] == 'n':
	return CHAR, chr(10), index + 1
    
    elif pattern[index] == 'v':
	return CHAR, chr(11), index + 1
    
    elif pattern[index] == 'r':
	return CHAR, chr(13), index + 1
    
    elif pattern[index] == 'f':
	return CHAR, chr(12), index + 1
    
    elif pattern[index] == 'a':
	return CHAR, chr(7), index + 1
    
    elif pattern[index] == 'x':
	# CAUTION: this is the Python rule, not the Perl rule!
	end = index + 1  # Skip over the 'x' character
	while (end < len(pattern)) and (pattern[end] in string.hexdigits):
	    end = end + 1
	if end == index:
	    raise error, "\\x must be followed by hex digit(s)"
	# let Python evaluate it, so we don't incorrectly 2nd-guess
	# what it's doing (and Python in turn passes it on to sscanf,
	# so that *it* doesn't incorrectly 2nd-guess what C does!)
	char = eval ('"' + pattern[index-1:end] + '"')
#	assert len(char) == 1
	return CHAR, char, end

    elif pattern[index] == 'b':
	if context != NORMAL:
	    return CHAR, chr(8), index + 1
	else:
	    return WORD_BOUNDARY, '', index + 1
	    
    elif pattern[index] == 'B':
	if context != NORMAL:
	    return CHAR, 'B', index + 1
	else:
	    return NOT_WORD_BOUNDARY, '', index + 1
	    
    elif pattern[index] == 'A':
	if context != NORMAL:
	    return CHAR, 'A', index + 1
	else:
	    return BEGINNING_OF_BUFFER, '', index + 1
	    
    elif pattern[index] == 'Z':
	if context != NORMAL:
	    return CHAR, 'Z', index + 1
	else:
	    return END_OF_BUFFER, '', index + 1
	    
    elif pattern[index] in 'GluLUQE':
	raise error, ('\\' + pattern[index] + ' is not allowed')
    
    elif pattern[index] == 'w':
	    return CHAR, 'w', index + 1
	
    elif pattern[index] == 'W':
	    return CHAR, 'W', index + 1
	
    elif pattern[index] == 's':
	    return CHAR, 's', index + 1
	
    elif pattern[index] == 'S':
	    return CHAR, 'S', index + 1
	
    elif pattern[index] == 'd':
	    return CHAR, 'd', index + 1
	
    elif pattern[index] == 'D':
	    return CHAR, 'D', index + 1

    elif pattern[index] in '0123456789':

	if pattern[index] == '0':
	    if (index + 1 < len(pattern)) and \
	       (pattern[index + 1] in string.octdigits):
		if (index + 2 < len(pattern)) and \
		   (pattern[index + 2] in string.octdigits):
		    value = string.atoi(pattern[index:index + 3], 8)
		    index = index + 3

		else:
		    value = string.atoi(pattern[index:index + 2], 8)
		    index = index + 2

	    else:
		value = 0
		index = index + 1

	    if value > 255:
		raise error, 'octal value out of range'

	    return CHAR, chr(value), index
	
	else:
	    if (index + 1 < len(pattern)) and \
	       (pattern[index + 1] in string.digits):
		if (index + 2 < len(pattern)) and \
		   (pattern[index + 2] in string.octdigits) and \
		   (pattern[index + 1] in string.octdigits) and \
		   (pattern[index] in string.octdigits):
		    value = string.atoi(pattern[index:index + 3], 8)
		    if value > 255:
			raise error, 'octal value out of range'

		    return CHAR, chr(value), index + 3

		else:
		    value = string.atoi(pattern[index:index + 2])
		    if (value < 1) or (value > 99):
			raise error, 'memory reference out of range'

		    if context == CHARCLASS:
			raise error, ('cannot reference a register from '
				      'inside a character class')
		    return MEMORY_REFERENCE, value, index + 2

	    else:
		if context == CHARCLASS:
		    raise error, ('cannot reference a register from '
				  'inside a character class')

		value = string.atoi(pattern[index])
		return MEMORY_REFERENCE, value, index + 1
	    
    elif pattern[index] == 'g':
	if context != REPLACEMENT:
	    return CHAR, 'g', index + 1

	index = index + 1
	if index >= len(pattern):
	    raise error, 'unfinished symbolic reference'
	if pattern[index] != '<':
	    raise error, 'missing < in symbolic reference'

	index = index + 1
	end = string.find(pattern, '>', index)
	if end == -1:
	    raise error, 'unfinished symbolic reference'
	value = pattern[index:end]
	if not valid_identifier(value):
	    raise error, 'illegal symbolic reference'
	return MEMORY_REFERENCE, value, end + 1
    
    else:
	return CHAR, pattern[index], index + 1


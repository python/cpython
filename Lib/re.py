#!/usr/bin/env python
# -*- mode: python -*-
# $Id$

import string
import reop

# reop.error and re.error should be the same, since exceptions can be
# raised from  either module.
error = reop.error # 're error'

from reop import NORMAL, CHARCLASS, REPLACEMENT
from reop import CHAR, MEMORY_REFERENCE, SYNTAX, NOT_SYNTAX, SET
from reop import WORD_BOUNDARY, NOT_WORD_BOUNDARY, BEGINNING_OF_BUFFER, END_OF_BUFFER

# compilation flags

IGNORECASE = I = 0x01

MULTILINE = M = 0x02
DOTALL = S = 0x04
VERBOSE = X = 0x08

repetition_operators = ['*', '*?', '+', '+?', '?', '??', '{n}', '{n}?',
			'{n,}', '{n,}?', '{n,m}', '{n,m}?']

#
#
#

def valid_identifier(id):
    if len(id) == 0:
	return 0
    if (not reop.syntax_table[id[0]] & reop.word) or \
       (reop.syntax_table[id[0]] & reop.digit):
	return 0
    for char in id[1:]:
	if not reop.syntax_table[char] & reop.word:
	    return 0
    return 1

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
	escape_type, value, index = expand_escape(repl, found+1, REPLACEMENT)
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

class RegexObject:
    def __init__(self, pattern, flags, code, num_regs, groupindex):
	self.code = code
	self.num_regs = num_regs
	self.flags = flags
	self.pattern = pattern
	self.groupindex = groupindex
	self.fastmap = build_fastmap(code)
	
	if code[0].name == 'bol':
	    self.anchor = 1
	    
	elif code[0].name == 'begbuf':
	    self.anchor = 2
	    
	else:
	    self.anchor = 0
	    
	self.buffer = assemble(code)
    def search(self, string, pos=0):
	regs = reop.search(self.buffer,
			   self.num_regs,
			   self.flags,
			   self.fastmap.can_be_null,
			   self.fastmap.fastmap(),
			   self.anchor,
			   string,
			   pos)
	if regs is None:
	    return None
	
	return MatchObject(self,
			   string,
			   pos,
			   regs)
    
    def match(self, string, pos=0):
	regs = reop.match(self.buffer,
			  self.num_regs,
			  self.flags,
			  self.fastmap.can_be_null,
			  self.fastmap.fastmap(),
			  self.anchor,
			  string,
			  pos)
	if regs is None:
	    return None
	
	return MatchObject(self,
			   string,
			   pos,
			   regs)
    
    def sub(self, repl, string, count=0):
        return self.subn(repl, string, count)[0]
    
    def subn(self, repl, source, count=0):
	if count < 0:
	    raise ValueError, "negative substibution count"
	if count == 0:
	    import sys
	    count = sys.maxint
	if type(repl) == type(''):
	    if '\\' in repl:
		repl = lambda m, r=repl: _expand(m, r)
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
	    if (self.regs[g][0] == -1) or (self.regs[g][1] == -1):
		result.append(None)
	    else:
		result.append(self.string[self.regs[g][0]:self.regs[g][1]])
	if use_all or len(result) > 1:
	    return tuple(result)
	elif len(result) == 1:
	    return result[0]
	else:
	    return ()

#
# A set of classes to make assembly a bit easier, if a bit verbose.
#

class Instruction:
    def __init__(self, opcode, size=1):
	self.opcode = opcode
	self.size = size
    def assemble(self, position, labels):
	return self.opcode
    def __repr__(self):
	return '%-15s' % (self.name)

class End(Instruction):
    name = 'end'
    def __init__(self):
	Instruction.__init__(self, chr(0))

class Bol(Instruction):
    name = 'bol'
    def __init__(self):
	self.name = 'bol'
	Instruction.__init__(self, chr(1))

class Eol(Instruction):
    name = 'eol'
    def __init__(self):
	Instruction.__init__(self, chr(2))

class Set(Instruction):
    name = 'set'
    def __init__(self, set, flags=0):
	self.set = set
	if flags & IGNORECASE: self.set=map(string.lower, self.set)
	if len(set)==1: 
	    # If only one element, use the "exact" opcode (it'll be faster)
	    Instruction.__init__(self, chr(4), 2)
	else:
	    # Use the "set" opcode
	    Instruction.__init__(self, chr(3), 33)
    def assemble(self, position, labels):
	if len(self.set)==1:
	    # If only one character in set, generate an "exact" opcode
	    return self.opcode + self.set[0]
	result = self.opcode
	temp = 0
	for i, c in map(lambda x: (x, chr(x)), range(256)):
	    if c in self.set:
		temp = temp | (1 << (i & 7))
	    if (i % 8) == 7:
		result = result + chr(temp)
		temp = 0
	return result
    def __repr__(self):
	result = '%-15s' % (self.name)
	self.set.sort()
	# XXX this should print more intelligently
	for char in self.set:
	    result = result + char
	return result
    
class Exact(Instruction):
    name = 'exact'
    def __init__(self, char, flags):
	self.char = char
	if flags & IGNORECASE: self.char=string.lower(self.char)
	Instruction.__init__(self, chr(4), 2)
    def assemble(self, position, labels):
	return self.opcode + self.char
    def __repr__(self):
	return '%-15s %s' % (self.name, `self.char`)
    
class AnyChar(Instruction):
    name = 'anychar'
    def __init__(self):
	Instruction.__init__(self, chr(5))
    def assemble(self, position, labels):
	return self.opcode

class MemoryInstruction(Instruction):
    def __init__(self, opcode, register):
	self.register = register
	Instruction.__init__(self, opcode, 2)
    def assemble(self, position, labels):
	return self.opcode + chr(self.register)
    def __repr__(self):
	return '%-15s %i' % (self.name, self.register)

class StartMemory(MemoryInstruction):
    name = 'start_memory'
    def __init__(self, register):
	MemoryInstruction.__init__(self, chr(6), register)

class EndMemory(MemoryInstruction):
    name = 'end_memory'
    def __init__(self, register):
	MemoryInstruction.__init__(self, chr(7), register)

class MatchMemory(MemoryInstruction):
    name = 'match_memory'
    def __init__(self, register):
	MemoryInstruction.__init__(self, chr(8), register)

class JumpInstruction(Instruction):
    def __init__(self, opcode, label):
	self.label = label
	Instruction.__init__(self, opcode, 3)
    def compute_offset(self, start, dest):
	return dest - (start + 3)
    def pack_offset(self, offset):
	if offset > 32767:
	    raise error, 'offset out of range (pos)'
	elif offset < -32768:
	    raise error, 'offset out of range (neg)'
	elif offset < 0:
	    offset = offset + 65536
	return chr(offset & 0xff) + chr((offset >> 8) & 0xff)
    def assemble(self, position, labels):
	return self.opcode + \
	       self.pack_offset(self.compute_offset(position,
						    labels[self.label]))
    def __repr__(self):
	return '%-15s %i' % (self.name, self.label)

class Jump(JumpInstruction):
    name = 'jump'
    def __init__(self, label):
	JumpInstruction.__init__(self, chr(9), label)

class StarJump(JumpInstruction):
    name = 'star_jump'
    def __init__(self, label):
	JumpInstruction.__init__(self, chr(10), label)

class FailureJump(JumpInstruction):
    name = 'failure_jump'
    def __init__(self, label):
	JumpInstruction.__init__(self, chr(11), label)

class UpdateFailureJump(JumpInstruction):
    name = 'update_failure_jump'
    def __init__(self, label):
	JumpInstruction.__init__(self, chr(12), label)

class DummyFailureJump(JumpInstruction):
    name = 'dummy_failure_jump'
    def __init__(self, label):
	JumpInstruction.__init__(self, chr(13), label)
	
class BegBuf(Instruction):
    name = 'begbuf'
    def __init__(self):
	Instruction.__init__(self, chr(14))

class EndBuf(Instruction):
    name = 'endbuf'
    def __init__(self):
	Instruction.__init__(self, chr(15))

class WordBeg(Instruction):
    name = 'wordbeg'
    def __init__(self):
	Instruction.__init__(self, chr(16))

class WordEnd(Instruction):
    name = 'wordend'
    def __init__(self):
	Instruction.__init__(self, chr(17))

class WordBound(Instruction):
    name = 'wordbound'
    def __init__(self):
	Instruction.__init__(self, chr(18))

class NotWordBound(Instruction):
    name = 'notwordbound'
    def __init__(self):
	Instruction.__init__(self, chr(18))

class SyntaxSpec(Instruction):
    name = 'syntaxspec'
    def __init__(self, syntax):
	self.syntax = syntax
	Instruction.__init__(self, chr(20), 2)
    def assemble(self, postition, labels):
	return self.opcode + chr(self.syntax)

class NotSyntaxSpec(Instruction):
    name = 'notsyntaxspec'
    def __init__(self, syntax):
	self.syntax = syntax
	Instruction.__init__(self, chr(21), 2)
    def assemble(self, postition, labels):
	return self.opcode + chr(self.syntax)

class Label(Instruction):
    name = 'label'
    def __init__(self, label):
	self.label = label
	Instruction.__init__(self, '', 0)
    def __repr__(self):
	return '%-15s %i' % (self.name, self.label)

class OpenParen(Instruction):
    name = '('
    def __init__(self, register):
	self.register = register
	Instruction.__init__(self, '', 0)
    def assemble(self, position, labels):
	raise error, 'unmatched open parenthesis'

class Alternation(Instruction):
    name = '|'
    def __init__(self):
	Instruction.__init__(self, '', 0)
    def assemble(self, position, labels):
	raise error, 'an alternation was not taken care of'

#
#
#

def assemble(instructions):
    labels = {}
    position = 0
    pass1 = []
    for instruction in instructions:
	if instruction.name == 'label':
	    labels[instruction.label] = position
	else:
	    pass1.append((position, instruction))
	    position = position + instruction.size
    pass2 = ''
    for position, instruction in pass1:
	pass2 = pass2 + instruction.assemble(position, labels)
    return pass2

#
#
#

def escape(pattern):
    result = []
    for char in pattern:
	if not reop.syntax_table[char] & reop.word:
	    result.append('\\')
	result.append(char)
    return string.join(result, '')

#
#
#

def registers_used(instructions):
    result = []
    for instruction in instructions:
	if (instruction.name in ['set_memory', 'end_memory']) and \
	   (instruction.register not in result):
	    result.append(instruction.register)
    return result

#
#
#

class Fastmap:
    def __init__(self):
	self.map = ['\000']*256
	self.can_be_null = 0
    def add(self, char):
	self.map[ord(char)] = '\001'
    def fastmap(self):
	return string.join(self.map, '')
    def __getitem__(self, char):
	return ord(self.map[ord(char)])
    def __repr__(self):
	self.map.sort()
	return 'Fastmap(' + `self.can_be_null` + ', ' + `self.map` + ')'

#
#
#

def find_label(code, label):
    line = 0
    for instruction in code:
	if (instruction.name == 'label') and (instruction.label == label):
	    return line + 1
	line = line + 1
	
def build_fastmap_aux(code, pos, visited, fastmap):
    if visited[pos]:
	return
    while 1:
	instruction = code[pos]
	visited[pos] = 1
	pos = pos + 1
	if instruction.name == 'end':
	    fastmap.can_be_null = 1
	    return
	elif instruction.name == 'syntaxspec':
	    for char in map(chr, range(256)):
		if reop.syntax_table[char] & instruction.syntax:
		    fastmap.add(char)
	    return
	elif instruction.name == 'notsyntaxspec':
	    for char in map(chr, range(256)):
		if not reop.syntax_table[char] & instruction.syntax:
		    fastmap.add(char)
	    return
	elif instruction.name == 'eol':
	    fastmap.add('\n')
	    if fastmap.can_be_null == 0:
		fastmap.can_be_null = 2
	    return
	elif instruction.name == 'set':
	    for char in instruction.set:
		fastmap.add(char)
	    return
	elif instruction.name == 'exact':
	    fastmap.add(instruction.char)
	elif instruction.name == 'anychar':
	    for char in map(chr, range(256)):
		if char != '\n':
		    fastmap.add(char)
	    return
	elif instruction.name == 'match_memory':
	    for char in map(chr, range(256)):
		fastmap.add(char)
	    fastmap.can_be_null = 1
	    return
	elif instruction.name in ['jump', 'dummy_failure_jump', \
				  'update_failure_jump', 'star_jump']:
	    pos = find_label(code, instruction.label)
	    if visited[pos]:
		return
	    visited[pos] = 1
	elif instruction.name  == 'failure_jump':
	    build_fastmap_aux(code,
			      find_label(code, instruction.label),
			      visited,
			      fastmap)
	
def build_fastmap(code, pos=0):
    visited = [0] * len(code)
    fastmap = Fastmap()
    build_fastmap_aux(code, pos, visited, fastmap)
    return fastmap

#
#
#

#[NORMAL, CHARCLASS, REPLACEMENT] = range(3)
#[CHAR, MEMORY_REFERENCE, SYNTAX, NOT_SYNTAX, SET, WORD_BOUNDARY,
# NOT_WORD_BOUNDARY, BEGINNING_OF_BUFFER, END_OF_BUFFER] = range(9)

def expand_escape(pattern, index, context=NORMAL):
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
	assert len(char) == 1
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
	if context == NORMAL:
	    return SYNTAX, reop.word, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if reop.syntax_table[char] & reop.word:
		    set.append(char)
	    return SET, set, index + 1
	else:
	    return CHAR, 'w', index + 1
	
    elif pattern[index] == 'W':
	if context == NORMAL:
	    return NOT_SYNTAX, reop.word, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if not reop.syntax_table[char] & reop.word:
		    set.append(char)
	    return SET, set, index + 1
	else:
	    return CHAR, 'W', index + 1
	
    elif pattern[index] == 's':
	if context == NORMAL:
	    return SYNTAX, reop.whitespace, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if reop.syntax_table[char] & reop.whitespace:
		    set.append(char)
	    return SET, set, index + 1
	else:
	    return CHAR, 's', index + 1
	
    elif pattern[index] == 'S':
	if context == NORMAL:
	    return NOT_SYNTAX, reop.whitespace, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if not reop.syntax_table[char] & reop.word:
		    set.append(char)
	    return SET, set, index + 1
	else:
	    return CHAR, 'S', index + 1
	
    elif pattern[index] == 'd':
	if context == NORMAL:
	    return SYNTAX, reop.digit, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if reop.syntax_table[char] & reop.digit:
		    set.append(char)
	    return SET, set, index + 1
	else:
	    return CHAR, 'd', index + 1
	
    elif pattern[index] == 'D':
	if context == NORMAL:
	    return NOT_SYNTAX, reop.digit, index + 1
	elif context == CHARCLASS:
	    set = []
	    for char in reop.syntax_table.keys():
		if not reop.syntax_table[char] & reop.digit:
		    set.append(char)
	    return SET, set, index + 1
	else:
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

def compile(pattern, flags=0):
    stack = []
    label = 0
    register = 1
    groupindex = {}
    lastop = ''

    # look for embedded pattern modifiers at the beginning of the pattern

    index = 0

    if len(pattern) >= 3 and \
       (pattern[:2] == '(?') and \
       (pattern[2] in 'iImMsSxX'):
	index = 2
	while (index < len(pattern)) and (pattern[index] != ')'):
	    if pattern[index] in 'iI':
		flags = flags | IGNORECASE
	    elif pattern[index] in 'mM':
		flags = flags | MULTILINE
	    elif pattern[index] in 'sS':
		flags = flags | DOTALL
	    elif pattern[index] in 'xX':
		flags = flags | VERBOSE
	    else:
		raise error, 'unknown modifier'
	    index = index + 1
	index = index + 1

    # compile the rest of the pattern
    
    while (index < len(pattern)):
	char = pattern[index]
	index = index + 1
	if char == '\\':
	    escape_type, value, index = expand_escape(pattern, index)

	    if escape_type == CHAR:
		stack.append([Exact(value, flags)])
		lastop = '\\' + value
		
	    elif escape_type == MEMORY_REFERENCE:
		if value >= register:
		    raise error, ('cannot reference a register '
				  'not yet used')
		stack.append([MatchMemory(value)])
		lastop = '\\1'
		
	    elif escape_type == BEGINNING_OF_BUFFER:
		stack.append([BegBuf()])
		lastop = '\\A'
		
	    elif escape_type == END_OF_BUFFER:
		stack.append([EndBuf()])
		lastop = '\\Z'
		
	    elif escape_type == WORD_BOUNDARY:
		stack.append([WordBound()])
		lastop = '\\b'
		
	    elif escape_type == NOT_WORD_BOUNDARY:
		stack.append([NotWordBound()])
		lastop = '\\B'
		
	    elif escape_type == SYNTAX:
		stack.append([SyntaxSpec(value)])
		if value == reop.word:
		    lastop = '\\w'
		elif value == reop.whitespace:
		    lastop = '\\s'
		elif value == reop.digit:
		    lastop = '\\d'
		else:
		    lastop = '\\?'
		    
	    elif escape_type == NOT_SYNTAX:
		stack.append([NotSyntaxSpec(value)])
		if value == reop.word:
		    lastop = '\\W'
		elif value == reop.whitespace:
		    lastop = '\\S'
		elif value == reop.digit:
		    lastop = '\\D'
		else:
		    lastop = '\\?'
		
	    elif escape_type == SET:
		raise error, 'cannot use set escape type here'
	    
	    else:
		raise error, 'unknown escape type'

	elif char == '|':
	    expr = []
	    
	    while (len(stack) != 0) and \
		  (stack[-1][0].name != '(') and \
		  (stack[-1][0].name != '|'):
		expr = stack[-1] + expr
		del stack[-1]
	    stack.append([FailureJump(label)] + \
			 expr + \
			 [Jump(-1),
			  Label(label)])
	    stack.append([Alternation()])
	    label = label + 1
	    lastop = '|'
	    
	elif char == '(':
	    if index >= len(pattern):
		raise error, 'no matching close paren'

	    elif pattern[index] == '?':
		# Perl style (?...) extensions
		index = index + 1
		if index >= len(pattern):
		    raise error, 'extension ends prematurely'

		elif pattern[index] == 'P':
		    # Python extensions
		    index = index + 1
		    if index >= len(pattern):
			raise error, 'extension ends prematurely'

		    elif pattern[index] == '<':
			# Handle Python symbolic group names (?P<...>...)
			index = index + 1
			end = string.find(pattern, '>', index)
			if end == -1:
			    raise error, 'no end to symbolic group name'
			name = pattern[index:end]
			if not valid_identifier(name):
			    raise error, ('symbolic group name must be a '
					  'valid identifier')
			index = end + 1
			groupindex[name] = register
			stack.append([OpenParen(register)])
			register = register + 1
			lastop = '('

		    elif pattern[index] == '=':
			# backreference to symbolic group name
			if index >= len(pattern):
			    raise error, '(?P= at the end of the pattern'
			start = index + 1
			end = string.find(pattern, ')', start)
			if end == -1:
			    raise error, 'no ) to end symbolic group name'
			name = pattern[start:end]
			if name not in groupindex.keys():
			    raise error, ('symbolic group name ' + name + \
					  ' has not been used yet')
			stack.append([MatchMemory(groupindex[name])])
			index = end + 1
			lastop = '(?P=)'
			
		    else:
			raise error, ('unknown Python extension: ' + \
				      pattern[index])
		    
		elif pattern[index] == ':':
		    # grouping, but no registers
		    index = index + 1
		    stack.append([OpenParen(-1)])
		    lastop = '('
		    
		elif pattern[index] == '#':
		    # comment
		    index = index + 1
		    end = string.find(pattern, ')', index)
		    if end == -1:
			raise error, 'no end to comment'
		    index = end + 1
		    # do not change lastop
		    
		elif pattern[index] == '=':
		    raise error, ('zero-width positive lookahead '
				  'assertion is unsupported')

		elif pattern[index] == '!':
		    raise error, ('zero-width negative lookahead '
				  'assertion is unsupported')

		elif pattern[index] in 'iImMsSxX':
		    raise error, ('embedded pattern modifiers are only '
				  'allowed at the beginning of the pattern')

		else:
		    raise error, 'unknown extension'

	    else:
		stack.append([OpenParen(register)])
		register = register + 1
		lastop = '('

	elif char == ')':
	    # make one expression out of everything on the stack up to
	    # the marker left by the last parenthesis
	    expr = []
	    while (len(stack) > 0) and (stack[-1][0].name != '('):
		expr = stack[-1] + expr
		del stack[-1]

	    if len(stack) == 0:
		raise error, 'too many close parens'
	    
	    # remove markers left by alternation
	    expr = filter(lambda x: x.name != '|', expr)

	    # clean up jumps inserted by alternation
	    need_label = 0
	    for i in range(len(expr)):
		if (expr[i].name == 'jump') and (expr[i].label == -1):
		    expr[i] = Jump(label)
		    need_label = 1
	    if need_label:
		expr.append(Label(label))
		label = label + 1

	    if stack[-1][0].register > 0:
		expr = [StartMemory(stack[-1][0].register)] + \
		       expr + \
		       [EndMemory(stack[-1][0].register)]
	    del stack[-1]
	    stack.append(expr)
	    lastop = ')'

	elif char == '{':
	    if len(stack) == 0:
		raise error, 'no expression to repeat'
	    end = string.find(pattern, '}', index)
	    if end == -1:
		raise error, ('no close curly bracket to match'
			      ' open curly bracket')

	    fields = map(string.strip,
			 string.split(pattern[index:end], ','))
	    index = end + 1

	    minimal = 0
	    if (index < len(pattern)) and (pattern[index] == '?'):
		minimal = 1
		index = index + 1

	    if len(fields) == 1:
		# {n} or {n}? (there's really no difference)
		try:
		    count = string.atoi(fields[0])
		except ValueError:
		    raise error, ('count must be an integer '
				  'inside curly braces')
		if count > 65535:
		    raise error, 'repeat count out of range'
		expr = []
		while count > 0:
		    expr = expr + stack[-1]
		    count = count - 1
		del stack[-1]
		stack.append(expr)
		if minimal:
		    lastop = '{n}?'
		else:
		    lastop = '{n}'

	    elif len(fields) == 2:
		# {n,} or {n,m}
		if fields[1] == '':
		    # {n,}
		    try:
			min = string.atoi(fields[0])
		    except ValueError:
			raise error, ('minimum must be an integer '
				      'inside curly braces')
		    if min > 65535:
			raise error, 'minimum repeat count out of range'

		    expr = []
		    while min > 0:
			expr = expr + stack[-1]
			min = min - 1
		    if minimal:
			expr = expr + \
			       ([Jump(label + 1),
				 Label(label)] + \
				stack[-1] + \
				[Label(label + 1),
				 FailureJump(label)])
			lastop = '{n,}?'
		    else:
			expr = expr + \
			       ([Label(label),
				 FailureJump(label + 1)] +
				stack[-1] +
				[StarJump(label),
				 Label(label + 1)])
			lastop = '{n,}'

		    del stack[-1]
		    stack.append(expr)
		    label = label + 2

		else:
		    # {n,m}
		    try:
			min = string.atoi(fields[0])
		    except ValueError:
			raise error, ('minimum must be an integer '
				      'inside curly braces')
		    try:
			max = string.atoi(fields[1])
		    except ValueError:
			raise error, ('maximum must be an integer '
				      'inside curly braces')
		    if min > 65535:
			raise error, ('minumim repeat count out '
				      'of range')
		    if max > 65535:
			raise error, ('maximum repeat count out '
				      'of range')
		    if min > max:
			raise error, ('minimum repeat count must be '
				      'less than the maximum '
				      'repeat count')
		    expr = []
		    while min > 0:
			expr = expr + stack[-1]
			min = min - 1
			max = max - 1
		    if minimal:
			while max > 0:
			    expr = expr + \
				   [FailureJump(label),
				    Jump(label + 1),
				    Label(label)] + \
				   stack[-1] + \
				   [Label(label + 1)]
			    max = max - 1
			    label = label + 2
			del stack[-1]
			stack.append(expr)
			lastop = '{n,m}?'
		    else:
			while max > 0:
			    expr = expr + \
				   [FailureJump(label)] + \
				   stack[-1]
			    max = max - 1
			del stack[-1]
			stack.append(expr + [Label(label)])
			label = label + 1
			lastop = '{n,m}'

	    else:
		raise error, ('there need to be one or two fields '
			      'in a {} expression')

	elif char == '}':
	    raise error, 'unbalanced close curly brace'

	elif char == '*':
	    # Kleene closure
	    if len(stack) == 0:
		raise error, '* needs something to repeat'

	    if lastop in ['(', '|']:
		raise error, '* needs something to repeat'

	    if lastop in repetition_operators:
		raise error, 'nested repetition operators'
	    
	    if (index < len(pattern)) and (pattern[index] == '?'):
		# non-greedy matching
		expr = [Jump(label + 1),
			Label(label)] + \
		       stack[-1] + \
		       [Label(label + 1),
			FailureJump(label)]
		index = index + 1
		lastop = '*?'
	    else:
		# greedy matching
		expr = [Label(label),
			FailureJump(label + 1)] + \
		       stack[-1] + \
		       [StarJump(label),
			Label(label + 1)]
		lastop = '*'
	    del stack[-1]
	    stack.append(expr)
	    label = label + 2

	elif char == '+':
	    # positive closure
	    if len(stack) == 0:
		raise error, '+ needs something to repeat'
	    
	    if lastop in ['(', '|']:
		raise error, '+ needs something to repeat'

	    if lastop in repetition_operators:
		raise error, 'nested repetition operators'

	    if (index < len(pattern)) and (pattern[index] == '?'):
		# non-greedy
		expr = [Label(label)] + \
		       stack[-1] + \
		       [FailureJump(label)]
		label = label + 1
		index = index + 1
		lastop = '+?'
		
	    else:
		# greedy
		expr = [DummyFailureJump(label + 1),
			Label(label),
			FailureJump(label + 2),
			Label(label + 1)] + \
		       stack[-1] + \
		       [StarJump(label),
			Label(label + 2)]
		label = label + 3
		lastop = '+'
		
	    del stack[-1]
	    stack.append(expr)

	elif char == '?':
	    if len(stack) == 0:
		raise error, 'need something to be optional'
	    
	    if len(stack) == 0:
		raise error, '? needs something to repeat'
	    
	    if lastop in ['(', '|']:
		raise error, '? needs something to repeat'

	    if lastop in repetition_operators:
		raise error, 'nested repetition operators'

	    if (index < len(pattern)) and (pattern[index] == '?'):
		# non-greedy matching
		expr = [FailureJump(label),
			Jump(label + 1),
			Label(label)] + \
		       stack[-1] + \
		       [Label(label + 1)]
		label = label + 2
		index = index + 1
		lastop = '??'
		
	    else:
		# greedy matching
		expr = [FailureJump(label)] + \
		       stack[-1] + \
		       [Label(label)]
		label = label + 1
		lastop = '?'
		
	    del stack[-1]
	    stack.append(expr)

	elif char == '.':
	    if flags & DOTALL:
		stack.append([Set(map(chr, range(256)), flags)])
	    else:
		stack.append([AnyChar()])
	    lastop = '.'

	elif char == '^':
	    if flags & MULTILINE:
		stack.append([Bol()])
	    else:
		stack.append([BegBuf()])
	    lastop = '^'

	elif char == '$':
	    if flags & MULTILINE:
		stack.append([Eol()])
	    else:
		stack.append([EndBuf()])
	    lastop = '$'

	elif char == '#':
	    if flags & VERBOSE:
		# comment
		index = index + 1
		end = string.find(pattern, '\n', index)
		if end == -1:
		    index = len(pattern)
		else:
		    index = end + 1
		# do not change lastop
	    else:
		stack.append([Exact(char, flags)])
		lastop = '#'

	elif char in string.whitespace:
	    if not (flags & VERBOSE):
		stack.append([Exact(char, flags)])
		lastop = char

	elif char == '[':
	    # compile character class
	    
	    if index >= len(pattern):
		raise error, 'unclosed character class'
	    
	    negate = 0
	    last = ''
	    set = []

	    if pattern[index] == '^':
		negate = 1
		index = index + 1
		if index >= len(pattern):
		    raise error, 'unclosed character class'

	    if pattern[index] == ']':
		set.append(']')
		index = index + 1
		if index >= len(pattern):
		    raise error, 'unclosed character class'
		
	    elif pattern[index] == '-':
		set.append('-')
		index = index + 1
		if index >= len(pattern):
		    raise error, 'unclosed character class'
		
	    while (index < len(pattern)) and (pattern[index] != ']'):
		next = pattern[index]
		index = index + 1
		if next == '-':
		    if index >= len(pattern):
			raise error, 'incomplete range in character class'

		    elif pattern[index] == ']':
			set.append('-')
			
		    else:
			if last == '':
			    raise error, ('improper use of range in '
					  'character class')

			start = last
			
			if pattern[index] == '\\':
			    escape_type,
			    value,
			    index = expand_escape(pattern,
						  index + 1,
						  CHARCLASS)

			    if escape_type == CHAR:
				end = value
				
			    else:
				raise error, ('illegal escape in character '
					      'class range')
			else:
			    end = pattern[index]
			    index = index + 1

			if start > end:
			    raise error, ('range arguments out of order '
					  'in character class')
			
			for char in map(chr, range(ord(start), ord(end) + 1)):
			    if char not in set:
				set.append(char)
			    
			last = ''

		elif next == '\\':
		    # expand syntax meta-characters and add to set
		    if index >= len(pattern):
			raise error, 'incomplete set'

		    escape_type, value, index = expand_escape(pattern,
							      index,
							      CHARCLASS)

		    if escape_type == CHAR:
			set.append(value)
			last = value

		    elif escape_type == SET:
			for char in value:
			    if char not in set:
				set.append(char)
			last = ''

		    else:
			raise error, 'illegal escape type in character class'

		else:
		    if next not in set:
			set.append(next)
		    last = next
		    
	    if (index >= len(pattern)) or ( pattern[index] != ']'):
		raise error, 'incomplete set'

	    index = index + 1

	    if negate:
		# If case is being ignored, then both upper- and lowercase
		# versions of the letters must be excluded.
		if flags & IGNORECASE: set=set+map(string.upper, set)
		notset = []
		for char in map(chr, range(256)):
		    if char not in set:
			notset.append(char)
		if len(notset) == 0:
		    raise error, 'empty negated set'
		stack.append([Set(notset, flags)])
	    else:
		if len(set) == 0:
		    raise error, 'empty set'
		stack.append([Set(set, flags)])

	    lastop = '[]'

	else:
	    stack.append([Exact(char, flags)])
	    lastop = char

    code = []
    while len(stack) > 0:
	if stack[-1][0].name == '(':
	    raise error, 'too many open parens'
	code = stack[-1] + code
	del stack[-1]
    if len(code) == 0:
	raise error, 'no code generated'
    code = filter(lambda x: x.name != '|', code)
    need_label = 0
    for i in range(len(code)):
	if (code[i].name == 'jump') and (code[i].label == -1):
	    code[i] = Jump(label)
	    need_label = 1
    if need_label:
	code.append(Label(label))
	label = label + 1
    code.append(End())
#    print code
    return RegexObject(pattern, flags, code, register, groupindex)

# Replace expand_escape and _expand functions with their C equivalents.
# If you suspect bugs in the C versions, comment out the next two lines
##expand_escape = reop.expand_escape
##_expand  = reop._expand

#
# Secret Labs' Regular Expression Engine
# $Id$
#
# convert template to internal format
#
# Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
#
# This code can only be used for 1.6 alpha testing.  All other use
# require explicit permission from Secret Labs AB.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

import array, string, sys

import _sre

from sre_constants import *

# find an array type code that matches the engine's code size
for WORDSIZE in "BHil":
    if len(array.array(WORDSIZE, [0]).tostring()) == _sre.getcodesize():
	break
else:
    raise RuntimeError, "cannot find a useable array type"

# FIXME: <fl> should move some optimizations from the parser to here!

class Code:
    def __init__(self):
	self.data = []
    def __len__(self):
	return len(self.data)
    def __getitem__(self, index):
	return self.data[index]
    def __setitem__(self, index, code):
	self.data[index] = code
    def append(self, code):
	self.data.append(code)
    def todata(self):
	# print self.data
	try:
	    return array.array(WORDSIZE, self.data).tostring()
	except OverflowError:
	    print self.data
	    raise

def _compile(code, pattern, flags):
    append = code.append
    for op, av in pattern:
	if op is ANY:
	    if flags & SRE_FLAG_DOTALL:
		append(OPCODES[op]) # any character at all!
	    else:
		append(OPCODES[CATEGORY])
		append(CHCODES[CATEGORY_NOT_LINEBREAK])
	elif op in (SUCCESS, FAILURE):
	    append(OPCODES[op])
	elif op is AT:
	    append(OPCODES[op])
	    if flags & SRE_FLAG_MULTILINE:
		append(ATCODES[AT_MULTILINE[av]])
	    else:
		append(ATCODES[av])
	elif op is BRANCH:
	    append(OPCODES[op])
	    tail = []
	    for av in av[1]:
		skip = len(code); append(0)
		_compile(code, av, flags)
##		append(OPCODES[SUCCESS])
 		append(OPCODES[JUMP])
 		tail.append(len(code)); append(0)
		code[skip] = len(code) - skip
	    append(0) # end of branch
 	    for tail in tail:
		code[tail] = len(code) - tail
	elif op is CALL:
	    append(OPCODES[op])
	    skip = len(code); append(0)
	    _compile(code, av, flags)
	    append(OPCODES[SUCCESS])
	    code[skip] = len(code) - skip
	elif op is CATEGORY:
	    append(OPCODES[op])
	    if flags & SRE_FLAG_LOCALE:
		append(CH_LOCALE[CHCODES[av]])
	    elif flags & SRE_FLAG_UNICODE:
		append(CH_UNICODE[CHCODES[av]])
	    else:
		append(CHCODES[av])
	elif op is GROUP:
	    if flags & SRE_FLAG_IGNORECASE:
		append(OPCODES[OP_IGNORE[op]])
	    else:
		append(OPCODES[op])
	    append(av-1)
	elif op is IN:
	    if flags & SRE_FLAG_IGNORECASE:
		append(OPCODES[OP_IGNORE[op]])
		def fixup(literal, flags=flags):
		    return _sre.getlower(ord(literal), flags)
	    else:
		append(OPCODES[op])
		fixup = ord
	    skip = len(code); append(0)
	    for op, av in av:
		append(OPCODES[op])
		if op is NEGATE:
		    pass
		elif op is LITERAL:
		    append(fixup(av))
		elif op is RANGE:
		    append(fixup(av[0]))
		    append(fixup(av[1]))
		elif op is CATEGORY:
		    if flags & SRE_FLAG_LOCALE:
			append(CH_LOCALE[CHCODES[av]])
		    elif flags & SRE_FLAG_UNICODE:
			append(CH_UNICODE[CHCODES[av]])
		    else:
			append(CHCODES[av])
		else:
		    raise ValueError, "unsupported set operator"
	    append(OPCODES[FAILURE])
	    code[skip] = len(code) - skip
	elif op in (LITERAL, NOT_LITERAL):
	    if flags & SRE_FLAG_IGNORECASE:
		append(OPCODES[OP_IGNORE[op]])
	    else:
		append(OPCODES[op])
	    append(ord(av))
	elif op is MARK:
	    append(OPCODES[op])
	    append(av)
 	elif op in (REPEAT, MIN_REPEAT, MAX_REPEAT):
	    if flags & SRE_FLAG_TEMPLATE:
		append(OPCODES[REPEAT])
		skip = len(code); append(0)
		append(av[0])
		append(av[1])
		_compile(code, av[2], flags)
		append(OPCODES[SUCCESS])
		code[skip] = len(code) - skip
	    else:
		lo, hi = av[2].getwidth()
		if lo == 0:
		    raise error, "nothing to repeat"
		if 0 and lo == hi == 1 and op is MAX_REPEAT:
		    # FIXME: <fl> need a better way to figure out when
		    # it's safe to use this one (in the parser, probably)
		    append(OPCODES[MAX_REPEAT_ONE])
		    skip = len(code); append(0)
		    append(av[0])
		    append(av[1])
		    _compile(code, av[2], flags)
		    append(OPCODES[SUCCESS])
		    code[skip] = len(code) - skip
		else:
		    append(OPCODES[op])
		    skip = len(code); append(0)
		    append(av[0])
		    append(av[1])
		    _compile(code, av[2], flags)
		    append(OPCODES[SUCCESS])
		    code[skip] = len(code) - skip
	elif op is SUBPATTERN:
 	    group = av[0]
 	    if group:
 		append(OPCODES[MARK])
 		append((group-1)*2)
	    _compile(code, av[1], flags)
 	    if group:
 		append(OPCODES[MARK])
 		append((group-1)*2+1)
	else:
	    raise ValueError, ("unsupported operand type", op)

def compile(p, flags=0):
    # convert pattern list to internal format
    if type(p) in (type(""), type(u"")):
	import sre_parse
	pattern = p
	p = sre_parse.parse(p)
    else:
	pattern = None
    flags = p.pattern.flags | flags
    code = Code()
    _compile(code, p.data, flags)
    code.append(OPCODES[SUCCESS])
    data = code.todata()
    if 0: # debugging
	print
	print "-" * 68
	import sre_disasm
	sre_disasm.disasm(data)
	print "-" * 68
    return _sre.compile(
	pattern, flags,
	data,
	p.pattern.groups-1, p.pattern.groupdict
	)

#
# Secret Labs' Regular Expression Engine
#
# convert template to internal format
#
# Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

import array
import _sre

from sre_constants import *

# find an array type code that matches the engine's code size
for WORDSIZE in "BHil":
    if len(array.array(WORDSIZE, [0]).tostring()) == _sre.getcodesize():
	break
else:
    raise RuntimeError, "cannot find a useable array type"

def _compile(code, pattern, flags):
    emit = code.append
    for op, av in pattern:
	if op is ANY:
	    if flags & SRE_FLAG_DOTALL:
		emit(OPCODES[op])
	    else:
		emit(OPCODES[CATEGORY])
		emit(CHCODES[CATEGORY_NOT_LINEBREAK])
	elif op in (SUCCESS, FAILURE):
	    emit(OPCODES[op])
	elif op is AT:
	    emit(OPCODES[op])
	    if flags & SRE_FLAG_MULTILINE:
		emit(ATCODES[AT_MULTILINE[av]])
	    else:
		emit(ATCODES[av])
	elif op is BRANCH:
	    emit(OPCODES[op])
	    tail = []
	    for av in av[1]:
		skip = len(code); emit(0)
		_compile(code, av, flags)
 		emit(OPCODES[JUMP])
 		tail.append(len(code)); emit(0)
		code[skip] = len(code) - skip
	    emit(0) # end of branch
 	    for tail in tail:
		code[tail] = len(code) - tail
	elif op is CALL:
	    emit(OPCODES[op])
	    skip = len(code); emit(0)
	    _compile(code, av, flags)
	    emit(OPCODES[SUCCESS])
	    code[skip] = len(code) - skip
	elif op is CATEGORY:
	    emit(OPCODES[op])
	    if flags & SRE_FLAG_LOCALE:
		emit(CH_LOCALE[CHCODES[av]])
	    elif flags & SRE_FLAG_UNICODE:
		emit(CH_UNICODE[CHCODES[av]])
	    else:
		emit(CHCODES[av])
	elif op is GROUP:
	    if flags & SRE_FLAG_IGNORECASE:
		emit(OPCODES[OP_IGNORE[op]])
	    else:
		emit(OPCODES[op])
	    emit(av-1)
	elif op is IN:
	    if flags & SRE_FLAG_IGNORECASE:
		emit(OPCODES[OP_IGNORE[op]])
		def fixup(literal, flags=flags):
		    return _sre.getlower(ord(literal), flags)
	    else:
		emit(OPCODES[op])
		fixup = ord
	    skip = len(code); emit(0)
	    for op, av in av:
		emit(OPCODES[op])
		if op is NEGATE:
		    pass
		elif op is LITERAL:
		    emit(fixup(av))
		elif op is RANGE:
		    emit(fixup(av[0]))
		    emit(fixup(av[1]))
		elif op is CATEGORY:
		    if flags & SRE_FLAG_LOCALE:
			emit(CH_LOCALE[CHCODES[av]])
		    elif flags & SRE_FLAG_UNICODE:
			emit(CH_UNICODE[CHCODES[av]])
		    else:
			emit(CHCODES[av])
		else:
		    raise error, "internal: unsupported set operator"
	    emit(OPCODES[FAILURE])
	    code[skip] = len(code) - skip
	elif op in (LITERAL, NOT_LITERAL):
	    if flags & SRE_FLAG_IGNORECASE:
		emit(OPCODES[OP_IGNORE[op]])
	    else:
		emit(OPCODES[op])
	    emit(ord(av))
	elif op is MARK:
	    emit(OPCODES[op])
	    emit(av)
 	elif op in (REPEAT, MIN_REPEAT, MAX_REPEAT):
	    if flags & SRE_FLAG_TEMPLATE:
		emit(OPCODES[REPEAT])
		skip = len(code); emit(0)
		emit(av[0])
		emit(av[1])
		_compile(code, av[2], flags)
		emit(OPCODES[SUCCESS])
		code[skip] = len(code) - skip
	    else:
		lo, hi = av[2].getwidth()
		if lo == 0:
		    raise error, "nothing to repeat"
		if 0 and lo == hi == 1 and op is MAX_REPEAT:
		    # FIXME: <fl> need a better way to figure out when
		    # it's safe to use this one (in the parser, probably)
		    emit(OPCODES[MAX_REPEAT_ONE])
		    skip = len(code); emit(0)
		    emit(av[0])
		    emit(av[1])
		    _compile(code, av[2], flags)
		    emit(OPCODES[SUCCESS])
		    code[skip] = len(code) - skip
		else:
		    emit(OPCODES[op])
		    skip = len(code); emit(0)
		    emit(av[0])
		    emit(av[1])
		    _compile(code, av[2], flags)
		    emit(OPCODES[SUCCESS])
		    code[skip] = len(code) - skip
	elif op is SUBPATTERN:
 	    group = av[0]
 	    if group:
 		emit(OPCODES[MARK])
 		emit((group-1)*2)
	    _compile(code, av[1], flags)
 	    if group:
 		emit(OPCODES[MARK])
 		emit((group-1)*2+1)
	else:
	    raise ValueError, ("unsupported operand type", op)

def compile(p, flags=0):
    # internal: convert pattern list to internal format
    if type(p) in (type(""), type(u"")):
	import sre_parse
	pattern = p
	p = sre_parse.parse(p)
    else:
	pattern = None
    flags = p.pattern.flags | flags
    code = []
    _compile(code, p.data, flags)
    code.append(OPCODES[SUCCESS])
    # FIXME: <fl> get rid of this limitation
    assert p.pattern.groups <= 100,\
	   "sorry, but this version only supports 100 named groups"
    return _sre.compile(
	pattern, flags,
	array.array(WORDSIZE, code).tostring(),
	p.pattern.groups-1, p.pattern.groupdict
	)

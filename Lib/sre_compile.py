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

# FIXME: <fl> formalize (objectify?) and document the compiler code
# format, so that other frontends can use this compiler

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
        return array.array(WORDSIZE, self.data).tostring()

def _lower(literal):
    # return _sre._lower(literal) # FIXME
    return string.lower(literal)

def _compile(code, pattern, flags):
    append = code.append
    for op, av in pattern:
        if op is ANY:
            if "s" in flags:
                append(CODES[op]) # any character at all!
            else:
                append(CODES[NOT_LITERAL])
                append(10)
        elif op in (SUCCESS, FAILURE):
            append(CODES[op])
        elif op is AT:
            append(CODES[op])
            append(POSITIONS[av])
        elif op is BRANCH:
            append(CODES[op])
            tail = []
            for av in av[1]:
                skip = len(code); append(0)
                _compile(code, av, flags)
                append(CODES[JUMP])
                tail.append(len(code)); append(0)
                code[skip] = len(code) - skip
            append(0) # end of branch
            for tail in tail:
                code[tail] = len(code) - tail
        elif op is CALL:
            append(CODES[op])
            skip = len(code); append(0)
            _compile(code, av, flags)
            append(CODES[SUCCESS])
            code[skip] = len(code) - skip
        elif op is CATEGORY: # not used by current parser
            append(CODES[op])
            append(CATEGORIES[av])
        elif op is GROUP:
            if "i" in flags:
                append(CODES[MAP_IGNORE[op]])
            else:
                append(CODES[op])
            append(av)
        elif op is IN:
            if "i" in flags:
                append(CODES[MAP_IGNORE[op]])
                def fixup(literal):
                    return ord(_lower(literal))
            else:
                append(CODES[op])
                fixup = ord
            skip = len(code); append(0)
            for op, av in av:
                append(CODES[op])
                if op is NEGATE:
                    pass
                elif op is LITERAL:
                    append(fixup(av))
                elif op is RANGE:
                    append(fixup(av[0]))
                    append(fixup(av[1]))
                elif op is CATEGORY:
                    append(CATEGORIES[av])
                else:
                    raise ValueError, "unsupported set operator"
            append(CODES[FAILURE])
            code[skip] = len(code) - skip
        elif op in (LITERAL, NOT_LITERAL):
            if "i" in flags:
                append(CODES[MAP_IGNORE[op]])
                append(ord(_lower(av)))
            else:
                append(CODES[op])
                append(ord(av))
        elif op is MARK:
            append(CODES[op])
            append(av)
        elif op in (REPEAT, MIN_REPEAT, MAX_REPEAT):
            lo, hi = av[2].getwidth()
            if lo == 0:
                raise SyntaxError, "cannot repeat zero-width items"
            if lo == hi == 1 and op is MAX_REPEAT:
                append(CODES[MAX_REPEAT_ONE])
                skip = len(code); append(0)
                append(av[0])
                append(av[1])
                _compile(code, av[2], flags)
                append(CODES[SUCCESS])
                code[skip] = len(code) - skip
            else:
                append(CODES[op])
                skip = len(code); append(0)
                append(av[0])
                append(av[1])
                _compile(code, av[2], flags)
                if op is MIN_REPEAT:
                    append(CODES[MIN_UNTIL])
                else:
                    # FIXME: MAX_REPEAT PROBABLY DOESN'T WORK (?)
                    append(CODES[MAX_UNTIL])
                code[skip] = len(code) - skip
        elif op is SUBPATTERN:
##          group = av[0]
##          if group:
##              append(CODES[MARK])
##              append((group-1)*2)
            _compile(code, av[1], flags)
##          if group:
##              append(CODES[MARK])
##              append((group-1)*2+1)
        else:
            raise ValueError, ("unsupported operand type", op)

def compile(p, flags=()):
    # convert pattern list to internal format
    if type(p) is type(""):
        import sre_parse
        pattern = p
        p = sre_parse.parse(p)
    else:
        pattern = None
    # print p.getwidth()
    # print p
    code = Code()
    _compile(code, p.data, p.pattern.flags)
    code.append(CODES[SUCCESS])
    # print list(code.data)
    data = code.todata()
    if 0: # debugging
        print
        print "-" * 68
        import sre_disasm
        sre_disasm.disasm(data)
        print "-" * 68
    # print len(data), p.pattern.groups, len(p.pattern.groupdict)
    return _sre.compile(pattern, data, p.pattern.groups-1, p.pattern.groupdict)

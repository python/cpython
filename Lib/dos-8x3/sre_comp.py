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
    # internal: compile a (sub)pattern
    emit = code.append
    for op, av in pattern:
        if op in (LITERAL, NOT_LITERAL):
            if flags & SRE_FLAG_IGNORECASE:
                emit(OPCODES[OP_IGNORE[op]])
            else:
                emit(OPCODES[op])
            emit(av)
        elif op is IN:
            if flags & SRE_FLAG_IGNORECASE:
                emit(OPCODES[OP_IGNORE[op]])
                def fixup(literal, flags=flags):
                    return _sre.getlower(literal, flags)
            else:
                emit(OPCODES[op])
		fixup = lambda x: x
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
                        emit(CHCODES[CH_LOCALE[av]])
                    elif flags & SRE_FLAG_UNICODE:
                        emit(CHCODES[CH_UNICODE[av]])
                    else:
                        emit(CHCODES[av])
                else:
                    raise error, "internal: unsupported set operator"
            emit(OPCODES[FAILURE])
            code[skip] = len(code) - skip
        elif op is ANY:
            if flags & SRE_FLAG_DOTALL:
                emit(OPCODES[op])
            else:
                emit(OPCODES[CATEGORY])
                emit(CHCODES[CATEGORY_NOT_LINEBREAK])
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
        elif op in (SUCCESS, FAILURE):
            emit(OPCODES[op])
        elif op in (ASSERT, ASSERT_NOT, CALL):
            emit(OPCODES[op])
            skip = len(code); emit(0)
            _compile(code, av, flags)
            emit(OPCODES[SUCCESS])
            code[skip] = len(code) - skip
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
        elif op is CATEGORY:
            emit(OPCODES[op])
            if flags & SRE_FLAG_LOCALE:
                emit(CHCODES[CH_LOCALE[av]])
            elif flags & SRE_FLAG_UNICODE:
                emit(CHCODES[CH_UNICODE[av]])
            else:
                emit(CHCODES[av])
        elif op is GROUP:
            if flags & SRE_FLAG_IGNORECASE:
                emit(OPCODES[OP_IGNORE[op]])
            else:
                emit(OPCODES[op])
            emit(av-1)
        elif op is MARK:
            emit(OPCODES[op])
            emit(av)
        else:
            raise ValueError, ("unsupported operand type", op)

def _compile_info(code, pattern, flags):
    # internal: compile an info block.  in the current version,
    # this contains min/max pattern width and a literal prefix,
    # if any
    lo, hi = pattern.getwidth()
    if lo == 0:
        return # not worth it
    # look for a literal prefix
    prefix = []
    if not (flags & SRE_FLAG_IGNORECASE):
        for op, av in pattern.data:
            if op is LITERAL:
                prefix.append(av)
            else:
                break
    # add an info block
    emit = code.append
    emit(OPCODES[INFO])
    skip = len(code); emit(0)
    # literal flag
    mask = 0
    if len(prefix) == len(pattern.data):
        mask = 1
    emit(mask)
    # pattern length
    emit(lo)
    if hi < 32768:
        emit(hi)
    else:
        emit(0)
    # add literal prefix
    emit(len(prefix))
    if prefix:
        code.extend(prefix)
        # generate overlap table
        table = [-1] + ([0]*len(prefix))
        for i in range(len(prefix)):
            table[i+1] = table[i]+1
            while table[i+1] > 0 and prefix[i] != prefix[table[i+1]-1]:
                table[i+1] = table[table[i+1]-1]+1
        code.extend(table[1:]) # don't store first entry
    code[skip] = len(code) - skip

def compile(p, flags=0):
    # internal: convert pattern list to internal format

    # compile, as necessary
    if type(p) in (type(""), type(u"")):
        import sre_parse
        pattern = p
        p = sre_parse.parse(p)
    else:
        pattern = None

    flags = p.pattern.flags | flags
    code = []

    # compile info block
    _compile_info(code, p, flags)

    # compile the pattern
    _compile(code, p.data, flags)

    code.append(OPCODES[SUCCESS])

    # FIXME: <fl> get rid of this limitation!
    assert p.pattern.groups <= 100,\
           "sorry, but this version only supports 100 named groups"

    return _sre.compile(
        pattern, flags,
        array.array(WORDSIZE, code).tostring(),
        p.pattern.groups-1, p.pattern.groupdict
        )

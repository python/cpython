#
# Secret Labs' Regular Expression Engine
#
# convert template to internal format
#
# Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
#
# See the sre.py file for information on usage and redistribution.
#

import _sre

from sre_constants import *

MAXCODE = 65535

def _charset(charset, fixup=None):
    # internal: optimize character set
    if not fixup:
        fixup = lambda x: x
    out = []
    charmap = [0]*256
    try:
        for op, av in charset:
            if op is NEGATE:
                out.append((op, av))
            elif op is LITERAL:
                charmap[fixup(av)] = 1
            elif op is RANGE:
                for i in range(fixup(av[0]), fixup(av[1])+1):
                    charmap[i] = 1
            elif op is CATEGORY:
                # FIXME: could append to charmap tail
                return charset # cannot compress
    except IndexError:
        # unicode
        return charset
    # compress character map
    i = p = n = 0
    runs = []
    for c in charmap:
        if c:
            if n == 0:
                p = i
            n = n + 1
        elif n:
            runs.append((p, n))
            n = 0
        i = i + 1
    if n:
        runs.append((p, n))
    if len(runs) <= 2:
        # use literal/range
        for p, n in runs:
            if n == 1:
                out.append((LITERAL, p))
            else:
                out.append((RANGE, (p, p+n-1)))
        if len(out) < len(charset):
            return out
    else:
        # use bitmap
        data = []
        m = 1; v = 0
        for c in charmap:
            if c:
                v = v + m
            m = m << 1
            if m > MAXCODE:
                data.append(v)
                m = 1; v = 0
        out.append((CHARSET, data))
        return out
    return charset

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
            for op, av in _charset(av, fixup):
                emit(OPCODES[op])
                if op is NEGATE:
                    pass
                elif op is LITERAL:
                    emit(fixup(av))
                elif op is RANGE:
                    emit(fixup(av[0]))
                    emit(fixup(av[1]))
                elif op is CHARSET:
                    code.extend(av)
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
                raise error, "internal: unsupported template operator"
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
                    # FIXME: <fl> fast and wrong (but we'll fix that)
                    emit(OPCODES[REPEAT_ONE])
                    skip = len(code); emit(0)
                    emit(av[0])
                    emit(av[1])
                    _compile(code, av[2], flags)
                    emit(OPCODES[SUCCESS])
                    code[skip] = len(code) - skip
                else:
                    emit(OPCODES[REPEAT])
                    skip = len(code); emit(0)
                    emit(av[0])
                    emit(av[1])
                    _compile(code, av[2], flags)
                    code[skip] = len(code) - skip
                    if op == MAX_REPEAT:
                        emit(OPCODES[MAX_UNTIL])
                    else:
                        emit(OPCODES[MIN_UNTIL])
        elif op is SUBPATTERN:
            if av[0]:
                emit(OPCODES[MARK])
                emit((av[0]-1)*2)
            _compile(code, av[1], flags)
            if av[0]:
                emit(OPCODES[MARK])
                emit((av[0]-1)*2+1)
        elif op in (SUCCESS, FAILURE):
            emit(OPCODES[op])
        elif op in (ASSERT, ASSERT_NOT):
            emit(OPCODES[op])
            skip = len(code); emit(0)
            if av[0] >= 0:
                emit(0) # look ahead
            else:
                lo, hi = av[1].getwidth()
                if lo != hi:
                    raise error, "look-behind requires fixed-width pattern"
                emit(lo) # look behind
            _compile(code, av[1], flags)
            emit(OPCODES[SUCCESS])
            code[skip] = len(code) - skip
        elif op is CALL:
            emit(OPCODES[op])
            skip = len(code); emit(0)
            _compile(code, av, flags)
            emit(OPCODES[SUCCESS])
            code[skip] = len(code) - skip
        elif op is AT:
            emit(OPCODES[op])
            if flags & SRE_FLAG_MULTILINE:
                emit(ATCODES[AT_MULTILINE.get(av, av)])
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
        elif op is GROUPREF:
            if flags & SRE_FLAG_IGNORECASE:
                emit(OPCODES[OP_IGNORE[op]])
            else:
                emit(OPCODES[op])
            emit(av-1)
        else:
            raise ValueError, ("unsupported operand type", op)

def _compile_info(code, pattern, flags):
    # internal: compile an info block.  in the current version,
    # this contains min/max pattern width, and an optional literal
    # prefix or a character map
    lo, hi = pattern.getwidth()
    if lo == 0:
        return # not worth it
    # look for a literal prefix
    prefix = []
    charset = [] # not used
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
    if prefix:
        mask = SRE_INFO_PREFIX
        if len(prefix) == len(pattern.data):
            mask = mask + SRE_INFO_LITERAL
    elif charset:
        mask = mask + SRE_INFO_CHARSET
    emit(mask)
    # pattern length
    if lo < MAXCODE:
        emit(lo)
    else:
        emit(MAXCODE)
        prefix = prefix[:MAXCODE]
    if hi < MAXCODE:
        emit(hi)
    else:
        emit(0)
    # add literal prefix
    if prefix:
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
    elif charset:
        # FIXME: use charset optimizer!
        for char in charset:
            emit(OPCODES[LITERAL])
            emit(char)
        emit(OPCODES[FAILURE])
    code[skip] = len(code) - skip

STRING_TYPES = [type("")]

try:
    STRING_TYPES.append(type(unicode("")))
except NameError:
    pass

def _code(p, flags):

    flags = p.pattern.flags | flags
    code = []

    # compile info block
    _compile_info(code, p, flags)

    # compile the pattern
    _compile(code, p.data, flags)

    code.append(OPCODES[SUCCESS])

    return code

def compile(p, flags=0):
    # internal: convert pattern list to internal format

    if type(p) in STRING_TYPES:
        import sre_parse
        pattern = p
        p = sre_parse.parse(p, flags)
    else:
        pattern = None

    code = _code(p, flags)

    # print code

    # FIXME: <fl> get rid of this limitation!
    assert p.pattern.groups <= 100,\
           "sorry, but this version only supports 100 named groups"

    # map in either direction
    groupindex = p.pattern.groupdict
    indexgroup = [None] * p.pattern.groups
    for k, i in groupindex.items():
        indexgroup[i] = k

    return _sre.compile(
        pattern, flags, code,
        p.pattern.groups-1,
        groupindex, indexgroup
        )

#
# Secret Labs' Regular Expression Engine
#
# convert template to internal format
#
# Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
#
# See the sre.py file for information on usage and redistribution.
#

"""Internal support module for sre"""

import _sre
import sre_parse
from sre_constants import *

assert _sre.MAGIC == MAGIC, "SRE module mismatch"

_LITERAL_CODES = {LITERAL, NOT_LITERAL}
_REPEATING_CODES = {REPEAT, MIN_REPEAT, MAX_REPEAT}
_SUCCESS_CODES = {SUCCESS, FAILURE}
_ASSERT_CODES = {ASSERT, ASSERT_NOT}

# Sets of lowercase characters which have the same uppercase.
_equivalences = (
    # LATIN SMALL LETTER I, LATIN SMALL LETTER DOTLESS I
    (0x69, 0x131), # iı
    # LATIN SMALL LETTER S, LATIN SMALL LETTER LONG S
    (0x73, 0x17f), # sſ
    # MICRO SIGN, GREEK SMALL LETTER MU
    (0xb5, 0x3bc), # µμ
    # COMBINING GREEK YPOGEGRAMMENI, GREEK SMALL LETTER IOTA, GREEK PROSGEGRAMMENI
    (0x345, 0x3b9, 0x1fbe), # \u0345ιι
    # GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS, GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA
    (0x390, 0x1fd3), # ΐΐ
    # GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS, GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA
    (0x3b0, 0x1fe3), # ΰΰ
    # GREEK SMALL LETTER BETA, GREEK BETA SYMBOL
    (0x3b2, 0x3d0), # βϐ
    # GREEK SMALL LETTER EPSILON, GREEK LUNATE EPSILON SYMBOL
    (0x3b5, 0x3f5), # εϵ
    # GREEK SMALL LETTER THETA, GREEK THETA SYMBOL
    (0x3b8, 0x3d1), # θϑ
    # GREEK SMALL LETTER KAPPA, GREEK KAPPA SYMBOL
    (0x3ba, 0x3f0), # κϰ
    # GREEK SMALL LETTER PI, GREEK PI SYMBOL
    (0x3c0, 0x3d6), # πϖ
    # GREEK SMALL LETTER RHO, GREEK RHO SYMBOL
    (0x3c1, 0x3f1), # ρϱ
    # GREEK SMALL LETTER FINAL SIGMA, GREEK SMALL LETTER SIGMA
    (0x3c2, 0x3c3), # ςσ
    # GREEK SMALL LETTER PHI, GREEK PHI SYMBOL
    (0x3c6, 0x3d5), # φϕ
    # LATIN SMALL LETTER S WITH DOT ABOVE, LATIN SMALL LETTER LONG S WITH DOT ABOVE
    (0x1e61, 0x1e9b), # ṡẛ
    # LATIN SMALL LIGATURE LONG S T, LATIN SMALL LIGATURE ST
    (0xfb05, 0xfb06), # ﬅﬆ
)

# Maps the lowercase code to lowercase codes which have the same uppercase.
_ignorecase_fixes = {i: tuple(j for j in t if i != j)
                     for t in _equivalences for i in t}

def _compile(code, pattern, flags):
    # internal: compile a (sub)pattern
    emit = code.append
    _len = len
    LITERAL_CODES = _LITERAL_CODES
    REPEATING_CODES = _REPEATING_CODES
    SUCCESS_CODES = _SUCCESS_CODES
    ASSERT_CODES = _ASSERT_CODES
    if (flags & SRE_FLAG_IGNORECASE and
            not (flags & SRE_FLAG_LOCALE) and
            flags & SRE_FLAG_UNICODE and
            not (flags & SRE_FLAG_ASCII)):
        fixes = _ignorecase_fixes
    else:
        fixes = None
    for op, av in pattern:
        if op in LITERAL_CODES:
            if flags & SRE_FLAG_IGNORECASE:
                lo = _sre.getlower(av, flags)
                if fixes and lo in fixes:
                    emit(IN_IGNORE)
                    skip = _len(code); emit(0)
                    if op is NOT_LITERAL:
                        emit(NEGATE)
                    for k in (lo,) + fixes[lo]:
                        emit(LITERAL)
                        emit(k)
                    emit(FAILURE)
                    code[skip] = _len(code) - skip
                else:
                    emit(OP_IGNORE[op])
                    emit(lo)
            else:
                emit(op)
                emit(av)
        elif op is IN:
            if flags & SRE_FLAG_IGNORECASE:
                emit(OP_IGNORE[op])
                def fixup(literal, flags=flags):
                    return _sre.getlower(literal, flags)
            else:
                emit(op)
                fixup = None
            skip = _len(code); emit(0)
            _compile_charset(av, flags, code, fixup, fixes)
            code[skip] = _len(code) - skip
        elif op is ANY:
            if flags & SRE_FLAG_DOTALL:
                emit(ANY_ALL)
            else:
                emit(ANY)
        elif op in REPEATING_CODES:
            if flags & SRE_FLAG_TEMPLATE:
                raise error("internal: unsupported template operator %r" % (op,))
            elif _simple(av) and op is not REPEAT:
                if op is MAX_REPEAT:
                    emit(REPEAT_ONE)
                else:
                    emit(MIN_REPEAT_ONE)
                skip = _len(code); emit(0)
                emit(av[0])
                emit(av[1])
                _compile(code, av[2], flags)
                emit(SUCCESS)
                code[skip] = _len(code) - skip
            else:
                emit(REPEAT)
                skip = _len(code); emit(0)
                emit(av[0])
                emit(av[1])
                _compile(code, av[2], flags)
                code[skip] = _len(code) - skip
                if op is MAX_REPEAT:
                    emit(MAX_UNTIL)
                else:
                    emit(MIN_UNTIL)
        elif op is SUBPATTERN:
            group, add_flags, del_flags, p = av
            if group:
                emit(MARK)
                emit((group-1)*2)
            # _compile_info(code, p, (flags | add_flags) & ~del_flags)
            _compile(code, p, (flags | add_flags) & ~del_flags)
            if group:
                emit(MARK)
                emit((group-1)*2+1)
        elif op in SUCCESS_CODES:
            emit(op)
        elif op in ASSERT_CODES:
            emit(op)
            skip = _len(code); emit(0)
            if av[0] >= 0:
                emit(0) # look ahead
            else:
                lo, hi = av[1].getwidth()
                if lo != hi:
                    raise error("look-behind requires fixed-width pattern")
                emit(lo) # look behind
            _compile(code, av[1], flags)
            emit(SUCCESS)
            code[skip] = _len(code) - skip
        elif op is CALL:
            emit(op)
            skip = _len(code); emit(0)
            _compile(code, av, flags)
            emit(SUCCESS)
            code[skip] = _len(code) - skip
        elif op is AT:
            emit(op)
            if flags & SRE_FLAG_MULTILINE:
                av = AT_MULTILINE.get(av, av)
            if flags & SRE_FLAG_LOCALE:
                av = AT_LOCALE.get(av, av)
            elif (flags & SRE_FLAG_UNICODE) and not (flags & SRE_FLAG_ASCII):
                av = AT_UNICODE.get(av, av)
            emit(av)
        elif op is BRANCH:
            emit(op)
            tail = []
            tailappend = tail.append
            for av in av[1]:
                skip = _len(code); emit(0)
                # _compile_info(code, av, flags)
                _compile(code, av, flags)
                emit(JUMP)
                tailappend(_len(code)); emit(0)
                code[skip] = _len(code) - skip
            emit(FAILURE) # end of branch
            for tail in tail:
                code[tail] = _len(code) - tail
        elif op is CATEGORY:
            emit(op)
            if flags & SRE_FLAG_LOCALE:
                av = CH_LOCALE[av]
            elif (flags & SRE_FLAG_UNICODE) and not (flags & SRE_FLAG_ASCII):
                av = CH_UNICODE[av]
            emit(av)
        elif op is GROUPREF:
            if flags & SRE_FLAG_IGNORECASE:
                emit(OP_IGNORE[op])
            else:
                emit(op)
            emit(av-1)
        elif op is GROUPREF_EXISTS:
            emit(op)
            emit(av[0]-1)
            skipyes = _len(code); emit(0)
            _compile(code, av[1], flags)
            if av[2]:
                emit(JUMP)
                skipno = _len(code); emit(0)
                code[skipyes] = _len(code) - skipyes + 1
                _compile(code, av[2], flags)
                code[skipno] = _len(code) - skipno
            else:
                code[skipyes] = _len(code) - skipyes + 1
        else:
            raise error("internal: unsupported operand type %r" % (op,))

def _compile_charset(charset, flags, code, fixup=None, fixes=None):
    # compile charset subprogram
    emit = code.append
    for op, av in _optimize_charset(charset, fixup, fixes):
        emit(op)
        if op is NEGATE:
            pass
        elif op is LITERAL:
            emit(av)
        elif op is RANGE or op is RANGE_IGNORE:
            emit(av[0])
            emit(av[1])
        elif op is CHARSET:
            code.extend(av)
        elif op is BIGCHARSET:
            code.extend(av)
        elif op is CATEGORY:
            if flags & SRE_FLAG_LOCALE:
                emit(CH_LOCALE[av])
            elif (flags & SRE_FLAG_UNICODE) and not (flags & SRE_FLAG_ASCII):
                emit(CH_UNICODE[av])
            else:
                emit(av)
        else:
            raise error("internal: unsupported set operator %r" % (op,))
    emit(FAILURE)

def _optimize_charset(charset, fixup, fixes):
    # internal: optimize character set
    out = []
    tail = []
    charmap = bytearray(256)
    for op, av in charset:
        while True:
            try:
                if op is LITERAL:
                    if fixup:
                        lo = fixup(av)
                        charmap[lo] = 1
                        if fixes and lo in fixes:
                            for k in fixes[lo]:
                                charmap[k] = 1
                    else:
                        charmap[av] = 1
                elif op is RANGE:
                    r = range(av[0], av[1]+1)
                    if fixup:
                        r = map(fixup, r)
                    if fixup and fixes:
                        for i in r:
                            charmap[i] = 1
                            if i in fixes:
                                for k in fixes[i]:
                                    charmap[k] = 1
                    else:
                        for i in r:
                            charmap[i] = 1
                elif op is NEGATE:
                    out.append((op, av))
                else:
                    tail.append((op, av))
            except IndexError:
                if len(charmap) == 256:
                    # character set contains non-UCS1 character codes
                    charmap += b'\0' * 0xff00
                    continue
                # Character set contains non-BMP character codes.
                # There are only two ranges of cased non-BMP characters:
                # 10400-1044F (Deseret) and 118A0-118DF (Warang Citi),
                # and for both ranges RANGE_IGNORE works.
                if fixup and op is RANGE:
                    op = RANGE_IGNORE
                tail.append((op, av))
            break

    # compress character map
    runs = []
    q = 0
    while True:
        p = charmap.find(1, q)
        if p < 0:
            break
        if len(runs) >= 2:
            runs = None
            break
        q = charmap.find(0, p)
        if q < 0:
            runs.append((p, len(charmap)))
            break
        runs.append((p, q))
    if runs is not None:
        # use literal/range
        for p, q in runs:
            if q - p == 1:
                out.append((LITERAL, p))
            else:
                out.append((RANGE, (p, q - 1)))
        out += tail
        # if the case was changed or new representation is more compact
        if fixup or len(out) < len(charset):
            return out
        # else original character set is good enough
        return charset

    # use bitmap
    if len(charmap) == 256:
        data = _mk_bitmap(charmap)
        out.append((CHARSET, data))
        out += tail
        return out

    # To represent a big charset, first a bitmap of all characters in the
    # set is constructed. Then, this bitmap is sliced into chunks of 256
    # characters, duplicate chunks are eliminated, and each chunk is
    # given a number. In the compiled expression, the charset is
    # represented by a 32-bit word sequence, consisting of one word for
    # the number of different chunks, a sequence of 256 bytes (64 words)
    # of chunk numbers indexed by their original chunk position, and a
    # sequence of 256-bit chunks (8 words each).

    # Compression is normally good: in a typical charset, large ranges of
    # Unicode will be either completely excluded (e.g. if only cyrillic
    # letters are to be matched), or completely included (e.g. if large
    # subranges of Kanji match). These ranges will be represented by
    # chunks of all one-bits or all zero-bits.

    # Matching can be also done efficiently: the more significant byte of
    # the Unicode character is an index into the chunk number, and the
    # less significant byte is a bit index in the chunk (just like the
    # CHARSET matching).

    charmap = bytes(charmap) # should be hashable
    comps = {}
    mapping = bytearray(256)
    block = 0
    data = bytearray()
    for i in range(0, 65536, 256):
        chunk = charmap[i: i + 256]
        if chunk in comps:
            mapping[i // 256] = comps[chunk]
        else:
            mapping[i // 256] = comps[chunk] = block
            block += 1
            data += chunk
    data = _mk_bitmap(data)
    data[0:0] = [block] + _bytes_to_codes(mapping)
    out.append((BIGCHARSET, data))
    out += tail
    return out

_CODEBITS = _sre.CODESIZE * 8
MAXCODE = (1 << _CODEBITS) - 1
_BITS_TRANS = b'0' + b'1' * 255
def _mk_bitmap(bits, _CODEBITS=_CODEBITS, _int=int):
    s = bits.translate(_BITS_TRANS)[::-1]
    return [_int(s[i - _CODEBITS: i], 2)
            for i in range(len(s), 0, -_CODEBITS)]

def _bytes_to_codes(b):
    # Convert block indices to word array
    a = memoryview(b).cast('I')
    assert a.itemsize == _sre.CODESIZE
    assert len(a) * a.itemsize == len(b)
    return a.tolist()

def _simple(av):
    # check if av is a "simple" operator
    lo, hi = av[2].getwidth()
    return lo == hi == 1 and av[2][0][0] != SUBPATTERN

def _generate_overlap_table(prefix):
    """
    Generate an overlap table for the following prefix.
    An overlap table is a table of the same size as the prefix which
    informs about the potential self-overlap for each index in the prefix:
    - if overlap[i] == 0, prefix[i:] can't overlap prefix[0:...]
    - if overlap[i] == k with 0 < k <= i, prefix[i-k+1:i+1] overlaps with
      prefix[0:k]
    """
    table = [0] * len(prefix)
    for i in range(1, len(prefix)):
        idx = table[i - 1]
        while prefix[i] != prefix[idx]:
            if idx == 0:
                table[i] = 0
                break
            idx = table[idx - 1]
        else:
            table[i] = idx + 1
    return table

def _get_literal_prefix(pattern):
    # look for literal prefix
    prefix = []
    prefixappend = prefix.append
    prefix_skip = None
    for op, av in pattern.data:
        if op is LITERAL:
            prefixappend(av)
        elif op is SUBPATTERN:
            group, add_flags, del_flags, p = av
            if add_flags & SRE_FLAG_IGNORECASE:
                break
            prefix1, prefix_skip1, got_all = _get_literal_prefix(p)
            if prefix_skip is None:
                if group is not None:
                    prefix_skip = len(prefix)
                elif prefix_skip1 is not None:
                    prefix_skip = len(prefix) + prefix_skip1
            prefix.extend(prefix1)
            if not got_all:
                break
        else:
            break
    else:
        return prefix, prefix_skip, True
    return prefix, prefix_skip, False

def _get_charset_prefix(pattern):
    charset = [] # not used
    charsetappend = charset.append
    if pattern.data:
        op, av = pattern.data[0]
        if op is SUBPATTERN:
            group, add_flags, del_flags, p = av
            if p and not (add_flags & SRE_FLAG_IGNORECASE):
                op, av = p[0]
                if op is LITERAL:
                    charsetappend((op, av))
                elif op is BRANCH:
                    c = []
                    cappend = c.append
                    for p in av[1]:
                        if not p:
                            break
                        op, av = p[0]
                        if op is LITERAL:
                            cappend((op, av))
                        else:
                            break
                    else:
                        charset = c
        elif op is BRANCH:
            c = []
            cappend = c.append
            for p in av[1]:
                if not p:
                    break
                op, av = p[0]
                if op is LITERAL:
                    cappend((op, av))
                else:
                    break
            else:
                charset = c
        elif op is IN:
            charset = av
    return charset

def _compile_info(code, pattern, flags):
    # internal: compile an info block.  in the current version,
    # this contains min/max pattern width, and an optional literal
    # prefix or a character map
    lo, hi = pattern.getwidth()
    if hi > MAXCODE:
        hi = MAXCODE
    if lo == 0:
        code.extend([INFO, 4, 0, lo, hi])
        return
    # look for a literal prefix
    prefix = []
    prefix_skip = 0
    charset = [] # not used
    if not (flags & SRE_FLAG_IGNORECASE):
        # look for literal prefix
        prefix, prefix_skip, got_all = _get_literal_prefix(pattern)
        # if no prefix, look for charset prefix
        if not prefix:
            charset = _get_charset_prefix(pattern)
##     if prefix:
##         print("*** PREFIX", prefix, prefix_skip)
##     if charset:
##         print("*** CHARSET", charset)
    # add an info block
    emit = code.append
    emit(INFO)
    skip = len(code); emit(0)
    # literal flag
    mask = 0
    if prefix:
        mask = SRE_INFO_PREFIX
        if prefix_skip is None and got_all:
            mask = mask | SRE_INFO_LITERAL
    elif charset:
        mask = mask | SRE_INFO_CHARSET
    emit(mask)
    # pattern length
    if lo < MAXCODE:
        emit(lo)
    else:
        emit(MAXCODE)
        prefix = prefix[:MAXCODE]
    emit(min(hi, MAXCODE))
    # add literal prefix
    if prefix:
        emit(len(prefix)) # length
        if prefix_skip is None:
            prefix_skip =  len(prefix)
        emit(prefix_skip) # skip
        code.extend(prefix)
        # generate overlap table
        code.extend(_generate_overlap_table(prefix))
    elif charset:
        _compile_charset(charset, flags, code)
    code[skip] = len(code) - skip

def isstring(obj):
    return isinstance(obj, (str, bytes))

def _code(p, flags):

    flags = p.pattern.flags | flags
    code = []

    # compile info block
    _compile_info(code, p, flags)

    # compile the pattern
    _compile(code, p.data, flags)

    code.append(SUCCESS)

    return code

def compile(p, flags=0):
    # internal: convert pattern list to internal format

    if isstring(p):
        pattern = p
        p = sre_parse.parse(p, flags)
    else:
        pattern = None

    code = _code(p, flags)

    # print(code)

    # map in either direction
    groupindex = p.pattern.groupdict
    indexgroup = [None] * p.pattern.groups
    for k, i in groupindex.items():
        indexgroup[i] = k

    return _sre.compile(
        pattern, flags | p.pattern.flags, code,
        p.pattern.groups-1,
        groupindex, indexgroup
        )

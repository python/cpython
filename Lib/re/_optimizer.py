#
# Secret Labs' Regular Expression Engine
#
# optimizations for the compiler
#
# Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
#
# See the __init__.py file for information on usage and redistribution.
#

"""Internal support module for sre.

Optimization passes used by the compiler: character-set optimization
(:func:`_optimize_charset`), the "simple" repeat-body test (:func:`_simple`),
and the literal/charset prefix info block (:func:`_compile_info`).
"""

import _sre
from . import _parser
from ._casefix import _EXTRA_CASES
from ._constants import *

_CHARSET_ALL = [(NEGATE, None)]
_UNIT_CODES = {LITERAL, NOT_LITERAL, ANY, IN, CATEGORY}

def _combine_flags(flags, add_flags, del_flags,
                   TYPE_FLAGS=_parser.TYPE_FLAGS):
    if add_flags & TYPE_FLAGS:
        flags &= ~TYPE_FLAGS
    return (flags | add_flags) & ~del_flags

def _compile_charset(charset, flags, code):
    # compile charset subprogram
    emit = code.append
    for op, av in charset:
        emit(op)
        if op is NEGATE:
            pass
        elif op is LITERAL:
            emit(av)
        elif op is RANGE or op is RANGE_UNI_IGNORE:
            emit(av[0])
            emit(av[1])
        elif op is CHARSET:
            code.extend(av)
        elif op is BIGCHARSET:
            code.extend(av)
        elif op is CATEGORY:
            if flags & SRE_FLAG_LOCALE:
                emit(CH_LOCALE[av])
            elif flags & SRE_FLAG_UNICODE:
                emit(CH_UNICODE[av])
            else:
                emit(av)
        else:
            raise PatternError(f"internal: unsupported set operator {op!r}")
    emit(FAILURE)

def _optimize_charset(charset, iscased=None, fixup=None, fixes=None):
    # internal: optimize character set.
    #
    # The engine's charset() walk toggles polarity on every NEGATE (see
    # Modules/_sre/sre_lib.h), so NEGATE markers split the set into
    # alternating-polarity segments: a leading NEGATE is a complemented class
    # [^...], an interior one is set difference (RL1.3).  Each segment is a
    # plain union, optimized on its own with the NEGATE boundaries kept in place.
    negates = [i for i, (op, _av) in enumerate(charset) if op is NEGATE]
    if not negates or negates == [0]:
        # Fast path: a plain union, optionally complemented as a whole -- every
        # charset the parser produces today, optimized as before.
        return _optimize_charset_segment(charset, iscased, fixup, fixes)

    # Optimize each NEGATE-delimited run on its own.  _allow_anyall is off: the
    # [\s\S] -> ANY_ALL / [^\s\S] -> empty shortcuts rewrite a whole set and
    # would inject or drop a NEGATE mid-segment.
    out = []
    hascased = False
    start = 0
    for i in negates + [len(charset)]:
        if i > start:                  # skip an empty run (e.g. a leading NEGATE)
            opt, cased = _optimize_charset_segment(
                charset[start:i], iscased, fixup, fixes, _allow_anyall=False)
            out.extend(opt)
            hascased |= cased
        if i < len(charset):
            out.append((NEGATE, None))
        start = i + 1
    return out, hascased

def _optimize_charset_segment(charset, iscased=None, fixup=None, fixes=None,
                              _allow_anyall=True):
    # internal: optimize one NEGATE-free union of character-set members
    out = []
    tail = []
    charmap = bytearray(256)
    hascased = False
    for op, av in charset:
        while True:
            try:
                if op is LITERAL:
                    if fixup: # IGNORECASE and not LOCALE
                        av = fixup(av)
                        charmap[av] = 1
                        if fixes and av in fixes:
                            for k in fixes[av]:
                                charmap[k] = 1
                        if not hascased and iscased(av):
                            hascased = True
                    else:
                        charmap[av] = 1
                elif op is RANGE:
                    r = range(av[0], av[1]+1)
                    if fixup: # IGNORECASE and not LOCALE
                        if fixes:
                            for i in map(fixup, r):
                                charmap[i] = 1
                                if i in fixes:
                                    for k in fixes[i]:
                                        charmap[k] = 1
                        else:
                            for i in map(fixup, r):
                                charmap[i] = 1
                        if not hascased:
                            hascased = any(map(iscased, r))
                    else:
                        for i in r:
                            charmap[i] = 1
                elif op is NEGATE:
                    out.append((op, av))
                elif op is CATEGORY and _allow_anyall and tail and (CATEGORY, CH_NEGATE[av]) in tail:
                    # Optimize [\s\S] etc.
                    out = [] if out else _CHARSET_ALL
                    return out, False
                else:
                    tail.append((op, av))
            except IndexError:
                if len(charmap) == 256:
                    # character set contains non-UCS1 character codes
                    charmap += b'\0' * 0xff00
                    continue
                # Character set contains non-BMP character codes.
                # For range, all BMP characters in the range are already
                # proceeded.
                if fixup: # IGNORECASE and not LOCALE
                    # For now, IN_UNI_IGNORE+LITERAL and
                    # IN_UNI_IGNORE+RANGE_UNI_IGNORE work for all non-BMP
                    # characters, because two characters (at least one of
                    # which is not in the BMP) match case-insensitively
                    # if and only if:
                    # 1) c1.lower() == c2.lower()
                    # 2) c1.lower() == c2 or c1.lower().upper() == c2
                    # Also, both c.lower() and c.lower().upper() are single
                    # characters for every non-BMP character.
                    if op is RANGE:
                        if fixes: # not ASCII
                            op = RANGE_UNI_IGNORE
                        hascased = True
                    else:
                        assert op is LITERAL
                        if not hascased and iscased(av):
                            hascased = True
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
        if hascased or len(out) < len(charset):
            return out, hascased
        # else original character set is good enough
        return charset, hascased

    # use bitmap
    if len(charmap) == 256:
        data = _mk_bitmap(charmap)
        out.append((CHARSET, data))
        out += tail
        return out, hascased

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

    charmap = charmap.take_bytes() # should be hashable
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
    return out, hascased

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

def _simple(p):
    # check if this subpattern is a "simple" operator
    if len(p) != 1:
        return False
    op, av = p[0]
    if op is SUBPATTERN:
        return av[0] is None and _simple(av[-1])
    return op in _UNIT_CODES

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

def _get_iscased(flags):
    if not flags & SRE_FLAG_IGNORECASE:
        return None
    elif flags & SRE_FLAG_UNICODE:
        return _sre.unicode_iscased
    else:
        return _sre.ascii_iscased

def _get_literal_prefix(pattern, flags):
    # look for literal prefix
    prefix = []
    prefixappend = prefix.append
    prefix_skip = None
    iscased = _get_iscased(flags)
    for op, av in pattern.data:
        if op is LITERAL:
            if iscased and iscased(av):
                break
            prefixappend(av)
        elif op is SUBPATTERN:
            group, add_flags, del_flags, p = av
            flags1 = _combine_flags(flags, add_flags, del_flags)
            if flags1 & SRE_FLAG_IGNORECASE and flags1 & SRE_FLAG_LOCALE:
                break
            prefix1, prefix_skip1, got_all = _get_literal_prefix(p, flags1)
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

def _get_charset_prefix(pattern, flags):
    while True:
        if not pattern.data:
            return None
        op, av = pattern.data[0]
        if op is not SUBPATTERN:
            break
        group, add_flags, del_flags, pattern = av
        flags = _combine_flags(flags, add_flags, del_flags)
        if flags & SRE_FLAG_IGNORECASE and flags & SRE_FLAG_LOCALE:
            return None

    iscased = _get_iscased(flags)
    if op is LITERAL:
        if iscased and iscased(av):
            return None
        return [(op, av)]
    elif op is CATEGORY:
        return [(op, av)]
    elif op is BRANCH:
        charset = []
        charsetappend = charset.append
        for p in av[1]:
            if not p:
                return None
            op, av = p[0]
            if op is LITERAL and not (iscased and iscased(av)):
                charsetappend((op, av))
            else:
                return None
        return charset
    elif op is IN:
        charset = av
        if iscased:
            for op, av in charset:
                if op is LITERAL:
                    if iscased(av):
                        return None
                elif op is RANGE:
                    if av[1] > 0xffff:
                        return None
                    if any(map(iscased, range(av[0], av[1]+1))):
                        return None
        return charset
    return None

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
    charset = None # not used
    if not (flags & SRE_FLAG_IGNORECASE and flags & SRE_FLAG_LOCALE):
        # look for literal prefix
        prefix, prefix_skip, got_all = _get_literal_prefix(pattern, flags)
        # if no prefix, look for charset prefix
        if not prefix:
            charset = _get_charset_prefix(pattern, flags)
            if charset:
                charset, hascased = _optimize_charset(charset)
                assert not hascased
                if charset == _CHARSET_ALL:
                    charset = None
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
    emit(hi)
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


# --- Auto-possessification pass ---------------------------------------------

_REPEAT_CODES = frozenset({MIN_REPEAT, MAX_REPEAT, POSSESSIVE_REPEAT})
_POSSESSIFY_UNITS = frozenset({LITERAL, NOT_LITERAL, ANY, ANY_ALL, IN, CATEGORY})

# \d, \w, \s and the line break category as unions of disjoint "atoms":
# d=digit, l=word non-digit, b=line break, s=space non-line-break, o=other.
# digit<=word, linebreak<=space and word disjoint from space hold for both
# ASCII and Unicode, so disjoint atom sets mean really disjoint categories.
_CAT_UNIVERSE = frozenset('dlbso')
_CAT_ATOMS = {
    CATEGORY_DIGIT:     frozenset('d'),
    CATEGORY_WORD:      frozenset('dl'),
    CATEGORY_SPACE:     frozenset('bs'),
    CATEGORY_LINEBREAK: frozenset('b'),
}
_CAT_ATOMS.update({
    CATEGORY_NOT_DIGIT:     _CAT_UNIVERSE - _CAT_ATOMS[CATEGORY_DIGIT],
    CATEGORY_NOT_WORD:      _CAT_UNIVERSE - _CAT_ATOMS[CATEGORY_WORD],
    CATEGORY_NOT_SPACE:     _CAT_UNIVERSE - _CAT_ATOMS[CATEGORY_SPACE],
    CATEGORY_NOT_LINEBREAK: _CAT_UNIVERSE - _CAT_ATOMS[CATEGORY_LINEBREAK],
})

_ASCII_SPACE = frozenset(b' \t\n\r\f\v')
_ASCII_WORD = frozenset(b'_') | frozenset(
    range(0x30, 0x3a)) | frozenset(range(0x41, 0x5b)) | frozenset(range(0x61, 0x7b))
_PROBE_LIMIT = 64  # cap on the size of a finite atom used as a witness set
_FOLLOW_LIMIT = 64  # cap on a follower set: an empty branch alternative
                    # re-appends the continuation, exponential for (|)(|)...
_DEPTH_LIMIT = 100  # cap on the follower-scan recursion: one level per group

def _tolower(c, flags):
    if flags & SRE_FLAG_UNICODE:
        return _sre.unicode_tolower(c)
    return _sre.ascii_tolower(c)

def _fold_set(c, flags):
    # Code points witnessing what LITERAL c matches: tolower() of any match
    # lies here and each element is itself a match (simple tolower plus
    # _EXTRA_CASES, like the IGNORECASE matcher).
    if not (flags & SRE_FLAG_IGNORECASE) or flags & SRE_FLAG_LOCALE:
        return (c,)
    lo = _tolower(c, flags)
    if flags & SRE_FLAG_UNICODE:
        extra = _EXTRA_CASES.get(lo)
        if extra:
            return (lo, *extra)
    return (lo,)

def _lit_matches(d, c, flags):
    # Whether LITERAL d matches input code point c.
    if not (flags & SRE_FLAG_IGNORECASE) or flags & SRE_FLAG_LOCALE:
        return c == d
    return _tolower(c, flags) in _fold_set(d, flags)

# Categories whose membership is invariant under case folding (verified over
# the full range); the others cannot be decided under IGNORECASE, where a
# charset member is matched against the lowercased character.
_FOLD_CLOSED = frozenset({
    CATEGORY_DIGIT, CATEGORY_WORD, CATEGORY_SPACE, CATEGORY_LINEBREAK,
    CATEGORY_NUMERIC, CATEGORY_PRINTABLE, CATEGORY_N, CATEGORY_LM,
    CATEGORY_NL, CATEGORY_NO, CATEGORY_CF, CATEGORY_Z, CATEGORY_ZS,
    CATEGORY_C, CATEGORY_CN, CATEGORY_XID_CONTINUE, CATEGORY_ASSIGNED,
    CATEGORY_BLANK, CATEGORY_GRAPH, CATEGORY_PRINT, CATEGORY_CASED,
})
_FOLD_CLOSED |= frozenset(CH_NEGATE[cat] for cat in _FOLD_CLOSED)

def _cat_matches(cat, c, flags):
    # Whether category cat matches code point c, decided by the engine's own
    # predicate; None if it depends on the runtime locale or on case folding.
    if flags & SRE_FLAG_LOCALE:
        return None
    if flags & SRE_FLAG_IGNORECASE and cat not in _FOLD_CLOSED:
        return None
    if flags & SRE_FLAG_UNICODE:
        cat = CH_UNICODE[cat]
    return _sre.category_matches(cat, c)

def _member_matches(op, av, c, flags):
    # Whether a charset member (op, av) matches code point c.  None if unknown.
    if op is LITERAL:
        return _lit_matches(av, c, flags)
    if op is RANGE:
        lo, hi = av
        if lo <= c <= hi:
            return True
        if not (flags & SRE_FLAG_IGNORECASE) or flags & SRE_FLAG_LOCALE:
            return False
        if lo <= _tolower(c, flags) <= hi or any(lo <= x <= hi
                                                 for x in _fold_set(c, flags)):
            return True
        return None  # case folding into the range can't be ruled out cheaply
    if op is CATEGORY:
        return _cat_matches(av, c, flags)
    return None

def _atom_matches(op, av, c, flags):
    # Whether the one-character atom (op, av) matches code point c.
    # Returns None when it cannot be decided (callers treat that as "maybe").
    if op is LITERAL:
        return _lit_matches(av, c, flags)
    if op is NOT_LITERAL:
        return not _lit_matches(av, c, flags)
    if op is CATEGORY:
        return _cat_matches(av, c, flags)
    if op is ANY:
        return True if flags & SRE_FLAG_DOTALL else c != 0x0a
    if op is ANY_ALL:
        return True
    if op is IN:
        # Evaluate the charset the way the engine's charset() walk does:
        # NEGATE toggles the polarity, a member hit returns the current
        # polarity, and the end returns the complement of the final one
        # (this also covers difference-fused charsets, see _fuse_difference).
        ok = True
        results = set()
        for iop, iav in av:
            if iop is NEGATE:
                ok = not ok
                continue
            r = _member_matches(iop, iav, c, flags)
            if r:
                results.add(ok)
                break
            if r is None:
                results.add(ok)     # may or may not hit this member
        else:
            results.add(not ok)
        if len(results) == 1:
            return results.pop()
        return None
    return None

def _finite_set(op, av, flags):
    # The set of code points the atom matches, if finite and small; else None.
    if op is LITERAL:
        return set(_fold_set(av, flags))
    if op is IN:
        if av and av[0] == (NEGATE, None):
            return None
        out = set()
        for iop, iav in av:
            if iop is LITERAL:
                out.update(_fold_set(iav, flags))
            elif iop is RANGE:
                if iav[1] - iav[0] >= _PROBE_LIMIT:
                    return None
                for x in range(iav[0], iav[1] + 1):
                    out.update(_fold_set(x, flags))
            elif iop is CATEGORY:
                if flags & SRE_FLAG_LOCALE or flags & SRE_FLAG_UNICODE:
                    return None  # Unicode/locale categories are not small
                if iav is CATEGORY_DIGIT:
                    out.update(range(0x30, 0x3a))
                elif iav is CATEGORY_WORD:
                    out.update(_ASCII_WORD)
                elif iav is CATEGORY_SPACE:
                    out.update(_ASCII_SPACE)
                elif iav is CATEGORY_LINEBREAK:
                    out.add(0x0a)
                else:
                    return None  # a negated ASCII category is not small
            else:
                return None
            if len(out) > _PROBE_LIMIT:
                return None
        return out
    return None

def _cat_atom_set(op, av):
    # The dlbso atom set the atom matches, if it is a bare category or a
    # charset of categories (the first member claiming an atom decides it
    # with the current NEGATE polarity, the end claims the rest).
    if op is CATEGORY:
        return _CAT_ATOMS.get(av)
    if op is not IN:
        return None
    ok = True
    decided = set()
    matched = set()
    for iop, iav in av:
        if iop is NEGATE:
            ok = not ok
            continue
        if iop is not CATEGORY:
            if not ok:
                # a non-category member of a fail segment only narrows the
                # set; ignoring it over-approximates, which stays sound
                continue
            return None
        atoms = _CAT_ATOMS.get(iav)
        if atoms is None:
            return None
        if ok:
            matched |= atoms - decided
        decided |= atoms
    if not ok:
        matched |= _CAT_UNIVERSE - decided
    return matched

def _as_single_category(op, av):
    # The category code if the atom is a bare category or a single-category
    # class, else None.
    if op is CATEGORY:
        return av
    if op is IN and len(av) == 1 and av[0][0] is CATEGORY:
        return av[0][1]
    return None

def _disjoint(atom, other, flags):
    # True only if atom and other provably cannot match a common character.
    if flags & SRE_FLAG_LOCALE and flags & SRE_FLAG_IGNORECASE:
        # case folding is decided by the runtime locale; prove nothing
        return False
    ca = _as_single_category(*atom)
    if ca is not None:
        cb = _as_single_category(*other)
        # A category and its complement are disjoint whatever they mean --
        # but only within one flag context (unicode \w and ascii \W
        # overlap), which holds because the walk never compares atoms
        # across a flag-scoping boundary.
        if cb is not None and cb == CH_NEGATE[ca]:
            return True
    if not (flags & SRE_FLAG_LOCALE):
        a1 = _cat_atom_set(*atom)
        if a1 is not None:
            a2 = _cat_atom_set(*other)
            if a2 is not None and a1.isdisjoint(a2):
                return True
    fa = _finite_set(*atom, flags)
    fb = _finite_set(*other, flags)
    if fa is not None and fb is not None:
        return fa.isdisjoint(fb)
    if fa is not None:
        return not any(_atom_matches(*other, c, flags) is not False for c in fa)
    if fb is not None:
        return not any(_atom_matches(*atom, c, flags) is not False for c in fb)
    return False

def _leading_atom(data):
    # The leading atom of a rigid body -- a concatenation of single-character
    # atoms with no internal choice.  A repeat of it gives back only whole
    # iterations, so its leading atom is all the follower must avoid.
    lead = None
    for op, av in data:
        if op is SUBPATTERN and not av[1] and not av[2]:
            a = _leading_atom(av[3].data)
        elif op is ATOMIC_GROUP:
            a = _leading_atom(av.data)
        elif op in _POSSESSIFY_UNITS:
            a = (op, av)
        else:
            return None
        if a is None:
            return None
        if lead is None:
            lead = a
    return lead

def _first_consumers(seq, i, flags, cont, depth=0):
    # Atoms for every character that could be consumed at position i of seq;
    # cont is the same for what follows seq.  None if it can't be analyzed.
    if depth >= _DEPTH_LIMIT:
        return None
    depth += 1
    acc = []
    n = len(seq)
    while i < n:
        op, av = seq[i]
        if op in _POSSESSIFY_UNITS:
            acc.append((op, av))
            return acc
        if op is SUBPATTERN:
            if av[1] or av[2]:
                return None  # flag-scoping group: atoms can't carry their flags
            after = _first_consumers(seq, i + 1, flags, cont, depth)
            if after is None:
                return None
            inner = _first_consumers(av[3].data, 0, flags, after, depth)
            return None if inner is None else acc + inner
        if op is ATOMIC_GROUP:
            after = _first_consumers(seq, i + 1, flags, cont, depth)
            if after is None:
                return None
            inner = _first_consumers(av.data, 0, flags, after, depth)
            return None if inner is None else acc + inner
        if op is BRANCH:
            after = _first_consumers(seq, i + 1, flags, cont, depth)
            if after is None:
                return None
            for alt in av[1]:
                a = _first_consumers(alt.data, 0, flags, after, depth)
                if a is None or len(acc) + len(a) > _FOLLOW_LIMIT:
                    return None
                acc += a
            return acc
        if op in _REPEAT_CODES:
            mn, mx, p = av
            sub = _first_consumers(p.data, 0, flags, None, depth)
            if sub is None or len(acc) + len(sub) > _FOLLOW_LIMIT:
                return None
            acc += sub
            if mn == 0:
                i += 1
                continue
            return acc
        if op is AT and av is AT_END_STRING:
            # \z matches only at the very end; backtracking the repeat moves
            # earlier and can never satisfy it, so nothing need be disjoint.
            return acc
        if op is AT and av is AT_END:
            # $ is like \z but also matches before a '\n'.  Only MULTILINE
            # exposes an interior one to backtracking, and then only if the
            # repeat can match '\n'.
            if flags & SRE_FLAG_MULTILINE:
                return acc + [(LITERAL, 0x0a)]
            return acc
        return None  # assertion, anchor, group reference, ... -> give up
    if cont is None or len(acc) + len(cont) > _FOLLOW_LIMIT:
        return None
    return acc + cont


# Difference-fusion peephole: rewrite [A--B]-style A(?<![B]) into a single
# charset (see the engine's NEGATE polarity toggle).
def _subpatterns(op, av):
    # Yield the nested SubPatterns of one item, to recurse into.
    if op is BRANCH:
        yield from av[1]
    elif op in (ASSERT, ASSERT_NOT):
        yield av[1]
    elif op is SUBPATTERN:
        yield av[3]
    elif op is ATOMIC_GROUP:
        yield av
    elif op in (MIN_REPEAT, MAX_REPEAT, POSSESSIVE_REPEAT):
        yield av[2]
    elif op is GROUPREF_EXISTS:
        yield av[1]                 # the "yes" branch is always present
        if av[2] is not None:       # the "no" branch is optional
            yield av[2]

def _fuse_branch(av):
    # Fold a BRANCH of one-character matchers into a single charset: their union
    # is the concatenation of the item lists.  charset() lets only the final
    # polarity segment subtract, so at most one alternative may be
    # complement-bearing (carry a NEGATE) and it must trail; two would cross
    # (e.g. [a-z--b]||[a-z--c]) and are left as a BRANCH.
    items = []
    tail = None
    for sp in av[1]:
        cs = _parser._flat_items(sp.data, True)
        if cs is None:
            return None
        if any(op is NEGATE for op, _av in cs):
            if tail is not None:
                return None
            tail = cs
        else:
            items += cs
    return items if tail is None else items + tail

def _fuse_difference(data):
    # Replace  <flat charset A> (?<![B1]) (?<![B2]) ...  with the single charset
    # [NEGATE] B1 B2 ... [NEGATE] A.  Each negative lookbehind over a flat
    # charset subtracts its set from the character A matches.
    out = []
    head = None        # _flat_items(A) for the fused difference now at out[-1]
    subtrahend = None  # its accumulated B items, or None when not fusing
    for op, av in data:
        if op is ASSERT_NOT and av[0] < 0:          # a negative lookbehind
            b = _parser._flat_items(av[1].data)
            if b is not None:
                if subtrahend is None and out:
                    # the first lookbehind of a run: only now is it worth
                    # checking whether the preceding item A is a flat charset.
                    head = _parser._flat_items([out[-1]])
                    if head is not None:
                        subtrahend = []
                if subtrahend is not None:
                    subtrahend += b
                    out[-1] = (IN, [(NEGATE, None)] + subtrahend
                                   + [(NEGATE, None)] + head)
                    continue
        head = subtrahend = None
        out.append((op, av))
    data[:] = out

def _walk(seq, flags, cont):
    # Rewrite the sequence in place: fuse set-operation charsets (see
    # _fuse_branch and _fuse_difference) and turn greedy repeats possessive
    # where the repeated atom and every possible follower are disjoint.
    for i, (op, av) in enumerate(seq):
        if op is SUBPATTERN:
            if av[1] or av[2]:
                # flag-scoping group: optimize inside it, but its boundary is
                # opaque since atoms there match under different flags
                _walk(av[3].data, _combine_flags(flags, av[1], av[2]), None)
            else:
                _walk(av[3].data, flags,
                      _first_consumers(seq, i + 1, flags, cont))
        elif op is BRANCH:
            after = _first_consumers(seq, i + 1, flags, cont)
            for alt in av[1]:
                _walk(alt.data, flags, after)
            items = _fuse_branch(av)
            if items is not None:
                seq[i] = (IN, items)
        else:
            # ATOMIC_GROUP / ASSERT(_NOT) / GROUPREF_EXISTS / repeat body have an
            # opaque boundary -- optimize inside with no follower context.
            for sub in _subpatterns(op, av):
                _walk(sub.data, flags, None)
            if op is MAX_REPEAT:
                atom = _leading_atom(av[2].data)
                if atom is not None:
                    follow = _first_consumers(seq, i + 1, flags, cont)
                    if follow is not None and \
                            all(_disjoint(atom, f, flags) for f in follow):
                        seq[i] = (POSSESSIVE_REPEAT, av)
    _fuse_difference(seq)

def optimize(pattern, flags):
    """Rewrite a parsed pattern in place and return it."""
    try:
        _walk(pattern.data, flags, [])
    except RecursionError:
        # A backstop for _DEPTH_LIMIT; the rewrites already applied are sound.
        pass
    return pattern

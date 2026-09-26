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
        if op in (NEGATE, INVERT):
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
    # The engine's charset() walk toggles its return polarity on every NEGATE
    # and its membership-test direction on every INVERT (see sre_lib.h) --
    # together they express complement, difference and intersection (RL1.3).
    # Each toggle-delimited segment is optimized on its own.
    bounds = [i for i, (op, _av) in enumerate(charset)
              if op in (NEGATE, INVERT)]
    if not bounds or (bounds == [0] and charset[0][0] is NEGATE):
        # Fast path: a plain union, optionally complemented as a whole -- every
        # charset the parser produces today, optimized as before.
        return _optimize_charset_segment(charset, iscased, fixup, fixes)

    # Optimize each toggle-delimited run on its own.  _allow_anyall is off: the
    # [\s\S] -> ANY_ALL / [^\s\S] -> empty shortcuts rewrite a whole set and
    # would inject or drop a toggle mid-segment.
    out = []
    hascased = False
    start = 0
    inv = False
    for i in bounds + [len(charset)]:
        if i > start:                  # skip an empty run (e.g. a leading NEGATE)
            if inv:
                # An INVERT run intersects its members: re-emit them as
                # they are (already optimized, see _single_member).
                out.extend(charset[start:i])
            else:
                opt, cased = _optimize_charset_segment(
                    charset[start:i], iscased, fixup, fixes, _allow_anyall=False)
                out.extend(opt)
                hascased |= cased
        if i < len(charset):
            tok = charset[i]
            out.append(tok)            # re-emit the toggle in place
            if tok[0] is INVERT:
                inv = not inv
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

def _single_member(operand):
    # Reduce a flat-charset operand to one member opcode (a lone bitmap,
    # range or category), or None.  An intersection operand must be a single
    # member, because each member under INVERT is a separate test.
    items = _parser._flat_items(operand)
    if items is None:
        return None
    opt, _hascased = _optimize_charset(items)
    return opt if len(opt) == 1 else None

def _fuse_setops(data, flags):
    # Fuse  <flat charset A> (?<![B]) (?<=[C]) ...  -- a left-associative
    # chain of set difference ([A--B]) and intersection ([A&&C]) -- into the
    # single charset  [NEGATE] <fail items> [NEGATE] [INVERT] A.  The chain
    # is a pure conjunction, so each lookbehind appends one fail item:
    # "ch in B" for a difference, "ch not in C" (under INVERT) for an
    # intersection.  Not fused under IGNORECASE, where case folding could
    # split a single-member intersection operand (see _single_member).
    out = []
    head = None    # _flat_items(A) for the fused set operation now at out[-1]
    fails = None   # its accumulated fail items, or None when not fusing
    inv = 0        # the INVERT state at the end of fails
    for op, av in data:
        if op in (ASSERT, ASSERT_NOT) and av[0] < 0:    # a lookbehind
            if op is ASSERT_NOT:                        # -- difference
                b = _parser._flat_items(av[1].data)
                tinv = 0
            elif not flags & SRE_FLAG_IGNORECASE:       # && intersection
                b = _single_member(av[1].data)
                tinv = 1
            else:
                b = None
            if b is not None:
                if fails is None and out:
                    # the first lookbehind of a run: only now is it worth
                    # checking whether the preceding item A is a flat charset.
                    head = _parser._flat_items([out[-1]])
                    if head is not None:
                        fails = []
                        inv = 0
                if fails is not None:
                    if inv != tinv:
                        fails.append((INVERT, None))
                        inv ^= 1
                    fails += b
                    tail = [(NEGATE, None)]
                    if inv:
                        tail.append((INVERT, None))
                    out[-1] = (IN, [(NEGATE, None)] + fails + tail + head)
                    continue
        head = fails = None
        out.append((op, av))
    data[:] = out

def _walk(seq, flags):
    for i, (op, av) in enumerate(seq):
        if op is SUBPATTERN:
            # A group can change the flag context, e.g. (?i:...).
            _walk(av[3].data, _combine_flags(flags, av[1], av[2]))
        else:
            for sub in _subpatterns(op, av):
                _walk(sub.data, flags)
        if op is BRANCH:
            items = _fuse_branch(av)
            if items is not None:
                seq[i] = (IN, items)
    _fuse_setops(seq, flags)

def optimize(pattern, flags):
    """Rewrite a parsed pattern in place and return it."""
    _walk(pattern.data, flags)
    return pattern

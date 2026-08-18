#
# Secret Labs' Regular Expression Engine
#
# support for Unicode property escapes \p{...} and \P{...}
#
# See https://unicode.org/reports/tr18/ "Unicode Regular Expressions",
# requirement RL1.2 "Properties".
#
# The supported properties are matched either as CATEGORY opcodes, or as fixed
# sets of character ranges:
#
# * Properties emitted as CATEGORY opcodes (see _CATEGORY_PROPERTIES): \d, \s
#   and \w (as digit, space and word, honouring the ASCII/LOCALE/UNICODE
#   flags), the binary properties Alphabetic, Lowercase, Uppercase, Numeric,
#   Printable, alnum, XID_Start, XID_Continue, Cased and Case_Ignorable, and
#   the POSIX classes blank, graph, print and assigned.
#
# * General_Category values (see _GC_CATEGORY): L, Lt, Nd, Lu, N, Lm, Nl, No,
#   Cf, Z, Zs, C and Cn (combinations of the simple predicates), plus Cc, Cs,
#   Co, Zl and Zp as fixed ranges (see _GC_ANALYTIC).
#
# * Code-point classes given by fixed ranges (see _analytic_ranges): ASCII,
#   Any, Noncharacter_Code_Point, Join_Control, Regional_Indicator, xdigit,
#   ASCII_Hex_Digit, Hex_Digit, cntrl, and the immutable Pattern_Syntax and
#   Pattern_White_Space.
#

from ._constants import (
    IN, CATEGORY, NEGATE, RANGE, LITERAL,
    CATEGORY_DIGIT, CATEGORY_NOT_DIGIT,
    CATEGORY_SPACE, CATEGORY_NOT_SPACE,
    CATEGORY_WORD, CATEGORY_NOT_WORD,
    CATEGORY_ALPHA, CATEGORY_NOT_ALPHA,
    CATEGORY_LOWER, CATEGORY_NOT_LOWER,
    CATEGORY_UPPER, CATEGORY_NOT_UPPER,
    CATEGORY_NUMERIC, CATEGORY_NOT_NUMERIC,
    CATEGORY_PRINTABLE, CATEGORY_NOT_PRINTABLE,
    CATEGORY_ALNUM, CATEGORY_NOT_ALNUM,
    CATEGORY_XID_START, CATEGORY_NOT_XID_START,
    CATEGORY_XID_CONTINUE, CATEGORY_NOT_XID_CONTINUE,
    CATEGORY_TITLE, CATEGORY_NOT_TITLE,
    CATEGORY_CASED, CATEGORY_NOT_CASED,
    CATEGORY_CASE_IGNORABLE, CATEGORY_NOT_CASE_IGNORABLE,
    CATEGORY_LU, CATEGORY_NOT_LU,
    CATEGORY_N, CATEGORY_NOT_N,
    CATEGORY_LM, CATEGORY_NOT_LM,
    CATEGORY_NL, CATEGORY_NOT_NL,
    CATEGORY_NO, CATEGORY_NOT_NO,
    CATEGORY_CF, CATEGORY_NOT_CF,
    CATEGORY_Z, CATEGORY_NOT_Z,
    CATEGORY_ZS, CATEGORY_NOT_ZS,
    CATEGORY_C, CATEGORY_NOT_C,
    CATEGORY_CN, CATEGORY_NOT_CN,
    CATEGORY_ASSIGNED, CATEGORY_NOT_ASSIGNED,
    CATEGORY_BLANK, CATEGORY_NOT_BLANK,
    CATEGORY_GRAPH, CATEGORY_NOT_GRAPH,
    CATEGORY_PRINT, CATEGORY_NOT_PRINT,
)

MAXUNICODE = 0x10FFFF

# Properties implemented directly by the engine as (positive, negative)
# CATEGORY codes.  The keys are normalised (see _normalize).  digit, space and
# word reuse the \d, \s and \w categories and so are affected by the ASCII,
# LOCALE and UNICODE flags; the rest are plain Unicode properties and are not.
_CATEGORY_PROPERTIES = {
    "digit":      (CATEGORY_DIGIT, CATEGORY_NOT_DIGIT),  # same as \d
    "space":      (CATEGORY_SPACE, CATEGORY_NOT_SPACE),  # same as \s
    # \p{White_Space} is approximated by \s (str.isspace), which also matches
    # the information separators U+001C..U+001F.
    "whitespace": (CATEGORY_SPACE, CATEGORY_NOT_SPACE),
    "wspace":     (CATEGORY_SPACE, CATEGORY_NOT_SPACE),
    "word":       (CATEGORY_WORD,  CATEGORY_NOT_WORD),   # same as \w

    "alphabetic": (CATEGORY_ALPHA, CATEGORY_NOT_ALPHA),
    "alpha":      (CATEGORY_ALPHA, CATEGORY_NOT_ALPHA),  # POSIX
    "lowercase":  (CATEGORY_LOWER, CATEGORY_NOT_LOWER),
    "lower":      (CATEGORY_LOWER, CATEGORY_NOT_LOWER),  # POSIX
    "uppercase":  (CATEGORY_UPPER, CATEGORY_NOT_UPPER),
    "upper":      (CATEGORY_UPPER, CATEGORY_NOT_UPPER),  # POSIX
    "numeric":    (CATEGORY_NUMERIC, CATEGORY_NOT_NUMERIC),
    "printable":  (CATEGORY_PRINTABLE, CATEGORY_NOT_PRINTABLE),
    "cased":      (CATEGORY_CASED, CATEGORY_NOT_CASED),
    "caseignorable": (CATEGORY_CASE_IGNORABLE, CATEGORY_NOT_CASE_IGNORABLE),
    # POSIX classes, the compatibility properties of UTS #18 Annex C (see the
    # compound predicates in sre.c).
    "blank":      (CATEGORY_BLANK, CATEGORY_NOT_BLANK),
    "graph":      (CATEGORY_GRAPH, CATEGORY_NOT_GRAPH),
    "print":      (CATEGORY_PRINT, CATEGORY_NOT_PRINT),
    "assigned":   (CATEGORY_ASSIGNED, CATEGORY_NOT_ASSIGNED),
    "alnum":      (CATEGORY_ALNUM, CATEGORY_NOT_ALNUM),  # POSIX
    "xidstart":    (CATEGORY_XID_START, CATEGORY_NOT_XID_START),
    "xids":        (CATEGORY_XID_START, CATEGORY_NOT_XID_START),
    "xidcontinue": (CATEGORY_XID_CONTINUE, CATEGORY_NOT_XID_CONTINUE),
    "xidc":        (CATEGORY_XID_CONTINUE, CATEGORY_NOT_XID_CONTINUE),
}

# General_Category values matched by an engine category.  CATEGORY_ALPHA
# matches exactly the L group, and CATEGORY_TITLE the Lt category;
# CATEGORY_DIGIT matches Nd (but, like \d, is restricted to ASCII under the
# ASCII flag).  The gc group memberships (L = Lu|Ll|Lt|Lm|Lo, N = Nd|Nl|No)
# are given by the Unicode Standard 4.5, Table 4-4 "General_Category Values"
# (https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-4/#G124142)
# and listed in
# https://www.unicode.org/Public/UCD/latest/ucd/PropertyValueAliases.txt
# The compound categories Lu, N, Lm, Nl, No, Cf, Z, Zs, C and Cn are
# combinations of the simple predicates (see sre.c) that reproduce the
# canonical gc partition; they are not Unicode-published identities.
_GC_CATEGORY = {
    "l":  (CATEGORY_ALPHA, CATEGORY_NOT_ALPHA),
    "lt": (CATEGORY_TITLE, CATEGORY_NOT_TITLE),
    "nd": (CATEGORY_DIGIT, CATEGORY_NOT_DIGIT),
    "lu": (CATEGORY_LU, CATEGORY_NOT_LU),
    "n":  (CATEGORY_N, CATEGORY_NOT_N),
    "lm": (CATEGORY_LM, CATEGORY_NOT_LM),
    "nl": (CATEGORY_NL, CATEGORY_NOT_NL),
    "no": (CATEGORY_NO, CATEGORY_NOT_NO),
    "cf": (CATEGORY_CF, CATEGORY_NOT_CF),
    "z":  (CATEGORY_Z, CATEGORY_NOT_Z),
    "zs": (CATEGORY_ZS, CATEGORY_NOT_ZS),
    "c":  (CATEGORY_C, CATEGORY_NOT_C),
    "cn": (CATEGORY_CN, CATEGORY_NOT_CN),
}

# General_Category values whose members are fixed in every Unicode version,
# so they need no table: Cc (control, = POSIX cntrl), Cs (surrogates), Co
# (private use) and the single code points Zl and Zp.  Cc, Cs and Co are the
# control codes, surrogate and private-use areas, fixed by the Unicode
# Standard 23.1, 23.6 and 23.5:
# https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-23/
# All five are listed in
# https://www.unicode.org/Public/UCD/latest/ucd/extracted/DerivedGeneralCategory.txt
_CC_RANGES = [(0x00, 0x1F), (0x7F, 0x9F)]
_CS_RANGES = [(0xD800, 0xDFFF)]
_CO_RANGES = [(0xE000, 0xF8FF), (0xF0000, 0xFFFFD), (0x100000, 0x10FFFD)]
_GC_ANALYTIC = {
    "cc": _CC_RANGES,
    "cs": _CS_RANGES,
    "co": _CO_RANGES,
    "zl": [(0x2028, 0x2028)],
    "zp": [(0x2029, 0x2029)],
}

# Pattern_Syntax and Pattern_White_Space are guaranteed immutable by the
# Unicode stability policy, so their members can be hardcoded.
# UAX #31 1.1, "Stability": https://www.unicode.org/reports/tr31/
# Members listed in https://www.unicode.org/Public/UCD/latest/ucd/PropList.txt
_PATTERN_WHITE_SPACE_RANGES = [
    (0x0009, 0x000D), (0x0020, 0x0020), (0x0085, 0x0085), (0x200E, 0x200F),
    (0x2028, 0x2029),
]
_PATTERN_SYNTAX_RANGES = [
    (0x0021, 0x002F), (0x003A, 0x0040), (0x005B, 0x005E), (0x0060, 0x0060),
    (0x007B, 0x007E), (0x00A1, 0x00A7), (0x00A9, 0x00A9), (0x00AB, 0x00AC),
    (0x00AE, 0x00AE), (0x00B0, 0x00B1), (0x00B6, 0x00B6), (0x00BB, 0x00BB),
    (0x00BF, 0x00BF), (0x00D7, 0x00D7), (0x00F7, 0x00F7), (0x2010, 0x2027),
    (0x2030, 0x203E), (0x2041, 0x2053), (0x2055, 0x205E), (0x2190, 0x245F),
    (0x2500, 0x2775), (0x2794, 0x2BFF), (0x2E00, 0x2E7F), (0x3001, 0x3003),
    (0x3008, 0x3020), (0x3030, 0x3030), (0xFD3E, 0xFD3F), (0xFE45, 0xFE46),
]

# Normalised property names that introduce a General_Category value.  A bare
# \p{Lu} is shorthand for \p{gc=Lu} (UTS #18 1.2.4, "Property Syntax").
_GC_KEYS = frozenset({"gc", "generalcategory"})

# Normalised value names for the truth value of a binary property; Yes/No and
# True/False are the binary value aliases of PropertyValueAliases.txt.
_TRUE_VALUES = frozenset({"yes", "y", "true", "t"})
_FALSE_VALUES = frozenset({"no", "n", "false", "f"})


def _analytic_ranges():
    # Properties whose members follow directly from the code point.  Keys are
    # normalised.
    # Noncharacter_Code_Point: U+FDD0..FDEF and the last two of every plane,
    # permanently reserved (the Unicode Standard 23.7, "Noncharacters":
    # https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-23/).
    noncharacter = [(0xFDD0, 0xFDEF)]
    noncharacter += [(plane | 0xFFFE, plane | 0xFFFF)
                     for plane in range(0, MAXUNICODE + 1, 0x10000)]
    # Regional_Indicator (RI): the 26 enclosed symbols A..Z, a complete fixed
    # block (PropList.txt binary property).
    regional_indicator = [(0x1F1E6, 0x1F1FF)]
    # ASCII_Hex_Digit (= POSIX xdigit) and Hex_Digit, which adds the fullwidth
    # forms.  Both are complete, fixed sets (PropList.txt binary properties).
    ascii_hex = [(0x30, 0x39), (0x41, 0x46), (0x61, 0x66)]
    hex_digit = ascii_hex + [(0xFF10, 0xFF19), (0xFF21, 0xFF26), (0xFF41, 0xFF46)]
    return {
        "ascii":                 [(0, 0x7F)],
        "any":                   [(0, MAXUNICODE)],
        # Join_Control (U+200C ZWNJ, U+200D ZWJ; the Unicode Standard 23.2,
        # "Layout Controls"), a PropList.txt binary property.
        "joincontrol":           [(0x200C, 0x200D)],
        "regionalindicator":     regional_indicator,
        "ri":                    regional_indicator,
        "noncharactercodepoint": noncharacter,
        "xdigit":                ascii_hex,   # POSIX, ASCII only
        "asciihexdigit":         ascii_hex,
        "ahex":                  ascii_hex,
        "hexdigit":              hex_digit,
        "hex":                   hex_digit,
        # POSIX cntrl is the General_Category Cc, a fixed set of code points.
        "cntrl":                 _CC_RANGES,
        "patternwhitespace":     _PATTERN_WHITE_SPACE_RANGES,
        "patws":                 _PATTERN_WHITE_SPACE_RANGES,
        "patternsyntax":         _PATTERN_SYNTAX_RANGES,
        "patsyn":                _PATTERN_SYNTAX_RANGES,
    }


def _normalize(name):
    # Unicode property and value names are matched loosely: case, spaces,
    # hyphens and underscores are not significant, and an initial "is" prefix
    # is ignored (UAX #44 5.9, "Matching Rules", UAX44-LM3;
    # https://www.unicode.org/reports/tr44/).
    name = name.lower().replace("_", "").replace("-", "").replace(" ", "")
    # Strip a leading "is", unless "is" is the whole name and so not a prefix
    # (e.g. the Line_Break value lb=IS).
    if name != "is":
        name = name.removeprefix("is")
    return name


def _from_ranges(ranges, negate):
    if ranges is None:
        return None
    items = [(LITERAL, lo) if lo == hi else (RANGE, (lo, hi))
             for lo, hi in ranges]
    if negate:
        items.insert(0, (NEGATE, None))
    return (IN, items)


def _general_category(value, negate):
    # Resolve a General_Category value to a subpattern using an engine category
    # or a fixed range set; unsupported values return None.
    cat = _GC_CATEGORY.get(value)
    if cat is not None:
        return (IN, [(CATEGORY, cat[1] if negate else cat[0])])
    return _from_ranges(_GC_ANALYTIC.get(value), negate)


def _truth(value):
    value = _normalize(value)
    if value in _TRUE_VALUES:
        return True
    if value in _FALSE_VALUES:
        return False
    return None


def parse_property(name, negate):
    """Parse the text inside \\p{...} / \\P{...}.

    Return an (IN, items) subpattern, or None if the property is unknown.
    """
    prop, sep, value = name.partition("=")
    if sep:
        key = _normalize(prop)
        if key in _GC_KEYS:
            return _general_category(_normalize(value), negate)
        # A binary property spelled name=yes or name=no.
        truth = _truth(value)
        if truth is None:
            return None
        negate ^= not truth
        cat = _CATEGORY_PROPERTIES.get(key)
        if cat is not None:
            return (IN, [(CATEGORY, cat[1] if negate else cat[0])])
        return _from_ranges(_analytic_ranges().get(key), negate)

    key = _normalize(name)
    cat = _CATEGORY_PROPERTIES.get(key)
    if cat is not None:
        return (IN, [(CATEGORY, cat[1] if negate else cat[0])])
    ranges = _analytic_ranges().get(key)
    if ranges is not None:
        return _from_ranges(ranges, negate)
    return _general_category(key, negate)

#
# (re)generate unicode property and type databases
#
# This script converts Unicode database files to Modules/unicodedata_db.h,
# Modules/unicodename_db.h, and Objects/unicodetype_db.h
#
# history:
# 2000-09-24 fl   created (based on bits and pieces from unidb)
# 2000-09-25 fl   merged tim's splitbin fixes, separate decomposition table
# 2000-09-25 fl   added character type table
# 2000-09-26 fl   added LINEBREAK, DECIMAL, and DIGIT flags/fields (2.0)
# 2000-11-03 fl   expand first/last ranges
# 2001-01-19 fl   added character name tables (2.1)
# 2001-01-21 fl   added decomp compression; dynamic phrasebook threshold
# 2002-09-11 wd   use string methods
# 2002-10-18 mvl  update to Unicode 3.2
# 2002-10-22 mvl  generate NFC tables
# 2002-11-24 mvl  expand all ranges, sort names version-independently
# 2002-11-25 mvl  add UNIDATA_VERSION
# 2004-05-29 perky add east asian width information
# 2006-03-10 mvl  update to Unicode 4.1; add UCD 3.2 delta
# 2008-06-11 gb   add PRINTABLE_MASK for Atsuo Ishimoto's ascii() patch
# 2011-10-21 ezio add support for name aliases and named sequences
# 2012-01    benjamin add full case mappings
#
# written by Fredrik Lundh (fredrik@pythonware.com)
#

import os
import sys
import zipfile

from textwrap import dedent
from functools import partial

SCRIPT = sys.argv[0]
VERSION = "3.3"

# The Unicode Database
# --------------------
# When changing UCD version please update
#   * Doc/library/stdtypes.rst, and
#   * Doc/library/unicodedata.rst
#   * Doc/reference/lexical_analysis.rst (two occurrences)
UNIDATA_VERSION = "12.1.0"
UNICODE_DATA = "UnicodeData%s.txt"
COMPOSITION_EXCLUSIONS = "CompositionExclusions%s.txt"
EASTASIAN_WIDTH = "EastAsianWidth%s.txt"
UNIHAN = "Unihan%s.zip"
DERIVED_CORE_PROPERTIES = "DerivedCoreProperties%s.txt"
DERIVEDNORMALIZATION_PROPS = "DerivedNormalizationProps%s.txt"
LINE_BREAK = "LineBreak%s.txt"
NAME_ALIASES = "NameAliases%s.txt"
NAMED_SEQUENCES = "NamedSequences%s.txt"
SPECIAL_CASING = "SpecialCasing%s.txt"
CASE_FOLDING = "CaseFolding%s.txt"

# Private Use Areas -- in planes 1, 15, 16
PUA_1 = range(0xE000, 0xF900)
PUA_15 = range(0xF0000, 0xFFFFE)
PUA_16 = range(0x100000, 0x10FFFE)

# we use this ranges of PUA_15 to store name aliases and named sequences
NAME_ALIASES_START = 0xF0000
NAMED_SEQUENCES_START = 0xF0200

old_versions = ["3.2.0"]

CATEGORY_NAMES = [ "Cn", "Lu", "Ll", "Lt", "Mn", "Mc", "Me", "Nd",
    "Nl", "No", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs", "Co", "Cn", "Lm",
    "Lo", "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po", "Sm", "Sc", "Sk",
    "So" ]

BIDIRECTIONAL_NAMES = [ "", "L", "LRE", "LRO", "R", "AL", "RLE", "RLO",
    "PDF", "EN", "ES", "ET", "AN", "CS", "NSM", "BN", "B", "S", "WS",
    "ON", "LRI", "RLI", "FSI", "PDI" ]

EASTASIANWIDTH_NAMES = [ "F", "H", "W", "Na", "A", "N" ]

MANDATORY_LINE_BREAKS = [ "BK", "CR", "LF", "NL" ]

# note: should match definitions in Objects/unicodectype.c
ALPHA_MASK = 0x01
DECIMAL_MASK = 0x02
DIGIT_MASK = 0x04
LOWER_MASK = 0x08
LINEBREAK_MASK = 0x10
SPACE_MASK = 0x20
TITLE_MASK = 0x40
UPPER_MASK = 0x80
XID_START_MASK = 0x100
XID_CONTINUE_MASK = 0x200
PRINTABLE_MASK = 0x400
NUMERIC_MASK = 0x800
CASE_IGNORABLE_MASK = 0x1000
CASED_MASK = 0x2000
EXTENDED_CASE_MASK = 0x4000

# these ranges need to match unicodedata.c:is_unified_ideograph
cjk_ranges = [
    ('3400', '4DB5'),
    ('4E00', '9FEF'),
    ('20000', '2A6D6'),
    ('2A700', '2B734'),
    ('2B740', '2B81D'),
    ('2B820', '2CEA1'),
    ('2CEB0', '2EBE0'),
]


def maketables(trace=0):

    print("--- Reading", UNICODE_DATA % "", "...")

    unicode = UnicodeData(UNIDATA_VERSION)

    print(len(list(filter(None, unicode.table))), "characters")

    for version in old_versions:
        print("--- Reading", UNICODE_DATA % ("-"+version), "...")
        old_unicode = UnicodeData(version, cjk_check=False)
        print(len(list(filter(None, old_unicode.table))), "characters")
        merge_old_version(version, unicode, old_unicode)

    makeunicodename(unicode, trace)
    makeunicodedata(unicode, trace)
    makeunicodetype(unicode, trace)


# --------------------------------------------------------------------
# unicode character properties

def makeunicodedata(unicode, trace):

    dummy = (0, 0, 0, 0, 0, 0)
    table = [dummy]
    cache = {0: dummy}
    index = [0] * len(unicode.chars)

    FILE = "Modules/unicodedata_db.h"

    print("--- Preparing", FILE, "...")

    # 1) database properties

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            # extract database properties
            category = CATEGORY_NAMES.index(record[2])
            combining = int(record[3])
            bidirectional = BIDIRECTIONAL_NAMES.index(record[4])
            mirrored = record[9] == "Y"
            eastasianwidth = EASTASIANWIDTH_NAMES.index(record[15])
            normalizationquickcheck = record[17]
            item = (
                category, combining, bidirectional, mirrored, eastasianwidth,
                normalizationquickcheck
                )
            # add entry to index and item tables
            i = cache.get(item)
            if i is None:
                cache[item] = i = len(table)
                table.append(item)
            index[char] = i

    # 2) decomposition data

    decomp_data = [0]
    decomp_prefix = [""]
    decomp_index = [0] * len(unicode.chars)
    decomp_size = 0

    comp_pairs = []
    comp_first = [None] * len(unicode.chars)
    comp_last = [None] * len(unicode.chars)

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            if record[5]:
                decomp = record[5].split()
                if len(decomp) > 19:
                    raise Exception("character %x has a decomposition too large for nfd_nfkd" % char)
                # prefix
                if decomp[0][0] == "<":
                    prefix = decomp.pop(0)
                else:
                    prefix = ""
                try:
                    i = decomp_prefix.index(prefix)
                except ValueError:
                    i = len(decomp_prefix)
                    decomp_prefix.append(prefix)
                prefix = i
                assert prefix < 256
                # content
                decomp = [prefix + (len(decomp)<<8)] + [int(s, 16) for s in decomp]
                # Collect NFC pairs
                if not prefix and len(decomp) == 3 and \
                   char not in unicode.exclusions and \
                   unicode.table[decomp[1]][3] == "0":
                    p, l, r = decomp
                    comp_first[l] = 1
                    comp_last[r] = 1
                    comp_pairs.append((l,r,char))
                try:
                    i = decomp_data.index(decomp)
                except ValueError:
                    i = len(decomp_data)
                    decomp_data.extend(decomp)
                    decomp_size = decomp_size + len(decomp) * 2
            else:
                i = 0
            decomp_index[char] = i

    f = l = 0
    comp_first_ranges = []
    comp_last_ranges = []
    prev_f = prev_l = None
    for i in unicode.chars:
        if comp_first[i] is not None:
            comp_first[i] = f
            f += 1
            if prev_f is None:
                prev_f = (i,i)
            elif prev_f[1]+1 == i:
                prev_f = prev_f[0],i
            else:
                comp_first_ranges.append(prev_f)
                prev_f = (i,i)
        if comp_last[i] is not None:
            comp_last[i] = l
            l += 1
            if prev_l is None:
                prev_l = (i,i)
            elif prev_l[1]+1 == i:
                prev_l = prev_l[0],i
            else:
                comp_last_ranges.append(prev_l)
                prev_l = (i,i)
    comp_first_ranges.append(prev_f)
    comp_last_ranges.append(prev_l)
    total_first = f
    total_last = l

    comp_data = [0]*(total_first*total_last)
    for f,l,char in comp_pairs:
        f = comp_first[f]
        l = comp_last[l]
        comp_data[f*total_last+l] = char

    print(len(table), "unique properties")
    print(len(decomp_prefix), "unique decomposition prefixes")
    print(len(decomp_data), "unique decomposition entries:", end=' ')
    print(decomp_size, "bytes")
    print(total_first, "first characters in NFC")
    print(total_last, "last characters in NFC")
    print(len(comp_pairs), "NFC pairs")

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)

        fprint("/* this file was generated by %s %s */" % (SCRIPT, VERSION))
        fprint()
        fprint('#define UNIDATA_VERSION "%s"' % UNIDATA_VERSION)
        fprint("/* a list of unique database records */")
        fprint("const _PyUnicode_DatabaseRecord _PyUnicode_Database_Records[] = {")
        for item in table:
            fprint("    {%d, %d, %d, %d, %d, %d}," % item)
        fprint("};")
        fprint()

        fprint("/* Reindexing of NFC first characters. */")
        fprint("#define TOTAL_FIRST",total_first)
        fprint("#define TOTAL_LAST",total_last)
        fprint("struct reindex{int start;short count,index;};")
        fprint("static struct reindex nfc_first[] = {")
        for start,end in comp_first_ranges:
            fprint("    { %d, %d, %d}," % (start,end-start,comp_first[start]))
        fprint("    {0,0,0}")
        fprint("};\n")
        fprint("static struct reindex nfc_last[] = {")
        for start,end in comp_last_ranges:
            fprint("  { %d, %d, %d}," % (start,end-start,comp_last[start]))
        fprint("  {0,0,0}")
        fprint("};\n")

        # FIXME: <fl> the following tables could be made static, and
        # the support code moved into unicodedatabase.c

        fprint("/* string literals */")
        fprint("const char *_PyUnicode_CategoryNames[] = {")
        for name in CATEGORY_NAMES:
            fprint("    \"%s\"," % name)
        fprint("    NULL")
        fprint("};")

        fprint("const char *_PyUnicode_BidirectionalNames[] = {")
        for name in BIDIRECTIONAL_NAMES:
            fprint("    \"%s\"," % name)
        fprint("    NULL")
        fprint("};")

        fprint("const char *_PyUnicode_EastAsianWidthNames[] = {")
        for name in EASTASIANWIDTH_NAMES:
            fprint("    \"%s\"," % name)
        fprint("    NULL")
        fprint("};")

        fprint("static const char *decomp_prefix[] = {")
        for name in decomp_prefix:
            fprint("    \"%s\"," % name)
        fprint("    NULL")
        fprint("};")

        # split record index table
        index1, index2, shift = splitbins(index, trace)

        fprint("/* index tables for the database records */")
        fprint("#define SHIFT", shift)
        Array("index1", index1).dump(fp, trace)
        Array("index2", index2).dump(fp, trace)

        # split decomposition index table
        index1, index2, shift = splitbins(decomp_index, trace)

        fprint("/* decomposition data */")
        Array("decomp_data", decomp_data).dump(fp, trace)

        fprint("/* index tables for the decomposition data */")
        fprint("#define DECOMP_SHIFT", shift)
        Array("decomp_index1", index1).dump(fp, trace)
        Array("decomp_index2", index2).dump(fp, trace)

        index, index2, shift = splitbins(comp_data, trace)
        fprint("/* NFC pairs */")
        fprint("#define COMP_SHIFT", shift)
        Array("comp_index", index).dump(fp, trace)
        Array("comp_data", index2).dump(fp, trace)

        # Generate delta tables for old versions
        for version, table, normalization in unicode.changed:
            cversion = version.replace(".","_")
            records = [table[0]]
            cache = {table[0]:0}
            index = [0] * len(table)
            for i, record in enumerate(table):
                try:
                    index[i] = cache[record]
                except KeyError:
                    index[i] = cache[record] = len(records)
                    records.append(record)
            index1, index2, shift = splitbins(index, trace)
            fprint("static const change_record change_records_%s[] = {" % cversion)
            for record in records:
                fprint("    { %s }," % ", ".join(map(str,record)))
            fprint("};")
            Array("changes_%s_index" % cversion, index1).dump(fp, trace)
            Array("changes_%s_data" % cversion, index2).dump(fp, trace)
            fprint("static const change_record* get_change_%s(Py_UCS4 n)" % cversion)
            fprint("{")
            fprint("    int index;")
            fprint("    if (n >= 0x110000) index = 0;")
            fprint("    else {")
            fprint("        index = changes_%s_index[n>>%d];" % (cversion, shift))
            fprint("        index = changes_%s_data[(index<<%d)+(n & %d)];" % \
                   (cversion, shift, ((1<<shift)-1)))
            fprint("    }")
            fprint("    return change_records_%s+index;" % cversion)
            fprint("}\n")
            fprint("static Py_UCS4 normalization_%s(Py_UCS4 n)" % cversion)
            fprint("{")
            fprint("    switch(n) {")
            for k, v in normalization:
                fprint("    case %s: return 0x%s;" % (hex(k), v))
            fprint("    default: return 0;")
            fprint("    }\n}\n")


# --------------------------------------------------------------------
# unicode character type tables

def makeunicodetype(unicode, trace):

    FILE = "Objects/unicodetype_db.h"

    print("--- Preparing", FILE, "...")

    # extract unicode types
    dummy = (0, 0, 0, 0, 0, 0)
    table = [dummy]
    cache = {0: dummy}
    index = [0] * len(unicode.chars)
    numeric = {}
    spaces = []
    linebreaks = []
    extra_casing = []

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            # extract database properties
            category = record[2]
            bidirectional = record[4]
            properties = record[16]
            flags = 0
            if category in ["Lm", "Lt", "Lu", "Ll", "Lo"]:
                flags |= ALPHA_MASK
            if "Lowercase" in properties:
                flags |= LOWER_MASK
            if 'Line_Break' in properties or bidirectional == "B":
                flags |= LINEBREAK_MASK
                linebreaks.append(char)
            if category == "Zs" or bidirectional in ("WS", "B", "S"):
                flags |= SPACE_MASK
                spaces.append(char)
            if category == "Lt":
                flags |= TITLE_MASK
            if "Uppercase" in properties:
                flags |= UPPER_MASK
            if char == ord(" ") or category[0] not in ("C", "Z"):
                flags |= PRINTABLE_MASK
            if "XID_Start" in properties:
                flags |= XID_START_MASK
            if "XID_Continue" in properties:
                flags |= XID_CONTINUE_MASK
            if "Cased" in properties:
                flags |= CASED_MASK
            if "Case_Ignorable" in properties:
                flags |= CASE_IGNORABLE_MASK
            sc = unicode.special_casing.get(char)
            cf = unicode.case_folding.get(char, [char])
            if record[12]:
                upper = int(record[12], 16)
            else:
                upper = char
            if record[13]:
                lower = int(record[13], 16)
            else:
                lower = char
            if record[14]:
                title = int(record[14], 16)
            else:
                title = upper
            if sc is None and cf != [lower]:
                sc = ([lower], [title], [upper])
            if sc is None:
                if upper == lower == title:
                    upper = lower = title = 0
                else:
                    upper = upper - char
                    lower = lower - char
                    title = title - char
                    assert (abs(upper) <= 2147483647 and
                            abs(lower) <= 2147483647 and
                            abs(title) <= 2147483647)
            else:
                # This happens either when some character maps to more than one
                # character in uppercase, lowercase, or titlecase or the
                # casefolded version of the character is different from the
                # lowercase. The extra characters are stored in a different
                # array.
                flags |= EXTENDED_CASE_MASK
                lower = len(extra_casing) | (len(sc[0]) << 24)
                extra_casing.extend(sc[0])
                if cf != sc[0]:
                    lower |= len(cf) << 20
                    extra_casing.extend(cf)
                upper = len(extra_casing) | (len(sc[2]) << 24)
                extra_casing.extend(sc[2])
                # Title is probably equal to upper.
                if sc[1] == sc[2]:
                    title = upper
                else:
                    title = len(extra_casing) | (len(sc[1]) << 24)
                    extra_casing.extend(sc[1])
            # decimal digit, integer digit
            decimal = 0
            if record[6]:
                flags |= DECIMAL_MASK
                decimal = int(record[6])
            digit = 0
            if record[7]:
                flags |= DIGIT_MASK
                digit = int(record[7])
            if record[8]:
                flags |= NUMERIC_MASK
                numeric.setdefault(record[8], []).append(char)
            item = (
                upper, lower, title, decimal, digit, flags
                )
            # add entry to index and item tables
            i = cache.get(item)
            if i is None:
                cache[item] = i = len(table)
                table.append(item)
            index[char] = i

    print(len(table), "unique character type entries")
    print(sum(map(len, numeric.values())), "numeric code points")
    print(len(spaces), "whitespace code points")
    print(len(linebreaks), "linebreak code points")
    print(len(extra_casing), "extended case array")

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)

        fprint("/* this file was generated by %s %s */" % (SCRIPT, VERSION))
        fprint()
        fprint("/* a list of unique character type descriptors */")
        fprint("const _PyUnicode_TypeRecord _PyUnicode_TypeRecords[] = {")
        for item in table:
            fprint("    {%d, %d, %d, %d, %d, %d}," % item)
        fprint("};")
        fprint()

        fprint("/* extended case mappings */")
        fprint()
        fprint("const Py_UCS4 _PyUnicode_ExtendedCase[] = {")
        for c in extra_casing:
            fprint("    %d," % c)
        fprint("};")
        fprint()

        # split decomposition index table
        index1, index2, shift = splitbins(index, trace)

        fprint("/* type indexes */")
        fprint("#define SHIFT", shift)
        Array("index1", index1).dump(fp, trace)
        Array("index2", index2).dump(fp, trace)

        # Generate code for _PyUnicode_ToNumeric()
        numeric_items = sorted(numeric.items())
        fprint('/* Returns the numeric value as double for Unicode characters')
        fprint(' * having this property, -1.0 otherwise.')
        fprint(' */')
        fprint('double _PyUnicode_ToNumeric(Py_UCS4 ch)')
        fprint('{')
        fprint('    switch (ch) {')
        for value, codepoints in numeric_items:
            # Turn text into float literals
            parts = value.split('/')
            parts = [repr(float(part)) for part in parts]
            value = '/'.join(parts)

            codepoints.sort()
            for codepoint in codepoints:
                fprint('    case 0x%04X:' % (codepoint,))
            fprint('        return (double) %s;' % (value,))
        fprint('    }')
        fprint('    return -1.0;')
        fprint('}')
        fprint()

        # Generate code for _PyUnicode_IsWhitespace()
        fprint("/* Returns 1 for Unicode characters having the bidirectional")
        fprint(" * type 'WS', 'B' or 'S' or the category 'Zs', 0 otherwise.")
        fprint(" */")
        fprint('int _PyUnicode_IsWhitespace(const Py_UCS4 ch)')
        fprint('{')
        fprint('    switch (ch) {')

        for codepoint in sorted(spaces):
            fprint('    case 0x%04X:' % (codepoint,))
        fprint('        return 1;')

        fprint('    }')
        fprint('    return 0;')
        fprint('}')
        fprint()

        # Generate code for _PyUnicode_IsLinebreak()
        fprint("/* Returns 1 for Unicode characters having the line break")
        fprint(" * property 'BK', 'CR', 'LF' or 'NL' or having bidirectional")
        fprint(" * type 'B', 0 otherwise.")
        fprint(" */")
        fprint('int _PyUnicode_IsLinebreak(const Py_UCS4 ch)')
        fprint('{')
        fprint('    switch (ch) {')
        for codepoint in sorted(linebreaks):
            fprint('    case 0x%04X:' % (codepoint,))
        fprint('        return 1;')

        fprint('    }')
        fprint('    return 0;')
        fprint('}')
        fprint()


# --------------------------------------------------------------------
# unicode name database

def makeunicodename(unicode, trace):

    FILE = "Modules/unicodename_db.h"

    print("--- Preparing", FILE, "...")

    # collect names
    names = [None] * len(unicode.chars)

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            name = record[1].strip()
            if name and name[0] != "<":
                names[char] = name + chr(0)

    print(len([n for n in names if n is not None]), "distinct names")

    # collect unique words from names (note that we differ between
    # words inside a sentence, and words ending a sentence.  the
    # latter includes the trailing null byte.

    words = {}
    n = b = 0
    for char in unicode.chars:
        name = names[char]
        if name:
            w = name.split()
            b = b + len(name)
            n = n + len(w)
            for w in w:
                l = words.get(w)
                if l:
                    l.append(None)
                else:
                    words[w] = [len(words)]

    print(n, "words in text;", b, "bytes")

    wordlist = list(words.items())

    # sort on falling frequency, then by name
    def word_key(a):
        aword, alist = a
        return -len(alist), aword
    wordlist.sort(key=word_key)

    # figure out how many phrasebook escapes we need
    escapes = 0
    while escapes * 256 < len(wordlist):
        escapes = escapes + 1
    print(escapes, "escapes")

    short = 256 - escapes

    assert short > 0

    print(short, "short indexes in lexicon")

    # statistics
    n = 0
    for i in range(short):
        n = n + len(wordlist[i][1])
    print(n, "short indexes in phrasebook")

    # pick the most commonly used words, and sort the rest on falling
    # length (to maximize overlap)

    wordlist, wordtail = wordlist[:short], wordlist[short:]
    wordtail.sort(key=lambda a: a[0], reverse=True)
    wordlist.extend(wordtail)

    # generate lexicon from words

    lexicon_offset = [0]
    lexicon = ""
    words = {}

    # build a lexicon string
    offset = 0
    for w, x in wordlist:
        # encoding: bit 7 indicates last character in word (chr(128)
        # indicates the last character in an entire string)
        ww = w[:-1] + chr(ord(w[-1])+128)
        # reuse string tails, when possible
        o = lexicon.find(ww)
        if o < 0:
            o = offset
            lexicon = lexicon + ww
            offset = offset + len(w)
        words[w] = len(lexicon_offset)
        lexicon_offset.append(o)

    lexicon = list(map(ord, lexicon))

    # generate phrasebook from names and lexicon
    phrasebook = [0]
    phrasebook_offset = [0] * len(unicode.chars)
    for char in unicode.chars:
        name = names[char]
        if name:
            w = name.split()
            phrasebook_offset[char] = len(phrasebook)
            for w in w:
                i = words[w]
                if i < short:
                    phrasebook.append(i)
                else:
                    # store as two bytes
                    phrasebook.append((i>>8) + short)
                    phrasebook.append(i&255)

    assert getsize(phrasebook) == 1

    #
    # unicode name hash table

    # extract names
    data = []
    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            name = record[1].strip()
            if name and name[0] != "<":
                data.append((name, char))

    # the magic number 47 was chosen to minimize the number of
    # collisions on the current data set.  if you like, change it
    # and see what happens...

    codehash = Hash("code", data, 47)

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)

        fprint("/* this file was generated by %s %s */" % (SCRIPT, VERSION))
        fprint()
        fprint("#define NAME_MAXLEN", 256)
        fprint()
        fprint("/* lexicon */")
        Array("lexicon", lexicon).dump(fp, trace)
        Array("lexicon_offset", lexicon_offset).dump(fp, trace)

        # split decomposition index table
        offset1, offset2, shift = splitbins(phrasebook_offset, trace)

        fprint("/* code->name phrasebook */")
        fprint("#define phrasebook_shift", shift)
        fprint("#define phrasebook_short", short)

        Array("phrasebook", phrasebook).dump(fp, trace)
        Array("phrasebook_offset1", offset1).dump(fp, trace)
        Array("phrasebook_offset2", offset2).dump(fp, trace)

        fprint("/* name->code dictionary */")
        codehash.dump(fp, trace)

        fprint()
        fprint('static const unsigned int aliases_start = %#x;' %
               NAME_ALIASES_START)
        fprint('static const unsigned int aliases_end = %#x;' %
               (NAME_ALIASES_START + len(unicode.aliases)))

        fprint('static const unsigned int name_aliases[] = {')
        for name, codepoint in unicode.aliases:
            fprint('    0x%04X,' % codepoint)
        fprint('};')

        # In Unicode 6.0.0, the sequences contain at most 4 BMP chars,
        # so we are using Py_UCS2 seq[4].  This needs to be updated if longer
        # sequences or sequences with non-BMP chars are added.
        # unicodedata_lookup should be adapted too.
        fprint(dedent("""
            typedef struct NamedSequence {
                int seqlen;
                Py_UCS2 seq[4];
            } named_sequence;
            """))

        fprint('static const unsigned int named_sequences_start = %#x;' %
               NAMED_SEQUENCES_START)
        fprint('static const unsigned int named_sequences_end = %#x;' %
               (NAMED_SEQUENCES_START + len(unicode.named_sequences)))

        fprint('static const named_sequence named_sequences[] = {')
        for name, sequence in unicode.named_sequences:
            seq_str = ', '.join('0x%04X' % cp for cp in sequence)
            fprint('    {%d, {%s}},' % (len(sequence), seq_str))
        fprint('};')


def merge_old_version(version, new, old):
    # Changes to exclusion file not implemented yet
    if old.exclusions != new.exclusions:
        raise NotImplementedError("exclusions differ")

    # In these change records, 0xFF means "no change"
    bidir_changes = [0xFF]*0x110000
    category_changes = [0xFF]*0x110000
    decimal_changes = [0xFF]*0x110000
    mirrored_changes = [0xFF]*0x110000
    east_asian_width_changes = [0xFF]*0x110000
    # In numeric data, 0 means "no change",
    # -1 means "did not have a numeric value
    numeric_changes = [0] * 0x110000
    # normalization_changes is a list of key-value pairs
    normalization_changes = []
    for i in range(0x110000):
        if new.table[i] is None:
            # Characters unassigned in the new version ought to
            # be unassigned in the old one
            assert old.table[i] is None
            continue
        # check characters unassigned in the old version
        if old.table[i] is None:
            # category 0 is "unassigned"
            category_changes[i] = 0
            continue
        # check characters that differ
        if old.table[i] != new.table[i]:
            for k in range(len(old.table[i])):
                if old.table[i][k] != new.table[i][k]:
                    value = old.table[i][k]
                    if k == 1 and i in PUA_15:
                        # the name is not set in the old.table, but in the
                        # new.table we are using it for aliases and named seq
                        assert value == ''
                    elif k == 2:
                        #print "CATEGORY",hex(i), old.table[i][k], new.table[i][k]
                        category_changes[i] = CATEGORY_NAMES.index(value)
                    elif k == 4:
                        #print "BIDIR",hex(i), old.table[i][k], new.table[i][k]
                        bidir_changes[i] = BIDIRECTIONAL_NAMES.index(value)
                    elif k == 5:
                        #print "DECOMP",hex(i), old.table[i][k], new.table[i][k]
                        # We assume that all normalization changes are in 1:1 mappings
                        assert " " not in value
                        normalization_changes.append((i, value))
                    elif k == 6:
                        #print "DECIMAL",hex(i), old.table[i][k], new.table[i][k]
                        # we only support changes where the old value is a single digit
                        assert value in "0123456789"
                        decimal_changes[i] = int(value)
                    elif k == 8:
                        # print "NUMERIC",hex(i), `old.table[i][k]`, new.table[i][k]
                        # Since 0 encodes "no change", the old value is better not 0
                        if not value:
                            numeric_changes[i] = -1
                        else:
                            numeric_changes[i] = float(value)
                            assert numeric_changes[i] not in (0, -1)
                    elif k == 9:
                        if value == 'Y':
                            mirrored_changes[i] = '1'
                        else:
                            mirrored_changes[i] = '0'
                    elif k == 11:
                        # change to ISO comment, ignore
                        pass
                    elif k == 12:
                        # change to simple uppercase mapping; ignore
                        pass
                    elif k == 13:
                        # change to simple lowercase mapping; ignore
                        pass
                    elif k == 14:
                        # change to simple titlecase mapping; ignore
                        pass
                    elif k == 15:
                        # change to east asian width
                        east_asian_width_changes[i] = EASTASIANWIDTH_NAMES.index(value)
                    elif k == 16:
                        # derived property changes; not yet
                        pass
                    elif k == 17:
                        # normalization quickchecks are not performed
                        # for older versions
                        pass
                    else:
                        class Difference(Exception):pass
                        raise Difference(hex(i), k, old.table[i], new.table[i])
    new.changed.append((version, list(zip(bidir_changes, category_changes,
                                          decimal_changes, mirrored_changes,
                                          east_asian_width_changes,
                                          numeric_changes)),
                        normalization_changes))


def open_data(template, version):
    local = template % ('-'+version,)
    if not os.path.exists(local):
        import urllib.request
        if version == '3.2.0':
            # irregular url structure
            url = 'http://www.unicode.org/Public/3.2-Update/' + local
        else:
            url = ('http://www.unicode.org/Public/%s/ucd/'+template) % (version, '')
        urllib.request.urlretrieve(url, filename=local)
    if local.endswith('.txt'):
        return open(local, encoding='utf-8')
    else:
        # Unihan.zip
        return open(local, 'rb')


# --------------------------------------------------------------------
# the following support code is taken from the unidb utilities
# Copyright (c) 1999-2000 by Secret Labs AB

# load a unicode-data file from disk

class UnicodeData:
    # Record structure:
    # [ID, name, category, combining, bidi, decomp,  (6)
    #  decimal, digit, numeric, bidi-mirrored, Unicode-1-name, (11)
    #  ISO-comment, uppercase, lowercase, titlecase, ea-width, (16)
    #  derived-props] (17)

    def __init__(self, version,
                 linebreakprops=False,
                 expand=1,
                 cjk_check=True):
        self.changed = []
        table = [None] * 0x110000
        with open_data(UNICODE_DATA, version) as file:
            while 1:
                s = file.readline()
                if not s:
                    break
                s = s.strip().split(";")
                char = int(s[0], 16)
                table[char] = s

        cjk_ranges_found = []

        # expand first-last ranges
        if expand:
            field = None
            for i in range(0, 0x110000):
                s = table[i]
                if s:
                    if s[1][-6:] == "First>":
                        s[1] = ""
                        field = s
                    elif s[1][-5:] == "Last>":
                        if s[1].startswith("<CJK Ideograph"):
                            cjk_ranges_found.append((field[0],
                                                     s[0]))
                        s[1] = ""
                        field = None
                elif field:
                    f2 = field[:]
                    f2[0] = "%X" % i
                    table[i] = f2
            if cjk_check and cjk_ranges != cjk_ranges_found:
                raise ValueError("CJK ranges deviate: have %r" % cjk_ranges_found)

        # public attributes
        self.filename = UNICODE_DATA % ''
        self.table = table
        self.chars = list(range(0x110000)) # unicode 3.2

        # check for name aliases and named sequences, see #12753
        # aliases and named sequences are not in 3.2.0
        if version != '3.2.0':
            self.aliases = []
            # store aliases in the Private Use Area 15, in range U+F0000..U+F00FF,
            # in order to take advantage of the compression and lookup
            # algorithms used for the other characters
            pua_index = NAME_ALIASES_START
            with open_data(NAME_ALIASES, version) as file:
                for s in file:
                    s = s.strip()
                    if not s or s.startswith('#'):
                        continue
                    char, name, abbrev = s.split(';')
                    char = int(char, 16)
                    self.aliases.append((name, char))
                    # also store the name in the PUA 1
                    self.table[pua_index][1] = name
                    pua_index += 1
            assert pua_index - NAME_ALIASES_START == len(self.aliases)

            self.named_sequences = []
            # store named sequences in the PUA 1, in range U+F0100..,
            # in order to take advantage of the compression and lookup
            # algorithms used for the other characters.

            assert pua_index < NAMED_SEQUENCES_START
            pua_index = NAMED_SEQUENCES_START
            with open_data(NAMED_SEQUENCES, version) as file:
                for s in file:
                    s = s.strip()
                    if not s or s.startswith('#'):
                        continue
                    name, chars = s.split(';')
                    chars = tuple(int(char, 16) for char in chars.split())
                    # check that the structure defined in makeunicodename is OK
                    assert 2 <= len(chars) <= 4, "change the Py_UCS2 array size"
                    assert all(c <= 0xFFFF for c in chars), ("use Py_UCS4 in "
                        "the NamedSequence struct and in unicodedata_lookup")
                    self.named_sequences.append((name, chars))
                    # also store these in the PUA 1
                    self.table[pua_index][1] = name
                    pua_index += 1
            assert pua_index - NAMED_SEQUENCES_START == len(self.named_sequences)

        self.exclusions = {}
        with open_data(COMPOSITION_EXCLUSIONS, version) as file:
            for s in file:
                s = s.strip()
                if not s:
                    continue
                if s[0] == '#':
                    continue
                char = int(s.split()[0],16)
                self.exclusions[char] = 1

        widths = [None] * 0x110000
        with open_data(EASTASIAN_WIDTH, version) as file:
            for s in file:
                s = s.strip()
                if not s:
                    continue
                if s[0] == '#':
                    continue
                s = s.split()[0].split(';')
                if '..' in s[0]:
                    first, last = [int(c, 16) for c in s[0].split('..')]
                    chars = list(range(first, last+1))
                else:
                    chars = [int(s[0], 16)]
                for char in chars:
                    widths[char] = s[1]

        for i in range(0, 0x110000):
            if table[i] is not None:
                table[i].append(widths[i])

        for i in range(0, 0x110000):
            if table[i] is not None:
                table[i].append(set())

        with open_data(DERIVED_CORE_PROPERTIES, version) as file:
            for s in file:
                s = s.split('#', 1)[0].strip()
                if not s:
                    continue

                r, p = s.split(";")
                r = r.strip()
                p = p.strip()
                if ".." in r:
                    first, last = [int(c, 16) for c in r.split('..')]
                    chars = list(range(first, last+1))
                else:
                    chars = [int(r, 16)]
                for char in chars:
                    if table[char]:
                        # Some properties (e.g. Default_Ignorable_Code_Point)
                        # apply to unassigned code points; ignore them
                        table[char][-1].add(p)

        with open_data(LINE_BREAK, version) as file:
            for s in file:
                s = s.partition('#')[0]
                s = [i.strip() for i in s.split(';')]
                if len(s) < 2 or s[1] not in MANDATORY_LINE_BREAKS:
                    continue
                if '..' not in s[0]:
                    first = last = int(s[0], 16)
                else:
                    first, last = [int(c, 16) for c in s[0].split('..')]
                for char in range(first, last+1):
                    table[char][-1].add('Line_Break')

        # We only want the quickcheck properties
        # Format: NF?_QC; Y(es)/N(o)/M(aybe)
        # Yes is the default, hence only N and M occur
        # In 3.2.0, the format was different (NF?_NO)
        # The parsing will incorrectly determine these as
        # "yes", however, unicodedata.c will not perform quickchecks
        # for older versions, and no delta records will be created.
        quickchecks = [0] * 0x110000
        qc_order = 'NFD_QC NFKD_QC NFC_QC NFKC_QC'.split()
        with open_data(DERIVEDNORMALIZATION_PROPS, version) as file:
            for s in file:
                if '#' in s:
                    s = s[:s.index('#')]
                s = [i.strip() for i in s.split(';')]
                if len(s) < 2 or s[1] not in qc_order:
                    continue
                quickcheck = 'MN'.index(s[2]) + 1 # Maybe or No
                quickcheck_shift = qc_order.index(s[1])*2
                quickcheck <<= quickcheck_shift
                if '..' not in s[0]:
                    first = last = int(s[0], 16)
                else:
                    first, last = [int(c, 16) for c in s[0].split('..')]
                for char in range(first, last+1):
                    assert not (quickchecks[char]>>quickcheck_shift)&3
                    quickchecks[char] |= quickcheck
        for i in range(0, 0x110000):
            if table[i] is not None:
                table[i].append(quickchecks[i])

        with open_data(UNIHAN, version) as file:
            zip = zipfile.ZipFile(file)
            if version == '3.2.0':
                data = zip.open('Unihan-3.2.0.txt').read()
            else:
                data = zip.open('Unihan_NumericValues.txt').read()
        for line in data.decode("utf-8").splitlines():
            if not line.startswith('U+'):
                continue
            code, tag, value = line.split(None, 3)[:3]
            if tag not in ('kAccountingNumeric', 'kPrimaryNumeric',
                           'kOtherNumeric'):
                continue
            value = value.strip().replace(',', '')
            i = int(code[2:], 16)
            # Patch the numeric field
            if table[i] is not None:
                table[i][8] = value
        sc = self.special_casing = {}
        with open_data(SPECIAL_CASING, version) as file:
            for s in file:
                s = s[:-1].split('#', 1)[0]
                if not s:
                    continue
                data = s.split("; ")
                if data[4]:
                    # We ignore all conditionals (since they depend on
                    # languages) except for one, which is hardcoded. See
                    # handle_capital_sigma in unicodeobject.c.
                    continue
                c = int(data[0], 16)
                lower = [int(char, 16) for char in data[1].split()]
                title = [int(char, 16) for char in data[2].split()]
                upper = [int(char, 16) for char in data[3].split()]
                sc[c] = (lower, title, upper)
        cf = self.case_folding = {}
        if version != '3.2.0':
            with open_data(CASE_FOLDING, version) as file:
                for s in file:
                    s = s[:-1].split('#', 1)[0]
                    if not s:
                        continue
                    data = s.split("; ")
                    if data[1] in "CF":
                        c = int(data[0], 16)
                        cf[c] = [int(char, 16) for char in data[2].split()]

    def uselatin1(self):
        # restrict character range to ISO Latin 1
        self.chars = list(range(256))


# hash table tools

# this is a straight-forward reimplementation of Python's built-in
# dictionary type, using a static data structure, and a custom string
# hash algorithm.

def myhash(s, magic):
    h = 0
    for c in map(ord, s.upper()):
        h = (h * magic) + c
        ix = h & 0xff000000
        if ix:
            h = (h ^ ((ix>>24) & 0xff)) & 0x00ffffff
    return h


SIZES = [
    (4,3), (8,3), (16,3), (32,5), (64,3), (128,3), (256,29), (512,17),
    (1024,9), (2048,5), (4096,83), (8192,27), (16384,43), (32768,3),
    (65536,45), (131072,9), (262144,39), (524288,39), (1048576,9),
    (2097152,5), (4194304,3), (8388608,33), (16777216,27)
]


class Hash:
    def __init__(self, name, data, magic):
        # turn a (key, value) list into a static hash table structure

        # determine table size
        for size, poly in SIZES:
            if size > len(data):
                poly = size + poly
                break
        else:
            raise AssertionError("ran out of polynomials")

        print(size, "slots in hash table")

        table = [None] * size

        mask = size-1

        n = 0

        hash = myhash

        # initialize hash table
        for key, value in data:
            h = hash(key, magic)
            i = (~h) & mask
            v = table[i]
            if v is None:
                table[i] = value
                continue
            incr = (h ^ (h >> 3)) & mask
            if not incr:
                incr = mask
            while 1:
                n = n + 1
                i = (i + incr) & mask
                v = table[i]
                if v is None:
                    table[i] = value
                    break
                incr = incr << 1
                if incr > mask:
                    incr = incr ^ poly

        print(n, "collisions")
        self.collisions = n

        for i in range(len(table)):
            if table[i] is None:
                table[i] = 0

        self.data = Array(name + "_hash", table)
        self.magic = magic
        self.name = name
        self.size = size
        self.poly = poly

    def dump(self, file, trace):
        # write data to file, as a C array
        self.data.dump(file, trace)
        file.write("#define %s_magic %d\n" % (self.name, self.magic))
        file.write("#define %s_size %d\n" % (self.name, self.size))
        file.write("#define %s_poly %d\n" % (self.name, self.poly))


# stuff to deal with arrays of unsigned integers

class Array:

    def __init__(self, name, data):
        self.name = name
        self.data = data

    def dump(self, file, trace=0):
        # write data to file, as a C array
        size = getsize(self.data)
        if trace:
            print(self.name+":", size*len(self.data), "bytes", file=sys.stderr)
        file.write("static const ")
        if size == 1:
            file.write("unsigned char")
        elif size == 2:
            file.write("unsigned short")
        else:
            file.write("unsigned int")
        file.write(" " + self.name + "[] = {\n")
        if self.data:
            s = "    "
            for item in self.data:
                i = str(item) + ", "
                if len(s) + len(i) > 78:
                    file.write(s.rstrip() + "\n")
                    s = "    " + i
                else:
                    s = s + i
            if s.strip():
                file.write(s.rstrip() + "\n")
        file.write("};\n\n")


def getsize(data):
    # return smallest possible integer size for the given array
    maxdata = max(data)
    if maxdata < 256:
        return 1
    elif maxdata < 65536:
        return 2
    else:
        return 4


def splitbins(t, trace=0):
    """t, trace=0 -> (t1, t2, shift).  Split a table to save space.

    t is a sequence of ints.  This function can be useful to save space if
    many of the ints are the same.  t1 and t2 are lists of ints, and shift
    is an int, chosen to minimize the combined size of t1 and t2 (in C
    code), and where for each i in range(len(t)),
        t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    where mask is a bitmask isolating the last "shift" bits.

    If optional arg trace is non-zero (default zero), progress info
    is printed to sys.stderr.  The higher the value, the more info
    you'll get.
    """

    if trace:
        def dump(t1, t2, shift, bytes):
            print("%d+%d bins at shift %d; %d bytes" % (
                len(t1), len(t2), shift, bytes), file=sys.stderr)
        print("Size of original table:", len(t)*getsize(t), "bytes",
              file=sys.stderr)
    n = len(t)-1    # last valid index
    maxshift = 0    # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxsize  # smallest total size so far
    t = tuple(t)    # so slices can be dict keys
    for shift in range(maxshift + 1):
        t1 = []
        t2 = []
        size = 2**shift
        bincache = {}
        for i in range(0, len(t), size):
            bin = t[i:i+size]
            index = bincache.get(bin)
            if index is None:
                index = len(t2)
                bincache[bin] = index
                t2.extend(bin)
            t1.append(index >> shift)
        # determine memory size
        b = len(t1)*getsize(t1) + len(t2)*getsize(t2)
        if trace > 1:
            dump(t1, t2, shift, b)
        if b < bytes:
            best = t1, t2, shift
            bytes = b
    t1, t2, shift = best
    if trace:
        print("Best:", end=' ', file=sys.stderr)
        dump(t1, t2, shift, bytes)
    if __debug__:
        # exhaustively verify that the decomposition is correct
        mask = ~((~0) << shift) # i.e., low-bit mask of shift bits
        for i in range(len(t)):
            assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best


if __name__ == "__main__":
    maketables(1)

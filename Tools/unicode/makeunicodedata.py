#
# (re)generate unicode property and type databases
#
# this script converts a unicode 3.2 database file to
# Modules/unicodedata_db.h, Modules/unicodename_db.h,
# and Objects/unicodetype_db.h
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
#
# written by Fredrik Lundh (fredrik@pythonware.com)
#

import sys

SCRIPT = sys.argv[0]
VERSION = "2.6"

# The Unicode Database
UNIDATA_VERSION = "5.1.0"
UNICODE_DATA = "UnicodeData%s.txt"
COMPOSITION_EXCLUSIONS = "CompositionExclusions%s.txt"
EASTASIAN_WIDTH = "EastAsianWidth%s.txt"

old_versions = ["3.2.0"]

CATEGORY_NAMES = [ "Cn", "Lu", "Ll", "Lt", "Mn", "Mc", "Me", "Nd",
    "Nl", "No", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs", "Co", "Cn", "Lm",
    "Lo", "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po", "Sm", "Sc", "Sk",
    "So" ]

BIDIRECTIONAL_NAMES = [ "", "L", "LRE", "LRO", "R", "AL", "RLE", "RLO",
    "PDF", "EN", "ES", "ET", "AN", "CS", "NSM", "BN", "B", "S", "WS",
    "ON" ]

EASTASIANWIDTH_NAMES = [ "F", "H", "W", "Na", "A", "N" ]

# note: should match definitions in Objects/unicodectype.c
ALPHA_MASK = 0x01
DECIMAL_MASK = 0x02
DIGIT_MASK = 0x04
LOWER_MASK = 0x08
LINEBREAK_MASK = 0x10
SPACE_MASK = 0x20
TITLE_MASK = 0x40
UPPER_MASK = 0x80
NODELTA_MASK = 0x100

def maketables(trace=0):

    print "--- Reading", UNICODE_DATA % "", "..."

    version = ""
    unicode = UnicodeData(UNICODE_DATA % version,
                          COMPOSITION_EXCLUSIONS % version,
                          EASTASIAN_WIDTH % version)

    print len(filter(None, unicode.table)), "characters"

    for version in old_versions:
        print "--- Reading", UNICODE_DATA % ("-"+version), "..."
        old_unicode = UnicodeData(UNICODE_DATA % ("-"+version),
                                  COMPOSITION_EXCLUSIONS % ("-"+version),
                                  EASTASIAN_WIDTH % ("-"+version))
        print len(filter(None, old_unicode.table)), "characters"
        merge_old_version(version, unicode, old_unicode)

    makeunicodename(unicode, trace)
    makeunicodedata(unicode, trace)
    makeunicodetype(unicode, trace)

# --------------------------------------------------------------------
# unicode character properties

def makeunicodedata(unicode, trace):

    dummy = (0, 0, 0, 0, 0)
    table = [dummy]
    cache = {0: dummy}
    index = [0] * len(unicode.chars)

    FILE = "Modules/unicodedata_db.h"

    print "--- Preparing", FILE, "..."

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
            item = (
                category, combining, bidirectional, mirrored, eastasianwidth
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
                    raise Exception, "character %x has a decomposition too large for nfd_nfkd" % char
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
                decomp = [prefix + (len(decomp)<<8)] +\
                         map(lambda s: int(s, 16), decomp)
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

    print len(table), "unique properties"
    print len(decomp_prefix), "unique decomposition prefixes"
    print len(decomp_data), "unique decomposition entries:",
    print decomp_size, "bytes"
    print total_first, "first characters in NFC"
    print total_last, "last characters in NFC"
    print len(comp_pairs), "NFC pairs"

    print "--- Writing", FILE, "..."

    fp = open(FILE, "w")
    print >>fp, "/* this file was generated by %s %s */" % (SCRIPT, VERSION)
    print >>fp
    print >>fp, '#define UNIDATA_VERSION "%s"' % UNIDATA_VERSION
    print >>fp, "/* a list of unique database records */"
    print >>fp, \
          "const _PyUnicode_DatabaseRecord _PyUnicode_Database_Records[] = {"
    for item in table:
        print >>fp, "    {%d, %d, %d, %d, %d}," % item
    print >>fp, "};"
    print >>fp

    print >>fp, "/* Reindexing of NFC first characters. */"
    print >>fp, "#define TOTAL_FIRST",total_first
    print >>fp, "#define TOTAL_LAST",total_last
    print >>fp, "struct reindex{int start;short count,index;};"
    print >>fp, "static struct reindex nfc_first[] = {"
    for start,end in comp_first_ranges:
        print >>fp,"  { %d, %d, %d}," % (start,end-start,comp_first[start])
    print >>fp,"  {0,0,0}"
    print >>fp,"};\n"
    print >>fp, "static struct reindex nfc_last[] = {"
    for start,end in comp_last_ranges:
        print >>fp,"  { %d, %d, %d}," % (start,end-start,comp_last[start])
    print >>fp,"  {0,0,0}"
    print >>fp,"};\n"

    # FIXME: <fl> the following tables could be made static, and
    # the support code moved into unicodedatabase.c

    print >>fp, "/* string literals */"
    print >>fp, "const char *_PyUnicode_CategoryNames[] = {"
    for name in CATEGORY_NAMES:
        print >>fp, "    \"%s\"," % name
    print >>fp, "    NULL"
    print >>fp, "};"

    print >>fp, "const char *_PyUnicode_BidirectionalNames[] = {"
    for name in BIDIRECTIONAL_NAMES:
        print >>fp, "    \"%s\"," % name
    print >>fp, "    NULL"
    print >>fp, "};"

    print >>fp, "const char *_PyUnicode_EastAsianWidthNames[] = {"
    for name in EASTASIANWIDTH_NAMES:
        print >>fp, "    \"%s\"," % name
    print >>fp, "    NULL"
    print >>fp, "};"

    print >>fp, "static const char *decomp_prefix[] = {"
    for name in decomp_prefix:
        print >>fp, "    \"%s\"," % name
    print >>fp, "    NULL"
    print >>fp, "};"

    # split record index table
    index1, index2, shift = splitbins(index, trace)

    print >>fp, "/* index tables for the database records */"
    print >>fp, "#define SHIFT", shift
    Array("index1", index1).dump(fp, trace)
    Array("index2", index2).dump(fp, trace)

    # split decomposition index table
    index1, index2, shift = splitbins(decomp_index, trace)

    print >>fp, "/* decomposition data */"
    Array("decomp_data", decomp_data).dump(fp, trace)

    print >>fp, "/* index tables for the decomposition data */"
    print >>fp, "#define DECOMP_SHIFT", shift
    Array("decomp_index1", index1).dump(fp, trace)
    Array("decomp_index2", index2).dump(fp, trace)

    index, index2, shift = splitbins(comp_data, trace)
    print >>fp, "/* NFC pairs */"
    print >>fp, "#define COMP_SHIFT", shift
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
        print >>fp, "static const change_record change_records_%s[] = {" % cversion
        for record in records:
            print >>fp, "\t{ %s }," % ", ".join(map(str,record))
        print >>fp, "};"
        Array("changes_%s_index" % cversion, index1).dump(fp, trace)
        Array("changes_%s_data" % cversion, index2).dump(fp, trace)
        print >>fp, "static const change_record* get_change_%s(Py_UCS4 n)" % cversion
        print >>fp, "{"
        print >>fp, "\tint index;"
        print >>fp, "\tif (n >= 0x110000) index = 0;"
        print >>fp, "\telse {"
        print >>fp, "\t\tindex = changes_%s_index[n>>%d];" % (cversion, shift)
        print >>fp, "\t\tindex = changes_%s_data[(index<<%d)+(n & %d)];" % \
              (cversion, shift, ((1<<shift)-1))
        print >>fp, "\t}"
        print >>fp, "\treturn change_records_%s+index;" % cversion
        print >>fp, "}\n"
        print >>fp, "static Py_UCS4 normalization_%s(Py_UCS4 n)" % cversion
        print >>fp, "{"
        print >>fp, "\tswitch(n) {"
        for k, v in normalization:
            print >>fp, "\tcase %s: return 0x%s;" % (hex(k), v)
        print >>fp, "\tdefault: return 0;"
        print >>fp, "\t}\n}\n"

    fp.close()

# --------------------------------------------------------------------
# unicode character type tables

def makeunicodetype(unicode, trace):

    FILE = "Objects/unicodetype_db.h"

    print "--- Preparing", FILE, "..."

    # extract unicode types
    dummy = (0, 0, 0, 0, 0, 0)
    table = [dummy]
    cache = {0: dummy}
    index = [0] * len(unicode.chars)

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            # extract database properties
            category = record[2]
            bidirectional = record[4]
            flags = 0
            delta = True
            if category in ["Lm", "Lt", "Lu", "Ll", "Lo"]:
                flags |= ALPHA_MASK
            if category == "Ll":
                flags |= LOWER_MASK
            if category == "Zl" or bidirectional == "B":
                flags |= LINEBREAK_MASK
            if category == "Zs" or bidirectional in ("WS", "B", "S"):
                flags |= SPACE_MASK
            if category == "Lt":
                flags |= TITLE_MASK
            if category == "Lu":
                flags |= UPPER_MASK
            # use delta predictor for upper/lower/title if it fits
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
                # UCD.html says that a missing title char means that
                # it defaults to the uppercase character, not to the
                # character itself. Apparently, in the current UCD (5.x)
                # this feature is never used
                title = upper
            upper_d = upper - char
            lower_d = lower - char
            title_d = title - char
            if -32768 <= upper_d <= 32767 and \
               -32768 <= lower_d <= 32767 and \
               -32768 <= title_d <= 32767:
                # use deltas
                upper = upper_d & 0xffff
                lower = lower_d & 0xffff
                title = title_d & 0xffff
            else:
                flags |= NODELTA_MASK
            # decimal digit, integer digit
            decimal = 0
            if record[6]:
                flags |= DECIMAL_MASK
                decimal = int(record[6])
            digit = 0
            if record[7]:
                flags |= DIGIT_MASK
                digit = int(record[7])
            item = (
                upper, lower, title, decimal, digit, flags
                )
            # add entry to index and item tables
            i = cache.get(item)
            if i is None:
                cache[item] = i = len(table)
                table.append(item)
            index[char] = i

    print len(table), "unique character type entries"

    print "--- Writing", FILE, "..."

    fp = open(FILE, "w")
    print >>fp, "/* this file was generated by %s %s */" % (SCRIPT, VERSION)
    print >>fp
    print >>fp, "/* a list of unique character type descriptors */"
    print >>fp, "const _PyUnicode_TypeRecord _PyUnicode_TypeRecords[] = {"
    for item in table:
        print >>fp, "    {%d, %d, %d, %d, %d, %d}," % item
    print >>fp, "};"
    print >>fp

    # split decomposition index table
    index1, index2, shift = splitbins(index, trace)

    print >>fp, "/* type indexes */"
    print >>fp, "#define SHIFT", shift
    Array("index1", index1).dump(fp, trace)
    Array("index2", index2).dump(fp, trace)

    fp.close()

# --------------------------------------------------------------------
# unicode name database

def makeunicodename(unicode, trace):

    FILE = "Modules/unicodename_db.h"

    print "--- Preparing", FILE, "..."

    # collect names
    names = [None] * len(unicode.chars)

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            name = record[1].strip()
            if name and name[0] != "<":
                names[char] = name + chr(0)

    print len(filter(lambda n: n is not None, names)), "distinct names"

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

    print n, "words in text;", b, "bytes"

    wordlist = words.items()

    # sort on falling frequency, then by name
    def cmpwords((aword, alist),(bword, blist)):
        r = -cmp(len(alist),len(blist))
        if r:
            return r
        return cmp(aword, bword)
    wordlist.sort(cmpwords)

    # figure out how many phrasebook escapes we need
    escapes = 0
    while escapes * 256 < len(wordlist):
        escapes = escapes + 1
    print escapes, "escapes"

    short = 256 - escapes

    assert short > 0

    print short, "short indexes in lexicon"

    # statistics
    n = 0
    for i in range(short):
        n = n + len(wordlist[i][1])
    print n, "short indexes in phrasebook"

    # pick the most commonly used words, and sort the rest on falling
    # length (to maximize overlap)

    wordlist, wordtail = wordlist[:short], wordlist[short:]
    wordtail.sort(lambda a, b: len(b[0])-len(a[0]))
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

    lexicon = map(ord, lexicon)

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

    print "--- Writing", FILE, "..."

    fp = open(FILE, "w")
    print >>fp, "/* this file was generated by %s %s */" % (SCRIPT, VERSION)
    print >>fp
    print >>fp, "#define NAME_MAXLEN", 256
    print >>fp
    print >>fp, "/* lexicon */"
    Array("lexicon", lexicon).dump(fp, trace)
    Array("lexicon_offset", lexicon_offset).dump(fp, trace)

    # split decomposition index table
    offset1, offset2, shift = splitbins(phrasebook_offset, trace)

    print >>fp, "/* code->name phrasebook */"
    print >>fp, "#define phrasebook_shift", shift
    print >>fp, "#define phrasebook_short", short

    Array("phrasebook", phrasebook).dump(fp, trace)
    Array("phrasebook_offset1", offset1).dump(fp, trace)
    Array("phrasebook_offset2", offset2).dump(fp, trace)

    print >>fp, "/* name->code dictionary */"
    codehash.dump(fp, trace)

    fp.close()


def merge_old_version(version, new, old):
    # Changes to exclusion file not implemented yet
    if old.exclusions != new.exclusions:
        raise NotImplementedError, "exclusions differ"

    # In these change records, 0xFF means "no change"
    bidir_changes = [0xFF]*0x110000
    category_changes = [0xFF]*0x110000
    decimal_changes = [0xFF]*0x110000
    mirrored_changes = [0xFF]*0x110000
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
                    if k == 2:
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
                        assert value != "0" and value != "-1"
                        if not value:
                            numeric_changes[i] = -1
                        else:
                            assert re.match("^[0-9]+$", value)
                            numeric_changes[i] = int(value)
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
                    else:
                        class Difference(Exception):pass
                        raise Difference, (hex(i), k, old.table[i], new.table[i])
    new.changed.append((version, zip(bidir_changes, category_changes,
                                     decimal_changes, mirrored_changes,
                                     numeric_changes),
                        normalization_changes))


# --------------------------------------------------------------------
# the following support code is taken from the unidb utilities
# Copyright (c) 1999-2000 by Secret Labs AB

# load a unicode-data file from disk

import sys

class UnicodeData:

    def __init__(self, filename, exclusions, eastasianwidth, expand=1):
        self.changed = []
        file = open(filename)
        table = [None] * 0x110000
        while 1:
            s = file.readline()
            if not s:
                break
            s = s.strip().split(";")
            char = int(s[0], 16)
            table[char] = s

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
                        s[1] = ""
                        field = None
                elif field:
                    f2 = field[:]
                    f2[0] = "%X" % i
                    table[i] = f2

        # public attributes
        self.filename = filename
        self.table = table
        self.chars = range(0x110000) # unicode 3.2

        file = open(exclusions)
        self.exclusions = {}
        for s in file:
            s = s.strip()
            if not s:
                continue
            if s[0] == '#':
                continue
            char = int(s.split()[0],16)
            self.exclusions[char] = 1

        widths = [None] * 0x110000
        for s in open(eastasianwidth):
            s = s.strip()
            if not s:
                continue
            if s[0] == '#':
                continue
            s = s.split()[0].split(';')
            if '..' in s[0]:
                first, last = [int(c, 16) for c in s[0].split('..')]
                chars = range(first, last+1)
            else:
                chars = [int(s[0], 16)]
            for char in chars:
                widths[char] = s[1]
        for i in range(0, 0x110000):
            if table[i] is not None:
                table[i].append(widths[i])

    def uselatin1(self):
        # restrict character range to ISO Latin 1
        self.chars = range(256)

# hash table tools

# this is a straight-forward reimplementation of Python's built-in
# dictionary type, using a static data structure, and a custom string
# hash algorithm.

def myhash(s, magic):
    h = 0
    for c in map(ord, s.upper()):
        h = (h * magic) + c
        ix = h & 0xff000000L
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
            raise AssertionError, "ran out of polynominals"

        print size, "slots in hash table"

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
            incr = (h ^ (h >> 3)) & mask;
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

        print n, "collisions"
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
            print >>sys.stderr, self.name+":", size*len(self.data), "bytes"
        file.write("static ")
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
                    file.write(s + "\n")
                    s = "    " + i
                else:
                    s = s + i
            if s.strip():
                file.write(s + "\n")
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

    import sys
    if trace:
        def dump(t1, t2, shift, bytes):
            print >>sys.stderr, "%d+%d bins at shift %d; %d bytes" % (
                len(t1), len(t2), shift, bytes)
        print >>sys.stderr, "Size of original table:", len(t)*getsize(t), \
                            "bytes"
    n = len(t)-1    # last valid index
    maxshift = 0    # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxint  # smallest total size so far
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
        print >>sys.stderr, "Best:",
        dump(t1, t2, shift, bytes)
    if __debug__:
        # exhaustively verify that the decomposition is correct
        mask = ~((~0) << shift) # i.e., low-bit mask of shift bits
        for i in xrange(len(t)):
            assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best

if __name__ == "__main__":
    maketables(1)

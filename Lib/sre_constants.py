#
# Secret Labs' Regular Expression Engine
# $Id$
#
# various symbols used by the regular expression engine.
# run this script to update the _sre include files!
#
# Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
#
# This code can only be used for 1.6 alpha testing.  All other use
# require explicit permission from Secret Labs AB.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

# operators

FAILURE = "failure"
SUCCESS = "success"

ANY = "any"
ASSERT = "assert"
AT = "at"
BRANCH = "branch"
CALL = "call"
CATEGORY = "category"
GROUP = "group"
GROUP_IGNORE = "group_ignore"
IN = "in"
IN_IGNORE = "in_ignore"
JUMP = "jump"
LITERAL = "literal"
LITERAL_IGNORE = "literal_ignore"
MARK = "mark"
MAX_REPEAT = "max_repeat"
MAX_REPEAT_ONE = "max_repeat_one"
MAX_UNTIL = "max_until"
MIN_REPEAT = "min_repeat"
MIN_UNTIL = "min_until"
NEGATE = "negate"
NOT_LITERAL = "not_literal"
NOT_LITERAL_IGNORE = "not_literal_ignore"
RANGE = "range"
REPEAT = "repeat"
SUBPATTERN = "subpattern"

# positions
AT_BEGINNING = "at_beginning"
AT_BEGINNING_LINE = "at_beginning_line"
AT_BOUNDARY = "at_boundary"
AT_NON_BOUNDARY = "at_non_boundary"
AT_END = "at_end"
AT_END_LINE = "at_end_line"

# categories
CATEGORY_DIGIT = "category_digit"
CATEGORY_NOT_DIGIT = "category_not_digit"
CATEGORY_SPACE = "category_space"
CATEGORY_NOT_SPACE = "category_not_space"
CATEGORY_WORD = "category_word"
CATEGORY_NOT_WORD = "category_not_word"
CATEGORY_LINEBREAK = "category_linebreak"
CATEGORY_NOT_LINEBREAK = "category_not_linebreak"
CATEGORY_LOC_DIGIT = "category_loc_digit"
CATEGORY_LOC_NOT_DIGIT = "category_loc_not_digit"
CATEGORY_LOC_SPACE = "category_loc_space"
CATEGORY_LOC_NOT_SPACE = "category_loc_not_space"
CATEGORY_LOC_WORD = "category_loc_word"
CATEGORY_LOC_NOT_WORD = "category_loc_not_word"
CATEGORY_LOC_LINEBREAK = "category_loc_linebreak"
CATEGORY_LOC_NOT_LINEBREAK = "category_loc_not_linebreak"

OPCODES = [

    # failure=0 success=1 (just because it looks better that way :-)
    FAILURE, SUCCESS,

    ANY,
    ASSERT,
    AT,
    BRANCH,
    CALL,
    CATEGORY,
    GROUP, GROUP_IGNORE,
    IN, IN_IGNORE,
    JUMP,
    LITERAL, LITERAL_IGNORE,
    MARK,
    MAX_REPEAT, MAX_UNTIL,
    MAX_REPEAT_ONE,
    MIN_REPEAT, MIN_UNTIL,
    NOT_LITERAL, NOT_LITERAL_IGNORE,
    NEGATE,
    RANGE,
    REPEAT

]

ATCODES = [
    AT_BEGINNING, AT_BEGINNING_LINE, AT_BOUNDARY,
    AT_NON_BOUNDARY, AT_END, AT_END_LINE
]

CHCODES = [
    CATEGORY_DIGIT, CATEGORY_NOT_DIGIT, CATEGORY_SPACE,
    CATEGORY_NOT_SPACE, CATEGORY_WORD, CATEGORY_NOT_WORD,
    CATEGORY_LINEBREAK, CATEGORY_NOT_LINEBREAK, CATEGORY_LOC_DIGIT,
    CATEGORY_LOC_NOT_DIGIT, CATEGORY_LOC_SPACE,
    CATEGORY_LOC_NOT_SPACE, CATEGORY_LOC_WORD, CATEGORY_LOC_NOT_WORD,
    CATEGORY_LOC_LINEBREAK, CATEGORY_LOC_NOT_LINEBREAK
]

def makedict(list):
    d = {}
    i = 0
    for item in list:
	d[item] = i
	i = i + 1
    return d

OPCODES = makedict(OPCODES)
ATCODES = makedict(ATCODES)
CHCODES = makedict(CHCODES)

# replacement operations for "ignore case" mode
OP_IGNORE = {
    GROUP: GROUP_IGNORE,
    IN: IN_IGNORE,
    LITERAL: LITERAL_IGNORE,
    NOT_LITERAL: NOT_LITERAL_IGNORE
}

AT_MULTILINE = {
    AT_BEGINNING: AT_BEGINNING_LINE,
    AT_END: AT_END_LINE
}

CH_LOCALE = {
    CATEGORY_DIGIT: CATEGORY_LOC_DIGIT,
    CATEGORY_NOT_DIGIT: CATEGORY_LOC_NOT_DIGIT,
    CATEGORY_SPACE: CATEGORY_LOC_SPACE,
    CATEGORY_NOT_SPACE: CATEGORY_LOC_NOT_SPACE,
    CATEGORY_WORD: CATEGORY_LOC_WORD,
    CATEGORY_NOT_WORD: CATEGORY_LOC_NOT_WORD,
    CATEGORY_LINEBREAK: CATEGORY_LOC_LINEBREAK,
    CATEGORY_NOT_LINEBREAK: CATEGORY_LOC_NOT_LINEBREAK
}

# flags
SRE_FLAG_TEMPLATE = 1 # NYI
SRE_FLAG_IGNORECASE = 2
SRE_FLAG_LOCALE = 4
SRE_FLAG_MULTILINE = 8
SRE_FLAG_DOTALL = 16
SRE_FLAG_VERBOSE = 32

if __name__ == "__main__":
    import string
    def dump(f, d, prefix):
	items = d.items()
	items.sort(lambda a, b: cmp(a[1], b[1]))
	for k, v in items:
	    f.write("#define %s_%s %s\n" % (prefix, string.upper(k), v))
    f = open("sre_constants.h", "w")
    f.write("/* generated from sre_constants.py */\n")
    dump(f, OPCODES, "SRE_OP")
    dump(f, ATCODES, "SRE")
    dump(f, CHCODES, "SRE")
    f.close()
    print "done"

""" Test script for the Unicode implementation.

Written by Bill Tutt.
Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
from test_support import verify, verbose

print 'Testing General Unicode Character Name, and case insensitivity...',

# General and case insensitivity test:
try:
    # put all \N escapes inside exec'd raw strings, to make sure this
    # script runs even if the compiler chokes on \N escapes
    exec r"""
s = u"\N{LATIN CAPITAL LETTER T}" \
    u"\N{LATIN SMALL LETTER H}" \
    u"\N{LATIN SMALL LETTER E}" \
    u"\N{SPACE}" \
    u"\N{LATIN SMALL LETTER R}" \
    u"\N{LATIN CAPITAL LETTER E}" \
    u"\N{LATIN SMALL LETTER D}" \
    u"\N{SPACE}" \
    u"\N{LATIN SMALL LETTER f}" \
    u"\N{LATIN CAPITAL LeTtEr o}" \
    u"\N{LATIN SMaLl LETTER x}" \
    u"\N{SPACE}" \
    u"\N{LATIN SMALL LETTER A}" \
    u"\N{LATIN SMALL LETTER T}" \
    u"\N{LATIN SMALL LETTER E}" \
    u"\N{SPACE}" \
    u"\N{LATIN SMALL LETTER T}" \
    u"\N{LATIN SMALL LETTER H}" \
    u"\N{LATIN SMALL LETTER E}" \
    u"\N{SpAcE}" \
    u"\N{LATIN SMALL LETTER S}" \
    u"\N{LATIN SMALL LETTER H}" \
    u"\N{LATIN SMALL LETTER E}" \
    u"\N{LATIN SMALL LETTER E}" \
    u"\N{LATIN SMALL LETTER P}" \
    u"\N{FULL STOP}"
verify(s == u"The rEd fOx ate the sheep.", s)
"""
except UnicodeError, v:
    print v
print "done."

import unicodedata

print "Testing name to code mapping....",
for char in "SPAM":
    name = "LATIN SMALL LETTER %s" % char
    code = unicodedata.lookup(name)
    verify(unicodedata.name(code) == name)
print "done."

print "Testing code to name mapping for all characters....",
count = 0
for code in range(65536):
    try:
        char = unichr(code)
        name = unicodedata.name(char)
        verify(unicodedata.lookup(name) == char)
        count += 1
    except (KeyError, ValueError):
        pass
print "done."

print "Found", count, "characters in the unicode name database"

# misc. symbol testing
print "Testing misc. symbols for unicode character name expansion....",
exec r"""
verify(u"\N{PILCROW SIGN}" == u"\u00b6")
verify(u"\N{REPLACEMENT CHARACTER}" == u"\uFFFD")
verify(u"\N{HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK}" == u"\uFF9F")
verify(u"\N{FULLWIDTH LATIN SMALL LETTER A}" == u"\uFF41")
"""
print "done."

# strict error testing:
print "Testing unicode character name expansion strict error handling....",
try:
    unicode("\N{blah}", 'unicode-escape', 'strict')
except UnicodeError:
    pass
else:
    raise AssertionError, "failed to raise an exception when given a bogus character name"

try:
    unicode("\N{" + "x" * 100000 + "}", 'unicode-escape', 'strict')
except UnicodeError:
    pass
else:
    raise AssertionError, "failed to raise an exception when given a very " \
                          "long bogus character name"

try:
    unicode("\N{SPACE", 'unicode-escape', 'strict')
except UnicodeError:
    pass
else:
    raise AssertionError, "failed to raise an exception for a missing closing brace."

try:
    unicode("\NSPACE", 'unicode-escape', 'strict')
except UnicodeError:
    pass
else:
    raise AssertionError, "failed to raise an exception for a missing opening brace."
print "done."

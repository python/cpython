""" Test script for the Unicode implementation.

Written by Bill Tutt.

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
from test_support import verify, verbose

print 'Testing General Unicode Character Name, and case insensitivity...',

# General and case insensitivity test:
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

import ucnhash

# minimal sanity check
for char in "SPAM":
    name = "LATIN SMALL LETTER %s" % char
    code = ucnhash.getcode(name)
    verify(ucnhash.getname(code) == name)

# loop over all characters in the database
for code in range(65536):
    try:
        name = ucnhash.getname(code)
        verify(ucnhash.getcode(name) == code)
    except ValueError:
        pass

print "done."

# misc. symbol testing
print "Testing misc. symbols for unicode character name expansion....",
verify(u"\N{PILCROW SIGN}" == u"\u00b6")
verify(u"\N{REPLACEMENT CHARACTER}" == u"\uFFFD")
verify(u"\N{HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK}" == u"\uFF9F")
verify(u"\N{FULLWIDTH LATIN SMALL LETTER A}" == u"\uFF41")
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

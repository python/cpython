"""Constants for selecting regexp syntaxes for the obsolete regex module.

This module is only for backward compatibility.  "regex" has now
been replaced by the new regular expression module, "re".

These bits are passed to regex.set_syntax() to choose among
alternative regexp syntaxes.
"""

# 1 means plain parentheses serve as grouping, and backslash
#   parentheses are needed for literal searching.
# 0 means backslash-parentheses are grouping, and plain parentheses
#   are for literal searching.
RE_NO_BK_PARENS = 1

# 1 means plain | serves as the "or"-operator, and \| is a literal.
# 0 means \| serves as the "or"-operator, and | is a literal.
RE_NO_BK_VBAR = 2

# 0 means plain + or ? serves as an operator, and \+, \? are literals.
# 1 means \+, \? are operators and plain +, ? are literals.
RE_BK_PLUS_QM = 4

# 1 means | binds tighter than ^ or $.
# 0 means the contrary.
RE_TIGHT_VBAR = 8

# 1 means treat \n as an _OR operator
# 0 means treat it as a normal character
RE_NEWLINE_OR = 16

# 0 means that a special characters (such as *, ^, and $) always have
#   their special meaning regardless of the surrounding context.
# 1 means that special characters may act as normal characters in some
#   contexts.  Specifically, this applies to:
#       ^ - only special at the beginning, or after ( or |
#       $ - only special at the end, or before ) or |
#       *, +, ? - only special when not after the beginning, (, or |
RE_CONTEXT_INDEP_OPS = 32

# ANSI sequences (\n etc) and \xhh
RE_ANSI_HEX = 64

# No GNU extensions
RE_NO_GNU_EXTENSIONS = 128

# Now define combinations of bits for the standard possibilities.
RE_SYNTAX_AWK = (RE_NO_BK_PARENS | RE_NO_BK_VBAR | RE_CONTEXT_INDEP_OPS)
RE_SYNTAX_EGREP = (RE_SYNTAX_AWK | RE_NEWLINE_OR)
RE_SYNTAX_GREP = (RE_BK_PLUS_QM | RE_NEWLINE_OR)
RE_SYNTAX_EMACS = 0

# (Python's obsolete "regexp" module used a syntax similar to awk.)

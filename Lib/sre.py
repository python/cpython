#
# Secret Labs' Regular Expression Engine
#
# re-compatible interface for the sre matching engine
#
# Copyright (c) 1998-2001 by Secret Labs AB.  All rights reserved.
#
# This version of the SRE library can be redistributed under CNRI's
# Python 1.6 license.  For any other use, please contact Secret Labs
# AB (info@pythonware.com).
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

r"""Support for regular expressions (RE).

This module provides regular expression matching operations similar to
those found in Perl. It's 8-bit clean: the strings being processed may
contain both null bytes and characters whose high bit is set. Regular
expression pattern strings may not contain null bytes, but can specify
the null byte using the \\number notation. Characters with the high
bit set may be included.

Regular expressions can contain both special and ordinary
characters. Most ordinary characters, like "A", "a", or "0", are the
simplest regular expressions; they simply match themselves. You can
concatenate ordinary characters, so last matches the string 'last'.

The special characters are:
    "."      Matches any character except a newline.
    "^"      Matches the start of the string.
    "$"      Matches the end of the string.
    "*"      Matches 0 or more (greedy) repetitions of the preceding RE.
             Greedy means that it will match as many repetitions as possible.
    "+"      Matches 1 or more (greedy) repetitions of the preceding RE.
    "?"      Matches 0 or 1 (greedy) of the preceding RE.
    *?,+?,?? Non-greedy versions of the previous three special characters.
    {m,n}    Matches from m to n repetitions of the preceding RE.
    {m,n}?   Non-greedy version of the above.
    "\\"      Either escapes special characters or signals a special sequence.
    []       Indicates a set of characters.
             A "^" as the first character indicates a complementing set.
    "|"      A|B, creates an RE that will match either A or B.
    (...)    Matches the RE inside the parentheses.
             The contents can be retrieved or matched later in the string.
    (?iLmsx) Set the I, L, M, S, or X flag for the RE (see below).
    (?:...)  Non-grouping version of regular parentheses.
    (?P<name>...) The substring matched by the group is accessible by name.
    (?P=name)     Matches the text matched earlier by the group named name.
    (?#...)  A comment; ignored.
    (?=...)  Matches if ... matches next, but doesn't consume the string.
    (?!...)  Matches if ... doesn't match next.

The special sequences consist of "\\" and a character from the list
below. If the ordinary character is not on the list, then the
resulting RE will match the second character.
    \number  Matches the contents of the group of the same number.
    \A       Matches only at the start of the string.
    \Z       Matches only at the end of the string.
    \b       Matches the empty string, but only at the start or end of a word.
    \B       Matches the empty string, but not at the start or end of a word.
    \d       Matches any decimal digit; equivalent to the set [0-9].
    \D       Matches any non-digit character; equivalent to the set [^0-9].
    \s       Matches any whitespace character; equivalent to [ \t\n\r\f\v].
    \S       Matches any non-whitespace character; equiv. to [^ \t\n\r\f\v].
    \w       Matches any alphanumeric character; equivalent to [a-zA-Z0-9_].
             With LOCALE, it will match the set [0-9_] plus characters defined
             as letters for the current locale.
    \W       Matches the complement of \w.
    \\       Matches a literal backslash.

This module exports the following functions:
    match    Match a regular expression pattern to the beginning of a string.
    search   Search a string for the presence of a pattern.
    sub      Substitute occurrences of a pattern found in a string.
    subn     Same as sub, but also return the number of substitutions made.
    split    Split a string by the occurrences of a pattern.
    findall  Find all occurrences of a pattern in a string.
    compile  Compile a pattern into a RegexObject.
    purge    Clear the regular expression cache.
    escape   Backslash all non-alphanumerics in a string.

Some of the functions in this module takes flags as optional parameters:
    I  IGNORECASE  Perform case-insensitive matching.
    L  LOCALE      Make \w, \W, \b, \B, dependent on the current locale.
    M  MULTILINE   "^" matches the beginning of lines as well as the string.
                   "$" matches the end of lines as well as the string.
    S  DOTALL      "." matches any character at all, including the newline.
    X  VERBOSE     Ignore whitespace and comments for nicer looking RE's.
    U  UNICODE     Make \w, \W, \b, \B, dependent on the Unicode locale.

This module also defines an exception 'error'.

"""

import sre_compile
import sre_parse

# public symbols
__all__ = [ "match", "search", "sub", "subn", "split", "findall",
    "compile", "purge", "template", "escape", "I", "L", "M", "S", "X",
    "U", "IGNORECASE", "LOCALE", "MULTILINE", "DOTALL", "VERBOSE",
    "UNICODE", "error" ]

__version__ = "2.2.0"

# this module works under 1.5.2 and later.  don't use string methods
import string

# flags
I = IGNORECASE = sre_compile.SRE_FLAG_IGNORECASE # ignore case
L = LOCALE = sre_compile.SRE_FLAG_LOCALE # assume current 8-bit locale
U = UNICODE = sre_compile.SRE_FLAG_UNICODE # assume unicode locale
M = MULTILINE = sre_compile.SRE_FLAG_MULTILINE # make anchors look for newline
S = DOTALL = sre_compile.SRE_FLAG_DOTALL # make dot match newline
X = VERBOSE = sre_compile.SRE_FLAG_VERBOSE # ignore whitespace and comments

# sre extensions (experimental, don't rely on these)
T = TEMPLATE = sre_compile.SRE_FLAG_TEMPLATE # disable backtracking
DEBUG = sre_compile.SRE_FLAG_DEBUG # dump pattern after compilation

# sre exception
error = sre_compile.error

# --------------------------------------------------------------------
# public interface

def match(pattern, string, flags=0):
    """Try to apply the pattern at the start of the string, returning
    a match object, or None if no match was found."""
    return _compile(pattern, flags).match(string)

def search(pattern, string, flags=0):
    """Scan through string looking for a match to the pattern, returning
    a match object, or None if no match was found."""
    return _compile(pattern, flags).search(string)

def sub(pattern, repl, string, count=0):
    """Return the string obtained by replacing the leftmost
    non-overlapping occurrences of the pattern in string by the
    replacement repl"""
    return _compile(pattern, 0).sub(repl, string, count)

def subn(pattern, repl, string, count=0):
    """Return a 2-tuple containing (new_string, number).
    new_string is the string obtained by replacing the leftmost
    non-overlapping occurrences of the pattern in the source
    string by the replacement repl.  number is the number of
    substitutions that were made."""
    return _compile(pattern, 0).subn(repl, string, count)

def split(pattern, string, maxsplit=0):
    """Split the source string by the occurrences of the pattern,
    returning a list containing the resulting substrings."""
    return _compile(pattern, 0).split(string, maxsplit)

def findall(pattern, string):
    """Return a list of all non-overlapping matches in the string.

    If one or more groups are present in the pattern, return a
    list of groups; this will be a list of tuples if the pattern
    has more than one group.

    Empty matches are included in the result."""
    return _compile(pattern, 0).findall(string)

def compile(pattern, flags=0):
    "Compile a regular expression pattern, returning a pattern object."
    return _compile(pattern, flags)

def purge():
    "Clear the regular expression cache"
    _cache.clear()
    _cache_repl.clear()

def template(pattern, flags=0):
    "Compile a template pattern, returning a pattern object"
    return _compile(pattern, flags|T)

def escape(pattern):
    "Escape all non-alphanumeric characters in pattern."
    s = list(pattern)
    for i in range(len(pattern)):
        c = pattern[i]
        if not ("a" <= c <= "z" or "A" <= c <= "Z" or "0" <= c <= "9"):
            if c == "\000":
                s[i] = "\\000"
            else:
                s[i] = "\\" + c
    return _join(s, pattern)

# --------------------------------------------------------------------
# internals

_cache = {}
_cache_repl = {}

_pattern_type = type(sre_compile.compile("", 0))

_MAXCACHE = 100

def _join(seq, sep):
    # internal: join into string having the same type as sep
    return string.join(seq, sep[:0])

def _compile(*key):
    # internal: compile pattern
    p = _cache.get(key)
    if p is not None:
        return p
    pattern, flags = key
    if type(pattern) is _pattern_type:
        return pattern
    if type(pattern) not in sre_compile.STRING_TYPES:
        raise TypeError, "first argument must be string or compiled pattern"
    try:
        p = sre_compile.compile(pattern, flags)
    except error, v:
        raise error, v # invalid expression
    if len(_cache) >= _MAXCACHE:
        _cache.clear()
    _cache[key] = p
    return p

def _compile_repl(*key):
    # internal: compile replacement pattern
    p = _cache_repl.get(key)
    if p is not None:
        return p
    repl, pattern = key
    try:
        p = sre_parse.parse_template(repl, pattern)
    except error, v:
        raise error, v # invalid expression
    if len(_cache_repl) >= _MAXCACHE:
        _cache_repl.clear()
    _cache_repl[key] = p
    return p

def _expand(pattern, match, template):
    # internal: match.expand implementation hook
    template = sre_parse.parse_template(template, pattern)
    return sre_parse.expand_template(template, match)

def _sub(pattern, template, text, count=0):
    # internal: pattern.sub implementation hook
    return _subn(pattern, template, text, count, 1)[0]

def _subn(pattern, template, text, count=0, sub=0):
    # internal: pattern.subn implementation hook
    if callable(template):
        filter = template
    else:
        template = _compile_repl(template, pattern)
        literals = template[1]
        if sub and not count:
            literal = pattern._getliteral()
            if literal and "\\" in literal:
                literal = None # may contain untranslated escapes
            if literal is not None and len(literals) == 1 and literals[0]:
                # shortcut: both pattern and string are literals
                return string.replace(text, pattern.pattern, literals[0]), 0
        def filter(match, template=template):
            return sre_parse.expand_template(template, match)
    n = i = 0
    s = []
    append = s.append
    c = pattern.scanner(text)
    while not count or n < count:
        m = c.search()
        if not m:
            break
        b, e = m.span()
        if i < b:
            append(text[i:b])
        elif i == b == e and n:
            append(text[i:b])
            continue # ignore empty match at previous position
        append(filter(m))
        i = e
        n = n + 1
    append(text[i:])
    return _join(s, text[:0]), n

def _split(pattern, text, maxsplit=0):
    # internal: pattern.split implementation hook
    n = i = 0
    s = []
    append = s.append
    extend = s.extend
    c = pattern.scanner(text)
    g = pattern.groups
    while not maxsplit or n < maxsplit:
        m = c.search()
        if not m:
            break
        b, e = m.span()
        if b == e:
            if i >= len(text):
                break
            continue
        append(text[i:b])
        if g and b != e:
            extend(list(m.groups()))
        i = e
        n = n + 1
    append(text[i:])
    return s

# register myself for pickling

import copy_reg

def _pickle(p):
    return _compile, (p.pattern, p.flags)

copy_reg.pickle(_pattern_type, _pickle, _compile)

# --------------------------------------------------------------------
# experimental stuff (see python-dev discussions for details)

class Scanner:
    def __init__(self, lexicon):
        from sre_constants import BRANCH, SUBPATTERN
        self.lexicon = lexicon
        # combine phrases into a compound pattern
        p = []
        s = sre_parse.Pattern()
        for phrase, action in lexicon:
            p.append(sre_parse.SubPattern(s, [
                (SUBPATTERN, (len(p), sre_parse.parse(phrase))),
                ]))
        p = sre_parse.SubPattern(s, [(BRANCH, (None, p))])
        s.groups = len(p)
        self.scanner = sre_compile.compile(p)
    def scan(self, string):
        result = []
        append = result.append
        match = self.scanner.match
        i = 0
        while 1:
            m = match(string, i)
            if not m:
                break
            j = m.end()
            if i == j:
                break
            action = self.lexicon[m.lastindex][1]
            if callable(action):
                self.match = m
                action = action(self, m.group())
            if action is not None:
                append(action)
            i = j
        return result, string[i:]

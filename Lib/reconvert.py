#! /usr/bin/env python1.5

r"""Convert old ("regex") regular expressions to new syntax ("re").

When imported as a module, there are two functions, with their own
strings:

  convert(s, syntax=None) -- convert a regex regular expression to re syntax

  quote(s) -- return a quoted string literal

When used as a script, read a Python string literal (or any other
expression evaluating to a string) from stdin, and write the
translated expression to stdout as a string literal.  Unless stdout is
a tty, no trailing \n is written to stdout.  This is done so that it
can be used with Emacs C-U M-| (shell-command-on-region with argument
which filters the region through the shell command).

No attempt has been made at coding for performance.

Translation table...

    \(    (     (unless RE_NO_BK_PARENS set)
    \)    )     (unless RE_NO_BK_PARENS set)
    \|    |     (unless RE_NO_BK_VBAR set)
    \<    \b    (not quite the same, but alla...)
    \>    \b    (not quite the same, but alla...)
    \`    \A
    \'    \Z

Not translated...

    .
    ^
    $
    *
    +           (unless RE_BK_PLUS_QM set, then to \+)
    ?           (unless RE_BK_PLUS_QM set, then to \?)
    \
    \b
    \B
    \w
    \W
    \1 ... \9

Special cases...

    Non-printable characters are always replaced by their 3-digit
    escape code (except \t, \n, \r, which use mnemonic escapes)

    Newline is turned into | when RE_NEWLINE_OR is set

XXX To be done...

    [...]     (different treatment of backslashed items?)
    [^...]    (different treatment of backslashed items?)
    ^ $ * + ? (in some error contexts these are probably treated differently)
    \vDD  \DD (in the regex docs but only works when RE_ANSI_HEX set)

"""


import warnings
warnings.filterwarnings("ignore", ".* regex .*", DeprecationWarning, __name__,
                        append=1)

import regex
from regex_syntax import * # RE_*

__all__ = ["convert","quote"]

# Default translation table
mastertable = {
    r'\<': r'\b',
    r'\>': r'\b',
    r'\`': r'\A',
    r'\'': r'\Z',
    r'\(': '(',
    r'\)': ')',
    r'\|': '|',
    '(': r'\(',
    ')': r'\)',
    '|': r'\|',
    '\t': r'\t',
    '\n': r'\n',
    '\r': r'\r',
}


def convert(s, syntax=None):
    """Convert a regex regular expression to re syntax.

    The first argument is the regular expression, as a string object,
    just like it would be passed to regex.compile().  (I.e., pass the
    actual string object -- string quotes must already have been
    removed and the standard escape processing has already been done,
    e.g. by eval().)

    The optional second argument is the regex syntax variant to be
    used.  This is an integer mask as passed to regex.set_syntax();
    the flag bits are defined in regex_syntax.  When not specified, or
    when None is given, the current regex syntax mask (as retrieved by
    regex.get_syntax()) is used -- which is 0 by default.

    The return value is a regular expression, as a string object that
    could be passed to re.compile().  (I.e., no string quotes have
    been added -- use quote() below, or repr().)

    The conversion is not always guaranteed to be correct.  More
    syntactical analysis should be performed to detect borderline
    cases and decide what to do with them.  For example, 'x*?' is not
    translated correctly.

    """
    table = mastertable.copy()
    if syntax is None:
        syntax = regex.get_syntax()
    if syntax & RE_NO_BK_PARENS:
        del table[r'\('], table[r'\)']
        del table['('], table[')']
    if syntax & RE_NO_BK_VBAR:
        del table[r'\|']
        del table['|']
    if syntax & RE_BK_PLUS_QM:
        table['+'] = r'\+'
        table['?'] = r'\?'
        table[r'\+'] = '+'
        table[r'\?'] = '?'
    if syntax & RE_NEWLINE_OR:
        table['\n'] = '|'
    res = ""

    i = 0
    end = len(s)
    while i < end:
        c = s[i]
        i = i+1
        if c == '\\':
            c = s[i]
            i = i+1
            key = '\\' + c
            key = table.get(key, key)
            res = res + key
        else:
            c = table.get(c, c)
            res = res + c
    return res


def quote(s, quote=None):
    """Convert a string object to a quoted string literal.

    This is similar to repr() but will return a "raw" string (r'...'
    or r"...") when the string contains backslashes, instead of
    doubling all backslashes.  The resulting string does *not* always
    evaluate to the same string as the original; however it will do
    just the right thing when passed into re.compile().

    The optional second argument forces the string quote; it must be
    a single character which is a valid Python string quote.

    """
    if quote is None:
        q = "'"
        altq = "'"
        if q in s and altq not in s:
            q = altq
    else:
        assert quote in ('"', "'")
        q = quote
    res = q
    for c in s:
        if c == q: c = '\\' + c
        elif c < ' ' or c > '~': c = "\\%03o" % ord(c)
        res = res + c
    res = res + q
    if '\\' in res:
        res = 'r' + res
    return res


def main():
    """Main program -- called when run as a script."""
    import sys
    s = eval(sys.stdin.read())
    sys.stdout.write(quote(convert(s)))
    if sys.stdout.isatty():
        sys.stdout.write("\n")


if __name__ == '__main__':
    main()

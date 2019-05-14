"""Filename matching with shell patterns.

fnmatch(FILENAME, PATTERN) matches according to the local convention.
fnmatchcase(FILENAME, PATTERN) always takes case in account.

The functions operate by translating the pattern into a regular
expression.  They cache the compiled regular expressions for speed.

The function translate(PATTERN) returns a regular expression
corresponding to PATTERN.  (It does not compile it.)
"""
import os
import posixpath
import re
import functools

__all__ = ["filter", "fnmatch", "fnmatchcase", "translate"]

def fnmatch(name, pat):
    """Test whether FILENAME matches PATTERN.

    Patterns are Unix shell style:

    *                 matches everything
    ?                 matches any single character
    [seq]             matches any character in seq
    [!seq]            matches any char not in seq
    {pat1,pat2,...}   matches pat1 or pat2 or ...

    An initial period in FILENAME is not special.
    Both FILENAME and PATTERN are first case-normalized
    if the operating system requires it.
    If you don't want this, use fnmatchcase(FILENAME, PATTERN).
    """
    name = os.path.normcase(name)
    pat = os.path.normcase(pat)
    return fnmatchcase(name, pat)

@functools.lru_cache(maxsize=256, typed=True)
def _compile_pattern(pat):
    if isinstance(pat, bytes):
        pat_str = str(pat, 'ISO-8859-1')
        res_str = translate(pat_str)
        res = bytes(res_str, 'ISO-8859-1')
    else:
        res = translate(pat)
    return re.compile(res).match

def filter(names, pat):
    """Return the subset of the list NAMES that match PAT."""
    result = []
    pat = os.path.normcase(pat)
    match = _compile_pattern(pat)
    if os.path is posixpath:
        # normcase on posix is NOP. Optimize it away from the loop.
        for name in names:
            if match(name):
                result.append(name)
    else:
        for name in names:
            if match(os.path.normcase(name)):
                result.append(name)
    return result

def fnmatchcase(name, pat):
    """Test whether FILENAME matches PATTERN, including case.

    This is a version of fnmatch() which doesn't case-normalize
    its arguments.
    """
    match = _compile_pattern(pat)
    return match(name) is not None


def _translate(pat):
    """Translate a shell PATTERN to a regular expression.

    There is no way to quote meta-characters.
    """

    i, n = 0, len(pat)
    res = ''
    while i < n:
        c = pat[i]
        i = i+1
        if c == '*':
            res = res + '.*'
        elif c == '?':
            res = res + '.'
        elif c == '[':
            j = i
            if j < n and pat[j] == '!':
                j = j+1
            if j < n and pat[j] == ']':
                j = j+1
            while j < n and pat[j] != ']':
                j = j+1
            if j >= n:
                res = res + '\\['
            else:
                stuff = pat[i:j]
                if '--' not in stuff:
                    stuff = stuff.replace('\\', r'\\')
                else:
                    chunks = []
                    k = i+2 if pat[i] == '!' else i+1
                    while True:
                        k = pat.find('-', k, j)
                        if k < 0:
                            break
                        chunks.append(pat[i:k])
                        i = k+1
                        k = k+3
                    chunks.append(pat[i:j])
                    # Escape backslashes and hyphens for set difference (--).
                    # Hyphens that create ranges shouldn't be escaped.
                    stuff = '-'.join(s.replace('\\', r'\\').replace('-', r'\-')
                                     for s in chunks)
                # Escape set operations (&&, ~~ and ||).
                stuff = re.sub(r'([&~|])', r'\\\1', stuff)
                i = j+1
                if stuff[0] == '!':
                    stuff = '^' + stuff[1:]
                elif stuff[0] in ('^', '['):
                    stuff = '\\' + stuff
                res = '%s[%s]' % (res, stuff)
        elif c == '{':
            # Handling of brace expression: '{PATTERN,PATTERN,...}'
            j = 1
            while j < n and pat[j] != '}':
                j = j + 1
            if j >= n:
                res = res + '\\{'
            else:
                stuff = pat[i:j]

                # Find indices of ',' in pattern excluding r'\,'.
                # E.g. for r'a\,a,b\b,c' it will be [4, 8]
                indices = [m.end() for m in re.finditer(r'[^\\],', stuff)]

                if len(indices) == 0:
                    res = res + '\\{'
                else:
                    i = j + 1
                    # Splitting pattern string based on ',' character.
                    # Also '\,' is translated to ','. E.g. for r'a\,a,b\b,c':
                    # * first_part = 'a,a'
                    # * last_part = 'c'
                    # * middle_part = ['b,b']
                    first_part = stuff[:indices[0] - 1].replace(r'\,', ',')
                    last_part = stuff[indices[-1]:].replace(r'\,', ',')
                    middle_parts = [
                            stuff[st:en - 1].replace(r'\,', ',')
                            for st, en in zip(indices, indices[1:])
                    ]

                    # creating the regex from splitted pattern. Each part is
                    # recursivelly evaluated.
                    expanded = functools.reduce(
                            lambda a,b: '|'.join((a, b)),
                            (_translate(elem) for elem in [first_part] + middle_parts + [last_part])
                    )
                    res = '%s(%s)' % (res, expanded)
        else:
            res = res + re.escape(c)
    return res

def translate(pat):
    res = _translate(pat)
    return r'(?s:%s)\Z' % res

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

    *       matches everything
    ?       matches any single character
    [seq]   matches any character in seq
    [!seq]  matches any char not in seq

    An initial period in FILENAME is not special.
    Both FILENAME and PATTERN are first case-normalized
    if the operating system requires it.
    If you don't want this, use fnmatchcase(FILENAME, PATTERN).
    """
    name = os.path.normcase(name)
    pat = os.path.normcase(pat)
    return fnmatchcase(name, pat)

@functools.lru_cache(maxsize=32768, typed=True)
def _compile_pattern(pat):
    if isinstance(pat, bytes):
        pat_str = str(pat, 'ISO-8859-1')
        res_str = translate(pat_str)
        res = bytes(res_str, 'ISO-8859-1')
    else:
        res = translate(pat)
    return re.compile(res).match

def filter(names, pat):
    """Construct a list from those elements of the iterable NAMES that match PAT."""
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


def translate(pat):
    """Translate a shell PATTERN to a regular expression.

    There is no way to quote meta-characters.
    """

    # Deal with STARs.
    inp = _scanner.scan(pat)[0]
    res = []
    add = res.append
    i, n = 0, len(inp)
    # Fixed pieces at the start?
    while i < n and inp[i] is not _STAR:
        add(inp[i])
        i += 1
    # Now deal with STAR fixed STAR fixed ...
    # For an interior `STAR fixed` pairing, we want to do a minimal
    # .*? match followed by `fixed`, with no possibility of backtracking.
    # Atomic groups ("(?>...)") allow us to spell that directly.
    # Note: people rely on the undocumented ability to join multiple
    # translate() results together via "|" to build large regexps matching
    # "one of many" shell patterns.
    while i < n:
        assert inp[i] is _STAR
        i += 1
        if i == n:
            add(".*")
            break
        assert inp[i] is not _STAR
        fixed = []
        while i < n and inp[i] is not _STAR:
            fixed.append(inp[i])
            i += 1
        fixed = "".join(fixed)
        if i == n:
            add(".*")
            add(fixed)
        else:
            add(f"(?>.*?{fixed})")
    assert i == n
    res = "".join(res)
    return fr'(?s:{res})\Z'


def _translate_literal(scanner, token):
    """Translate a literal token to a regular expression."""
    return re.escape(token)


def _translate_range(scanner, token):
    """Translate a character range, like 'a-z', to a regular expression."""
    start, end = token[0], token[2]
    if start > end:
        # Remove empty ranges -- invalid in RE.
        return ''
    return f'{re.escape(start)}-{re.escape(end)}'


def _translate_set(scanner, token):
    """Translate a set wildcard, like '[a-z]' or '[!ij]', to a regular expression."""
    negated = token[1] == '!'
    token = token[1+negated:-1]
    token = ''.join(_set_scanner.scan(token)[0])
    if negated:
        return f'[^{token}]' if token else '.'
    else:
        return f'[{token}]' if token else '(?!)'


_STAR = object()

_scanner = re.Scanner([
    (r'\*+', _STAR),
    (r'\?', '.'),
    (r'\[!?+\]?+[^\]]*\]', _translate_set),
    (r'.', _translate_literal),
], flags=re.DOTALL)

_set_scanner = re.Scanner([
    (r'.-.', _translate_range),
    (r'.', _translate_literal),
], flags=re.DOTALL)

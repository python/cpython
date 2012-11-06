"""Filename globbing utility."""

import os
import re
import fnmatch

__all__ = ["glob", "iglob"]

def glob(pathname):
    """Return a list of paths matching a pathname pattern.

    The pattern may contain simple shell-style wildcards a la fnmatch.

    """
    return list(iglob(pathname))


def iglob(pathname):
    """Return an iterator which yields the paths matching a pathname pattern.

    The pattern may contain simple shell-style wildcards a la fnmatch.

    """
    if not has_magic(pathname):
        if os.path.lexists(pathname):
            yield pathname
        return
    pathnames = expand_braces(pathname)
    for pathname in pathnames:
      dirname, basename = os.path.split(pathname)
      if not dirname:
          yield from glob1(None, basename)
          return

      if has_magic(dirname):
          dirs = iglob(dirname)
      else:
          dirs = [dirname]
      if has_magic(basename):
          glob_in_dir = glob1
      else:
          glob_in_dir = glob0
      for dirname in dirs:
          for name in glob_in_dir(dirname, basename):
              yield os.path.join(dirname, name)

# These 2 helper functions non-recursively glob inside a literal directory.
# They return a list of basenames. `glob1` accepts a pattern while `glob0`
# takes a literal basename (so it only has to check for its existence).

def glob1(dirname, pattern):
    if not dirname:
        if isinstance(pattern, bytes):
            dirname = bytes(os.curdir, 'ASCII')
        else:
            dirname = os.curdir
    try:
        names = os.listdir(dirname)
    except os.error:
        return []
    if pattern[0] != '.':
        names = [x for x in names if x[0] != '.']
    return fnmatch.filter(names, pattern)

def glob0(dirname, basename):
    if basename == '':
        # `os.path.split()` returns an empty basename for paths ending with a
        # directory separator.  'q*x/' should match only directories.
        if os.path.isdir(dirname):
            return [basename]
    else:
        if os.path.lexists(os.path.join(dirname, basename)):
            return [basename]
    return []


magic_check = re.compile('[*?[{]')
magic_check_bytes = re.compile(b'[*?[{]')
def has_magic(s):
    if isinstance(s, bytes):
        match = magic_check_bytes.search(s)
    else:
        match = magic_check.search(s)
    return match is not None

brace_matcher = re.compile(r'.*(\{.+?[^\\]\})')
def expand_braces(text):
    """Find the rightmost, innermost set of braces and, if it contains a
    comma-separated list, expand its contents recursively (any of its items
    may itself be a list enclosed in braces).

    Return the full set of expanded strings.
    """
    res = set()

    match = brace_matcher.search(text)
    if match is not None:
        sub = match.group(1)
        open_brace, close_brace = match.span(1)
        if "," in sub:
            for pat in sub.strip('{}').split(','):
                res.update(expand_braces(text[:open_brace] + pat + text[close_brace:]))

        else:
            res.update(expand_braces(text[:open_brace] + sub.replace('}', '\\}') + text[close_brace:]))

    else:
        res.add(text.replace('\\}', '}'))

    return res

"""
File sets and globbing helper for make_layout.
"""

__author__ = "Jeremy Paige <jeremyp@activestate.com>"
__version__ = "2.7"

import os
from glob import glob


class FileStemSet:
    def __init__(self, *patterns):
        self._names = set()
        self._prefixes = []
        self._suffixes = []
        for p in map(os.path.normcase, patterns):
            if p.endswith("*"):
                self._prefixes.append(p[:-1])
            elif p.startswith("*"):
                self._suffixes.append(p[1:])
            else:
                self._names.add(p)

    def _make_name(self, f):
        return os.path.normcase(os.path.splitext(f)[0])

    def __contains__(self, f):
        bn = self._make_name(f)
        return (
            bn in self._names
            or any(map(bn.startswith, self._prefixes))
            or any(map(bn.endswith, self._suffixes))
        )


class FileNameSet(FileStemSet):
    def _make_name(self, f):
        return os.path.normcase(os.path.basename(f))


class FileSuffixSet:
    def __init__(self, *patterns):
        self._names = set()
        self._prefixes = []
        self._suffixes = []
        for p in map(os.path.normcase, patterns):
            if p.startswith("*."):
                self._names.add(p[1:])
            elif p.startswith("*"):
                self._suffixes.append(p[1:])
            elif p.endswith("*"):
                self._prefixes.append(p[:-1])
            elif p.startswith("."):
                self._names.add(p)
            else:
                self._names.add("." + p)

    def _make_name(self, f):
        return os.path.normcase(os.path.splitext(f)[1])

    def __contains__(self, f):
        bn = self._make_name(f)
        return (
            bn in self._names
            or any(map(bn.startswith, self._prefixes))
            or any(map(bn.endswith, self._suffixes))
        )


def _rglob(root, pattern, condition):
    dirs = [root]
    recurse = pattern[:3] in {"**/", "**\\"}
    if recurse:
        pattern = pattern[3:]

    while dirs:
        d = dirs.pop(0)
        if recurse:
            dirs.extend(
                filter(
                    condition, (os.path.join(d, f2) for f2 in os.listdir(d) if os.path.isdir(os.path.join(d, f2)))
                )
            )
        for item in (
            (os.path.relpath(f, root), f)
            for f in glob(os.path.join(d, pattern))
            if os.path.isfile(f) and condition(f)
        ):
            yield item


def _return_true(f):
    return True


def rglob(root, patterns, condition=None):
    if not os.path.isdir(root):
        return
    if isinstance(patterns, tuple):
        for p in patterns:
            for item in _rglob(root, p, condition or _return_true):
                yield item
    else:
        for item in _rglob(root, patterns, condition or _return_true):
            yield item

"""
File sets and globbing helper for make_layout.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import os


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
        return os.path.normcase(f.stem)

    def __contains__(self, f):
        bn = self._make_name(f)
        return (
            bn in self._names
            or any(map(bn.startswith, self._prefixes))
            or any(map(bn.endswith, self._suffixes))
        )


class FileNameSet(FileStemSet):
    def _make_name(self, f):
        return os.path.normcase(f.name)


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
        return os.path.normcase(f.suffix)

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
                    condition, (type(root)(f2) for f2 in os.scandir(d) if f2.is_dir())
                )
            )
        yield from (
            (f.relative_to(root), f)
            for f in d.glob(pattern)
            if f.is_file() and condition(f)
        )


def _return_true(f):
    return True


def rglob(root, patterns, condition=None):
    if not os.path.isdir(root):
        return
    if isinstance(patterns, tuple):
        for p in patterns:
            yield from _rglob(root, p, condition or _return_true)
    else:
        yield from _rglob(root, patterns, condition or _return_true)

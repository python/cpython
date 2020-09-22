"""Utilities for comparing files and directories.

Classes:
    dircmp

Functions:
    cmp(f1, f2, shallow=True) -> int
    cmpfiles(a, b, common) -> ([], [], [])
    clear_cache()

"""

import os
import stat
from functools import cached_property
from itertools import filterfalse
from pathlib import Path

__all__ = ["clear_cache", "cmp", "dircmp", "cmpfiles", "DEFAULT_IGNORES"]

_cache = {}
BUFSIZE = 8 * 1024

DEFAULT_IGNORES = [
    "RCS",
    "CVS",
    "tags",
    ".git",
    ".hg",
    ".bzr",
    "_darcs",
    "__pycache__",
]


def clear_cache():
    """Clear the filecmp cache."""
    _cache.clear()


def cmp(f1, f2, shallow=True):
    """Compare two files.

    Arguments:

    f1 -- First file name

    f2 -- Second file name

    shallow -- Just check stat signature (do not read the files).
               defaults to True.

    Return value:

    True if the files are the same, False otherwise.

    This function uses a cache for past comparisons and the results,
    with cache entries invalidated if their stat information
    changes.  The cache may be cleared by calling clear_cache().

    """

    s1 = _sig(os.stat(f1))
    s2 = _sig(os.stat(f2))
    if s1[0] != stat.S_IFREG or s2[0] != stat.S_IFREG:
        return False
    if shallow and s1 == s2:
        return True
    if s1[1] != s2[1]:
        return False

    outcome = _cache.get((f1, f2, s1, s2))
    if outcome is None:
        outcome = _do_cmp(f1, f2)
        if len(_cache) > 100:  # limit the maximum size of the cache
            clear_cache()
        _cache[f1, f2, s1, s2] = outcome
    return outcome


def _sig(st):
    return (stat.S_IFMT(st.st_mode), st.st_size, st.st_mtime)


def _do_cmp(f1, f2):
    bufsize = BUFSIZE
    with open(f1, "rb") as fp1, open(f2, "rb") as fp2:
        while True:
            b1 = fp1.read(bufsize)
            b2 = fp2.read(bufsize)
            if b1 != b2:
                return False
            if not b1:
                return True


class dircmp:
    """A class that manages the comparison of 2 directories.

    dircmp(a, b, ignore=None, hide=None)
      A and B are directories.
      IGNORE is a list of names to ignore,
        defaults to DEFAULT_IGNORES.
      HIDE is a list of names to hide,
        defaults to [os.curdir, os.pardir].

    High level usage:
      x = dircmp(dir1, dir2)
      x.report() -> prints a report on the differences between dir1 and dir2
       or
      x.report_partial_closure() -> prints report on differences between dir1
            and dir2, and reports on common immediate subdirectories.
      x.report_full_closure() -> like report_partial_closure,
            but fully recursive.

    Attributes:
     left_list, right_list: The files in dir1 and dir2,
        filtered by hide and ignore.
     common: a list of names in both dir1 and dir2.
     left_only, right_only: names only in dir1, dir2.
     common_dirs: subdirectories in both dir1 and dir2.
     common_files: files in both dir1 and dir2.
     common_funny: names in both dir1 and dir2 where the type differs between
        dir1 and dir2, or the name is not stat-able.
     same_files: list of identical files.
     diff_files: list of filenames which differ.
     funny_files: list of files which could not be compared.
     subdirs: a dictionary of dircmp objects, keyed by names in common_dirs.
     """

    def __init__(self, a, b, ignore=None, hide=None):  # Initialize
        self.left = a
        self.right = b
        if hide is None:
            self.hide = [os.curdir, os.pardir]  # Names never to be shown
        else:
            self.hide = hide
        if ignore is None:
            self.ignore = DEFAULT_IGNORES
        else:
            self.ignore = ignore

    @cached_property
    def left_list(self):
        return sorted(_filter(os.listdir(self.left), self.hide + self.ignore))

    @cached_property
    def right_list(self):
        return sorted(_filter(os.listdir(self.right), self.hide + self.ignore))

    @cached_property
    def common(self):
        left_set = set(self.left_list)
        return list(left_set.intersection(self.right_list))

    @cached_property
    def left_only(self):
        left_set = set(self.left_list)
        return list(left_set.difference(self.common))

    @cached_property
    def right_only(self):
        right_set = set(self.right_list)
        return list(right_set.difference(self.common))

    @cached_property
    def common_dirs(self):
        left_path = Path(self.left)
        right_path = Path(self.right)
        return [
            name
            for name in self.common
            if (left_path / name).isdir() and (right_path / name).isdir()
        ]

    @cached_property
    def common_dirs(self):
        left_path = Path(self.left)
        right_path = Path(self.right)
        return [
            name
            for name in self.common
            if (left_path / name).is_dir() and (right_path / name).is_dir()
        ]

    @cached_property
    def common_files(self):
        left_path = Path(self.left)
        right_path = Path(self.right)
        return [
            name
            for name in self.common
            if (left_path / name).is_file() and (right_path / name).is_file()
        ]

    @cached_property
    def common_funny(self):
        return sorted(
            _filter(self.common, self.common_files + self.common_dirs)
        )

    @cached_property
    def cmpfiles(self):
        return cmpfiles(self.left, self.right, self.common_files)

    @cached_property
    def same_files(self):
        same_files, _, _ = self.cmpfiles
        return same_files

    @cached_property
    def diff_files(self):
        _, diff_files, _ = self.cmpfiles
        return diff_files

    @cached_property
    def funny_files(self):
        _, _, funny_files = self.cmpfiles
        return funny_files

    @cached_property
    def subdirs(self):
        return {
            subdir: dircmp(
                os.path.join(self.left, subdir),
                os.path.join(self.right, subdir),
                self.ignore,
                self.hide,
            )
            for subdir in self.common_dirs
        }

    def compare_subdirs(self):
        """Recursively compare on subdirectories"""
        for sd in self.subdirs.values():
            sd.compare_subdirs()

    def report(self):
        """Print a report on the differences between a and b
        Output format is purposely lousy
        """
        print("diff", self.left, self.right)
        if self.left_only:
            self.left_only.sort()
            print("Only in", self.left, ":", self.left_only)
        if self.right_only:
            self.right_only.sort()
            print("Only in", self.right, ":", self.right_only)
        if self.same_files:
            self.same_files.sort()
            print("Identical files :", self.same_files)
        if self.diff_files:
            self.diff_files.sort()
            print("Differing files :", self.diff_files)
        if self.funny_files:
            self.funny_files.sort()
            print("Trouble with common files :", self.funny_files)
        if self.common_dirs:
            self.common_dirs.sort()
            print("Common subdirectories :", self.common_dirs)
        if self.common_funny:
            self.common_funny.sort()
            print("Common funny cases :", self.common_funny)

    def report_partial_closure(self):
        """Print reports on self and on subdirs """
        self.report()
        for sd in self.subdirs.values():
            print()
            sd.report()

    def report_full_closure(self):
        """Report on self and subdirs recursively"""
        self.report()
        for sd in self.subdirs.values():
            print()
            sd.report_full_closure()


def cmpfiles(a, b, common, shallow=True):
    """Compare common files in two directories.

    a, b -- directory names
    common -- list of file names found in both directories
    shallow -- if true, do comparison based solely on stat() information

    Returns a tuple of three lists:
      files that compare equal
      files that are different
      filenames that aren't regular files.

    """
    res = ([], [], [])
    for x in common:
        ax = os.path.join(a, x)
        bx = os.path.join(b, x)
        res[_cmp(ax, bx, shallow)].append(x)
    return res


# Compare two files.
# Return:
#       0 for equal
#       1 for different
#       2 for funny cases (can't stat, etc.)
#
def _cmp(a, b, sh, abs=abs, cmp=cmp):
    try:
        return not abs(cmp(a, b, sh))
    except OSError:
        return 2


# Return a copy with items that occur in skip removed.
#
def _filter(flist, skip):
    return {name for name in flist if name not in skip}


# Demonstration and testing.
#
def demo():
    import sys
    import getopt

    options, args = getopt.getopt(sys.argv[1:], "r")
    if len(args) != 2:
        raise getopt.GetoptError("need exactly two args", None)
    dd = dircmp(args[0], args[1])
    if ("-r", "") in options:
        dd.report_full_closure()
    else:
        dd.report()


if __name__ == "__main__":
    demo()

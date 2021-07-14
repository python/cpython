"""Utilities for comparing files and directories.

Classes:
    dircmp

Functions:
    cmp(f1, f2, shallow=True) -> int
    cmpfiles(a, b, common) -> ([], [], [])
    clear_cache()

"""

from os import stat, pardir, curdir, listdir
from os.path import normcase, join
from stat import S_IFREG, S_IFMT, S_ISDIR, S_ISREG
from itertools import filterfalse

__all__ = ['clear_cache', 'cmp', 'dircmp', 'cmpfiles', 'DEFAULT_IGNORES']

_cache = {}
BUFSIZE = 8*1024

DEFAULT_IGNORES = [
    'RCS', 'CVS', 'tags', '.git', '.hg', '.bzr', '_darcs', '__pycache__']

def clear_cache():
    """ Clear the filecmp cache. """
    _cache.clear()

def cmp(filename_0, filename_1, shallow = True):
    """Compare two files.

    Arguments:

    filename_0 -- First file name

    filename_1 -- Second file name

    shallow -- Just check stat signature (do not read the files).
               defaults to True.

    Return value:

    True if the files are the same, False otherwise.

    This function uses a cache for past comparisons and the results,
    with cache entries invalidated if their stat information
    changes.  The cache may be cleared by calling clear_cache().

    """

    metadata_0 = _metadata(stat(filename_0))
    metadata_1 = _metadata(stat(filename_1))
    if metadata_0[0] != S_IFREG or metadata_1[0] != S_IFREG:
        return False
    if shallow and metadata_0 == metadata_1:
        return True
    if metadata_0[1] != metadata_1[1]:
        return False
    compare_tuple = (filename_0, filename_1)
    outcome = _cache.get(compare_tuple)
    if outcome is None:
        if len(_cache) > 100: # limit the maximum size of the cache
            clear_cache()
        _cache[compare_tuple] = outcome = _do_cmp(*compare_tuple)
    return outcome

def _metadata(metadata):
    return (S_IFMT(metadata.st_mode), metadata.st_size, metadata.st_mtime)

def _do_cmp(filename_1, filename_2):
    with open(filename_1, 'rb') as fp1, open(filename_2, 'rb') as fp2:
        while True:
            bytes_1, bytes_2 = fp1.read(BUFSIZE), fp2.read(BUFSIZE)
            if bytes_1 != bytes_2:
                return False
            if not bytes_1:
                return True

def cmpfiles(a, b, common, shallow = True):
    """Compare common files in two directories.

    a, b -- directory names
    common -- list of file names found in both directories
    shallow -- if true, do comparison based solely on stat() information

    Returns a tuple of three lists:
      files that compare equal
      files that are different
      filenames that aren't regular files.

    """
    for x in common:
        case = _cmp(join(a, x), join(b, x), shallow)
        if case == 0:
            yield (x, None, None)
        elif case == 1:
            yield (None, x, None)
        elif case == 2:
            yield (None, None, x)

# Compare two files.
# Return:
#       0 for equal
#       1 for different
#       2 for funny cases (can't stat, etc.)
#
def _cmp(a, b, shallow, abs = abs, cmp = cmp):
    try:
        return not abs(cmp(a, b, shallow))
    except OSError:
        return 2

def _filter_list(dirs, hide, ignore):
    for directory in filterfalse((hide + ignore).__contains__, listdir(dirs)):
        yield directory

def _filter_only(a, b):
    for x in map(a.__getitem__, filterfalse(b.__contains__, a)):
        yield x


# Return a copy with items that occur in skip removed.
#
class dircmp:
    def __init__(self, left, right, *args):
        self.left, self.right = left, right
        print(args, len(args))
        self.hide = [curdir, pardir] if len(args) == 0 else args[0]
        self.ignore = DEFAULT_IGNORES if len(args) < 2 else args[1]

    def _left_list(self):
        yield from _filter_list(self.left, self.hide, self.ignore)

    def _right_list(self):
        yield from _filter_list(self.right, self.hide, self.ignore)

    def _left_only(self):
        left_zip = dict(zip(map(normcase, self._left_list()), self._left_list()))
        # right_zip = zip(map(normcase, self._right_list()), self._right_list())
        for left, right in zip(left_zip, map(normcase, self._right_list())):
            if not (right in left_zip):
                yield left_zip[left]

    def _right_only(self):
        # left_zip = zip(map(normcase, self._left_list()), self._left_list())
        right_zip = dict(zip(map(normcase, self._right_list()), self._right_list()))
        for left, right in zip(map(normcase, self._left_list()), right_zip):
            if not (left in right_zip):
                yield right_zip[right]

    def _common(self):
        left_zip = dict(zip(map(normcase, self._left_list()), self._left_list()))
        # right_zip = zip(map(normcase, self._right_list()), self._right_list())
        for left, right in zip(left_zip, map(normcase, self._right_list())):
            if right in left_zip:
                yield left_zip[right]

    def _common_dirs(self):
        for name in self._common():
            if S_ISDIR(S_IFMT(stat(join(self.left, name)).st_mode)):
                yield name

    def _common_files(self):
        for name in self._common():
            if S_ISREG(S_IFMT(stat(join(self.left, name)).st_mode)):
                yield name

    def _common_funny(self):
        for name in self._common():
            a_type = S_IFMT(stat(join(self.left, name)).st_mode)
            b_type = S_IFMT(stat(join(self.right, name)).st_mode)
            if a_type != b_type:
                yield name
            elif not S_ISREG(a_type):
                yield name
            elif not S_ISDIR(a_type):
                yield name

    def _same_files(self):
        for file, *_ in cmpfiles(self.left, self.right, self._common_files()):
            yield file

    def _diff_files(self):
        for _, file, _ in cmpfiles(self.left, self.right, self._common_files()):
            yield file

    def _funny_files(self):
        for *_, file in cmpfiles(self.left, self.right, self._common_files()):
            yield file
    
    def _subdirs(self):
        for directory in self._common_dirs():
            yield (directory, dircmp(
                join(self.left, directory), join(self.right, directory),
                self.ignore, self.hide))
    
    def recurs_subdirs(self):
        if self._subdirs():
            for directory, dircmp_object in self._subdirs():
                yield dircmp_object
                for inner_dircmp_object in dircmp_object.recurs_subdirs():
                    yield inner_dircmp_object
        else:
            yield self
    
    methodmap = {"subdirs":_subdirs, "same_files": _same_files,
                 "diff_files": _diff_files, "funny_files": _funny_files,
                 "common_dirs": _common_dirs, "common_files": _common_files,
                 "common_funny": _common_funny, "common": _common,
                 "left_only": _left_only, "right_only": _right_only,
                 "left_list": _left_list, "right_list": _right_list}
    
    def __getattr__(self, attr):
        if attr not in self.methodmap:
            raise AttributeError(attr)
        yield from self.methodmap[attr](self)
    
    def report(self): # Print a report on the differences between a and b
        # Output format is purposely lousy
        print(f" Differencing {self.left} {self.right}")
        if self.left_only:
            print(f"Only in {self.left} :")
            for x in self.left_only:
                print('\t', x)
        if self.right_only:
            print(f"Only in {self.right} :")
            for x in self.right_only:
                print('\t', x)
        if self.same_files:
            print("Identical files :")
            for x in self.same_files:
                print('\t', x)
        if self.diff_files:
            print("Differing files :")
            for x in self.diff_files:
                print('\t', x)
        if self.funny_files:
            print("Trouble with common files :")
            for x in self.funny_files:
                print('\t', x)
        if self.common_dirs:
            print("Common subdirectories :")
            for x in self.common_dirs:
                print('\t', x)
        if self.common_funny:
            print("Common funny cases :")
            for x in self.common_funny:
                print('\t', x)

    def report_partial_closure(self): # Print reports on self and on subdirs
        self.report()
        for _, dircmp_object in self.subdirs():
            print()
            dircmp_object.report()

    def report_full_closure(self): # Report on self and subdirs recursively
        self.report()
        for dircmp_object in self.recurs_subdirs():
            print()
            dircmp_object.report_full_closure()

# Demonstration and testing.
#
def demo():
    from sys import argv
    from getopt import getopt, GetoptError
    options, args = getopt(argv[1:], 'r')
    if len(args) != 2:
        raise GetoptError("need exactly two args", None)
    directory_comparision = dircmp(args[0], args[1])
    if ('-r', '') in options:
        directory_comparision.report_full_closure()
    else:
        directory_comparision.report()

if __name__ == '__main__':
    demo()

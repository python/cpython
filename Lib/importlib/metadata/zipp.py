"""
>>> root = Path(getfixture('zipfile_abcde'))
>>> a, b = root.iterdir()
>>> a
Path('abcde.zip', 'a.txt')
>>> b
Path('abcde.zip', 'b/')
>>> b.name
'b'
>>> c = b / 'c.txt'
>>> c
Path('abcde.zip', 'b/c.txt')
>>> c.name
'c.txt'
>>> c.read_text()
'content of c'
>>> c.exists()
True
>>> (b / 'missing.txt').exists()
False
>>> str(c)
'abcde.zip/b/c.txt'
"""

from __future__ import division

import io
import sys
import posixpath
import zipfile
import operator
import functools

__metaclass__ = type


class Path:
    __repr = '{self.__class__.__name__}({self.root.filename!r}, {self.at!r})'

    def __init__(self, root, at=''):
        self.root = root if isinstance(root, zipfile.ZipFile) \
            else zipfile.ZipFile(self._pathlib_compat(root))
        self.at = at

    @staticmethod
    def _pathlib_compat(path):
        """
        For path-like objects, convert to a filename for compatibility
        on Python 3.6.1 and earlier.
        """
        try:
            return path.__fspath__()
        except AttributeError:
            return str(path)

    @property
    def open(self):
        return functools.partial(self.root.open, self.at)

    @property
    def name(self):
        return posixpath.basename(self.at.rstrip('/'))

    def read_text(self, *args, **kwargs):
        with self.open() as strm:
            return io.TextIOWrapper(strm, *args, **kwargs).read()

    def read_bytes(self):
        with self.open() as strm:
            return strm.read()

    def _is_child(self, path):
        return posixpath.dirname(path.at.rstrip('/')) == self.at.rstrip('/')

    def _next(self, at):
        return Path(self.root, at)

    def is_dir(self):
        return not self.at or self.at.endswith('/')

    def is_file(self):
        return not self.is_dir()

    def exists(self):
        return self.at in self.root.namelist()

    def iterdir(self):
        if not self.is_dir():
            raise ValueError("Can't listdir a file")
        names = map(operator.attrgetter('filename'), self.root.infolist())
        subs = map(self._next, names)
        return filter(self._is_child, subs)

    def __str__(self):
        return posixpath.join(self.root.filename, self.at)

    def __repr__(self):
        return self.__repr.format(self=self)

    def __truediv__(self, add):
        add = self._pathlib_compat(add)
        next = posixpath.join(self.at, add)
        next_dir = posixpath.join(self.at, add, '')
        names = self.root.namelist()
        return self._next(
            next_dir if next not in names and next_dir in names else next
        )

    if sys.version_info < (3,):
        __div__ = __truediv__
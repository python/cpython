"""
Simple implementation of JoinablePath, for use in pathlib tests.
"""

import ntpath
import os.path
import posixpath

from . import is_pypi

if is_pypi:
    from pathlib_abc import _JoinablePath
else:
    from pathlib.types import _JoinablePath


class LexicalPath(_JoinablePath):
    __slots__ = ('_segments',)
    parser = os.path

    def __init__(self, *pathsegments):
        self._segments = pathsegments

    def __hash__(self):
        return hash(str(self))

    def __eq__(self, other):
        if not isinstance(other, LexicalPath):
            return NotImplemented
        return str(self) == str(other)

    def __str__(self):
        if not self._segments:
            return ''
        return self.parser.join(*self._segments)

    def __repr__(self):
        return f'{type(self).__name__}({str(self)!r})'

    def with_segments(self, *pathsegments):
        return type(self)(*pathsegments)


class LexicalPosixPath(LexicalPath):
    __slots__ = ()
    parser = posixpath


class LexicalWindowsPath(LexicalPath):
    __slots__ = ()
    parser = ntpath

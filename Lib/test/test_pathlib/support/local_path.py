"""
Implementations of ReadablePath and WritablePath for local paths, for use in
pathlib tests.

LocalPathGround is also defined here. It helps establish the "ground truth"
about local paths in tests.
"""

import os

from . import is_pypi
from .lexical_path import LexicalPath

if is_pypi:
    from shutil import rmtree
    from pathlib_abc import PathInfo, _ReadablePath, _WritablePath
    can_symlink = True
    testfn = "TESTFN"
else:
    from pathlib.types import PathInfo, _ReadablePath, _WritablePath
    from test.support import os_helper
    can_symlink = os_helper.can_symlink()
    testfn = os_helper.TESTFN
    rmtree = os_helper.rmtree


class LocalPathGround:
    can_symlink = can_symlink

    def __init__(self, path_cls):
        self.path_cls = path_cls

    def setup(self, local_suffix=""):
        root = self.path_cls(testfn + local_suffix)
        os.mkdir(root)
        return root

    def teardown(self, root):
        rmtree(root)

    def create_file(self, p, data=b''):
        with open(p, 'wb') as f:
            f.write(data)

    def create_dir(self, p):
        os.mkdir(p)

    def create_symlink(self, p, target):
        os.symlink(target, p)

    def create_hierarchy(self, p):
        os.mkdir(os.path.join(p, 'dirA'))
        os.mkdir(os.path.join(p, 'dirB'))
        os.mkdir(os.path.join(p, 'dirC'))
        os.mkdir(os.path.join(p, 'dirC', 'dirD'))
        with open(os.path.join(p, 'fileA'), 'wb') as f:
            f.write(b"this is file A\n")
        with open(os.path.join(p, 'dirB', 'fileB'), 'wb') as f:
            f.write(b"this is file B\n")
        with open(os.path.join(p, 'dirC', 'fileC'), 'wb') as f:
            f.write(b"this is file C\n")
        with open(os.path.join(p, 'dirC', 'novel.txt'), 'wb') as f:
            f.write(b"this is a novel\n")
        with open(os.path.join(p, 'dirC', 'dirD', 'fileD'), 'wb') as f:
            f.write(b"this is file D\n")
        if self.can_symlink:
            # Relative symlinks.
            os.symlink('fileA', os.path.join(p, 'linkA'))
            os.symlink('non-existing', os.path.join(p, 'brokenLink'))
            os.symlink('dirB',
                       os.path.join(p, 'linkB'),
                       target_is_directory=True)
            os.symlink(os.path.join('..', 'dirB'),
                       os.path.join(p, 'dirA', 'linkC'),
                       target_is_directory=True)
            # Broken symlink (pointing to itself).
            os.symlink('brokenLinkLoop', os.path.join(p, 'brokenLinkLoop'))

    isdir = staticmethod(os.path.isdir)
    isfile = staticmethod(os.path.isfile)
    islink = staticmethod(os.path.islink)
    readlink = staticmethod(os.readlink)

    def readtext(self, p):
        with open(p, 'r', encoding='utf-8') as f:
            return f.read()

    def readbytes(self, p):
        with open(p, 'rb') as f:
            return f.read()


class LocalPathInfo(PathInfo):
    """
    Simple implementation of PathInfo for a local path
    """
    __slots__ = ('_path', '_exists', '_is_dir', '_is_file', '_is_symlink')

    def __init__(self, path):
        self._path = os.fspath(path)
        self._exists = None
        self._is_dir = None
        self._is_file = None
        self._is_symlink = None

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        if not follow_symlinks and self.is_symlink():
            return True
        if self._exists is None:
            self._exists = os.path.exists(self._path)
        return self._exists

    def is_dir(self, *, follow_symlinks=True):
        """Whether this path is a directory."""
        if not follow_symlinks and self.is_symlink():
            return False
        if self._is_dir is None:
            self._is_dir = os.path.isdir(self._path)
        return self._is_dir

    def is_file(self, *, follow_symlinks=True):
        """Whether this path is a regular file."""
        if not follow_symlinks and self.is_symlink():
            return False
        if self._is_file is None:
            self._is_file = os.path.isfile(self._path)
        return self._is_file

    def is_symlink(self):
        """Whether this path is a symbolic link."""
        if self._is_symlink is None:
            self._is_symlink = os.path.islink(self._path)
        return self._is_symlink


class ReadableLocalPath(_ReadablePath, LexicalPath):
    """
    Simple implementation of a ReadablePath class for local filesystem paths.
    """
    __slots__ = ('info',)
    __fspath__ = LexicalPath.__vfspath__

    def __init__(self, *pathsegments):
        super().__init__(*pathsegments)
        self.info = LocalPathInfo(self)

    def __open_rb__(self, buffering=-1):
        return open(self, 'rb')

    def iterdir(self):
        return (self / name for name in os.listdir(self))

    def readlink(self):
        return self.with_segments(os.readlink(self))


class WritableLocalPath(_WritablePath, LexicalPath):
    """
    Simple implementation of a WritablePath class for local filesystem paths.
    """

    __slots__ = ()
    __fspath__ = LexicalPath.__vfspath__

    def __open_wb__(self, buffering=-1):
        return open(self, 'wb')

    def mkdir(self, mode=0o777):
        os.mkdir(self, mode)

    def symlink_to(self, target, target_is_directory=False):
        os.symlink(target, self, target_is_directory)

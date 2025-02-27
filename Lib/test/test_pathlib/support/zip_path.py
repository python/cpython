import errno
import io
import pathlib._abc
import posixpath
import stat
import zipfile


class MissingZipPathInfo:
    """
    PathInfo implementation that is used when a zip file member is missing.
    """

    def exists(self, follow_symlinks=True):
        return False

    def is_dir(self, follow_symlinks=True):
        return False

    def is_file(self, follow_symlinks=True):
        return False

    def is_symlink(self, follow_symlinks=True):
        return False


class ZipPathInfo:
    """
    PathInfo implementation for an existing zip file member.
    """
    __slots__ = ('parent', 'children', 'zip_info')

    def __init__(self, parent=None):
        self.parent = parent or self
        self.children = {}
        self.zip_info = None

    def exists(self, follow_symlinks=True):
        return True

    def is_dir(self, follow_symlinks=True):
        if self.zip_info is None:
            # This directory is implied to exist by another member.
            return True
        return self.zip_info.filename.endswith('/')

    def is_file(self, follow_symlinks=True):
        return not self.is_dir(follow_symlinks=follow_symlinks)

    def is_symlink(self):
        return False


class ZipFileList:
    """
    `list`-like object that we inject as `ZipFile.filelist`. We maintain a
    tree of `ZipPathInfo` objects representing the zip file members. A new
     `resolve()` method fetches an entry from the tree.
    """

    __slots__ = ('_roots', '_items')

    def __init__(self, items):
        self._roots = {
            '': ZipPathInfo(),
            '/': ZipPathInfo(),
            '//': ZipPathInfo(),
        }
        self._items = []
        for item in items:
            self.append(item)

    def __len__(self):
        return len(self._items)

    def __iter__(self):
        return iter(self._items)

    def append(self, item):
        self._items.append(item)
        self.resolve(item.filename, create=True).zip_info = item

    def resolve(self, path, create=False):
        """
        Returns a PathInfo object for the given path by walking the tree.
        """
        _drive, root, path = posixpath.splitroot(path)
        path_info = self._roots[root]
        for name in path.split('/'):
            if not name or name == '.':
                pass
            elif name == '..':
                path_info = path_info.parent
            elif name in path_info.children:
                path_info = path_info.children[name]
            elif create:
                path_info.children[name] = path_info = ZipPathInfo(path_info)
            else:
                path_info = MissingZipPathInfo()
                break
        return path_info


class ReadableZipPath(pathlib._abc.ReadablePath):
    """
    Simple implementation of a ReadablePath class for .zip files.
    """

    __slots__ = ('_segments', 'zip_file')
    parser = posixpath

    def __init__(self, *pathsegments, zip_file):
        self._segments = pathsegments
        self.zip_file = zip_file
        if not isinstance(zip_file.filelist, ZipFileList):
            zip_file.filelist = ZipFileList(zip_file.filelist)

    def __hash__(self):
        return hash((str(self), self.zip_file))

    def __eq__(self, other):
        if not isinstance(other, ReadableZipPath):
            return NotImplemented
        return str(self) == str(other) and self.zip_file is other.zip_file

    def __str__(self):
        if not self._segments:
            return ''
        return self.parser.join(*self._segments)

    def with_segments(self, *pathsegments):
        return type(self)(*pathsegments, zip_file=self.zip_file)

    def __open_rb__(self, buffering=-1):
        info = self.info
        if not info.exists():
            raise FileNotFoundError(errno.ENOENT, "File not found", self)
        elif info.is_dir():
            raise IsADirectoryError(errno.EISDIR, "Is a directory", self)
        return self.zip_file.open(info.zip_info, 'r')

    def iterdir(self):
        info = self.info
        if not info.exists():
            raise FileNotFoundError(errno.ENOENT, "File not found", self)
        elif not info.is_dir():
            raise NotADirectoryError(errno.ENOTDIR, "Not a directory", self)
        return (self / name for name in info.children)

    def readlink(self):
        raise OSError(errno.ENOTSUP, "Not supported", self)

    @property
    def info(self):
        return self.zip_file.filelist.resolve(str(self))


class WritableZipPath(pathlib._abc.WritablePath):
    """
    Simple implementation of a WritablePath class for .zip files.
    """

    __slots__ = ('_segments', 'zip_file')
    parser = posixpath

    def __init__(self, *pathsegments, zip_file):
        self._segments = pathsegments
        self.zip_file = zip_file

    def __hash__(self):
        return hash((str(self), self.zip_file))

    def __eq__(self, other):
        if not isinstance(other, WritableZipPath):
            return NotImplemented
        return str(self) == str(other) and self.zip_file is other.zip_file

    def __str__(self):
        if not self._segments:
            return ''
        return self.parser.join(*self._segments)

    def with_segments(self, *pathsegments):
        return type(self)(*pathsegments, zip_file=self.zip_file)

    def __open_wb__(self, buffering=-1):
        return self.zip_file.open(str(self), 'w')

    def mkdir(self, mode=0o777):
        self.zip_file.mkdir(str(self), mode)

    def symlink_to(self, target, target_is_directory=False):
        zinfo = zipfile.ZipInfo(str(self))._for_archive(self.zip_file)
        zinfo.external_attr = stat.S_IFLNK << 16
        if target_is_directory:
            zinfo.external_attr |= 0x10
        self.zip_file.writestr(zinfo, str(target))


class ZipPathGround:
    can_symlink = True

    def __init__(self, path_cls):
        self.path_cls = path_cls

    def setup(self):
        return self.path_cls(zip_file=zipfile.ZipFile(io.BytesIO(), "w"))

    def teardown(self, root):
        root.zip_file.close()

    def create_file(self, path, data=b''):
        path.zip_file.writestr(str(path), data)

    def create_link(self, path, target):
        zip_info = zipfile.ZipInfo(str(path))
        zip_info.external_attr = stat.S_IFLNK << 16
        path.zip_file.writestr(zip_info, target.encode())

    def create_hierarchy(self, p):
        self.create_file(p.joinpath('fileA'), b'this is file A\n')
        self.create_link(p.joinpath('linkA'), 'fileA')
        self.create_link(p.joinpath('linkB'), 'dirB')
        self.create_link(p.joinpath('dirA/linkC'), '../dirB')
        self.create_file(p.joinpath('dirB/fileB'), b'this is file B\n')
        self.create_link(p.joinpath('dirB/linkD'), '../dirB')
        self.create_file(p.joinpath('dirC/fileC'), b'this is file C\n')
        self.create_file(p.joinpath('dirC/dirD/fileD'), b'this is file D\n')
        self.create_file(p.joinpath('dirC/novel.txt'), b'this is a novel\n')
        self.create_link(p.joinpath('brokenLink'), 'non-existing')
        self.create_link(p.joinpath('brokenLinkLoop'), 'brokenLinkLoop')

    def readtext(self, p):
        return p.zip_file.read(str(p)).decode()

    def readbytes(self, p):
        return p.zip_file.read(str(p))

    readlink = readtext

    def isdir(self, p):
        path_str = str(p) + "/"
        return path_str in p.zip_file.NameToInfo

    def isfile(self, p):
        info = p.zip_file.NameToInfo.get(str(p))
        if info is None:
            return False
        return not stat.S_ISLNK(info.external_attr >> 16)

    def islink(self, p):
        info = p.zip_file.NameToInfo.get(str(p))
        if info is None:
            return False
        return stat.S_ISLNK(info.external_attr >> 16)

"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import *
from stat import S_IMODE, S_ISLNK
import io
import os
import sys
try:
    import fcntl
except ImportError:
    fcntl = None
try:
    import posix
except ImportError:
    posix = None
try:
    import _winapi
except ImportError:
    _winapi = None


def _get_copy_blocksize(infd):
    """Determine blocksize for fastcopying on Linux.
    Hopefully the whole file will be copied in a single call.
    The copying itself should be performed in a loop 'till EOF is
    reached (0 return) so a blocksize smaller or bigger than the actual
    file size should not make any difference, also in case the file
    content changes while being copied.
    """
    try:
        blocksize = max(os.fstat(infd).st_size, 2 ** 23)  # min 8 MiB
    except OSError:
        blocksize = 2 ** 27  # 128 MiB
    # On 32-bit architectures truncate to 1 GiB to avoid OverflowError,
    # see gh-82500.
    if sys.maxsize < 2 ** 32:
        blocksize = min(blocksize, 2 ** 30)
    return blocksize


if fcntl and hasattr(fcntl, 'FICLONE'):
    def _ficlone(source_fd, target_fd):
        """
        Perform a lightweight copy of two files, where the data blocks are
        copied only when modified. This is known as Copy on Write (CoW),
        instantaneous copy or reflink.
        """
        fcntl.ioctl(target_fd, fcntl.FICLONE, source_fd)
else:
    _ficlone = None


if posix and hasattr(posix, '_fcopyfile'):
    def _fcopyfile(source_fd, target_fd):
        """
        Copy a regular file content using high-performance fcopyfile(3)
        syscall (macOS).
        """
        posix._fcopyfile(source_fd, target_fd, posix._COPYFILE_DATA)
else:
    _fcopyfile = None


if hasattr(os, 'copy_file_range'):
    def _copy_file_range(source_fd, target_fd):
        """
        Copy data from one regular mmap-like fd to another by using a
        high-performance copy_file_range(2) syscall that gives filesystems
        an opportunity to implement the use of reflinks or server-side
        copy.
        This should work on Linux >= 4.5 only.
        """
        blocksize = _get_copy_blocksize(source_fd)
        offset = 0
        while True:
            sent = os.copy_file_range(source_fd, target_fd, blocksize,
                                      offset_dst=offset)
            if sent == 0:
                break  # EOF
            offset += sent
else:
    _copy_file_range = None


if hasattr(os, 'sendfile'):
    def _sendfile(source_fd, target_fd):
        """Copy data from one regular mmap-like fd to another by using
        high-performance sendfile(2) syscall.
        This should work on Linux >= 2.6.33 only.
        """
        blocksize = _get_copy_blocksize(source_fd)
        offset = 0
        while True:
            sent = os.sendfile(target_fd, source_fd, offset, blocksize)
            if sent == 0:
                break  # EOF
            offset += sent
else:
    _sendfile = None


if _winapi and hasattr(_winapi, 'CopyFile2'):
    def copyfile(source, target):
        """
        Copy from one file to another using CopyFile2 (Windows only).
        """
        _winapi.CopyFile2(source, target, 0)
else:
    copyfile = None


def copyfileobj(source_f, target_f):
    """
    Copy data from file-like object source_f to file-like object target_f.
    """
    try:
        source_fd = source_f.fileno()
        target_fd = target_f.fileno()
    except Exception:
        pass  # Fall through to generic code.
    else:
        try:
            # Use OS copy-on-write where available.
            if _ficlone:
                try:
                    _ficlone(source_fd, target_fd)
                    return
                except OSError as err:
                    if err.errno not in (EBADF, EOPNOTSUPP, ETXTBSY, EXDEV):
                        raise err

            # Use OS copy where available.
            if _fcopyfile:
                try:
                    _fcopyfile(source_fd, target_fd)
                    return
                except OSError as err:
                    if err.errno not in (EINVAL, ENOTSUP):
                        raise err
            if _copy_file_range:
                try:
                    _copy_file_range(source_fd, target_fd)
                    return
                except OSError as err:
                    if err.errno not in (ETXTBSY, EXDEV):
                        raise err
            if _sendfile:
                try:
                    _sendfile(source_fd, target_fd)
                    return
                except OSError as err:
                    if err.errno != ENOTSOCK:
                        raise err
        except OSError as err:
            # Produce more useful error messages.
            err.filename = source_f.name
            err.filename2 = target_f.name
            raise err

    # Last resort: copy with fileobj read() and write().
    read_source = source_f.read
    write_target = target_f.write
    while buf := read_source(1024 * 1024):
        write_target(buf)


def magic_open(path, mode='r', buffering=-1, encoding=None, errors=None,
               newline=None):
    """
    Open the file pointed to by this path and return a file object, as
    the built-in open() function does.
    """
    try:
        return io.open(path, mode, buffering, encoding, errors, newline)
    except TypeError:
        pass
    cls = type(path)
    text = 'b' not in mode
    mode = ''.join(sorted(c for c in mode if c not in 'bt'))
    if text:
        try:
            attr = getattr(cls, f'__open_{mode}__')
        except AttributeError:
            pass
        else:
            return attr(path, buffering, encoding, errors, newline)

    try:
        attr = getattr(cls, f'__open_{mode}b__')
    except AttributeError:
        pass
    else:
        stream = attr(path, buffering)
        if text:
            stream = io.TextIOWrapper(stream, encoding, errors, newline)
        return stream

    raise TypeError(f"{cls.__name__} can't be opened with mode {mode!r}")


class Copier:
    """
    Class that implements the "write" part of copying between path objects. An
    instance of this class is available from the WritablePath._copy_writer
    property.
    """

    def __init__(self, preserve_metadata, follow_symlinks, dirs_exist_ok):
        self.preserve_metadata = preserve_metadata
        self.follow_symlinks = follow_symlinks
        self.dirs_exist_ok = dirs_exist_ok

    def copy(self, source, target):
        if not self.follow_symlinks and source.info.is_symlink():
            self.copy_symlink(source, target)
        elif source.info.is_dir():
            self.copy_dir(source, target)
        else:
            self.copy_file(source, target)
        return target

    def copy_dir(self, source, target):
        """Copy the given directory to our path."""
        children = list(source.iterdir())
        target.mkdir(exist_ok=self.dirs_exist_ok)
        for src in children:
            self.copy(src, target.joinpath(src.name))
        if self.preserve_metadata:
            self.copy_metadata(source, target)

    def copy_file(self, source, target):
        """Copy the given file to our path."""
        self.ensure_different_file(source, target)
        with magic_open(source, 'rb') as source_f:
            try:
                with magic_open(target, 'wb') as target_f:
                    copyfileobj(source_f, target_f)
            except IsADirectoryError as e:
                if not target.exists():
                    # Raise a less confusing exception.
                    raise FileNotFoundError(
                        f'Directory does not exist: {target}') from e
                raise
        if self.preserve_metadata:
            self.copy_metadata(source, target)

    def copy_symlink(self, source, target):
        """Copy the given symbolic link to our path."""
        target.symlink_to(source.readlink())
        if self.preserve_metadata:
            self.copy_symlink_metadata(source, target)

    def copy_metadata(self, source, target):
        pass

    def copy_symlink_metadata(self, source, target):
        pass

    @classmethod
    def ensure_different_file(cls, source, target):
        """
        Raise OSError(EINVAL) if both paths refer to the same file.
        """
        pass

    @classmethod
    def ensure_distinct_path(cls, source, target):
        """
        Raise OSError(EINVAL) if the other path is within this path.
        """
        # Note: there is no straightforward, foolproof algorithm to determine
        # if one directory is within another (a particularly perverse example
        # would be a single network share mounted in one location via NFS, and
        # in another location via CIFS), so we simply checks whether the
        # other path is lexically equal to, or within, this path.
        if source == target:
            err = OSError(EINVAL, "Source and target are the same path")
        elif source in target.parents:
            err = OSError(EINVAL, "Source path is a parent of target path")
        else:
            return
        err.filename = str(source)
        err.filename2 = str(target)
        raise err


class LocalCopier(Copier):
    """This object implements the "write" part of copying local paths. Don't
    try to construct it yourself.
    """

    if copyfile:
        # Use fast OS routine for local file copying where available.
        def copy_file(self, source, target):
            """Copy the given file to the given target."""
            try:
                source = os.fspath(source)
            except TypeError:
                super().copy_file(source, target)
            else:
                copyfile(source, os.fspath(target))

    if os.name == 'nt':
        # Windows: symlink target might not exist yet if we're copying several
        # files, so ensure we pass is_dir to os.symlink().
        def copy_symlink(self, source, target):
            """Copy the given symlink to the given target."""
            target.symlink_to(source.readlink(), source.info.is_dir())
            if self.preserve_metadata:
                self.copy_symlink_metadata(source, target)

    def copy_metadata(self, source, target):
        info = source.info
        st = info._stat() if hasattr(info, '_stat') else None
        copy_times = hasattr(st, 'st_atime_ns') and hasattr(st, 'st_mtime_ns')
        copy_xattrs = hasattr(info, '_xattrs') and hasattr(os, 'setxattr')
        copy_mode = hasattr(st, 'st_mode')
        copy_flags = hasattr(st, 'st_flags') and hasattr(os, 'chflags')

        if copy_times:
            os.utime(target, ns=(st.st_atime_ns, st.st_mtime_ns))
        if copy_xattrs:
            for attr, value in info._xattrs():
                try:
                    os.setxattr(target, attr, value)
                except OSError as e:
                    if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
        if copy_mode:
            os.chmod(target, S_IMODE(st.st_mode))
        if copy_flags:
            try:
                os.chflags(target, st.st_flags)
            except OSError as why:
                if why.errno not in (EOPNOTSUPP, ENOTSUP):
                    raise

    def copy_symlink_metadata(self, source, target):
        info = source.info
        st = info._stat(follow_symlinks=False) if hasattr(info, '_stat') else None
        copy_times = (hasattr(st, 'st_atime_ns') and hasattr(st, 'st_mtime_ns') and
                      os.utime in os.supports_follow_symlinks)
        copy_xattrs = (hasattr(info, '_xattrs') and hasattr(os, 'setxattr') and
                       os.setxattr in os.supports_follow_symlinks)
        copy_mode = hasattr(st, 'st_mode') and hasattr(os, 'lchmod')
        copy_flags = (hasattr(st, 'st_flags') and hasattr(os, 'chflags') and
                      os.chflags in os.supports_follow_symlinks)

        if copy_times:
            os.utime(target, ns=(st.st_atime_ns, st.st_mtime_ns), follow_symlinks=False)
        if copy_xattrs:
            for attr, value in info._xattrs(follow_symlinks=False):
                try:
                    os.setxattr(target, attr, value, follow_symlinks=False)
                except OSError as e:
                    if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
        if copy_mode:
            try:
                os.lchmod(target, S_IMODE(st.st_mode))
            except NotImplementedError:
                pass
        if copy_flags:
            try:
                os.chflags(target, st.st_flags, follow_symlinks=False)
            except OSError as why:
                if why.errno not in (EOPNOTSUPP, ENOTSUP):
                    raise

    @classmethod
    def ensure_different_file(cls, source, target):
        """
        Raise OSError(EINVAL) if both paths refer to the same file.
        """
        try:
            if not target.samefile(source):
                return
        except (OSError, ValueError):
            return
        err = OSError(EINVAL, "Source and target are the same file")
        err.filename = str(source)
        err.filename2 = str(target)
        raise err


class PathInfo:
    __slots__ = ('_path', '_stat_cache', '_xattrs_cache',
                 '_exists', '_is_dir', '_is_file', '_is_symlink')

    def __init__(self, path):
        self._path = str(path)
        self._stat_cache = [None, None]
        self._xattrs_cache = [None, None]

    def __repr__(self):
        path_type = "WindowsPath" if os.name == "nt" else "PosixPath"
        return f"<{path_type}.info>"

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        if not follow_symlinks and self.is_symlink():
            return True
        try:
            return self._exists
        except AttributeError:
            if os.path.exists(self._path):
                self._exists = True
                return True
            else:
                self._exists = self._is_dir = self._is_file = False
                return False

    def is_dir(self, *, follow_symlinks=True):
        """Whether this path is a directory."""
        if not follow_symlinks and self.is_symlink():
            return False
        try:
            return self._is_dir
        except AttributeError:
            if os.path.isdir(self._path):
                self._is_dir = self._exists = True
                return True
            else:
                self._is_dir = False
                return False

    def is_file(self, *, follow_symlinks=True):
        """Whether this path is a regular file."""
        if not follow_symlinks and self.is_symlink():
            return False
        try:
            return self._is_file
        except AttributeError:
            if os.path.isfile(self._path):
                self._is_file = self._exists = True
                return True
            else:
                self._is_file = False
                return False

    def is_symlink(self):
        """Whether this path is a symbolic link."""
        try:
            return self._is_symlink
        except AttributeError:
            self._is_symlink = os.path.islink(self._path)
            return self._is_symlink

    def _stat(self, *, follow_symlinks=True):
        idx = bool(follow_symlinks)
        st = self._stat_cache[idx]
        if st is None:
            st = os.stat(self._path, follow_symlinks=follow_symlinks)
            if follow_symlinks or S_ISLNK(st.st_mode):
                self._stat_cache[idx] = st
            else:
                # Not a symlink, so stat() will give the same result
                self._stat_cache = [st, st]
        return st

    if hasattr(os, 'listxattr'):
        def _xattrs(self, *, follow_symlinks=True):
            idx = bool(follow_symlinks)
            xattrs = self._xattrs_cache[idx]
            if xattrs is None:
                try:
                    xattrs = [
                        (attr, os.getxattr(self._path, attr, follow_symlinks=follow_symlinks))
                        for attr in os.listxattr(self._path, follow_symlinks=follow_symlinks)]
                except OSError as err:
                    if err.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
                    xattrs = []
                self._xattrs_cache[idx] = xattrs
            return xattrs


class DirEntryInfo(PathInfo):
    """Implementation of pathlib.types.PathInfo that provides status
    information by querying a wrapped os.DirEntry object. Don't try to
    construct it yourself."""
    __slots__ = ('_entry',)

    def __init__(self, entry):
        super().__init__(entry.path)
        self._entry = entry

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        if not follow_symlinks:
            return True
        try:
            return self._exists
        except AttributeError:
            try:
                self._entry.stat()
            except OSError:
                self._exists = False
            else:
                self._exists = True
            return self._exists

    def is_dir(self, *, follow_symlinks=True):
        """Whether this path is a directory."""
        try:
            return self._entry.is_dir(follow_symlinks=follow_symlinks)
        except OSError:
            return False

    def is_file(self, *, follow_symlinks=True):
        """Whether this path is a regular file."""
        try:
            return self._entry.is_file(follow_symlinks=follow_symlinks)
        except OSError:
            return False

    def is_symlink(self):
        """Whether this path is a symbolic link."""
        try:
            return self._entry.is_symlink()
        except OSError:
            return False

    def _stat(self, *, follow_symlinks=True):
        return self._entry.stat(follow_symlinks=follow_symlinks)

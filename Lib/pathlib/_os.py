"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import *
from stat import S_ISDIR, S_ISREG, S_ISLNK, S_IMODE
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


class CopyWriter:
    """
    Class that implements the "write" part of copying between path objects. An
    instance of this class is available from the WritablePath._copy_writer
    property.
    """
    __slots__ = ('_path',)

    def __init__(self, path):
        self._path = path

    def _create(self, source, follow_symlinks, dirs_exist_ok, preserve_metadata):
        self._ensure_distinct_path(source)
        if not follow_symlinks and source.is_symlink():
            self._create_symlink(source, preserve_metadata)
        elif source.is_dir():
            self._create_dir(source, follow_symlinks, dirs_exist_ok, preserve_metadata)
        else:
            self._create_file(source, preserve_metadata)
        return self._path

    def _create_dir(self, source, follow_symlinks, dirs_exist_ok, preserve_metadata):
        """Copy the given directory to our path."""
        children = list(source.iterdir())
        self._path.mkdir(exist_ok=dirs_exist_ok)
        for src in children:
            dst = self._path.joinpath(src.name)
            if not follow_symlinks and src.is_symlink():
                dst._copy_writer._create_symlink(src, preserve_metadata)
            elif src.is_dir():
                dst._copy_writer._create_dir(src, follow_symlinks, dirs_exist_ok, preserve_metadata)
            else:
                dst._copy_writer._create_file(src, preserve_metadata)

        if preserve_metadata:
            self._create_metadata(source)

    def _create_file(self, source, preserve_metadata):
        """Copy the given file to our path."""
        self._ensure_different_file(source)
        with magic_open(source, 'rb') as source_f:
            try:
                with magic_open(self._path, 'wb') as target_f:
                    copyfileobj(source_f, target_f)
            except IsADirectoryError as e:
                if not self._path.exists():
                    # Raise a less confusing exception.
                    raise FileNotFoundError(
                        f'Directory does not exist: {self._path}') from e
                raise
        if preserve_metadata:
            self._create_metadata(source)

    def _create_symlink(self, source, preserve_metadata):
        """Copy the given symbolic link to our path."""
        self._path.symlink_to(source.readlink())
        if preserve_metadata:
            self._create_symlink_metadata(source)

    def _create_metadata(self, source):
        """Copy metadata from the given path to our path."""
        pass

    def _create_symlink_metadata(self, source):
        """Copy metadata from the given symlink to our symlink."""
        pass

    def _ensure_different_file(self, source):
        """
        Raise OSError(EINVAL) if both paths refer to the same file.
        """
        pass

    def _ensure_distinct_path(self, source):
        """
        Raise OSError(EINVAL) if the other path is within this path.
        """
        # Note: there is no straightforward, foolproof algorithm to determine
        # if one directory is within another (a particularly perverse example
        # would be a single network share mounted in one location via NFS, and
        # in another location via CIFS), so we simply checks whether the
        # other path is lexically equal to, or within, this path.
        if source == self._path:
            err = OSError(EINVAL, "Source and target are the same path")
        elif source in self._path.parents:
            err = OSError(EINVAL, "Source path is a parent of target path")
        else:
            return
        err.filename = str(source)
        err.filename2 = str(self._path)
        raise err


class LocalCopyWriter(CopyWriter):
    """This object implements the "write" part of copying local paths. Don't
    try to construct it yourself.
    """
    __slots__ = ()

    if copyfile:
        # Use fast OS routine for local file copying where available.
        def _create_file(self, source, preserve_metadata):
            """Copy the given file to the given target."""
            try:
                source = os.fspath(source)
            except TypeError:
                super()._create_file(source, preserve_metadata)
            else:
                copyfile(source, os.fspath(self._path))

    if os.name == 'nt':
        # Windows: symlink target might not exist yet if we're copying several
        # files, so ensure we pass is_dir to os.symlink().
        def _create_symlink(self, source, preserve_metadata):
            """Copy the given symlink to the given target."""
            self._path.symlink_to(source.readlink(), source.is_dir())
            if preserve_metadata:
                self._create_symlink_metadata(source)

    def _create_metadata(self, source):
        """Copy metadata from the given path to our path."""
        target = self._path
        info = source.info
        copy_times_ns = hasattr(info, '_get_atime_ns') and hasattr(info, '_get_mtime_ns')
        copy_xattrs = hasattr(info, '_get_xattrs') and hasattr(os, 'setxattr')
        copy_mode = hasattr(info, '_get_mode')
        copy_flags = hasattr(info, '_get_flags') and hasattr(os, 'chflags')

        if copy_times_ns:
            atime_ns = info._get_atime_ns()
            mtime_ns = info._get_mtime_ns()
            if atime_ns and mtime_ns:
                os.utime(target, ns=(atime_ns, mtime_ns))
        if copy_xattrs:
            xattrs = info._get_xattrs()
            for attr, value in xattrs:
                try:
                    os.setxattr(target, attr, value)
                except OSError as e:
                    if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
        if copy_mode:
            mode = info._get_mode()
            if mode:
                os.chmod(target, S_IMODE(mode))
        if copy_flags:
            flags = info._get_flags()
            if flags:
                try:
                    os.chflags(target, flags)
                except OSError as why:
                    if why.errno not in (EOPNOTSUPP, ENOTSUP):
                        raise

    def _create_symlink_metadata(self, source):
        """Copy metadata from the given symlink to our symlink."""
        target = self._path
        info = source.info
        copy_times_ns = (hasattr(info, '_get_atime_ns') and
                         hasattr(info, '_get_mtime_ns') and
                         os.utime in os.supports_follow_symlinks)
        copy_xattrs = (hasattr(info, '_get_xattrs') and
                       hasattr(os, 'setxattr') and
                       os.setxattr in os.supports_fd)
        copy_mode = (hasattr(info, '_get_mode') and
                     hasattr(os, 'lchmod'))
        copy_flags = (hasattr(info, '_get_flags') and
                      hasattr(os, 'chflags') and
                      os.chflags in os.supports_follow_symlinks)

        if copy_times_ns:
            atime_ns = info._get_atime_ns(follow_symlinks=False)
            mtime_ns = info._get_mtime_ns(follow_symlinks=False)
            if atime_ns and mtime_ns:
                os.utime(target, ns=(atime_ns, mtime_ns), follow_symlinks=False)
        if copy_xattrs:
            xattrs = info._get_xattrs(follow_symlinks=False)
            for attr, value in xattrs:
                try:
                    os.setxattr(target, attr, value, follow_symlinks=False)
                except OSError as e:
                    if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
        if copy_mode:
            mode = info._get_mode(follow_symlinks=False)
            if mode:
                try:
                    os.lchmod(target, S_IMODE(mode))
                except NotImplementedError:
                    pass
        if copy_flags:
            flags = info._get_flags(follow_symlinks=False)
            if flags:
                try:
                    os.chflags(target, flags, follow_symlinks=False)
                except OSError as why:
                    if why.errno not in (EOPNOTSUPP, ENOTSUP):
                        raise

    def _ensure_different_file(self, source):
        """
        Raise OSError(EINVAL) if both paths refer to the same file.
        """
        try:
            if not self._path.samefile(source):
                return
        except (OSError, ValueError):
            return
        err = OSError(EINVAL, "Source and target are the same file")
        err.filename = str(source)
        err.filename2 = str(self._path)
        raise err


class _PathInfoBase:
    __slots__ = ('_path', '_stat_result', '_lstat_result')

    def __init__(self, path):
        self._path = str(path)

    def __repr__(self):
        path_type = "WindowsPath" if os.name == "nt" else "PosixPath"
        return f"<{path_type}.info>"

    def _raw_stat(self, *, follow_symlinks=True):
        return os.stat(self._path, follow_symlinks=follow_symlinks)

    def _stat(self, *, follow_symlinks=True):
        """Return the status as an os.stat_result, or None if stat() fails."""
        if follow_symlinks:
            try:
                return self._stat_result
            except AttributeError:
                try:
                    self._stat_result = self._raw_stat(follow_symlinks=True)
                except (OSError, ValueError):
                    self._stat_result = None
                return self._stat_result
        else:
            try:
                return self._lstat_result
            except AttributeError:
                try:
                    self._lstat_result = self._raw_stat(follow_symlinks=False)
                except (OSError, ValueError):
                    self._lstat_result = None
                return self._lstat_result

    def _get_mode(self, *, follow_symlinks=True):
        """Return the POSIX file mode, or zero if stat() fails."""
        st = self._stat(follow_symlinks=follow_symlinks)
        if st is None:
            return 0
        return st.st_mode

    def _get_atime_ns(self, *, follow_symlinks=True):
        """Return the access time in nanoseconds, or zero if stat() fails."""
        st = self._stat(follow_symlinks=follow_symlinks)
        if st is None:
            return 0
        return st.st_atime_ns

    def _get_mtime_ns(self, *, follow_symlinks=True):
        """Return the modify time in nanoseconds, or zero if stat() fails."""
        st = self._stat(follow_symlinks=follow_symlinks)
        if st is None:
            return 0
        return st.st_mtime_ns

    if hasattr(os.stat_result, 'st_flags'):
        def _get_flags(self, *, follow_symlinks=True):
            """Return the flags, or zero if stat() fails."""
            st = self._stat(follow_symlinks=follow_symlinks)
            if st is None:
                return 0
            return st.st_flags

    if hasattr(os, 'listxattr'):
        def _get_xattrs(self, *, follow_symlinks=True):
            """Return the xattrs as a list of (attr, value) pairs, or an empty
            list if extended attributes aren't supported."""
            try:
                return [
                    (attr, os.getxattr(self._path, attr, follow_symlinks=follow_symlinks))
                    for attr in os.listxattr(self._path, follow_symlinks=follow_symlinks)]
            except OSError as err:
                if err.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                    raise
                return []


class _WindowsPathInfo(_PathInfoBase):
    """Implementation of pathlib.types.PathInfo that provides status
    information for Windows paths. Don't try to construct it yourself."""
    __slots__ = ('_exists', '_is_dir', '_is_file', '_is_symlink')

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


class _PosixPathInfo(_PathInfoBase):
    __slots__ = ()

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        return self._get_mode(follow_symlinks=follow_symlinks) > 0

    def is_dir(self, *, follow_symlinks=True):
        """Whether this path is a directory."""
        return S_ISDIR(self._get_mode(follow_symlinks=follow_symlinks))

    def is_file(self, *, follow_symlinks=True):
        """Whether this path is a regular file."""
        return S_ISREG(self._get_mode(follow_symlinks=follow_symlinks))

    def is_symlink(self):
        """Whether this path is a symbolic link."""
        return S_ISLNK(self._get_mode(follow_symlinks=False))


PathInfo = _WindowsPathInfo if os.name == 'nt' else _PosixPathInfo


class DirEntryInfo(_PosixPathInfo):
    """Implementation of pathlib.types.PathInfo that provides status
    information by querying a wrapped os.DirEntry object. Don't try to
    construct it yourself."""
    __slots__ = ('_entry', '_exists')

    def __init__(self, entry):
        super().__init__(entry.path)
        self._entry = entry

    def _raw_stat(self, *, follow_symlinks=True):
        return self._entry.stat(follow_symlinks=follow_symlinks)

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        if not follow_symlinks:
            return True
        return super().exists()

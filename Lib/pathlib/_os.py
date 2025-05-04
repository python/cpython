"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import *
from io import TextIOWrapper, text_encoding
from stat import S_ISDIR, S_ISREG, S_ISLNK, S_IMODE
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
    def copyfile2(source, target):
        """
        Copy from one file to another using CopyFile2 (Windows only).
        """
        _winapi.CopyFile2(source, target, 0)
else:
    copyfile2 = None


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
    text = 'b' not in mode
    if text:
        # Call io.text_encoding() here to ensure any warning is raised at an
        # appropriate stack level.
        encoding = text_encoding(encoding)
    try:
        return open(path, mode, buffering, encoding, errors, newline)
    except TypeError:
        pass
    cls = type(path)
    mode = ''.join(sorted(c for c in mode if c not in 'bt'))
    if text:
        try:
            attr = getattr(cls, f'__open_{mode}__')
        except AttributeError:
            pass
        else:
            return attr(path, buffering, encoding, errors, newline)
    elif encoding is not None:
        raise ValueError("binary mode doesn't take an encoding argument")
    elif errors is not None:
        raise ValueError("binary mode doesn't take an errors argument")
    elif newline is not None:
        raise ValueError("binary mode doesn't take a newline argument")

    try:
        attr = getattr(cls, f'__open_{mode}b__')
    except AttributeError:
        pass
    else:
        stream = attr(path, buffering)
        if text:
            stream = TextIOWrapper(stream, encoding, errors, newline)
        return stream

    raise TypeError(f"{cls.__name__} can't be opened with mode {mode!r}")


def ensure_distinct_paths(source, target):
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


def ensure_different_files(source, target):
    """
    Raise OSError(EINVAL) if both paths refer to the same file.
    """
    try:
        source_file_id = source.info._file_id
        target_file_id = target.info._file_id
    except AttributeError:
        if source != target:
            return
    else:
        try:
            if source_file_id() != target_file_id():
                return
        except (OSError, ValueError):
            return
    err = OSError(EINVAL, "Source and target are the same file")
    err.filename = str(source)
    err.filename2 = str(target)
    raise err


def copy_info(info, target, follow_symlinks=True):
    """Copy metadata from the given PathInfo to the given local path."""
    copy_times_ns = (
        hasattr(info, '_access_time_ns') and
        hasattr(info, '_mod_time_ns') and
        (follow_symlinks or os.utime in os.supports_follow_symlinks))
    if copy_times_ns:
        t0 = info._access_time_ns(follow_symlinks=follow_symlinks)
        t1 = info._mod_time_ns(follow_symlinks=follow_symlinks)
        os.utime(target, ns=(t0, t1), follow_symlinks=follow_symlinks)

    # We must copy extended attributes before the file is (potentially)
    # chmod()'ed read-only, otherwise setxattr() will error with -EACCES.
    copy_xattrs = (
        hasattr(info, '_xattrs') and
        hasattr(os, 'setxattr') and
        (follow_symlinks or os.setxattr in os.supports_follow_symlinks))
    if copy_xattrs:
        xattrs = info._xattrs(follow_symlinks=follow_symlinks)
        for attr, value in xattrs:
            try:
                os.setxattr(target, attr, value, follow_symlinks=follow_symlinks)
            except OSError as e:
                if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                    raise

    copy_posix_permissions = (
        hasattr(info, '_posix_permissions') and
        (follow_symlinks or os.chmod in os.supports_follow_symlinks))
    if copy_posix_permissions:
        posix_permissions = info._posix_permissions(follow_symlinks=follow_symlinks)
        try:
            os.chmod(target, posix_permissions, follow_symlinks=follow_symlinks)
        except NotImplementedError:
            # if we got a NotImplementedError, it's because
            #   * follow_symlinks=False,
            #   * lchown() is unavailable, and
            #   * either
            #       * fchownat() is unavailable or
            #       * fchownat() doesn't implement AT_SYMLINK_NOFOLLOW.
            #         (it returned ENOSUP.)
            # therefore we're out of options--we simply cannot chown the
            # symlink.  give up, suppress the error.
            # (which is what shutil always did in this circumstance.)
            pass

    copy_bsd_flags = (
        hasattr(info, '_bsd_flags') and
        hasattr(os, 'chflags') and
        (follow_symlinks or os.chflags in os.supports_follow_symlinks))
    if copy_bsd_flags:
        bsd_flags = info._bsd_flags(follow_symlinks=follow_symlinks)
        try:
            os.chflags(target, bsd_flags, follow_symlinks=follow_symlinks)
        except OSError as why:
            if why.errno not in (EOPNOTSUPP, ENOTSUP):
                raise


class _PathInfoBase:
    __slots__ = ('_path', '_stat_result', '_lstat_result')

    def __init__(self, path):
        self._path = str(path)

    def __repr__(self):
        path_type = "WindowsPath" if os.name == "nt" else "PosixPath"
        return f"<{path_type}.info>"

    def _stat(self, *, follow_symlinks=True, ignore_errors=False):
        """Return the status as an os.stat_result, or None if stat() fails and
        ignore_errors is true."""
        if follow_symlinks:
            try:
                result = self._stat_result
            except AttributeError:
                pass
            else:
                if ignore_errors or result is not None:
                    return result
            try:
                self._stat_result = os.stat(self._path)
            except (OSError, ValueError):
                self._stat_result = None
                if not ignore_errors:
                    raise
            return self._stat_result
        else:
            try:
                result = self._lstat_result
            except AttributeError:
                pass
            else:
                if ignore_errors or result is not None:
                    return result
            try:
                self._lstat_result = os.lstat(self._path)
            except (OSError, ValueError):
                self._lstat_result = None
                if not ignore_errors:
                    raise
            return self._lstat_result

    def _posix_permissions(self, *, follow_symlinks=True):
        """Return the POSIX file permissions."""
        return S_IMODE(self._stat(follow_symlinks=follow_symlinks).st_mode)

    def _file_id(self, *, follow_symlinks=True):
        """Returns the identifier of the file."""
        st = self._stat(follow_symlinks=follow_symlinks)
        return st.st_dev, st.st_ino

    def _access_time_ns(self, *, follow_symlinks=True):
        """Return the access time in nanoseconds."""
        return self._stat(follow_symlinks=follow_symlinks).st_atime_ns

    def _mod_time_ns(self, *, follow_symlinks=True):
        """Return the modify time in nanoseconds."""
        return self._stat(follow_symlinks=follow_symlinks).st_mtime_ns

    if hasattr(os.stat_result, 'st_flags'):
        def _bsd_flags(self, *, follow_symlinks=True):
            """Return the flags."""
            return self._stat(follow_symlinks=follow_symlinks).st_flags

    if hasattr(os, 'listxattr'):
        def _xattrs(self, *, follow_symlinks=True):
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
    """Implementation of pathlib.types.PathInfo that provides status
    information for POSIX paths. Don't try to construct it yourself."""
    __slots__ = ()

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        st = self._stat(follow_symlinks=follow_symlinks, ignore_errors=True)
        if st is None:
            return False
        return True

    def is_dir(self, *, follow_symlinks=True):
        """Whether this path is a directory."""
        st = self._stat(follow_symlinks=follow_symlinks, ignore_errors=True)
        if st is None:
            return False
        return S_ISDIR(st.st_mode)

    def is_file(self, *, follow_symlinks=True):
        """Whether this path is a regular file."""
        st = self._stat(follow_symlinks=follow_symlinks, ignore_errors=True)
        if st is None:
            return False
        return S_ISREG(st.st_mode)

    def is_symlink(self):
        """Whether this path is a symbolic link."""
        st = self._stat(follow_symlinks=False, ignore_errors=True)
        if st is None:
            return False
        return S_ISLNK(st.st_mode)


PathInfo = _WindowsPathInfo if os.name == 'nt' else _PosixPathInfo


class DirEntryInfo(_PathInfoBase):
    """Implementation of pathlib.types.PathInfo that provides status
    information by querying a wrapped os.DirEntry object. Don't try to
    construct it yourself."""
    __slots__ = ('_entry',)

    def __init__(self, entry):
        super().__init__(entry.path)
        self._entry = entry

    def _stat(self, *, follow_symlinks=True, ignore_errors=False):
        try:
            return self._entry.stat(follow_symlinks=follow_symlinks)
        except OSError:
            if not ignore_errors:
                raise
            return None

    def exists(self, *, follow_symlinks=True):
        """Whether this path exists."""
        if not follow_symlinks:
            return True
        return self._stat(ignore_errors=True) is not None

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

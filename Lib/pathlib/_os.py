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


class CopyReader:
    """
    Class that implements the "read" part of copying between path objects.
    An instance of this class is available from the ReadablePath._copy_reader
    property.
    """
    __slots__ = ('_path',)

    def __init__(self, path):
        self._path = path

    _readable_metakeys = frozenset()

    def _read_metadata(self, metakeys, *, follow_symlinks=True):
        """
        Returns path metadata as a dict with string keys.
        """
        raise NotImplementedError


class CopyWriter:
    """
    Class that implements the "write" part of copying between path objects. An
    instance of this class is available from the WritablePath._copy_writer
    property.
    """
    __slots__ = ('_path',)

    def __init__(self, path):
        self._path = path

    _writable_metakeys = frozenset()

    def _write_metadata(self, metadata, *, follow_symlinks=True):
        """
        Sets path metadata from the given dict with string keys.
        """
        raise NotImplementedError

    def _create(self, source, follow_symlinks, dirs_exist_ok, preserve_metadata):
        self._ensure_distinct_path(source)
        if preserve_metadata:
            metakeys = self._writable_metakeys & source._copy_reader._readable_metakeys
        else:
            metakeys = None
        if not follow_symlinks and source.is_symlink():
            self._create_symlink(source, metakeys)
        elif source.is_dir():
            self._create_dir(source, metakeys, follow_symlinks, dirs_exist_ok)
        else:
            self._create_file(source, metakeys)
        return self._path

    def _create_dir(self, source, metakeys, follow_symlinks, dirs_exist_ok):
        """Copy the given directory to our path."""
        children = list(source.iterdir())
        self._path.mkdir(exist_ok=dirs_exist_ok)
        for src in children:
            dst = self._path.joinpath(src.name)
            if not follow_symlinks and src.is_symlink():
                dst._copy_writer._create_symlink(src, metakeys)
            elif src.is_dir():
                dst._copy_writer._create_dir(src, metakeys, follow_symlinks, dirs_exist_ok)
            else:
                dst._copy_writer._create_file(src, metakeys)
        if metakeys:
            metadata = source._copy_reader._read_metadata(metakeys)
            if metadata:
                self._write_metadata(metadata)

    def _create_file(self, source, metakeys):
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
        if metakeys:
            metadata = source._copy_reader._read_metadata(metakeys)
            if metadata:
                self._write_metadata(metadata)

    def _create_symlink(self, source, metakeys):
        """Copy the given symbolic link to our path."""
        self._path.symlink_to(source.readlink())
        if metakeys:
            metadata = source._copy_reader._read_metadata(metakeys, follow_symlinks=False)
            if metadata:
                self._write_metadata(metadata, follow_symlinks=False)

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


class LocalCopyReader(CopyReader):
    """This object implements the "read" part of copying local paths. Don't
    try to construct it yourself.
    """
    __slots__ = ()

    _readable_metakeys = {'mode', 'times_ns'}
    if hasattr(os.stat_result, 'st_flags'):
        _readable_metakeys.add('flags')
    if hasattr(os, 'listxattr'):
        _readable_metakeys.add('xattrs')
    _readable_metakeys = frozenset(_readable_metakeys)

    def _read_metadata(self, metakeys, *, follow_symlinks=True):
        metadata = {}
        if 'mode' in metakeys or 'times_ns' in metakeys or 'flags' in metakeys:
            st = self._path.stat(follow_symlinks=follow_symlinks)
            if 'mode' in metakeys:
                metadata['mode'] = S_IMODE(st.st_mode)
            if 'times_ns' in metakeys:
                metadata['times_ns'] = st.st_atime_ns, st.st_mtime_ns
            if 'flags' in metakeys:
                metadata['flags'] = st.st_flags
        if 'xattrs' in metakeys:
            try:
                metadata['xattrs'] = [
                    (attr, os.getxattr(self._path, attr, follow_symlinks=follow_symlinks))
                    for attr in os.listxattr(self._path, follow_symlinks=follow_symlinks)]
            except OSError as err:
                if err.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                    raise
        return metadata


class LocalCopyWriter(CopyWriter):
    """This object implements the "write" part of copying local paths. Don't
    try to construct it yourself.
    """
    __slots__ = ()

    _writable_metakeys = LocalCopyReader._readable_metakeys

    def _write_metadata(self, metadata, *, follow_symlinks=True):
        def _nop(*args, ns=None, follow_symlinks=None):
            pass

        if follow_symlinks:
            # use the real function if it exists
            def lookup(name):
                return getattr(os, name, _nop)
        else:
            # use the real function only if it exists
            # *and* it supports follow_symlinks
            def lookup(name):
                fn = getattr(os, name, _nop)
                if fn in os.supports_follow_symlinks:
                    return fn
                return _nop

        times_ns = metadata.get('times_ns')
        if times_ns is not None:
            lookup("utime")(self._path, ns=times_ns, follow_symlinks=follow_symlinks)
        # We must copy extended attributes before the file is (potentially)
        # chmod()'ed read-only, otherwise setxattr() will error with -EACCES.
        xattrs = metadata.get('xattrs')
        if xattrs is not None:
            for attr, value in xattrs:
                try:
                    os.setxattr(self._path, attr, value, follow_symlinks=follow_symlinks)
                except OSError as e:
                    if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                        raise
        mode = metadata.get('mode')
        if mode is not None:
            try:
                lookup("chmod")(self._path, mode, follow_symlinks=follow_symlinks)
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
        flags = metadata.get('flags')
        if flags is not None:
            try:
                lookup("chflags")(self._path, flags, follow_symlinks=follow_symlinks)
            except OSError as why:
                if why.errno not in (EOPNOTSUPP, ENOTSUP):
                    raise

    if copyfile:
        # Use fast OS routine for local file copying where available.
        def _create_file(self, source, metakeys):
            """Copy the given file to the given target."""
            try:
                source = os.fspath(source)
            except TypeError:
                super()._create_file(source, metakeys)
            else:
                copyfile(source, os.fspath(self._path))

    if os.name == 'nt':
        # Windows: symlink target might not exist yet if we're copying several
        # files, so ensure we pass is_dir to os.symlink().
        def _create_symlink(self, source, metakeys):
            """Copy the given symlink to the given target."""
            self._path.symlink_to(source.readlink(), source.is_dir())
            if metakeys:
                metadata = source._copy_reader._read_metadata(metakeys, follow_symlinks=False)
                if metadata:
                    self._write_metadata(metadata, follow_symlinks=False)

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
    __slots__ = ()

    def __repr__(self):
        path_type = "WindowsPath" if os.name == "nt" else "PosixPath"
        return f"<{path_type}.info>"


class _WindowsPathInfo(_PathInfoBase):
    """Implementation of pathlib.types.PathInfo that provides status
    information for Windows paths. Don't try to construct it yourself."""
    __slots__ = ('_path', '_exists', '_is_dir', '_is_file', '_is_symlink')

    def __init__(self, path):
        self._path = str(path)

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
    __slots__ = ('_path', '_mode')

    def __init__(self, path):
        self._path = str(path)
        self._mode = [None, None]

    def _get_mode(self, *, follow_symlinks=True):
        idx = bool(follow_symlinks)
        mode = self._mode[idx]
        if mode is None:
            try:
                st = os.stat(self._path, follow_symlinks=follow_symlinks)
            except (OSError, ValueError):
                mode = 0
            else:
                mode = st.st_mode
            if follow_symlinks or S_ISLNK(mode):
                self._mode[idx] = mode
            else:
                # Not a symlink, so stat() will give the same result
                self._mode = [mode, mode]
        return mode

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


class DirEntryInfo(_PathInfoBase):
    """Implementation of pathlib.types.PathInfo that provides status
    information by querying a wrapped os.DirEntry object. Don't try to
    construct it yourself."""
    __slots__ = ('_entry', '_exists')

    def __init__(self, entry):
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

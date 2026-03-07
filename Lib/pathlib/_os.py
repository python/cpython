"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import *
from io import TextIOWrapper, text_encoding
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


def _open_reader(obj):
    cls = type(obj)
    try:
        open_reader = cls.__open_reader__
    except AttributeError:
        cls_name = cls.__name__
        raise TypeError(f"{cls_name} can't be opened for reading") from None
    else:
        return open_reader(obj)


def _open_writer(obj, mode):
    cls = type(obj)
    try:
        open_writer = cls.__open_writer__
    except AttributeError:
        cls_name = cls.__name__
        raise TypeError(f"{cls_name} can't be opened for writing") from None
    else:
        return open_writer(obj, mode)


def _open_updater(obj, mode):
    cls = type(obj)
    try:
        open_updater = cls.__open_updater__
    except AttributeError:
        cls_name = cls.__name__
        raise TypeError(f"{cls_name} can't be opened for updating") from None
    else:
        return open_updater(obj, mode)


def vfsopen(obj, mode='r', buffering=-1, encoding=None, errors=None,
            newline=None):
    """
    Open the file pointed to by this path and return a file object, as
    the built-in open() function does.

    Unlike the built-in open() function, this function additionally accepts
    'openable' objects, which are objects with any of these special methods:

        __open_reader__()
        __open_writer__(mode)
        __open_updater__(mode)

    '__open_reader__' is called for 'r' mode; '__open_writer__' for 'a', 'w'
    and 'x' modes; and '__open_updater__' for 'r+' and 'w+' modes. If text
    mode is requested, the result is wrapped in an io.TextIOWrapper object.
    """
    if buffering != -1:
        raise ValueError("buffer size can't be customized")
    text = 'b' not in mode
    if text:
        # Call io.text_encoding() here to ensure any warning is raised at an
        # appropriate stack level.
        encoding = text_encoding(encoding)
    try:
        return open(obj, mode, buffering, encoding, errors, newline)
    except TypeError:
        pass
    if not text:
        if encoding is not None:
            raise ValueError("binary mode doesn't take an encoding argument")
        if errors is not None:
            raise ValueError("binary mode doesn't take an errors argument")
        if newline is not None:
            raise ValueError("binary mode doesn't take a newline argument")
    mode = ''.join(sorted(c for c in mode if c not in 'bt'))
    if mode == 'r':
        stream = _open_reader(obj)
    elif mode in ('a', 'w', 'x'):
        stream = _open_writer(obj, mode)
    elif mode in ('+r', '+w'):
        stream = _open_updater(obj, mode[1])
    else:
        raise ValueError(f'invalid mode: {mode}')
    if text:
        stream = TextIOWrapper(stream, encoding, errors, newline)
    return stream


def vfspath(obj):
    """
    Return the string representation of a virtual path object.
    """
    cls = type(obj)
    try:
        vfspath_method = cls.__vfspath__
    except AttributeError:
        cls_name = cls.__name__
        raise TypeError(f"expected JoinablePath object, not {cls_name}") from None
    else:
        return vfspath_method(obj)


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
    err.filename = vfspath(source)
    err.filename2 = vfspath(target)
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
    err.filename = vfspath(source)
    err.filename2 = vfspath(target)
    raise err

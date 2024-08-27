"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import *
import os
import stat
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


# Kinds of metadata supported by the operating system.
file_metadata_keys = {'mode', 'times_ns'}
if hasattr(os.stat_result, 'st_flags'):
    file_metadata_keys.add('flags')
if hasattr(os, 'listxattr'):
    file_metadata_keys.add('xattrs')
file_metadata_keys = frozenset(file_metadata_keys)


def read_file_metadata(path, keys=None, *, follow_symlinks=True):
    """
    Returns local path metadata as a dict with string keys.
    """
    if keys is None:
        keys = file_metadata_keys
    assert keys.issubset(file_metadata_keys)
    result = {}
    for key in keys:
        if key == 'xattrs':
            try:
                result['xattrs'] = [
                    (attr, os.getxattr(path, attr, follow_symlinks=follow_symlinks))
                    for attr in os.listxattr(path, follow_symlinks=follow_symlinks)]
            except OSError as err:
                if err.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                    raise
            continue
        st = os.stat(path, follow_symlinks=follow_symlinks)
        if key == 'mode':
            result['mode'] = stat.S_IMODE(st.st_mode)
        elif key == 'times_ns':
            result['times_ns'] = st.st_atime_ns, st.st_mtime_ns
        elif key == 'flags':
            result['flags'] = st.st_flags
    return result


def write_file_metadata(path, metadata, *, follow_symlinks=True):
    """
    Sets local path metadata from the given dict with string keys.
    """
    assert frozenset(metadata.keys()).issubset(file_metadata_keys)

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
        lookup("utime")(path, ns=times_ns, follow_symlinks=follow_symlinks)
    # We must copy extended attributes before the file is (potentially)
    # chmod()'ed read-only, otherwise setxattr() will error with -EACCES.
    xattrs = metadata.get('xattrs')
    if xattrs is not None:
        for attr, value in xattrs:
            try:
                os.setxattr(path, attr, value, follow_symlinks=follow_symlinks)
            except OSError as e:
                if e.errno not in (EPERM, ENOTSUP, ENODATA, EINVAL, EACCES):
                    raise
    mode = metadata.get('mode')
    if mode is not None:
        try:
            lookup("chmod")(path, mode, follow_symlinks=follow_symlinks)
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
            lookup("chflags")(path, flags, follow_symlinks=follow_symlinks)
        except OSError as why:
            if why.errno not in (EOPNOTSUPP, ENOTSUP):
                raise

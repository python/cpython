"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import EBADF, EOPNOTSUPP, ETXTBSY, EXDEV
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


def get_copy_blocksize(infd):
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
    def clonefd(source_fd, target_fd):
        """
        Perform a lightweight copy of two files, where the data blocks are
        copied only when modified. This is known as Copy on Write (CoW),
        instantaneous copy or reflink.
        """
        fcntl.ioctl(target_fd, fcntl.FICLONE, source_fd)
else:
    clonefd = None


if posix and hasattr(posix, '_fcopyfile'):
    def copyfd(source_fd, target_fd):
        """
        Copy a regular file content using high-performance fcopyfile(3)
        syscall (macOS).
        """
        posix._fcopyfile(source_fd, target_fd, posix._COPYFILE_DATA)
elif hasattr(os, 'copy_file_range'):
    def copyfd(source_fd, target_fd):
        """
        Copy data from one regular mmap-like fd to another by using a
        high-performance copy_file_range(2) syscall that gives filesystems
        an opportunity to implement the use of reflinks or server-side
        copy.
        This should work on Linux >= 4.5 only.
        """
        blocksize = get_copy_blocksize(source_fd)
        offset = 0
        while True:
            sent = os.copy_file_range(source_fd, target_fd, blocksize,
                                      offset_dst=offset)
            if sent == 0:
                break  # EOF
            offset += sent
elif hasattr(os, 'sendfile'):
    def copyfd(source_fd, target_fd):
        """Copy data from one regular mmap-like fd to another by using
        high-performance sendfile(2) syscall.
        This should work on Linux >= 2.6.33 only.
        """
        blocksize = get_copy_blocksize(source_fd)
        offset = 0
        while True:
            sent = os.sendfile(target_fd, source_fd, offset, blocksize)
            if sent == 0:
                break  # EOF
            offset += sent
else:
    copyfd = None


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
            if clonefd:
                try:
                    clonefd(source_fd, target_fd)
                    return
                except OSError as err:
                    if err.errno not in (EBADF, EOPNOTSUPP, ETXTBSY, EXDEV):
                        raise err

            # Use OS copy where available.
            if copyfd:
                copyfd(source_fd, target_fd)
                return
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

"""
Low-level OS functionality wrappers used by pathlib.
"""

from errno import EBADF, EOPNOTSUPP, ETXTBSY, EXDEV
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

if ({os.open, os.stat, os.unlink, os.rmdir} <= os.supports_dir_fd and
    os.scandir in os.supports_fd and os.stat in os.supports_follow_symlinks):

    def _rmtree_safe_fd_step(stack, onexc):
        # Each stack item has four elements:
        # * func: The first operation to perform: os.lstat, os.close or os.rmdir.
        #   Walking a directory starts with an os.lstat() to detect symlinks; in
        #   this case, func is updated before subsequent operations and passed to
        #   onexc() if an error occurs.
        # * dirfd: Open file descriptor, or None if we're processing the top-level
        #   directory given to rmtree() and the user didn't supply dir_fd.
        # * path: Path of file to operate upon. This is passed to onexc() if an
        #   error occurs.
        # * orig_entry: os.DirEntry, or None if we're processing the top-level
        #   directory given to rmtree(). We used the cached stat() of the entry to
        #   save a call to os.lstat() when walking subdirectories.
        func, dirfd, path, orig_entry = stack.pop()
        name = path if orig_entry is None else orig_entry.name
        try:
            if func is os.close:
                os.close(dirfd)
                return
            if func is os.rmdir:
                os.rmdir(name, dir_fd=dirfd)
                return

            # Note: To guard against symlink races, we use the standard
            # lstat()/open()/fstat() trick.
            assert func is os.lstat
            if orig_entry is None:
                orig_st = os.lstat(name, dir_fd=dirfd)
            else:
                orig_st = orig_entry.stat(follow_symlinks=False)

            func = os.open  # For error reporting.
            topfd = os.open(name, os.O_RDONLY | os.O_NONBLOCK, dir_fd=dirfd)

            func = os.path.islink  # For error reporting.
            try:
                if not os.path.samestat(orig_st, os.fstat(topfd)):
                    # Symlinks to directories are forbidden, see GH-46010.
                    raise OSError("Cannot call rmtree on a symbolic link")
                stack.append((os.rmdir, dirfd, path, orig_entry))
            finally:
                stack.append((os.close, topfd, path, orig_entry))

            func = os.scandir  # For error reporting.
            with os.scandir(topfd) as scandir_it:
                entries = list(scandir_it)
            for entry in entries:
                fullname = os.path.join(path, entry.name)
                try:
                    if entry.is_dir(follow_symlinks=False):
                        # Traverse into sub-directory.
                        stack.append((os.lstat, topfd, fullname, entry))
                        continue
                except FileNotFoundError:
                    continue
                except OSError:
                    pass
                try:
                    os.unlink(entry.name, dir_fd=topfd)
                except FileNotFoundError:
                    continue
                except OSError as err:
                    onexc(os.unlink, fullname, err)
        except FileNotFoundError as err:
            if orig_entry is None or func is os.close:
                err.filename = path
                onexc(func, path, err)
        except OSError as err:
            err.filename = path
            onexc(func, path, err)

    # Version using fd-based APIs to protect against races
    def rmtree(path, dir_fd, onexc):
        # While the unsafe rmtree works fine on bytes, the fd based does not.
        if isinstance(path, bytes):
            path = os.fsdecode(path)
        stack = [(os.lstat, dir_fd, path, None)]
        try:
            while stack:
                _rmtree_safe_fd_step(stack, onexc)
        finally:
            # Close any file descriptors still on the stack.
            while stack:
                func, fd, path, entry = stack.pop()
                if func is not os.close:
                    continue
                try:
                    os.close(fd)
                except OSError as err:
                    onexc(os.close, path, err)

    rmtree.avoids_symlink_attacks = True

else:
    if hasattr(os.stat_result, 'st_file_attributes'):
        def _rmtree_islink(st):
            return (stat.S_ISLNK(st.st_mode) or
                    (st.st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT
                     and st.st_reparse_tag == stat.IO_REPARSE_TAG_MOUNT_POINT))
    else:
        def _rmtree_islink(st):
            return stat.S_ISLNK(st.st_mode)

    # version vulnerable to race conditions
    def rmtree(path, dir_fd, onexc):
        if dir_fd is not None:
            raise NotImplementedError("dir_fd unavailable on this platform")
        try:
            st = os.lstat(path)
        except OSError as err:
            onexc(os.lstat, path, err)
            return
        try:
            if _rmtree_islink(st):
                # symlinks to directories are forbidden, see bug #1669
                raise OSError("Cannot call rmtree on a symbolic link")
        except OSError as err:
            onexc(os.path.islink, path, err)
            # can't continue even if onexc hook returns
            return

        def onerror(err):
            if not isinstance(err, FileNotFoundError):
                onexc(os.scandir, err.filename, err)

        results = os.walk(path, topdown=False, onerror=onerror,
                          followlinks=os._walk_symlinks_as_files)
        for dirpath, dirnames, filenames in results:
            for name in dirnames:
                fullname = os.path.join(dirpath, name)
                try:
                    os.rmdir(fullname)
                except FileNotFoundError:
                    continue
                except OSError as err:
                    onexc(os.rmdir, fullname, err)
            for name in filenames:
                fullname = os.path.join(dirpath, name)
                try:
                    os.unlink(fullname)
                except FileNotFoundError:
                    continue
                except OSError as err:
                    onexc(os.unlink, fullname, err)
        try:
            os.rmdir(path)
        except FileNotFoundError:
            pass
        except OSError as err:
            onexc(os.rmdir, path, err)

    rmtree.avoids_symlink_attacks = False

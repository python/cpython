"""Temporary files.

This module provides generic, low- and high-level interfaces for
creating temporary files and directories.  The interfaces listed
as "safe" just below can be used without fear of race conditions.
Those listed as "unsafe" cannot, and are provided for backward
compatibility only.

This module also provides some data items to the user:

  TMP_MAX  - maximum number of names that will be tried before
             giving up.
  template - the default prefix for all temporary names.
             You may change this to control the default prefix.
  tempdir  - If this is set to a string before the first use of
             any routine from this module, it will be considered as
             another candidate location to store temporary files.
"""

__all__ = [
    "NamedTemporaryFile", "TemporaryFile", # high level safe interfaces
    "SpooledTemporaryFile", "TemporaryDirectory",
    "mkstemp", "mkdtemp",                  # low level safe interfaces
    "mktemp",                              # deprecated unsafe interface
    "TMP_MAX", "gettempprefix",            # constants
    "tempdir", "gettempdir"
   ]


# Imports.

import io as _io
import os as _os
import errno as _errno
from random import Random as _Random

try:
    import fcntl as _fcntl
except ImportError:
    def _set_cloexec(fd):
        pass
else:
    def _set_cloexec(fd):
        try:
            flags = _fcntl.fcntl(fd, _fcntl.F_GETFD, 0)
        except IOError:
            pass
        else:
            # flags read successfully, modify
            flags |= _fcntl.FD_CLOEXEC
            _fcntl.fcntl(fd, _fcntl.F_SETFD, flags)


try:
    import _thread
except ImportError:
    import _dummy_thread as _thread
_allocate_lock = _thread.allocate_lock

_text_openflags = _os.O_RDWR | _os.O_CREAT | _os.O_EXCL
if hasattr(_os, 'O_NOINHERIT'):
    _text_openflags |= _os.O_NOINHERIT
if hasattr(_os, 'O_NOFOLLOW'):
    _text_openflags |= _os.O_NOFOLLOW

_bin_openflags = _text_openflags
if hasattr(_os, 'O_BINARY'):
    _bin_openflags |= _os.O_BINARY

if hasattr(_os, 'TMP_MAX'):
    TMP_MAX = _os.TMP_MAX
else:
    TMP_MAX = 10000

template = "tmp"

# Internal routines.

_once_lock = _allocate_lock()

if hasattr(_os, "lstat"):
    _stat = _os.lstat
elif hasattr(_os, "stat"):
    _stat = _os.stat
else:
    # Fallback.  All we need is something that raises os.error if the
    # file doesn't exist.
    def _stat(fn):
        try:
            f = open(fn)
        except IOError:
            raise _os.error
        f.close()

def _exists(fn):
    try:
        _stat(fn)
    except _os.error:
        return False
    else:
        return True

class _RandomNameSequence:
    """An instance of _RandomNameSequence generates an endless
    sequence of unpredictable strings which can safely be incorporated
    into file names.  Each string is six characters long.  Multiple
    threads can safely use the same instance at the same time.

    _RandomNameSequence is an iterator."""

    characters = "abcdefghijklmnopqrstuvwxyz0123456789_"

    def __init__(self):
        self.rng = _Random()

    def __iter__(self):
        return self

    def __next__(self):
        c = self.characters
        choose = self.rng.choice
        letters = [choose(c) for dummy in "123456"]
        return ''.join(letters)

def _candidate_tempdir_list():
    """Generate a list of candidate temporary directories which
    _get_default_tempdir will try."""

    dirlist = []

    # First, try the environment.
    for envname in 'TMPDIR', 'TEMP', 'TMP':
        dirname = _os.getenv(envname)
        if dirname: dirlist.append(dirname)

    # Failing that, try OS-specific locations.
    if _os.name == 'nt':
        dirlist.extend([ r'c:\temp', r'c:\tmp', r'\temp', r'\tmp' ])
    else:
        dirlist.extend([ '/tmp', '/var/tmp', '/usr/tmp' ])

    # As a last resort, the current directory.
    try:
        dirlist.append(_os.getcwd())
    except (AttributeError, _os.error):
        dirlist.append(_os.curdir)

    return dirlist

def _get_default_tempdir():
    """Calculate the default directory to use for temporary files.
    This routine should be called exactly once.

    We determine whether or not a candidate temp dir is usable by
    trying to create and write to a file in that directory.  If this
    is successful, the test file is deleted.  To prevent denial of
    service, the name of the test file must be randomized."""

    namer = _RandomNameSequence()
    dirlist = _candidate_tempdir_list()

    for dir in dirlist:
        if dir != _os.curdir:
            dir = _os.path.normcase(_os.path.abspath(dir))
        # Try only a few names per directory.
        for seq in range(100):
            name = next(namer)
            filename = _os.path.join(dir, name)
            try:
                fd = _os.open(filename, _bin_openflags, 0o600)
                fp = _io.open(fd, 'wb')
                fp.write(b'blat')
                fp.close()
                _os.unlink(filename)
                del fp, fd
                return dir
            except (OSError, IOError) as e:
                if e.args[0] != _errno.EEXIST:
                    break # no point trying more names in this directory
                pass
    raise IOError(_errno.ENOENT,
                  "No usable temporary directory found in %s" % dirlist)

_name_sequence = None

def _get_candidate_names():
    """Common setup sequence for all user-callable interfaces."""

    global _name_sequence
    if _name_sequence is None:
        _once_lock.acquire()
        try:
            if _name_sequence is None:
                _name_sequence = _RandomNameSequence()
        finally:
            _once_lock.release()
    return _name_sequence


def _mkstemp_inner(dir, pre, suf, flags):
    """Code common to mkstemp, TemporaryFile, and NamedTemporaryFile."""

    names = _get_candidate_names()

    for seq in range(TMP_MAX):
        name = next(names)
        file = _os.path.join(dir, pre + name + suf)
        try:
            fd = _os.open(file, flags, 0o600)
            _set_cloexec(fd)
            return (fd, _os.path.abspath(file))
        except OSError as e:
            if e.errno == _errno.EEXIST:
                continue # try again
            raise

    raise IOError(_errno.EEXIST, "No usable temporary file name found")


# User visible interfaces.

def gettempprefix():
    """Accessor for tempdir.template."""
    return template

tempdir = None

def gettempdir():
    """Accessor for tempfile.tempdir."""
    global tempdir
    if tempdir is None:
        _once_lock.acquire()
        try:
            if tempdir is None:
                tempdir = _get_default_tempdir()
        finally:
            _once_lock.release()
    return tempdir

def mkstemp(suffix="", prefix=template, dir=None, text=False):
    """User-callable function to create and return a unique temporary
    file.  The return value is a pair (fd, name) where fd is the
    file descriptor returned by os.open, and name is the filename.

    If 'suffix' is specified, the file name will end with that suffix,
    otherwise there will be no suffix.

    If 'prefix' is specified, the file name will begin with that prefix,
    otherwise a default prefix is used.

    If 'dir' is specified, the file will be created in that directory,
    otherwise a default directory is used.

    If 'text' is specified and true, the file is opened in text
    mode.  Else (the default) the file is opened in binary mode.  On
    some operating systems, this makes no difference.

    The file is readable and writable only by the creating user ID.
    If the operating system uses permission bits to indicate whether a
    file is executable, the file is executable by no one. The file
    descriptor is not inherited by children of this process.

    Caller is responsible for deleting the file when done with it.
    """

    if dir is None:
        dir = gettempdir()

    if text:
        flags = _text_openflags
    else:
        flags = _bin_openflags

    return _mkstemp_inner(dir, prefix, suffix, flags)


def mkdtemp(suffix="", prefix=template, dir=None):
    """User-callable function to create and return a unique temporary
    directory.  The return value is the pathname of the directory.

    Arguments are as for mkstemp, except that the 'text' argument is
    not accepted.

    The directory is readable, writable, and searchable only by the
    creating user.

    Caller is responsible for deleting the directory when done with it.
    """

    if dir is None:
        dir = gettempdir()

    names = _get_candidate_names()

    for seq in range(TMP_MAX):
        name = next(names)
        file = _os.path.join(dir, prefix + name + suffix)
        try:
            _os.mkdir(file, 0o700)
            return file
        except OSError as e:
            if e.errno == _errno.EEXIST:
                continue # try again
            raise

    raise IOError(_errno.EEXIST, "No usable temporary directory name found")

def mktemp(suffix="", prefix=template, dir=None):
    """User-callable function to return a unique temporary file name.  The
    file is not created.

    Arguments are as for mkstemp, except that the 'text' argument is
    not accepted.

    This function is unsafe and should not be used.  The file name
    refers to a file that did not exist at some point, but by the time
    you get around to creating it, someone else may have beaten you to
    the punch.
    """

##    from warnings import warn as _warn
##    _warn("mktemp is a potential security risk to your program",
##          RuntimeWarning, stacklevel=2)

    if dir is None:
        dir = gettempdir()

    names = _get_candidate_names()
    for seq in range(TMP_MAX):
        name = next(names)
        file = _os.path.join(dir, prefix + name + suffix)
        if not _exists(file):
            return file

    raise IOError(_errno.EEXIST, "No usable temporary filename found")


class _TemporaryFileWrapper:
    """Temporary file wrapper

    This class provides a wrapper around files opened for
    temporary use.  In particular, it seeks to automatically
    remove the file when it is no longer needed.
    """

    def __init__(self, file, name, delete=True):
        self.file = file
        self.name = name
        self.close_called = False
        self.delete = delete

    def __getattr__(self, name):
        # Attribute lookups are delegated to the underlying file
        # and cached for non-numeric results
        # (i.e. methods are cached, closed and friends are not)
        file = self.__dict__['file']
        a = getattr(file, name)
        if not isinstance(a, int):
            setattr(self, name, a)
        return a

    # The underlying __enter__ method returns the wrong object
    # (self.file) so override it to return the wrapper
    def __enter__(self):
        self.file.__enter__()
        return self

    # iter() doesn't use __getattr__ to find the __iter__ method
    def __iter__(self):
        return iter(self.file)

    # NT provides delete-on-close as a primitive, so we don't need
    # the wrapper to do anything special.  We still use it so that
    # file.name is useful (i.e. not "(fdopen)") with NamedTemporaryFile.
    if _os.name != 'nt':
        # Cache the unlinker so we don't get spurious errors at
        # shutdown when the module-level "os" is None'd out.  Note
        # that this must be referenced as self.unlink, because the
        # name TemporaryFileWrapper may also get None'd out before
        # __del__ is called.
        unlink = _os.unlink

        def close(self):
            if not self.close_called:
                self.close_called = True
                self.file.close()
                if self.delete:
                    self.unlink(self.name)

        def __del__(self):
            self.close()

        # Need to trap __exit__ as well to ensure the file gets
        # deleted when used in a with statement
        def __exit__(self, exc, value, tb):
            result = self.file.__exit__(exc, value, tb)
            self.close()
            return result
    else:
        def __exit__(self, exc, value, tb):
            self.file.__exit__(exc, value, tb)


def NamedTemporaryFile(mode='w+b', buffering=-1, encoding=None,
                       newline=None, suffix="", prefix=template,
                       dir=None, delete=True):
    """Create and return a temporary file.
    Arguments:
    'prefix', 'suffix', 'dir' -- as for mkstemp.
    'mode' -- the mode argument to io.open (default "w+b").
    'buffering' -- the buffer size argument to io.open (default -1).
    'encoding' -- the encoding argument to io.open (default None)
    'newline' -- the newline argument to io.open (default None)
    'delete' -- whether the file is deleted on close (default True).
    The file is created as mkstemp() would do it.

    Returns an object with a file-like interface; the name of the file
    is accessible as file.name.  The file will be automatically deleted
    when it is closed unless the 'delete' argument is set to False.
    """

    if dir is None:
        dir = gettempdir()

    flags = _bin_openflags

    # Setting O_TEMPORARY in the flags causes the OS to delete
    # the file when it is closed.  This is only supported by Windows.
    if _os.name == 'nt' and delete:
        flags |= _os.O_TEMPORARY

    (fd, name) = _mkstemp_inner(dir, prefix, suffix, flags)
    file = _io.open(fd, mode, buffering=buffering,
                    newline=newline, encoding=encoding)

    return _TemporaryFileWrapper(file, name, delete)

if _os.name != 'posix' or _os.sys.platform == 'cygwin':
    # On non-POSIX and Cygwin systems, assume that we cannot unlink a file
    # while it is open.
    TemporaryFile = NamedTemporaryFile

else:
    def TemporaryFile(mode='w+b', buffering=-1, encoding=None,
                      newline=None, suffix="", prefix=template,
                      dir=None):
        """Create and return a temporary file.
        Arguments:
        'prefix', 'suffix', 'dir' -- as for mkstemp.
        'mode' -- the mode argument to io.open (default "w+b").
        'buffering' -- the buffer size argument to io.open (default -1).
        'encoding' -- the encoding argument to io.open (default None)
        'newline' -- the newline argument to io.open (default None)
        The file is created as mkstemp() would do it.

        Returns an object with a file-like interface.  The file has no
        name, and will cease to exist when it is closed.
        """

        if dir is None:
            dir = gettempdir()

        flags = _bin_openflags

        (fd, name) = _mkstemp_inner(dir, prefix, suffix, flags)
        try:
            _os.unlink(name)
            return _io.open(fd, mode, buffering=buffering,
                            newline=newline, encoding=encoding)
        except:
            _os.close(fd)
            raise

class SpooledTemporaryFile:
    """Temporary file wrapper, specialized to switch from
    StringIO to a real file when it exceeds a certain size or
    when a fileno is needed.
    """
    _rolled = False

    def __init__(self, max_size=0, mode='w+b', buffering=-1,
                 encoding=None, newline=None,
                 suffix="", prefix=template, dir=None):
        if 'b' in mode:
            self._file = _io.BytesIO()
        else:
            # Setting newline="\n" avoids newline translation;
            # this is important because otherwise on Windows we'd
            # hget double newline translation upon rollover().
            self._file = _io.StringIO(newline="\n")
        self._max_size = max_size
        self._rolled = False
        self._TemporaryFileArgs = {'mode': mode, 'buffering': buffering,
                                   'suffix': suffix, 'prefix': prefix,
                                   'encoding': encoding, 'newline': newline,
                                   'dir': dir}

    def _check(self, file):
        if self._rolled: return
        max_size = self._max_size
        if max_size and file.tell() > max_size:
            self.rollover()

    def rollover(self):
        if self._rolled: return
        file = self._file
        newfile = self._file = TemporaryFile(**self._TemporaryFileArgs)
        del self._TemporaryFileArgs

        newfile.write(file.getvalue())
        newfile.seek(file.tell(), 0)

        self._rolled = True

    # The method caching trick from NamedTemporaryFile
    # won't work here, because _file may change from a
    # _StringIO instance to a real file. So we list
    # all the methods directly.

    # Context management protocol
    def __enter__(self):
        if self._file.closed:
            raise ValueError("Cannot enter context with closed file")
        return self

    def __exit__(self, exc, value, tb):
        self._file.close()

    # file protocol
    def __iter__(self):
        return self._file.__iter__()

    def close(self):
        self._file.close()

    @property
    def closed(self):
        return self._file.closed

    @property
    def encoding(self):
        return self._file.encoding

    def fileno(self):
        self.rollover()
        return self._file.fileno()

    def flush(self):
        self._file.flush()

    def isatty(self):
        return self._file.isatty()

    @property
    def mode(self):
        return self._file.mode

    @property
    def name(self):
        return self._file.name

    @property
    def newlines(self):
        return self._file.newlines

    def next(self):
        return self._file.next

    def read(self, *args):
        return self._file.read(*args)

    def readline(self, *args):
        return self._file.readline(*args)

    def readlines(self, *args):
        return self._file.readlines(*args)

    def seek(self, *args):
        self._file.seek(*args)

    @property
    def softspace(self):
        return self._file.softspace

    def tell(self):
        return self._file.tell()

    def truncate(self):
        self._file.truncate()

    def write(self, s):
        file = self._file
        rv = file.write(s)
        self._check(file)
        return rv

    def writelines(self, iterable):
        file = self._file
        rv = file.writelines(iterable)
        self._check(file)
        return rv

    def xreadlines(self, *args):
        return self._file.xreadlines(*args)


class TemporaryDirectory(object):
    """Create and return a temporary directory.  This has the same
    behavior as mkdtemp but can be used as a context manager.  For
    example:

        with TemporaryDirectory() as tmpdir:
            ...

    Upon exiting the context, the directory and everthing contained
    in it are removed.
    """

    def __init__(self, suffix="", prefix=template, dir=None):
        self.name = mkdtemp(suffix, prefix, dir)
        self._closed = False

    def __enter__(self):
        return self.name

    def cleanup(self):
        if not self._closed:
            self._rmtree(self.name)
            self._closed = True

    def __exit__(self, exc, value, tb):
        self.cleanup()

    __del__ = cleanup


    # XXX (ncoghlan): The following code attempts to make
    # this class tolerant of the module nulling out process
    # that happens during CPython interpreter shutdown
    # Alas, it doesn't actually manage it. See issue #10188
    _listdir = staticmethod(_os.listdir)
    _path_join = staticmethod(_os.path.join)
    _isdir = staticmethod(_os.path.isdir)
    _remove = staticmethod(_os.remove)
    _rmdir = staticmethod(_os.rmdir)
    _os_error = _os.error

    def _rmtree(self, path):
        # Essentially a stripped down version of shutil.rmtree.  We can't
        # use globals because they may be None'ed out at shutdown.
        for name in self._listdir(path):
            fullname = self._path_join(path, name)
            try:
                isdir = self._isdir(fullname)
            except self._os_error:
                isdir = False
            if isdir:
                self._rmtree(fullname)
            else:
                try:
                    self._remove(fullname)
                except self._os_error:
                    pass
        try:
            self._rmdir(path)
        except self._os_error:
            pass

"""Temporary files and filenames."""

# XXX This tries to be not UNIX specific, but I don't know beans about
# how to choose a temp directory or filename on MS-DOS or other
# systems so it may have to be changed...

import os

__all__ = ["mktemp", "TemporaryFile", "tempdir", "gettempprefix"]

# Parameters that the caller may set to override the defaults
tempdir = None
template = None

def gettempdir():
    """Function to calculate the directory to use."""
    global tempdir
    if tempdir is not None:
        return tempdir

    # _gettempdir_inner deduces whether a candidate temp dir is usable by
    # trying to create a file in it, and write to it.  If that succeeds,
    # great, it closes the file and unlinks it.  There's a race, though:
    # the *name* of the test file it tries is the same across all threads
    # under most OSes (Linux is an exception), and letting multiple threads
    # all try to open, write to, close, and unlink a single file can cause
    # a variety of bogus errors (e.g., you cannot unlink a file under
    # Windows if anyone has it open, and two threads cannot create the
    # same file in O_EXCL mode under Unix).  The simplest cure is to serialize
    # calls to _gettempdir_inner.  This isn't a real expense, because the
    # first thread to succeed sets the global tempdir, and all subsequent
    # calls to gettempdir() reuse that without trying _gettempdir_inner.
    _tempdir_lock.acquire()
    try:
        return _gettempdir_inner()
    finally:
        _tempdir_lock.release()

def _gettempdir_inner():
    """Function to calculate the directory to use."""
    global tempdir
    if tempdir is not None:
        return tempdir
    try:
        pwd = os.getcwd()
    except (AttributeError, os.error):
        pwd = os.curdir
    attempdirs = ['/tmp', '/var/tmp', '/usr/tmp', pwd]
    if os.name == 'nt':
        attempdirs.insert(0, 'C:\\TEMP')
        attempdirs.insert(0, '\\TEMP')
    elif os.name == 'mac':
        import macfs, MACFS
        try:
            refnum, dirid = macfs.FindFolder(MACFS.kOnSystemDisk,
                                             MACFS.kTemporaryFolderType, 1)
            dirname = macfs.FSSpec((refnum, dirid, '')).as_pathname()
            attempdirs.insert(0, dirname)
        except macfs.error:
            pass
    elif os.name == 'riscos':
        scrapdir = os.getenv('Wimp$ScrapDir')
        if scrapdir:
            attempdirs.insert(0, scrapdir)
    for envname in 'TMPDIR', 'TEMP', 'TMP':
        if envname in os.environ:
            attempdirs.insert(0, os.environ[envname])
    testfile = gettempprefix() + 'test'
    for dir in attempdirs:
        try:
            filename = os.path.join(dir, testfile)
            if os.name == 'posix':
                try:
                    fd = os.open(filename,
                                 os.O_RDWR | os.O_CREAT | os.O_EXCL, 0700)
                except OSError:
                    pass
                else:
                    fp = os.fdopen(fd, 'w')
                    fp.write('blat')
                    fp.close()
                    os.unlink(filename)
                    del fp, fd
                    tempdir = dir
                    break
            else:
                fp = open(filename, 'w')
                fp.write('blat')
                fp.close()
                os.unlink(filename)
                tempdir = dir
                break
        except IOError:
            pass
    if tempdir is None:
        msg = "Can't find a usable temporary directory amongst " + `attempdirs`
        raise IOError, msg
    return tempdir


# template caches the result of gettempprefix, for speed, when possible.
# XXX unclear why this isn't "_template"; left it "template" for backward
# compatibility.
if os.name == "posix":
    # We don't try to cache the template on posix:  the pid may change on us
    # between calls due to a fork, and on Linux the pid changes even for
    # another thread in the same process.  Since any attempt to keep the
    # cache in synch would have to call os.getpid() anyway in order to make
    # sure the pid hasn't changed between calls, a cache wouldn't save any
    # time.  In addition, a cache is difficult to keep correct with the pid
    # changing willy-nilly, and earlier attempts proved buggy (races).
    template = None

# Else the pid never changes, so gettempprefix always returns the same
# string.
elif os.name == "nt":
    template = '~' + `os.getpid()` + '-'
elif os.name in ('mac', 'riscos'):
    template = 'Python-Tmp-'
else:
    template = 'tmp' # XXX might choose a better one

def gettempprefix():
    """Function to calculate a prefix of the filename to use.

    This incorporates the current process id on systems that support such a
    notion, so that concurrent processes don't generate the same prefix.
    """

    global template
    if template is None:
        return '@' + `os.getpid()` + '.'
    else:
        return template


def mktemp(suffix=""):
    """User-callable function to return a unique temporary file name."""
    dir = gettempdir()
    pre = gettempprefix()
    while 1:
        i = _counter.get_next()
        file = os.path.join(dir, pre + str(i) + suffix)
        if not os.path.exists(file):
            return file


class TemporaryFileWrapper:
    """Temporary file wrapper

    This class provides a wrapper around files opened for temporary use.
    In particular, it seeks to automatically remove the file when it is
    no longer needed.
    """

    # Cache the unlinker so we don't get spurious errors at shutdown
    # when the module-level "os" is None'd out.  Note that this must
    # be referenced as self.unlink, because the name TemporaryFileWrapper
    # may also get None'd out before __del__ is called.
    unlink = os.unlink

    def __init__(self, file, path):
        self.file = file
        self.path = path
        self.close_called = 0

    def close(self):
        if not self.close_called:
            self.close_called = 1
            self.file.close()
            self.unlink(self.path)

    def __del__(self):
        self.close()

    def __getattr__(self, name):
        file = self.__dict__['file']
        a = getattr(file, name)
        if type(a) != type(0):
            setattr(self, name, a)
        return a

try:
    import fcntl as _fcntl
    def _set_cloexec(fd, flag=_fcntl.FD_CLOEXEC):
        flags = _fcntl.fcntl(fd, _fcntl.F_GETFD, 0)
        if flags >= 0:
            # flags read successfully, modify
            flags |= flag
            _fcntl.fcntl(fd, _fcntl.F_SETFD, flags)
except (ImportError, AttributeError):
    def _set_cloexec(fd):
        pass

def TemporaryFile(mode='w+b', bufsize=-1, suffix=""):
    """Create and return a temporary file (opened read-write by default)."""
    name = mktemp(suffix)
    if os.name == 'posix':
        # Unix -- be very careful
        fd = os.open(name, os.O_RDWR|os.O_CREAT|os.O_EXCL, 0700)
        _set_cloexec(fd)
        try:
            os.unlink(name)
            return os.fdopen(fd, mode, bufsize)
        except:
            os.close(fd)
            raise
    elif os.name == 'nt':
        # Windows -- can't unlink an open file, but O_TEMPORARY creates a
        # file that "deletes itself" when the last handle is closed.
        # O_NOINHERIT ensures processes created via spawn() don't get a
        # handle to this too.  That would be a security hole, and, on my
        # Win98SE box, when an O_TEMPORARY file is inherited by a spawned
        # process, the fd in the spawned process seems to lack the
        # O_TEMPORARY flag, so the file doesn't go away by magic then if the
        # spawning process closes it first.
        flags = (os.O_RDWR | os.O_CREAT | os.O_EXCL |
                 os.O_TEMPORARY | os.O_NOINHERIT)
        if 'b' in mode:
            flags |= os.O_BINARY
        fd = os.open(name, flags, 0700)
        return os.fdopen(fd, mode, bufsize)
    else:
        # Assume we can't unlink a file that's still open, or arrange for
        # an automagically self-deleting file -- use wrapper.
        file = open(name, mode, bufsize)
        return TemporaryFileWrapper(file, name)

# In order to generate unique names, mktemp() uses _counter.get_next().
# This returns a unique integer on each call, in a threadsafe way (i.e.,
# multiple threads will never see the same integer).  The integer will
# usually be a Python int, but if _counter.get_next() is called often
# enough, it will become a Python long.
# Note that the only names that survive this next block of code
# are "_counter" and "_tempdir_lock".

class _ThreadSafeCounter:
    def __init__(self, mutex, initialvalue=0):
        self.mutex = mutex
        self.i = initialvalue

    def get_next(self):
        self.mutex.acquire()
        result = self.i
        try:
            newi = result + 1
        except OverflowError:
            newi = long(result) + 1
        self.i = newi
        self.mutex.release()
        return result

try:
    import thread

except ImportError:
    class _DummyMutex:
        def acquire(self):
            pass

        release = acquire

    _counter = _ThreadSafeCounter(_DummyMutex())
    _tempdir_lock = _DummyMutex()
    del _DummyMutex

else:
    _counter = _ThreadSafeCounter(thread.allocate_lock())
    _tempdir_lock = thread.allocate_lock()
    del thread

del _ThreadSafeCounter

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
        if os.environ.has_key(envname):
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


def TemporaryFile(mode='w+b', bufsize=-1, suffix=""):
    """Create and return a temporary file (opened read-write by default)."""
    name = mktemp(suffix)
    if os.name == 'posix':
        # Unix -- be very careful
        fd = os.open(name, os.O_RDWR|os.O_CREAT|os.O_EXCL, 0700)
        try:
            os.unlink(name)
            return os.fdopen(fd, mode, bufsize)
        except:
            os.close(fd)
            raise
    else:
        # Non-unix -- can't unlink file that's still open, use wrapper
        file = open(name, mode, bufsize)
        return TemporaryFileWrapper(file, name)

# In order to generate unique names, mktemp() uses _counter.get_next().
# This returns a unique integer on each call, in a threadsafe way (i.e.,
# multiple threads will never see the same integer).  The integer will
# usually be a Python int, but if _counter.get_next() is called often
# enough, it will become a Python long.
# Note that the only name that survives this next block of code
# is "_counter".

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
    del _DummyMutex

else:
    _counter = _ThreadSafeCounter(thread.allocate_lock())
    del thread

del _ThreadSafeCounter

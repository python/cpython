"""Temporary files and filenames."""

# XXX This tries to be not UNIX specific, but I don't know beans about
# how to choose a temp directory or filename on MS-DOS or other
# systems so it may have to be changed...


import os


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
    attempdirs = ['/var/tmp', '/usr/tmp', '/tmp', pwd]
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
    for envname in 'TMPDIR', 'TEMP', 'TMP':
        if os.environ.has_key(envname):
            attempdirs.insert(0, os.environ[envname])
    testfile = gettempprefix() + 'test'
    for dir in attempdirs:
        try:
           filename = os.path.join(dir, testfile)
           if os.name == 'posix':
               try:
                   fd = os.open(filename, os.O_RDWR|os.O_CREAT|os.O_EXCL, 0700)
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


_pid = None

def gettempprefix():
    """Function to calculate a prefix of the filename to use."""
    global template, _pid
    if os.name == 'posix' and _pid and _pid != os.getpid():
        # Our pid changed; we must have forked -- zap the template
        template = None
    if template is None:
        if os.name == 'posix':
            _pid = os.getpid()
            template = '@' + `_pid` + '.'
        elif os.name == 'nt':
            template = '~' + `os.getpid()` + '-'
        elif os.name == 'mac':
            template = 'Python-Tmp-'
        else:
            template = 'tmp' # XXX might choose a better one
    return template


# Counter for generating unique names

counter = 0


def mktemp(suffix=""):
    """User-callable function to return a unique temporary file name."""
    global counter
    dir = gettempdir()
    pre = gettempprefix()
    while 1:
        counter = counter + 1
        file = os.path.join(dir, pre + `counter` + suffix)
        if not os.path.exists(file):
            return file


class TemporaryFileWrapper:
    """Temporary file wrapper

    This class provides a wrapper around files opened for temporary use.
    In particular, it seeks to automatically remove the file when it is
    no longer needed.
    """
    def __init__(self, file, path):
        self.file = file
        self.path = path

    def close(self):
        self.file.close()
        os.unlink(self.path)

    def __del__(self):
        try: self.close()
        except: pass

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

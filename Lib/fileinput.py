"""Helper class to quickly write a loop over all standard input files.

Typical use is:

    import fileinput
    for line in fileinput.input():
        process(line)

This iterates over the lines of all files listed in sys.argv[1:],
defaulting to sys.stdin if the list is empty.  If a filename is '-' it
is also replaced by sys.stdin.  To specify an alternative list of
filenames, pass it as the argument to input().  A single file name is
also allowed.

Functions filename(), lineno() return the filename and cumulative line
number of the line that has just been read; filelineno() returns its
line number in the current file; isfirstline() returns true iff the
line just read is the first line of its file; isstdin() returns true
iff the line was read from sys.stdin.  Function nextfile() closes the
current file so that the next iteration will read the first line from
the next file (if any); lines not read from the file will not count
towards the cumulative line count; the filename is not changed until
after the first line of the next file has been read.  Function close()
closes the sequence.

Before any lines have been read, filename() returns None and both line
numbers are zero; nextfile() has no effect.  After all lines have been
read, filename() and the line number functions return the values
pertaining to the last line read; nextfile() has no effect.

All files are opened in text mode.  If an I/O error occurs during
opening or reading a file, the IOError exception is raised.

If sys.stdin is used more than once, the second and further use will
return no lines, except perhaps for interactive use, or if it has been
explicitly reset (e.g. using sys.stdin.seek(0)).

Empty files are opened and immediately closed; the only time their
presence in the list of filenames is noticeable at all is when the
last file opened is empty.

It is possible that the last line of a file doesn't end in a newline
character; otherwise lines are returned including the trailing
newline.

Class FileInput is the implementation; its methods filename(),
lineno(), fileline(), isfirstline(), isstdin(), nextfile() and close()
correspond to the functions in the module.  In addition it has a
readline() method which returns the next input line, and a
__getitem__() method which implements the sequence behavior.  The
sequence must be accessed in strictly sequential order; sequence
access and readline() cannot be mixed.

Optional in-place filtering: if the keyword argument inplace=1 is
passed to input() or to the FileInput constructor, the file is moved
to a backup file and standard output is directed to the input file.
This makes it possible to write a filter that rewrites its input file
in place.  If the keyword argument backup=".<some extension>" is also
given, it specifies the extension for the backup file, and the backup
file remains around; by default, the extension is ".bak" and it is
deleted when the output file is closed.  In-place filtering is
disabled when standard input is read.  XXX The current implementation
does not work for MS-DOS 8+3 filesystems.

XXX Possible additions:

- optional getopt argument processing
- specify open mode ('r' or 'rb')
- specify buffer size
- fileno()
- isatty()
- read(), read(size), even readlines()

"""

import sys, os, stat

_state = None

def input(files=None, inplace=0, backup=""):
    global _state
    if _state and _state._file:
        raise RuntimeError, "input() already active"
    _state = FileInput(files, inplace, backup)
    return _state

def close():
    global _state
    state = _state
    _state = None
    if state:
        state.close()

def nextfile():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.nextfile()

def filename():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.filename()

def lineno():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.lineno()

def filelineno():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.filelineno()

def isfirstline():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.isfirstline()

def isstdin():
    if not _state:
        raise RuntimeError, "no active input()"
    return _state.isstdin()

class FileInput:

    def __init__(self, files=None, inplace=0, backup=""):
        if type(files) == type(''):
            files = (files,)
        else:
            if files is None:
                files = sys.argv[1:]
            if not files:
                files = ('-',)
            else:
                files = tuple(files)
        self._files = files
        self._inplace = inplace
        self._backup = backup
        self._savestdout = None
        self._output = None
        self._filename = None
        self._lineno = 0
        self._filelineno = 0
        self._file = None
        self._isstdin = 0
        self._backupfilename = None

    def __del__(self):
        self.close()

    def close(self):
        self.nextfile()
        self._files = ()

    def __getitem__(self, i):
        if i != self._lineno:
            raise RuntimeError, "accessing lines out of order"
        line = self.readline()
        if not line:
            raise IndexError, "end of input reached"
        return line

    def nextfile(self):
        savestdout = self._savestdout
        self._savestdout = 0
        if savestdout:
            sys.stdout = savestdout

        output = self._output
        self._output = 0
        if output:
            output.close()

        file = self._file
        self._file = 0
        if file and not self._isstdin:
            file.close()

        backupfilename = self._backupfilename
        self._backupfilename = 0
        if backupfilename and not self._backup:
            try: os.unlink(backupfilename)
            except: pass

        self._isstdin = 0

    def readline(self):
        if not self._file:
            if not self._files:
                return ""
            self._filename = self._files[0]
            self._files = self._files[1:]
            self._filelineno = 0
            self._file = None
            self._isstdin = 0
            self._backupfilename = 0
            if self._filename == '-':
                self._filename = '<stdin>'
                self._file = sys.stdin
                self._isstdin = 1
            else:
                if self._inplace:
                    self._backupfilename = (
                        self._filename + (self._backup or ".bak"))
                    try: os.unlink(self._backupfilename)
                    except os.error: pass
                    # The next few lines may raise IOError
                    os.rename(self._filename, self._backupfilename)
                    self._file = open(self._backupfilename, "r")
                    try:
                        perm = os.fstat(self._file.fileno())[stat.ST_MODE]
                    except:
                        self._output = open(self._filename, "w")
                    else:
                        fd = os.open(self._filename,
                                     os.O_CREAT | os.O_WRONLY | os.O_TRUNC,
                                     perm)
                        self._output = os.fdopen(fd, "w")
                        try:
                            os.chmod(self._filename, perm)
                        except:
                            pass
                    self._savestdout = sys.stdout
                    sys.stdout = self._output
                else:
                    # This may raise IOError
                    self._file = open(self._filename, "r")
        line = self._file.readline()
        if line:
            self._lineno = self._lineno + 1
            self._filelineno = self._filelineno + 1
            return line
        self.nextfile()
        # Recursive call
        return self.readline()

    def filename(self):
        return self._filename

    def lineno(self):
        return self._lineno

    def filelineno(self):
        return self._filelineno

    def isfirstline(self):
        return self._filelineno == 1

    def isstdin(self):
        return self._isstdin

def _test():
    import getopt
    inplace = 0
    backup = 0
    opts, args = getopt.getopt(sys.argv[1:], "ib:")
    for o, a in opts:
        if o == '-i': inplace = 1
        if o == '-b': backup = a
    for line in input(args, inplace=inplace, backup=backup):
        if line[-1:] == '\n': line = line[:-1]
        if line[-1:] == '\r': line = line[:-1]
        print "%d: %s[%d]%s %s" % (lineno(), filename(), filelineno(),
                                   isfirstline() and "*" or "", line)
    print "%d: %s[%d]" % (lineno(), filename(), filelineno())

if __name__ == '__main__':
    _test()

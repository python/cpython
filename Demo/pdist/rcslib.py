"""RCS interface module.

Defines the class RCS, which represents a directory with rcs version
files and (possibly) corresponding work files.

"""


import fnmatch
import os
import re
import string
import tempfile


class RCS:

    """RCS interface class (local filesystem version).

    An instance of this class represents a directory with rcs version
    files and (possible) corresponding work files.

    Methods provide access to most rcs operations such as
    checkin/checkout, access to the rcs metadata (revisions, logs,
    branches etc.) as well as some filesystem operations such as
    listing all rcs version files.

    XXX BUGS / PROBLEMS

    - The instance always represents the current directory so it's not
    very useful to have more than one instance around simultaneously

    """

    # Characters allowed in work file names
    okchars = string.ascii_letters + string.digits + '-_=+'

    def __init__(self):
        """Constructor."""
        pass

    def __del__(self):
        """Destructor."""
        pass

    # --- Informational methods about a single file/revision ---

    def log(self, name_rev, otherflags = ''):
        """Return the full log text for NAME_REV as a string.

        Optional OTHERFLAGS are passed to rlog.

        """
        f = self._open(name_rev, 'rlog ' + otherflags)
        data = f.read()
        status = self._closepipe(f)
        if status:
            data = data + "%s: %s" % status
        elif data[-1] == '\n':
            data = data[:-1]
        return data

    def head(self, name_rev):
        """Return the head revision for NAME_REV"""
        dict = self.info(name_rev)
        return dict['head']

    def info(self, name_rev):
        """Return a dictionary of info (from rlog -h) for NAME_REV

        The dictionary's keys are the keywords that rlog prints
        (e.g. 'head' and its values are the corresponding data
        (e.g. '1.3').

        XXX symbolic names and locks are not returned

        """
        f = self._open(name_rev, 'rlog -h')
        dict = {}
        while 1:
            line = f.readline()
            if not line: break
            if line[0] == '\t':
                # XXX could be a lock or symbolic name
                # Anything else?
                continue
            i = string.find(line, ':')
            if i > 0:
                key, value = line[:i], string.strip(line[i+1:])
                dict[key] = value
        status = self._closepipe(f)
        if status:
            raise IOError, status
        return dict

    # --- Methods that change files ---

    def lock(self, name_rev):
        """Set an rcs lock on NAME_REV."""
        name, rev = self.checkfile(name_rev)
        cmd = "rcs -l%s %s" % (rev, name)
        return self._system(cmd)

    def unlock(self, name_rev):
        """Clear an rcs lock on NAME_REV."""
        name, rev = self.checkfile(name_rev)
        cmd = "rcs -u%s %s" % (rev, name)
        return self._system(cmd)

    def checkout(self, name_rev, withlock=0, otherflags=""):
        """Check out NAME_REV to its work file.

        If optional WITHLOCK is set, check out locked, else unlocked.

        The optional OTHERFLAGS is passed to co without
        interpretation.

        Any output from co goes to directly to stdout.

        """
        name, rev = self.checkfile(name_rev)
        if withlock: lockflag = "-l"
        else: lockflag = "-u"
        cmd = 'co %s%s %s %s' % (lockflag, rev, otherflags, name)
        return self._system(cmd)

    def checkin(self, name_rev, message=None, otherflags=""):
        """Check in NAME_REV from its work file.

        The optional MESSAGE argument becomes the checkin message
        (default "<none>" if None); or the file description if this is
        a new file.

        The optional OTHERFLAGS argument is passed to ci without
        interpretation.

        Any output from ci goes to directly to stdout.

        """
        name, rev = self._unmangle(name_rev)
        new = not self.isvalid(name)
        if not message: message = "<none>"
        if message and message[-1] != '\n':
            message = message + '\n'
        lockflag = "-u"
        if new:
            f = tempfile.NamedTemporaryFile()
            f.write(message)
            f.flush()
            cmd = 'ci %s%s -t%s %s %s' % \
                  (lockflag, rev, f.name, otherflags, name)
        else:
            message = re.sub(r'([\"$`])', r'\\\1', message)
            cmd = 'ci %s%s -m"%s" %s %s' % \
                  (lockflag, rev, message, otherflags, name)
        return self._system(cmd)

    # --- Exported support methods ---

    def listfiles(self, pat = None):
        """Return a list of all version files matching optional PATTERN."""
        files = os.listdir(os.curdir)
        files = filter(self._isrcs, files)
        if os.path.isdir('RCS'):
            files2 = os.listdir('RCS')
            files2 = filter(self._isrcs, files2)
            files = files + files2
        files = map(self.realname, files)
        return self._filter(files, pat)

    def isvalid(self, name):
        """Test whether NAME has a version file associated."""
        namev = self.rcsname(name)
        return (os.path.isfile(namev) or
                os.path.isfile(os.path.join('RCS', namev)))

    def rcsname(self, name):
        """Return the pathname of the version file for NAME.

        The argument can be a work file name or a version file name.
        If the version file does not exist, the name of the version
        file that would be created by "ci" is returned.

        """
        if self._isrcs(name): namev = name
        else: namev = name + ',v'
        if os.path.isfile(namev): return namev
        namev = os.path.join('RCS', os.path.basename(namev))
        if os.path.isfile(namev): return namev
        if os.path.isdir('RCS'):
            return os.path.join('RCS', namev)
        else:
            return namev

    def realname(self, namev):
        """Return the pathname of the work file for NAME.

        The argument can be a work file name or a version file name.
        If the work file does not exist, the name of the work file
        that would be created by "co" is returned.

        """
        if self._isrcs(namev): name = namev[:-2]
        else: name = namev
        if os.path.isfile(name): return name
        name = os.path.basename(name)
        return name

    def islocked(self, name_rev):
        """Test whether FILE (which must have a version file) is locked.

        XXX This does not tell you which revision number is locked and
        ignores any revision you may pass in (by virtue of using rlog
        -L -R).

        """
        f = self._open(name_rev, 'rlog -L -R')
        line = f.readline()
        status = self._closepipe(f)
        if status:
            raise IOError, status
        if not line: return None
        if line[-1] == '\n':
            line = line[:-1]
        return self.realname(name_rev) == self.realname(line)

    def checkfile(self, name_rev):
        """Normalize NAME_REV into a (NAME, REV) tuple.

        Raise an exception if there is no corresponding version file.

        """
        name, rev = self._unmangle(name_rev)
        if not self.isvalid(name):
            raise os.error, 'not an rcs file %r' % (name,)
        return name, rev

    # --- Internal methods ---

    def _open(self, name_rev, cmd = 'co -p', rflag = '-r'):
        """INTERNAL: open a read pipe to NAME_REV using optional COMMAND.

        Optional FLAG is used to indicate the revision (default -r).

        Default COMMAND is "co -p".

        Return a file object connected by a pipe to the command's
        output.

        """
        name, rev = self.checkfile(name_rev)
        namev = self.rcsname(name)
        if rev:
            cmd = cmd + ' ' + rflag + rev
        return os.popen("%s %r" % (cmd, namev))

    def _unmangle(self, name_rev):
        """INTERNAL: Normalize NAME_REV argument to (NAME, REV) tuple.

        Raise an exception if NAME contains invalid characters.

        A NAME_REV argument is either NAME string (implying REV='') or
        a tuple of the form (NAME, REV).

        """
        if type(name_rev) == type(''):
            name_rev = name, rev = name_rev, ''
        else:
            name, rev = name_rev
        for c in rev:
            if c not in self.okchars:
                raise ValueError, "bad char in rev"
        return name_rev

    def _closepipe(self, f):
        """INTERNAL: Close PIPE and print its exit status if nonzero."""
        sts = f.close()
        if not sts: return None
        detail, reason = divmod(sts, 256)
        if reason == 0: return 'exit', detail   # Exit status
        signal = reason&0x7F
        if signal == 0x7F:
            code = 'stopped'
            signal = detail
        else:
            code = 'killed'
        if reason&0x80:
            code = code + '(coredump)'
        return code, signal

    def _system(self, cmd):
        """INTERNAL: run COMMAND in a subshell.

        Standard input for the command is taken from /dev/null.

        Raise IOError when the exit status is not zero.

        Return whatever the calling method should return; normally
        None.

        A derived class may override this method and redefine it to
        capture stdout/stderr of the command and return it.

        """
        cmd = cmd + " </dev/null"
        sts = os.system(cmd)
        if sts: raise IOError, "command exit status %d" % sts

    def _filter(self, files, pat = None):
        """INTERNAL: Return a sorted copy of the given list of FILES.

        If a second PATTERN argument is given, only files matching it
        are kept.  No check for valid filenames is made.

        """
        if pat:
            def keep(name, pat = pat):
                return fnmatch.fnmatch(name, pat)
            files = filter(keep, files)
        else:
            files = files[:]
        files.sort()
        return files

    def _remove(self, fn):
        """INTERNAL: remove FILE without complaints."""
        try:
            os.unlink(fn)
        except os.error:
            pass

    def _isrcs(self, name):
        """INTERNAL: Test whether NAME ends in ',v'."""
        return name[-2:] == ',v'

#! /usr/bin/env python

"""Classes to handle Unix style, MMDF style, and MH style mailboxes."""


import rfc822
import os

__all__ = ["UnixMailbox","MmdfMailbox","MHMailbox","Maildir","BabylMailbox",
           "PortableUnixMailbox"]

class _Mailbox:

    def __init__(self, fp, factory=rfc822.Message):
        self.fp = fp
        self.seekp = 0
        self.factory = factory

    def __iter__(self):
        return iter(self.next, None)

    def next(self):
        while 1:
            self.fp.seek(self.seekp)
            try:
                self._search_start()
            except EOFError:
                self.seekp = self.fp.tell()
                return None
            start = self.fp.tell()
            self._search_end()
            self.seekp = stop = self.fp.tell()
            if start != stop:
                break
        return self.factory(_Subfile(self.fp, start, stop))


class _Subfile:

    def __init__(self, fp, start, stop):
        self.fp = fp
        self.start = start
        self.stop = stop
        self.pos = self.start


    def _read(self, length, read_function):
        if self.pos >= self.stop:
            return ''
        remaining = self.stop - self.pos
        if length is None or length < 0 or length > remaining:
            length = remaining
        self.fp.seek(self.pos)
        data = read_function(length)
        self.pos = self.fp.tell()
        return data

    def read(self, length = None):
        return self._read(length, self.fp.read)

    def readline(self, length = None):
        return self._read(length, self.fp.readline)

    def readlines(self, sizehint = -1):
        lines = []
        while 1:
            line = self.readline()
            if not line:
                break
            lines.append(line)
            if sizehint >= 0:
                sizehint = sizehint - len(line)
                if sizehint <= 0:
                    break
        return lines

    def tell(self):
        return self.pos - self.start

    def seek(self, pos, whence=0):
        if whence == 0:
            self.pos = self.start + pos
        elif whence == 1:
            self.pos = self.pos + pos
        elif whence == 2:
            self.pos = self.stop + pos

    def close(self):
        del self.fp


# Recommended to use PortableUnixMailbox instead!
class UnixMailbox(_Mailbox):

    def _search_start(self):
        while 1:
            pos = self.fp.tell()
            line = self.fp.readline()
            if not line:
                raise EOFError
            if line[:5] == 'From ' and self._isrealfromline(line):
                self.fp.seek(pos)
                return

    def _search_end(self):
        self.fp.readline()      # Throw away header line
        while 1:
            pos = self.fp.tell()
            line = self.fp.readline()
            if not line:
                return
            if line[:5] == 'From ' and self._isrealfromline(line):
                self.fp.seek(pos)
                return

    # An overridable mechanism to test for From-line-ness.  You can either
    # specify a different regular expression or define a whole new
    # _isrealfromline() method.  Note that this only gets called for lines
    # starting with the 5 characters "From ".
    #
    # BAW: According to
    #http://home.netscape.com/eng/mozilla/2.0/relnotes/demo/content-length.html
    # the only portable, reliable way to find message delimiters in a BSD (i.e
    # Unix mailbox) style folder is to search for "\n\nFrom .*\n", or at the
    # beginning of the file, "^From .*\n".  While _fromlinepattern below seems
    # like a good idea, in practice, there are too many variations for more
    # strict parsing of the line to be completely accurate.
    #
    # _strict_isrealfromline() is the old version which tries to do stricter
    # parsing of the From_ line.  _portable_isrealfromline() simply returns
    # true, since it's never called if the line doesn't already start with
    # "From ".
    #
    # This algorithm, and the way it interacts with _search_start() and
    # _search_end() may not be completely correct, because it doesn't check
    # that the two characters preceding "From " are \n\n or the beginning of
    # the file.  Fixing this would require a more extensive rewrite than is
    # necessary.  For convenience, we've added a PortableUnixMailbox class
    # which uses the more lenient _fromlinepattern regular expression.

    _fromlinepattern = r"From \s*[^\s]+\s+\w\w\w\s+\w\w\w\s+\d?\d\s+" \
                       r"\d?\d:\d\d(:\d\d)?(\s+[^\s]+)?\s+\d\d\d\d\s*$"
    _regexp = None

    def _strict_isrealfromline(self, line):
        if not self._regexp:
            import re
            self._regexp = re.compile(self._fromlinepattern)
        return self._regexp.match(line)

    def _portable_isrealfromline(self, line):
        return True

    _isrealfromline = _strict_isrealfromline


class PortableUnixMailbox(UnixMailbox):
    _isrealfromline = UnixMailbox._portable_isrealfromline


class MmdfMailbox(_Mailbox):

    def _search_start(self):
        while 1:
            line = self.fp.readline()
            if not line:
                raise EOFError
            if line[:5] == '\001\001\001\001\n':
                return

    def _search_end(self):
        while 1:
            pos = self.fp.tell()
            line = self.fp.readline()
            if not line:
                return
            if line == '\001\001\001\001\n':
                self.fp.seek(pos)
                return


class MHMailbox:

    def __init__(self, dirname, factory=rfc822.Message):
        import re
        pat = re.compile('^[1-9][0-9]*$')
        self.dirname = dirname
        # the three following lines could be combined into:
        # list = map(long, filter(pat.match, os.listdir(self.dirname)))
        list = os.listdir(self.dirname)
        list = filter(pat.match, list)
        list = map(long, list)
        list.sort()
        # This only works in Python 1.6 or later;
        # before that str() added 'L':
        self.boxes = map(str, list)
        self.boxes.reverse()
        self.factory = factory

    def __iter__(self):
        return iter(self.next, None)

    def next(self):
        if not self.boxes:
            return None
        fn = self.boxes.pop()
        fp = open(os.path.join(self.dirname, fn))
        msg = self.factory(fp)
        try:
            msg._mh_msgno = fn
        except (AttributeError, TypeError):
            pass
        return msg


class Maildir:
    # Qmail directory mailbox

    def __init__(self, dirname, factory=rfc822.Message):
        self.dirname = dirname
        self.factory = factory

        # check for new mail
        newdir = os.path.join(self.dirname, 'new')
        boxes = [os.path.join(newdir, f)
                 for f in os.listdir(newdir) if f[0] != '.']

        # Now check for current mail in this maildir
        curdir = os.path.join(self.dirname, 'cur')
        boxes += [os.path.join(curdir, f)
                  for f in os.listdir(curdir) if f[0] != '.']
        boxes.reverse()
        self.boxes = boxes

    def __iter__(self):
        return iter(self.next, None)

    def next(self):
        if not self.boxes:
            return None
        fn = self.boxes.pop()
        fp = open(fn)
        return self.factory(fp)


class BabylMailbox(_Mailbox):

    def _search_start(self):
        while 1:
            line = self.fp.readline()
            if not line:
                raise EOFError
            if line == '*** EOOH ***\n':
                return

    def _search_end(self):
        while 1:
            pos = self.fp.tell()
            line = self.fp.readline()
            if not line:
                return
            if line == '\037\014\n' or line == '\037':
                self.fp.seek(pos)
                return


def _test():
    import sys

    args = sys.argv[1:]
    if not args:
        for key in 'MAILDIR', 'MAIL', 'LOGNAME', 'USER':
            if key in os.environ:
                mbox = os.environ[key]
                break
        else:
            print "$MAIL, $LOGNAME nor $USER set -- who are you?"
            return
    else:
        mbox = args[0]
    if mbox[:1] == '+':
        mbox = os.environ['HOME'] + '/Mail/' + mbox[1:]
    elif not '/' in mbox:
        if os.path.isfile('/var/mail/' + mbox):
            mbox = '/var/mail/' + mbox
        else:
            mbox = '/usr/mail/' + mbox
    if os.path.isdir(mbox):
        if os.path.isdir(os.path.join(mbox, 'cur')):
            mb = Maildir(mbox)
        else:
            mb = MHMailbox(mbox)
    else:
        fp = open(mbox, 'r')
        mb = PortableUnixMailbox(fp)

    msgs = []
    while 1:
        msg = mb.next()
        if msg is None:
            break
        msgs.append(msg)
        if len(args) <= 1:
            msg.fp = None
    if len(args) > 1:
        num = int(args[1])
        print 'Message %d body:'%num
        msg = msgs[num-1]
        msg.rewindbody()
        sys.stdout.write(msg.fp.read())
    else:
        print 'Mailbox',mbox,'has',len(msgs),'messages:'
        for msg in msgs:
            f = msg.getheader('from') or ""
            s = msg.getheader('subject') or ""
            d = msg.getheader('date') or ""
            print '-%20.20s   %20.20s   %-30.30s'%(f, d[5:], s)


if __name__ == '__main__':
    _test()

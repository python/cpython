"""Generic MIME parser.

Classes:

        MimeParser - Generic MIME parser.

Exceptions:

        MimeError - Exception raised by MimeParser class.

XXX To do:

- Content-transfer-encoding issues
- Use Content-length header in rawbody()?
- Cache parts instead of reparsing each time
- The message strings in exceptions could use some work

"""

from types import *                     # Python types, not MIME types :-)
import string
import regex
import SubFile
import mimetools


MimeError = "MimeParser.MimeError"      # Exception raised by this class


class MimeParser:

    """Generic MIME parser.

    This requires a seekable file.

    """

    def __init__(self, fp):
        """Constructor: store the file pointer and parse the headers."""
        self._fp = fp
        self._start = fp.tell()
        self._headers = h = mimetools.Message(fp)
        self._bodystart = fp.tell()
        self._multipart = h.getmaintype() == 'multipart'

    def multipart(self):
        """Return whether this is a multipart message."""
        return self._multipart

    def headers(self):
        """Return the headers of the MIME message, as a Message object."""
        return self._headers

    def rawbody(self):
        """Return the raw body of the MIME message, as a file-like object.

        This is a fairly low-level interface -- for a multipart
        message, you'd have to parse the body yourself, and it doesn't
        translate the Content-transfer-encoding.
        
        """
        # XXX Use Content-length to set end if it exists?
        return SubFile.SubFile(self._fp, self._bodystart)

    def body(self):
        """Return the body of a 1-part MIME message, as a file-like object.

        This should interpret the Content-transfer-encoding, if any
        (XXX currently it doesn't).
        
        """
        if self._multipart:
            raise MimeError, "body() only works for 1-part messages"
        return self.rawbody()

    _re_content_length = regex.compile('content-length:[ \t]*\([0-9]+\)',
                                       regex.casefold)

    def rawparts(self):
        """Return the raw body parts of a multipart MIME message.

        This returns a list of SubFile() objects corresponding to the
        parts.  Note that the phantom part before the first separator
        is returned too, as list item 0.  If the final part is not
        followed by a terminator, it is ignored, and this error is not
        reported.  (XXX: the error should be raised).

        """
        if not self._multipart:
            raise MimeError, "[raw]parts() only works for multipart messages"
        h = self._headers
        separator = h.getparam('boundary')
        if not separator:
            raise MimeError, "multipart boundary not specified"
        separator = "--" + separator
        terminator = separator + "--"
        ns = len(separator)
        list = []
        f = self._fp
        start = f.tell()
        clength = -1
        bodystart = -1
        inheaders = 0
        while 1:
            end = f.tell()
            line = f.readline()
            if not line:
                break
            if line[:2] != "--" or line[:ns] != separator:
                if inheaders:
                    re = self._re_content_length
                    if re.match(line) > 0:
                        try:
                            clength = string.atoi(re.group(1))
                        except string.atoi_error:
                            pass
                    if not string.strip(line):
                        inheaders = 0
                        bodystart = f.tell()
                        if clength > 0:
                            # Skip binary data
                            f.read(clength)
                continue
            line = string.strip(line)
            if line == terminator or line == separator:
                if clength >= 0:
                    # The Content-length header determines the subfile size
                    end = bodystart + clength
                else:
                    # The final newline is not part of the content
                    end = end-1
                list.append(SubFile.SubFile(f, start, end))
                start = f.tell()
                clength = -1
                inheaders = 1
                if line == terminator:
                    break
        return list

    def parts(self):
        """Return the parsed body parts of a multipart MIME message.

        This returns a list of MimeParser() instances corresponding to
        the parts.  The phantom part before the first separator is not
        included.

        """
        return map(MimeParser, self.rawparts()[1:])

    def getsubpartbyposition(self, indices):
        part = self
        for i in indices:
            part = part.parts()[i]
        return part

    def getsubpartbyid(self, id):
        h = self._headers
        cid = h.getheader('content-id')
        if cid and cid == id:
            return self
        if self._multipart:
            for part in self.parts():
                parser = MimeParser(part)
                hit = parser.getsubpartbyid(id)
                if hit:
                    return hit
        return None

    def index(self):
        """Return an index of the MIME file.

        This parses the entire file and returns index information
        about it, in the form of a tuple

            (ctype, headers, body)

        where 'ctype' is the content type string of the message
        (e.g. `text/plain' or `multipart/mixed') and 'headers' is a
        Message instance containing the message headers (which should
        be treated as read-only).

        The 'body' item depends on the content type:

        - If it is an atomic message (anything except for content type
          multipart/*), it is the file-like object returned by
          self.body().

        - For a content type of multipart/*, it is the list of
          MimeParser() objects returned by self.parts().

        """
        if self._multipart:
            body = self.parts()
        else:
            body = self.body()
        return self._headers.gettype(), self._headers, body


def _show(parser, level=0):
    """Helper for _test()."""
    ctype, headers, body = parser.index()
    print ctype,
    if type(body) == ListType:
        nparts = len(body)
        print "(%d part%s):" % (nparts, nparts != 1 and "s" or "")
        n = 0
        for part in body:
            n = n+1
            print "%*d." % (4*level+2, n),
            _show(part, level+1)
    else:
        bodylines = body.readlines()
        print "(%d header lines, %d body lines)" % (
            len(headers.headers), len(bodylines))
        for line in headers.headers + ['\n'] + bodylines:
            if line[-1:] == '\n': line = line[:-1]
            print "    "*level + line

def _test(args = None):
    """Test program invoked when run as a script.

    When a filename argument is specified, it reads from that file.
    When no arguments are present, it defaults to 'testkp.txt' if it
    exists, else it defaults to stdin.

    """
    if not args:
        import sys
        args = sys.argv[1:]
    if args:
        fn = args[0]
    else:
        import os
        fn = 'testkp.txt'
        if not os.path.exists(fn):
            fn = '-'
    if fn == '-':
        fp = sys.stdin
    else:
        fp = open(fn)
    mp = MimeParser(fp)
    _show(mp)

if __name__ == '__main__':
    import sys
    _test()

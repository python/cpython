"""Generic MIME writer.

Classes:

MimeWriter - the only thing here.

"""


import string
import mimetools


class MimeWriter:

    """Generic MIME writer.

    Methods:

    __init__()
    addheader()
    flushheaders()
    startbody()
    startmultipartbody()
    nextpart()
    lastpart()

    A MIME writer is much more primitive than a MIME parser.  It
    doesn't seek around on the output file, and it doesn't use large
    amounts of buffer space, so you have to write the parts in the
    order they should occur on the output file.  It does buffer the
    headers you add, allowing you to rearrange their order.
    
    General usage is:

    f = <open the output file>
    w = MimeWriter(f)
    ...call w.addheader(key, value) 0 or more times...

    followed by either:

    f = w.startbody(content_type)
    ...call f.write(data) for body data...

    or:

    w.startmultipartbody(subtype)
    for each part:
        subwriter = w.nextpart()
        ...use the subwriter's methods to create the subpart...
    w.lastpart()

    The subwriter is another MimeWriter instance, and should be
    treated in the same way as the toplevel MimeWriter.  This way,
    writing recursive body parts is easy.

    Warning: don't forget to call lastpart()!

    XXX There should be more state so calls made in the wrong order
    are detected.

    Some special cases:

    - startbody() just returns the file passed to the constructor;
      but don't use this knowledge, as it may be changed.

    - startmultipartbody() actually returns a file as well;
      this can be used to write the initial 'if you can read this your
      mailer is not MIME-aware' message.

    - If you call flushheaders(), the headers accumulated so far are
      written out (and forgotten); this is useful if you don't need a
      body part at all, e.g. for a subpart of type message/rfc822
      that's (mis)used to store some header-like information.

    - Passing a keyword argument 'prefix=<flag>' to addheader(),
      start*body() affects where the header is inserted; 0 means
      append at the end, 1 means insert at the start; default is
      append for addheader(), but insert for start*body(), which use
      it to determine where the Content-Type header goes.

    """

    def __init__(self, fp):
        self._fp = fp
        self._headers = []

    def addheader(self, key, value, prefix=0):
        lines = string.splitfields(value, "\n")
        while lines and not lines[-1]: del lines[-1]
        while lines and not lines[0]: del lines[0]
        for i in range(1, len(lines)):
            lines[i] = "    " + string.strip(lines[i])
        value = string.joinfields(lines, "\n") + "\n"
        line = key + ": " + value
        if prefix:
            self._headers.insert(0, line)
        else:
            self._headers.append(line)

    def flushheaders(self):
        self._fp.writelines(self._headers)
        self._headers = []

    def startbody(self, ctype, plist=[], prefix=1):
        for name, value in plist:
            ctype = ctype + ';\n %s=\"%s\"' % (name, value)
        self.addheader("Content-Type", ctype, prefix=prefix)
        self.flushheaders()
        self._fp.write("\n")
        return self._fp

    def startmultipartbody(self, subtype, boundary=None, plist=[], prefix=1):
        self._boundary = boundary or mimetools.choose_boundary()
        return self.startbody("multipart/" + subtype,
                              [("boundary", self._boundary)] + plist,
                              prefix=prefix)

    def nextpart(self):
        self._fp.write("\n--" + self._boundary + "\n")
        return self.__class__(self._fp)

    def lastpart(self):
        self._fp.write("\n--" + self._boundary + "--\n")


if __name__ == '__main__':
    import test.test_MimeWriter

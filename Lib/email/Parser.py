# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""A parser of RFC 2822 and MIME email messages.
"""

import re
from cStringIO import StringIO
from types import ListType

from email import Errors
from email import Message

EMPTYSTRING = ''
NL = '\n'

try:
    True, False
except NameError:
    True = 1
    False = 0

NLCRE = re.compile('\r\n|\r|\n')

class TextUtil:
    """ A utility class for wrapping a file object and providing a 
        couple of additional useful functions.
    """

    def __init__(self, fp):
        self.fp = fp
        self.unread = []

    def readline(self):
        """ Return a line of data.

        If data has been pushed back with unreadline(), the most recently
        returned unreadline()d data will be returned.
        """
        if self.unread:
            return self.unread.pop()
        else:
            return self.fp.readline()

    def unreadline(self, line):
        """Push a line back into the object. 
        """
        self.unread.append(line)

    def peekline(self):
        """Non-destructively look at the next line"""
        line = self.readline()
        self.unreadline(line)
        return line

    def read(self):
        """Return the remaining data
        """
        r = self.fp.read()
        if self.unread:
            r = "\n".join(self.unread) + r
            self.unread = []
        return r

    def readuntil(self, re, afterblank=0, includematch=0):
        """Read a line at a time until we get the specified RE. 

        Returns the text up to (and including, if includematch is true) the 
        matched text, and the RE match object. If afterblank is true, 
        there must be a blank line before the matched text. Moves current 
        filepointer to the line following the matched line. If we reach 
        end-of-file, return what we've got so far, and return None as the
        RE match object.
        """
        prematch = []
        blankseen = 0
        while 1:
            line = self.readline()
            if not line:
                # end of file
                return EMPTYSTRING.join(prematch), None
            if afterblank:
                if NLCRE.match(line):
                    blankseen = 1
                    continue
                else:
                    blankseen = 0
            m = re.match(line)
            if (m and not afterblank) or (m and afterblank and blankseen):
                if includematch:
                    prematch.append(line)
                return EMPTYSTRING.join(prematch), m
            prematch.append(line)


class Parser:
    def __init__(self, _class=Message.Message, strict=False):
        """Parser of RFC 2822 and MIME email messages.

        Creates an in-memory object tree representing the email message, which
        can then be manipulated and turned over to a Generator to return the
        textual representation of the message.

        The string must be formatted as a block of RFC 2822 headers and header
        continuation lines, optionally preceeded by a `Unix-from' header.  The
        header block is terminated either by the end of the string or by a
        blank line.

        _class is the class to instantiate for new message objects when they
        must be created.  This class must have a constructor that can take
        zero arguments.  Default is Message.Message.

        Optional strict tells the parser to be strictly RFC compliant or to be
        more forgiving in parsing of ill-formatted MIME documents.  When
        non-strict mode is used, the parser will try to make up for missing or
        erroneous boundaries and other peculiarities seen in the wild.
        Default is non-strict parsing.
        """
        self._class = _class
        self._strict = strict

    def parse(self, fp, headersonly=False):
        """Create a message structure from the data in a file.

        Reads all the data from the file and returns the root of the message
        structure.  Optional headersonly is a flag specifying whether to stop
        parsing after reading the headers or not.  The default is False,
        meaning it parses the entire contents of the file.
        """
        root = self._class()
        fp = TextUtil(fp)
        self._parseheaders(root, fp)
        if not headersonly:
            obj = self._parsemessage(root, fp)
            trailer = fp.read()
            if obj and trailer:
                self._attach_trailer(obj, trailer)
        return root

    def parsestr(self, text, headersonly=False):
        """Create a message structure from a string.

        Returns the root of the message structure.  Optional headersonly is a
        flag specifying whether to stop parsing after reading the headers or
        not.  The default is False, meaning it parses the entire contents of
        the file.
        """
        return self.parse(StringIO(text), headersonly=headersonly)

    def _parseheaders(self, container, fp):
        # Parse the headers, returning a list of header/value pairs.  None as
        # the header means the Unix-From header.
        lastheader = ''
        lastvalue = []
        lineno = 0
        while True:
            # Don't strip the line before we test for the end condition,
            # because whitespace-only header lines are RFC compliant
            # continuation lines.
            line = fp.readline()
            if not line:
                break
            line = line.splitlines()[0]
            if not line:
                break
            # Ignore the trailing newline
            lineno += 1
            # Check for initial Unix From_ line
            if line.startswith('From '):
                if lineno == 1:
                    container.set_unixfrom(line)
                    continue
                elif self._strict:
                    raise Errors.HeaderParseError(
                        'Unix-from in headers after first rfc822 header')
                else:
                    # ignore the wierdly placed From_ line
                    # XXX: maybe set unixfrom anyway? or only if not already?
                    continue
            # Header continuation line
            if line[0] in ' \t':
                if not lastheader:
                    raise Errors.HeaderParseError(
                        'Continuation line seen before first header')
                lastvalue.append(line)
                continue
            # Normal, non-continuation header.  BAW: this should check to make
            # sure it's a legal header, e.g. doesn't contain spaces.  Also, we
            # should expose the header matching algorithm in the API, and
            # allow for a non-strict parsing mode (that ignores the line
            # instead of raising the exception).
            i = line.find(':')
            if i < 0:
                if self._strict:
                    raise Errors.HeaderParseError(
                        "Not a header, not a continuation: ``%s''" % line)
                elif lineno == 1 and line.startswith('--'):
                    # allow through duplicate boundary tags.
                    continue
                else:
                    # There was no separating blank line as mandated by RFC
                    # 2822, but we're in non-strict mode.  So just offer up
                    # this current line as the first body line.
                    fp.unreadline(line)
                    break
            if lastheader:
                container[lastheader] = NL.join(lastvalue)
            lastheader = line[:i]
            lastvalue = [line[i+1:].lstrip()]
        # Make sure we retain the last header
        if lastheader:
            container[lastheader] = NL.join(lastvalue)
        return 

    def _parsemessage(self, container, fp):
        # Parse the body. We walk through the body from top to bottom,
        # keeping track of the current multipart nesting as we go.
        # We return the object that gets the data at the end of this 
        # block.
        boundary = container.get_boundary()
        isdigest = (container.get_content_type() == 'multipart/digest')
        if boundary: 
            separator = '--' + boundary
            boundaryRE = re.compile(
                    r'(?P<sep>' + re.escape(separator) + 
                    r')(?P<end>--)?(?P<ws>[ \t]*)(?P<linesep>\r\n|\r|\n)$')
            preamble, matchobj = fp.readuntil(boundaryRE)
            if not matchobj:
                # Broken - we hit the end of file. Just set the body 
                # to the text.
                container.set_payload(preamble)
                return container
            if preamble:
                container.preamble = preamble
            else:
                # The module docs specify an empty preamble is None, not ''
                container.preamble = None
            while 1:
                subobj = self._class()
                if isdigest:
                    subobj.set_default_type('message/rfc822')
                    firstline = fp.peekline()
                    if firstline.strip():
                        # we have MIME headers. all good. 
                        self._parseheaders(subobj, fp)
                    else:
                        # no MIME headers. this is allowed for multipart/digest
                        # Consume the extra blank line
                        fp.readline()
                        pass
                else:
                    self._parseheaders(subobj, fp)
                container.attach(subobj)
                maintype = subobj.get_content_maintype()
                hassubparts = (subobj.get_content_maintype() in 
                                                ( "message", "multipart" ))
                if hassubparts:
                    subobj = self._parsemessage(subobj, fp)

                trailer, matchobj = fp.readuntil(boundaryRE)
                if matchobj is None or trailer:
                    mo = re.search('(?P<sep>\r\n|\r|\n){2}$', trailer)
                    if not mo:
                        mo = re.search('(?P<sep>\r\n|\r|\n)$', trailer)
                        if not mo:
                            raise Errors.BoundaryError(
                          'No terminating boundary and no trailing empty line')
                    linesep = mo.group('sep')
                    trailer = trailer[:-len(linesep)]
                if trailer:
                    self._attach_trailer(subobj, trailer)
                if matchobj is None or matchobj.group('end'):
                    # That was the last piece of data. Let our caller attach
                    # the epilogue to us. But before we do that, push the
                    # line ending of the match group back into the readline
                    # buffer, as it's part of the epilogue.
                    if matchobj:
                        fp.unreadline(matchobj.group('linesep'))
                    return container

        elif container.get_content_maintype() == "multipart":
            # Very bad.  A message is a multipart with no boundary!
            raise Errors.BoundaryError(
                    'multipart message with no defined boundary')
        elif container.get_content_maintype() == "message":
            ct = container.get_content_type()
            if ct == "message/rfc822":
                submessage = self._class()
                self._parseheaders(submessage, fp)
                self._parsemessage(submessage, fp)
                container.attach(submessage)
                return submessage
            elif ct == "message/delivery-status":
                # This special kind of type contains blocks of headers 
                # separated by a blank line.  We'll represent each header 
                # block as a separate Message object
                while 1:
                    nextblock = self._class()
                    self._parseheaders(nextblock, fp)
                    container.attach(nextblock)
                    # next peek ahead to see whether we've hit the end or not
                    nextline = fp.peekline()
                    if nextline[:2] == "--":
                        break
                return container
            else:
                # Other sort of message object (e.g. external-body)
                msg = self._class()
                self._parsemessage(msg, fp)
                container.attach(msg)
                return msg
        else:
            # single body section. We let our caller set the payload.
            return container

    def _attach_trailer(self, obj, trailer):
        if obj.get_content_maintype() in ("message", "multipart"):
            obj.epilogue = trailer
        else:
            obj.set_payload(trailer)


class HeaderParser(Parser):
    """A subclass of Parser, this one only meaningfully parses message headers.

    This class can be used if all you're interested in is the headers of a
    message.  While it consumes the message body, it does not parse it, but
    simply makes it available as a string payload.

    Parsing with this subclass can be considerably faster if all you're
    interested in is the message headers.
    """
    def _parsemessage(self, container, fp):
        # Consume but do not parse, the body
        text = fp.read()
        container.set_payload(text)
        return None

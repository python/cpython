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



class Parser:
    def __init__(self, _class=Message.Message):
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
        """
        self._class = _class

    def parse(self, fp):
        root = self._class()
        self._parseheaders(root, fp)
        self._parsebody(root, fp)
        return root

    def parsestr(self, text):
        return self.parse(StringIO(text))

    def _parseheaders(self, container, fp):
        # Parse the headers, returning a list of header/value pairs.  None as
        # the header means the Unix-From header.
        lastheader = ''
        lastvalue = []
        lineno = 0
        while 1:
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
                else:
                    raise Errors.HeaderParseError(
                        'Unix-from in headers after first rfc822 header')
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
                raise Errors.HeaderParseError(
                    'Not a header, not a continuation')
            if lastheader:
                container[lastheader] = NL.join(lastvalue)
            lastheader = line[:i]
            lastvalue = [line[i+1:].lstrip()]
        # Make sure we retain the last header
        if lastheader:
            container[lastheader] = NL.join(lastvalue)

    def _parsebody(self, container, fp):
        # Parse the body, but first split the payload on the content-type
        # boundary if present.
        boundary = container.get_boundary()
        isdigest = (container.get_type() == 'multipart/digest')
        # If there's a boundary, split the payload text into its constituent
        # parts and parse each separately.  Otherwise, just parse the rest of
        # the body as a single message.  Note: any exceptions raised in the
        # recursive parse need to have their line numbers coerced.
        if boundary:
            preamble = epilogue = None
            # Split into subparts.  The first boundary we're looking for won't
            # have the leading newline since we're at the start of the body
            # text.
            separator = '--' + boundary
            payload = fp.read()
            start = payload.find(separator)
            if start < 0:
                raise Errors.BoundaryError(
                    "Couldn't find starting boundary: %s" % boundary)
            if start > 0:
                # there's some pre-MIME boundary preamble
                preamble = payload[0:start]
            # Find out what kind of line endings we're using
            start += len(separator)
            cre = re.compile('\r\n|\r|\n')
            mo = cre.search(payload, start)
            if mo:
                start += len(mo.group(0)) * (1 + isdigest)
            # We create a compiled regexp first because we need to be able to
            # specify the start position, and the module function doesn't
            # support this signature. :(
            cre = re.compile('(?P<sep>\r\n|\r|\n)' +
                             re.escape(separator) + '--')
            mo = cre.search(payload, start)
            if not mo:
                raise Errors.BoundaryError(
                    "Couldn't find terminating boundary: %s" % boundary)
            terminator = mo.start()
            linesep = mo.group('sep')
            if mo.end() < len(payload):
                # there's some post-MIME boundary epilogue
                epilogue = payload[mo.end():]
            # We split the textual payload on the boundary separator, which
            # includes the trailing newline.  If the container is a
            # multipart/digest then the subparts are by default message/rfc822
            # instead of text/plain.  In that case, they'll have an extra
            # newline before the headers to distinguish the message's headers
            # from the subpart headers.
            separator += linesep * (1 + isdigest)
            parts = payload[start:terminator].split(linesep + separator)
            for part in parts:
                msgobj = self.parsestr(part)
                container.preamble = preamble
                container.epilogue = epilogue
                container.attach(msgobj)
        elif container.get_main_type() == 'multipart':
            # Very bad.  A message is a multipart with no boundary!
            raise Errors.BoundaryError(
                'multipart message with no defined boundary')
        elif container.get_type() == 'message/delivery-status':
            # This special kind of type contains blocks of headers separated
            # by a blank line.  We'll represent each header block as a
            # separate Message object
            blocks = []
            while 1:
                blockmsg = self._class()
                self._parseheaders(blockmsg, fp)
                if not len(blockmsg):
                    # No more header blocks left
                    break
                blocks.append(blockmsg)
            container.set_payload(blocks)
        elif container.get_main_type() == 'message':
            # Create a container for the payload, but watch out for there not
            # being any headers left
            try:
                msg = self.parse(fp)
            except Errors.HeaderParseError:
                msg = self._class()
                self._parsebody(msg, fp)
            container.attach(msg)
        else:
            container.set_payload(fp.read())



class HeaderParser(Parser):
    """A subclass of Parser, this one only meaningfully parses message headers.

    This class can be used if all you're interested in is the headers of a
    message.  While it consumes the message body, it does not parse it, but
    simply makes it available as a string payload.

    Parsing with this subclass can be considerably faster if all you're
    interested in is the message headers.
    """
    def _parsebody(self, container, fp):
        # Consume but do not parse, the body
        container.set_payload(fp.read())

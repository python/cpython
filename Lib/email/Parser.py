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
        firstbodyline = self._parseheaders(root, fp)
        if not headersonly:
            self._parsebody(root, fp, firstbodyline)
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
        firstbodyline = None
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
                    firstbodyline = line
                    break
            if lastheader:
                container[lastheader] = NL.join(lastvalue)
            lastheader = line[:i]
            lastvalue = [line[i+1:].lstrip()]
        # Make sure we retain the last header
        if lastheader:
            container[lastheader] = NL.join(lastvalue)
        return firstbodyline

    def _parsebody(self, container, fp, firstbodyline=None):
        # Parse the body, but first split the payload on the content-type
        # boundary if present.
        boundary = container.get_boundary()
        isdigest = (container.get_content_type() == 'multipart/digest')
        # If there's a boundary, split the payload text into its constituent
        # parts and parse each separately.  Otherwise, just parse the rest of
        # the body as a single message.  Note: any exceptions raised in the
        # recursive parse need to have their line numbers coerced.
        if boundary:
            preamble = epilogue = None
            # Split into subparts.  The first boundary we're looking for won't
            # always have a leading newline since we're at the start of the
            # body text, and there's not always a preamble before the first
            # boundary.
            separator = '--' + boundary
            payload = fp.read()
            if firstbodyline is not None:
                payload = firstbodyline + '\n' + payload
            # We use an RE here because boundaries can have trailing
            # whitespace.
            mo = re.search(
                r'(?P<sep>' + re.escape(separator) + r')(?P<ws>[ \t]*)',
                payload)
            if not mo:
                if self._strict:
                    raise Errors.BoundaryError(
                        "Couldn't find starting boundary: %s" % boundary)
                container.set_payload(payload)
                return
            start = mo.start()
            if start > 0:
                # there's some pre-MIME boundary preamble
                preamble = payload[0:start]
            # Find out what kind of line endings we're using
            start += len(mo.group('sep')) + len(mo.group('ws'))
            mo = NLCRE.search(payload, start)
            if mo:
                start += len(mo.group(0))
            # We create a compiled regexp first because we need to be able to
            # specify the start position, and the module function doesn't
            # support this signature. :(
            cre = re.compile('(?P<sep>\r\n|\r|\n)' +
                             re.escape(separator) + '--')
            mo = cre.search(payload, start)
            if mo:
                terminator = mo.start()
                linesep = mo.group('sep')
                if mo.end() < len(payload):
                    # There's some post-MIME boundary epilogue
                    epilogue = payload[mo.end():]
            elif self._strict:
                raise Errors.BoundaryError(
                        "Couldn't find terminating boundary: %s" % boundary)
            else:
                # Handle the case of no trailing boundary.  Check that it ends
                # in a blank line.  Some cases (spamspamspam) don't even have
                # that!
                mo = re.search('(?P<sep>\r\n|\r|\n){2}$', payload)
                if not mo:
                    mo = re.search('(?P<sep>\r\n|\r|\n)$', payload)
                    if not mo:
                        raise Errors.BoundaryError(
                          'No terminating boundary and no trailing empty line')
                linesep = mo.group('sep')
                terminator = len(payload)
            # We split the textual payload on the boundary separator, which
            # includes the trailing newline. If the container is a
            # multipart/digest then the subparts are by default message/rfc822
            # instead of text/plain.  In that case, they'll have a optional
            # block of MIME headers, then an empty line followed by the
            # message headers.
            parts = re.split(
                linesep + re.escape(separator) + r'[ \t]*' + linesep,
                payload[start:terminator])
            for part in parts:
                if isdigest:
                    if part.startswith(linesep):
                        # There's no header block so create an empty message
                        # object as the container, and lop off the newline so
                        # we can parse the sub-subobject
                        msgobj = self._class()
                        part = part[len(linesep):]
                    else:
                        parthdrs, part = part.split(linesep+linesep, 1)
                        # msgobj in this case is the "message/rfc822" container
                        msgobj = self.parsestr(parthdrs, headersonly=1)
                    # while submsgobj is the message itself
                    msgobj.set_default_type('message/rfc822')
                    maintype = msgobj.get_content_maintype()
                    if maintype in ('message', 'multipart'):
                        submsgobj = self.parsestr(part)
                        msgobj.attach(submsgobj)
                    else:
                        msgobj.set_payload(part)
                else:
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
            while True:
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
            text = fp.read()
            if firstbodyline is not None:
                text = firstbodyline + '\n' + text
            container.set_payload(text)



class HeaderParser(Parser):
    """A subclass of Parser, this one only meaningfully parses message headers.

    This class can be used if all you're interested in is the headers of a
    message.  While it consumes the message body, it does not parse it, but
    simply makes it available as a string payload.

    Parsing with this subclass can be considerably faster if all you're
    interested in is the message headers.
    """
    def _parsebody(self, container, fp, firstbodyline=None):
        # Consume but do not parse, the body
        text = fp.read()
        if firstbodyline is not None:
            text = firstbodyline + '\n' + text
        container.set_payload(text)

# Copyright (C) 2001-2004 Python Software Foundation
# Author: barry@python.org (Barry Warsaw)

"""email package exception classes."""



class MessageError(Exception):
    """Base class for errors in the email package."""


class MessageParseError(MessageError):
    """Base class for message parsing errors."""


class HeaderParseError(MessageParseError):
    """Error while parsing headers."""


class BoundaryError(MessageParseError):
    """Couldn't find terminating boundary."""


class MultipartConversionError(MessageError, TypeError):
    """Conversion to a multipart is prohibited."""



# These are parsing defects which the parser was able to work around.
class MessageDefect:
    """Base class for a message defect."""

    def __init__(self, line=None):
        self.line = line

class NoBoundaryInMultipart(MessageDefect):
    """A message claimed to be a multipart but had no boundary parameter."""

class StartBoundaryNotFound(MessageDefect):
    """The claimed start boundary was never found."""

class FirstHeaderLineIsContinuation(MessageDefect):
    """A message had a continuation line as its first header line."""

class MisplacedEnvelopeHeader(MessageDefect):
    """A 'Unix-from' header was found in the middle of a header block."""

class MalformedHeader(MessageDefect):
    """Found a header that was missing a colon, or was otherwise malformed"""

# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""email package exception classes.
"""



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

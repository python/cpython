# Copyright (C) 2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Base class for MIME type messages that are not multipart.
"""

from email import Errors
from email import MIMEBase



class MIMENonMultipart(MIMEBase.MIMEBase):
    """Base class for MIME multipart/* type messages."""

    __pychecker__ = 'unusednames=payload'

    def attach(self, payload):
        # The public API prohibits attaching multiple subparts to MIMEBase
        # derived subtypes since none of them are, by definition, of content
        # type multipart/*
        raise Errors.MultipartConversionError(
            'Cannot attach additional subparts to non-multipart/*')

    del __pychecker__

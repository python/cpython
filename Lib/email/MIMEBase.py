# Copyright (C) 2001-2004 Python Software Foundation
# Author: Barry Warsaw
# Contact: email-sig@python.org

"""Base class for MIME specializations."""

from email import Message



class MIMEBase(Message.Message):
    """Base class for MIME specializations."""

    def __init__(self, _maintype, _subtype, **_params):
        """This constructor adds a Content-Type: and a MIME-Version: header.

        The Content-Type: header is taken from the _maintype and _subtype
        arguments.  Additional parameters for this header are taken from the
        keyword arguments.
        """
        Message.Message.__init__(self)
        ctype = '%s/%s' % (_maintype, _subtype)
        self.add_header('Content-Type', ctype, **_params)
        self['MIME-Version'] = '1.0'

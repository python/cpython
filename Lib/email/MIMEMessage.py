# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Class representing message/* MIME documents.
"""

import Message
import MIMEBase



class MIMEMessage(MIMEBase.MIMEBase):
    """Class representing message/* MIME documents."""

    def __init__(self, _msg, _subtype='rfc822'):
        """Create a message/* type MIME document.

        _msg is a message object and must be an instance of Message, or a
        derived class of Message, otherwise a TypeError is raised.

        Optional _subtype defines the subtype of the contained message.  The
        default is "rfc822" (this is defined by the MIME standard, even though
        the term "rfc822" is technically outdated by RFC 2822).
        """
        MIMEBase.MIMEBase.__init__(self, 'message', _subtype)
        if not isinstance(_msg, Message.Message):
            raise TypeError, 'Argument is not an instance of Message'
        self.set_payload(_msg)

# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Class for generating message/rfc822 MIME documents.
"""

import Message
import MIMEBase



class MessageRFC822(MIMEBase.MIMEBase):
    """Class for generating message/rfc822 MIME documents."""

    def __init__(self, _msg):
        """Create a message/rfc822 type MIME document.

        _msg is a message object and must be an instance of Message, or a
        derived class of Message, otherwise a TypeError is raised.
        """
        MIMEBase.MIMEBase.__init__(self, 'message', 'rfc822')
        if not isinstance(_msg, Message.Message):
            raise TypeError, 'Argument is not an instance of Message'
        self.set_payload(_msg)

# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Class representing text/* type MIME documents.
"""

import MIMEBase
from Encoders import encode_7or8bit



class Text(MIMEBase.MIMEBase):
    """Class for generating text/* type MIME documents."""

    def __init__(self, _text, _minor='plain', _charset='us-ascii',
                 _encoder=encode_7or8bit):
        """Create a text/* type MIME document.

        _text is the string for this message object.  If the text does not end
        in a newline, one is added.

        _minor is the minor content type, defaulting to "plain".

        _charset is the character set parameter added to the Content-Type:
        header.  This defaults to "us-ascii".

        _encoder is a function which will perform the actual encoding for
        transport of the text data.  It takes one argument, which is this
        Text instance.  It should use get_payload() and set_payload() to
        change the payload to the encoded form.  It should also add any
        Content-Transfer-Encoding: or other headers to the message as
        necessary.  The default encoding doesn't actually modify the payload,
        but it does set Content-Transfer-Encoding: to either `7bit' or `8bit'
        as appropriate.
        """
        MIMEBase.MIMEBase.__init__(self, 'text', _minor,
                                   **{'charset': _charset})
        if _text and _text[-1] <> '\n':
            _text += '\n'
        self.set_payload(_text)
        _encoder(self)

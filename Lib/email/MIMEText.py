# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Class representing text/* type MIME documents.
"""

import warnings
from email.MIMENonMultipart import MIMENonMultipart
from email.Encoders import encode_7or8bit



class MIMEText(MIMENonMultipart):
    """Class for generating text/* type MIME documents."""

    def __init__(self, _text, _subtype='plain', _charset='us-ascii',
                 _encoder=None):
        """Create a text/* type MIME document.

        _text is the string for this message object.

        _subtype is the MIME sub content type, defaulting to "plain".

        _charset is the character set parameter added to the Content-Type
        header.  This defaults to "us-ascii".  Note that as a side-effect, the
        Content-Transfer-Encoding header will also be set.

        The use of the _encoder is deprecated.  The encoding of the payload,
        and the setting of the character set parameter now happens implicitly
        based on the _charset argument.  If _encoder is supplied, then a
        DeprecationWarning is used, and the _encoder functionality may
        override any header settings indicated by _charset.  This is probably
        not what you want.
        """
        MIMENonMultipart.__init__(self, 'text', _subtype,
                                  **{'charset': _charset})
        self.set_payload(_text, _charset)
        if _encoder is not None:
            warnings.warn('_encoder argument is obsolete.',
                          DeprecationWarning, 2)
            # Because set_payload() with a _charset will set its own
            # Content-Transfer-Encoding header, we need to delete the
            # existing one or will end up with two of them. :(
            del self['content-transfer-encoding']
            _encoder(self)

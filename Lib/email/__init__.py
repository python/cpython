# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""A package for parsing, handling, and generating email messages.
"""

__version__ = '2.2'

__all__ = ['Charset',
           'Encoders',
           'Errors',
           'Generator',
           'Header',
           'Iterators',
           'MIMEAudio',
           'MIMEBase',
           'MIMEImage',
           'MIMEMessage',
           'MIMEText',
           'Message',
           'Parser',
           'Utils',
           'base64MIME',
           'quopriMIME',
           'message_from_string',
           'message_from_file',
           ]



# Some convenience routines
from email.Parser import Parser as _Parser
from email.Message import Message as _Message

def message_from_string(s, _class=_Message, strict=0):
    return _Parser(_class, strict=strict).parsestr(s)

def message_from_file(fp, _class=_Message, strict=0):
    return _Parser(_class, strict=strict).parse(fp)

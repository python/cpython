# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""A package for parsing, handling, and generating email messages.
"""

__version__ = '2.0'

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
from Parser import Parser as _Parser
from Message import Message as _Message

def message_from_string(s, _class=_Message):
    return _Parser(_class).parsestr(s)

def message_from_file(fp, _class=_Message):
    return _Parser(_class).parse(fp)

# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""A package for parsing, handling, and generating email messages.
"""

__version__ = '1.0'

__all__ = ['Encoders',
           'Errors',
           'Generator',
           'Image',
           'Iterators',
           'MIMEBase',
           'Message',
           'MessageRFC822',
           'Parser',
           'Text',
           'Utils',
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

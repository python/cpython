# Copyright (C) 2001-2006 Python Software Foundation
# Author: barry@python.org (Barry Warsaw)

"""A package for parsing, handling, and generating email messages."""

__version__ = '2.5.8'

__all__ = [
    'base64MIME',
    'Charset',
    'Encoders',
    'Errors',
    'Generator',
    'Header',
    'Iterators',
    'Message',
    'MIMEAudio',
    'MIMEBase',
    'MIMEImage',
    'MIMEMessage',
    'MIMEMultipart',
    'MIMENonMultipart',
    'MIMEText',
    'Parser',
    'quopriMIME',
    'Utils',
    'message_from_string',
    'message_from_file',
    ]

try:
    True, False
except NameError:
    True = 1
    False = 0



# Some convenience routines.  Don't import Parser and Message as side-effects
# of importing email since those cascadingly import most of the rest of the
# email package.
def message_from_string(s, _class=None, strict=False):
    """Parse a string into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.Parser import Parser
    if _class is None:
        from email.Message import Message
        _class = Message
    return Parser(_class, strict=strict).parsestr(s)

def message_from_file(fp, _class=None, strict=False):
    """Read a file and parse its contents into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.Parser import Parser
    if _class is None:
        from email.Message import Message
        _class = Message
    return Parser(_class, strict=strict).parse(fp)



# Patch encodings.aliases to recognize 'ansi_x3.4_1968' which isn't a standard
# alias in Python 2.1.3, but is used by the email package test suite.
from encodings.aliases import aliases # The aliases dictionary
if not aliases.has_key('ansi_x3.4_1968'):
    aliases['ansi_x3.4_1968'] = 'ascii'
del aliases # Not needed any more

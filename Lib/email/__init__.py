# Copyright (C) 2001-2004 Python Software Foundation
# Author: Barry Warsaw
# Contact: email-sig@python.org

"""A package for parsing, handling, and generating email messages."""

__version__ = '3.0+'

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



# Some convenience routines.  Don't import Parser and Message as side-effects
# of importing email since those cascadingly import most of the rest of the
# email package.
def message_from_string(s, *args, **kws):
    """Parse a string into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.Parser import Parser
    return Parser(*args, **kws).parsestr(s)


def message_from_file(fp, *args, **kws):
    """Read a file and parse its contents into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.Parser import Parser
    return Parser(*args, **kws).parse(fp)

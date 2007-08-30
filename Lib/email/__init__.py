# Copyright (C) 2001-2007 Python Software Foundation
# Author: Barry Warsaw
# Contact: email-sig@python.org

"""A package for parsing, handling, and generating email messages."""

__version__ = '5.0.0'

__all__ = [
    'base64mime',
    'charset',
    'encoders',
    'errors',
    'generator',
    'header',
    'iterators',
    'message',
    'message_from_file',
    'message_from_string',
    'mime',
    'parser',
    'quoprimime',
    'utils',
    ]



# Some convenience routines.  Don't import Parser and Message as side-effects
# of importing email since those cascadingly import most of the rest of the
# email package.
def message_from_string(s, *args, **kws):
    """Parse a string into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.parser import Parser
    return Parser(*args, **kws).parsestr(s)


def message_from_file(fp, *args, **kws):
    """Read a file and parse its contents into a Message object model.

    Optional _class and strict are passed to the Parser constructor.
    """
    from email.parser import Parser
    return Parser(*args, **kws).parse(fp)

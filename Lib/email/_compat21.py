# Copyright (C) 2002 Python Software Foundation
# Author: barry@zope.com

"""Module containing compatibility functions for Python 2.1.
"""

from cStringIO import StringIO
from types import StringType, UnicodeType



# This function will become a method of the Message class
def walk(self):
    """Walk over the message tree, yielding each subpart.

    The walk is performed in depth-first order.  This method is a
    generator.
    """
    parts = []
    parts.append(self)
    if self.is_multipart():
        for subpart in self.get_payload():
            parts.extend(subpart.walk())
    return parts


# Used internally by the Header class
def _floordiv(x, y):
    """Do integer division."""
    return x / y



# These two functions are imported into the Iterators.py interface module.
# The Python 2.2 version uses generators for efficiency.
def body_line_iterator(msg):
    """Iterate over the parts, returning string payloads line-by-line."""
    lines = []
    for subpart in msg.walk():
        payload = subpart.get_payload()
        if isinstance(payload, StringType) or isinstance(payload, UnicodeType):
            for line in StringIO(payload).readlines():
                lines.append(line)
    return lines


def typed_subpart_iterator(msg, maintype='text', subtype=None):
    """Iterate over the subparts with a given MIME type.

    Use `maintype' as the main MIME type to match against; this defaults to
    "text".  Optional `subtype' is the MIME subtype to match against; if
    omitted, only the main type is matched.
    """
    parts = []
    for subpart in msg.walk():
        if subpart.get_main_type('text') == maintype:
            if subtype is None or subpart.get_subtype('plain') == subtype:
                parts.append(subpart)
    return parts

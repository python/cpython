# Copyright (C) 2002 Python Software Foundation
# Author: barry@zope.com

"""Module containing compatibility functions for Python 2.2.
"""

from __future__ import generators
from __future__ import division
from cStringIO import StringIO
from types import StringTypes

# Python 2.2.x where x < 1 lacks True/False
try:
    True, False
except NameError:
    True = 1
    False = 0



# This function will become a method of the Message class
def walk(self):
    """Walk over the message tree, yielding each subpart.

    The walk is performed in depth-first order.  This method is a
    generator.
    """
    yield self
    if self.is_multipart():
        for subpart in self.get_payload():
            for subsubpart in subpart.walk():
                yield subsubpart


# Python 2.2 spells floor division //
def _floordiv(i, j):
    """Do a floor division, i/j."""
    return i // j


def _isstring(obj):
    return isinstance(obj, StringTypes)



# These two functions are imported into the Iterators.py interface module.
# The Python 2.2 version uses generators for efficiency.
def body_line_iterator(msg, decode=False):
    """Iterate over the parts, returning string payloads line-by-line.

    Optional decode (default False) is passed through to .get_payload().
    """
    for subpart in msg.walk():
        payload = subpart.get_payload(decode=decode)
        if _isstring(payload):
            for line in StringIO(payload):
                yield line


def typed_subpart_iterator(msg, maintype='text', subtype=None):
    """Iterate over the subparts with a given MIME type.

    Use `maintype' as the main MIME type to match against; this defaults to
    "text".  Optional `subtype' is the MIME subtype to match against; if
    omitted, only the main type is matched.
    """
    for subpart in msg.walk():
        if subpart.get_content_maintype() == maintype:
            if subtype is None or subpart.get_content_subtype() == subtype:
                yield subpart

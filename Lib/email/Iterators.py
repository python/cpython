# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Various types of useful iterators and generators.
"""

from __future__ import generators
from cStringIO import StringIO
from types import StringType



def body_line_iterator(msg):
    """Iterate over the parts, returning string payloads line-by-line."""
    for subpart in msg.walk():
        payload = subpart.get_payload()
        if type(payload) is StringType:
            for line in StringIO(payload):
                yield line



def typed_subpart_iterator(msg, maintype='text', subtype=None):
    """Iterate over the subparts with a given MIME type.

    Use `maintype' as the main MIME type to match against; this defaults to
    "text".  Optional `subtype' is the MIME subtype to match against; if
    omitted, only the main type is matched.
    """
    for subpart in msg.walk():
        if subpart.get_main_type('text') == maintype:
            if subtype is None or subpart.get_subtype('plain') == subtype:
                yield subpart

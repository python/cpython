# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Various types of useful iterators and generators.
"""

from __future__ import generators
from cStringIO import StringIO
from types import StringType



def body_line_iterator(msg):
    """Iterator over the parts, returning the lines in a string payload."""
    for subpart in msg.walk():
        payload = subpart.get_payload()
        if type(payload) is StringType:
            for line in StringIO(payload):
                yield line



def typed_subpart_iterator(msg, major='text', minor=None):
    """Iterator over the subparts with a given MIME type.

    Use `major' as the main MIME type to match against; this defaults to
    "text".  Optional `minor' is the MIME subtype to match against; if
    omitted, only the main type is matched.
    """
    for subpart in msg.walk():
        if subpart.get_main_type() == major:
            if minor is None or subpart.get_subtype() == minor:
                yield subpart

# Copyright (C) 2001 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Miscellaneous utilities.
"""

import re

from rfc822 import unquote, quote, parseaddr
from rfc822 import dump_address_pair
from rfc822 import AddrlistClass as _AddrlistClass
from rfc822 import parsedate_tz, parsedate, mktime_tz, formatdate

from quopri import decodestring as _qdecode
import base64

# Intrapackage imports
from Encoders import _bencode, _qencode

COMMASPACE = ', '
UEMPTYSTRING = u''



# Helpers

def _identity(s):
    return s


def _bdecode(s):
    if not s:
        return s
    # We can't quite use base64.encodestring() since it tacks on a "courtesy
    # newline".  Blech!
    if not s:
        return s
    hasnewline = (s[-1] == '\n')
    value = base64.decodestring(s)
    if not hasnewline and value[-1] == '\n':
        return value[:-1]
    return value



def getaddresses(fieldvalues):
    """Return a list of (REALNAME, EMAIL) for each fieldvalue."""
    all = COMMASPACE.join(fieldvalues)
    a = _AddrlistClass(all)
    return a.getaddrlist()



ecre = re.compile(r'''
  =\?                   # literal =?
  (?P<charset>[^?]*?)   # non-greedy up to the next ? is the charset
  \?                    # literal ?
  (?P<encoding>[qb])    # either a "q" or a "b", case insensitive
  \?                    # literal ?
  (?P<atom>.*?)         # non-greedy up to the next ?= is the atom
  \?=                   # literal ?=
  ''', re.VERBOSE | re.IGNORECASE)


def decode(s):
    """Return a decoded string according to RFC 2047, as a unicode string."""
    rtn = []
    parts = ecre.split(s, 1)
    while parts:
        # If there are less than 4 parts, it can't be encoded and we're done
        if len(parts) < 5:
            rtn.extend(parts)
            break
        # The first element is any non-encoded leading text
        rtn.append(parts[0])
        charset = parts[1]
        encoding = parts[2]
        atom = parts[3]
        # The next chunk to decode should be in parts[4]
        parts = ecre.split(parts[4])
        # The encoding must be either `q' or `b', case-insensitive
        if encoding.lower() == 'q':
            func = _qdecode
        elif encoding.lower() == 'b':
            func = _bdecode
        else:
            func = _identity
        # Decode and get the unicode in the charset
        rtn.append(unicode(func(atom), charset))
    # Now that we've decoded everything, we just need to join all the parts
    # together into the final string.
    return UEMPTYSTRING.join(rtn)



def encode(s, charset='iso-8859-1', encoding='q'):
    """Encode a string according to RFC 2047."""
    if encoding.lower() == 'q':
        estr = _qencode(s)
    elif encoding.lower() == 'b':
        estr = _bencode(s)
    else:
        raise ValueError, 'Illegal encoding code: ' + encoding
    return '=?%s?%s?%s?=' % (charset.lower(), encoding.lower(), estr)

# Copyright (C) 2001,2002 Python Software Foundation
# Author: barry@zope.com (Barry Warsaw)

"""Miscellaneous utilities.
"""

import time
import socket
import re
import random
import os
import warnings
from cStringIO import StringIO
from types import ListType

from rfc822 import unquote, quote
from rfc822 import AddressList as _AddressList
from rfc822 import mktime_tz

# We need wormarounds for bugs in these methods in older Pythons (see below)
from rfc822 import parsedate as _parsedate
from rfc822 import parsedate_tz as _parsedate_tz
from rfc822 import parseaddr as _parseaddr

from quopri import decodestring as _qdecode
import base64

# Intrapackage imports
from Encoders import _bencode, _qencode

COMMASPACE = ', '
UEMPTYSTRING = u''
CRLF = '\r\n'

specialsre = re.compile(r'[][\()<>@,:;".]')
escapesre = re.compile(r'[][\()"]')



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



def fix_eols(s):
    """Replace all line-ending characters with \r\n."""
    # Fix newlines with no preceding carriage return
    s = re.sub(r'(?<!\r)\n', CRLF, s)
    # Fix carriage returns with no following newline
    s = re.sub(r'\r(?!\n)', CRLF, s)
    return s



def formataddr(pair):
    """The inverse of parseaddr(), this takes a 2-tuple of the form
    (realname, email_address) and returns the string value suitable
    for an RFC 2822 From:, To: or Cc:.
    
    If the first element of pair is false, then the second element is
    returned unmodified.
    """
    name, address = pair
    if name:
        quotes = ''
        if specialsre.search(name):
            quotes = '"'
        name = escapesre.sub(r'\\\g<0>', name)
        return '%s%s%s <%s>' % (quotes, name, quotes, address)
    return address

# For backwards compatibility
def dump_address_pair(pair):
    warnings.warn('Use email.Utils.formataddr() instead',
                  DeprecationWarning, 2)
    return formataddr(pair)



def getaddresses(fieldvalues):
    """Return a list of (REALNAME, EMAIL) for each fieldvalue."""
    all = COMMASPACE.join(fieldvalues)
    a = _AddressList(all)
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
    """Return a decoded string according to RFC 2047, as a unicode string.

    NOTE: This function is deprecated.  Use Header.decode_header() instead.
    """
    warnings.warn('Use Header.decode_header() instead.', DeprecationWarning, 2)
    # Intra-package import here to avoid circular import problems.
    from Header import decode_header
    L = decode_header(s)
    if not isinstance(L, ListType):
        # s wasn't decoded
        return s

    rtn = []
    for atom, charset in L:
        if charset is None:
            rtn.append(atom)
        else:
            # Convert the string to Unicode using the given encoding.  Leave
            # Unicode conversion errors to strict.
            rtn.append(unicode(atom, charset))
    # Now that we've decoded everything, we just need to join all the parts
    # together into the final string.
    return UEMPTYSTRING.join(rtn)



def encode(s, charset='iso-8859-1', encoding='q'):
    """Encode a string according to RFC 2047."""
    warnings.warn('Use Header.Header.encode() instead.', DeprecationWarning, 2)
    encoding = encoding.lower()
    if encoding == 'q':
        estr = _qencode(s)
    elif encoding == 'b':
        estr = _bencode(s)
    else:
        raise ValueError, 'Illegal encoding code: ' + encoding
    return '=?%s?%s?%s?=' % (charset.lower(), encoding, estr)



def formatdate(timeval=None, localtime=0):
    """Returns a date string as specified by RFC 2822, e.g.:

    Fri, 09 Nov 2001 01:08:47 -0000

    Optional timeval if given is a floating point time value as accepted by
    gmtime() and localtime(), otherwise the current time is used.

    Optional localtime is a flag that when true, interprets timeval, and
    returns a date relative to the local timezone instead of UTC, properly
    taking daylight savings time into account.
    """
    # Note: we cannot use strftime() because that honors the locale and RFC
    # 2822 requires that day and month names be the English abbreviations.
    if timeval is None:
        timeval = time.time()
    if localtime:
        now = time.localtime(timeval)
        # Calculate timezone offset, based on whether the local zone has
        # daylight savings time, and whether DST is in effect.
        if time.daylight and now[-1]:
            offset = time.altzone
        else:
            offset = time.timezone
        hours, minutes = divmod(abs(offset), 3600)
        # Remember offset is in seconds west of UTC, but the timezone is in
        # minutes east of UTC, so the signs differ.
        if offset > 0:
            sign = '-'
        else:
            sign = '+'
        zone = '%s%02d%02d' % (sign, hours, minutes / 60)
    else:
        now = time.gmtime(timeval)
        # Timezone offset is always -0000
        zone = '-0000'
    return '%s, %02d %s %04d %02d:%02d:%02d %s' % (
        ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'][now[6]],
        now[2],
        ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
         'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'][now[1] - 1],
        now[0], now[3], now[4], now[5],
        zone)



def make_msgid(idstring=None):
    """Returns a string suitable for RFC 2822 compliant Message-ID:, e.g:

    <20020201195627.33539.96671@nightshade.la.mastaler.com>

    Optional idstring if given is a string used to strengthen the
    uniqueness of the Message-ID, otherwise an empty string is used.
    """
    timeval = time.time()
    utcdate = time.strftime('%Y%m%d%H%M%S', time.gmtime(timeval))
    pid = os.getpid()
    randint = random.randrange(100000)
    if idstring is None:
        idstring = ''
    else:
        idstring = '.' + idstring
    idhost = socket.getfqdn()
    msgid = '<%s.%s.%s%s@%s>' % (utcdate, pid, randint, idstring, idhost)
    return msgid



# These functions are in the standalone mimelib version only because they've
# subsequently been fixed in the latest Python versions.  We use this to worm
# around broken older Pythons.
def parsedate(data):
    if not data:
        return None
    return _parsedate(data)


def parsedate_tz(data):
    if not data:
        return None
    return _parsedate_tz(data)


def parseaddr(addr):
    realname, emailaddr = _parseaddr(addr)
    if realname == '' and emailaddr is None:
        return '', ''
    return realname, emailaddr

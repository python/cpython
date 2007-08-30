# Copyright (C) 2001-2006 Python Software Foundation
# Author: Ben Gertzfield
# Contact: email-sig@python.org

"""Quoted-printable content transfer encoding per RFCs 2045-2047.

This module handles the content transfer encoding method defined in RFC 2045
to encode US ASCII-like 8-bit data called `quoted-printable'.  It is used to
safely encode text that is in a character set similar to the 7-bit US ASCII
character set, but that includes some 8-bit characters that are normally not
allowed in email bodies or headers.

Quoted-printable is very space-inefficient for encoding binary files; use the
email.base64MIME module for that instead.

This module provides an interface to encode and decode both headers and bodies
with quoted-printable encoding.

RFC 2045 defines a method for including character set information in an
`encoded-word' in a header.  This method is commonly used for 8-bit real names
in To:/From:/Cc: etc. fields, as well as Subject: lines.

This module does not do the line wrapping or end-of-line character
conversion necessary for proper internationalized headers; it only
does dumb encoding and decoding.  To deal with the various line
wrapping issues, use the email.Header module.
"""

__all__ = [
    'body_decode',
    'body_encode',
    'body_quopri_check',
    'body_quopri_len',
    'decode',
    'decodestring',
    'encode',
    'encodestring',
    'header_decode',
    'header_encode',
    'header_quopri_check',
    'header_quopri_len',
    'quote',
    'unquote',
    ]

import re

from string import ascii_letters, digits, hexdigits
from email.utils import fix_eols

CRLF = '\r\n'
NL = '\n'
EMPTYSTRING = ''

# See also Charset.py
MISC_LEN = 7

HEADER_SAFE_BYTES = b'-!*+/ ' + bytes(ascii_letters) + bytes(digits)
BODY_SAFE_BYTES   = (b' !"#$%&\'()*+,-./0123456789:;<>'
                     b'?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`'
                     b'abcdefghijklmnopqrstuvwxyz{|}~\t')



# Helpers
def header_quopri_check(c):
    """Return True if the character should be escaped with header quopri."""
    return c not in HEADER_SAFE_BYTES


def body_quopri_check(c):
    """Return True if the character should be escaped with body quopri."""
    return c not in BODY_SAFE_BYTES


def header_quopri_len(bytearray):
    """Return the length of bytearray when it is encoded with header quopri.

    Note that this does not include any RFC 2047 chrome added by
    `header_encode()`.
    """
    count = 0
    for c in bytearray:
        count += (3 if header_quopri_check(c) else 1)
    return count


def body_quopri_len(bytearray):
    """Return the length of bytearray when it is encoded with body quopri."""
    count = 0
    for c in bytearray:
        count += (3 if body_quopri_check(c) else 1)
    return count


def _max_append(L, s, maxlen, extra=''):
    if not isinstance(s, str):
        s = chr(s)
    if not L:
        L.append(s.lstrip())
    elif len(L[-1]) + len(s) <= maxlen:
        L[-1] += extra + s
    else:
        L.append(s.lstrip())


def unquote(s):
    """Turn a string in the form =AB to the ASCII character with value 0xab"""
    return chr(int(s[1:3], 16))


def quote(c):
    return '=%02X' % ord(c)



def header_encode(header_bytes, charset='iso-8859-1'):
    """Encode a single header line with quoted-printable (like) encoding.

    Defined in RFC 2045, this `Q' encoding is similar to quoted-printable, but
    used specifically for email header fields to allow charsets with mostly 7
    bit characters (and some 8 bit) to remain more or less readable in non-RFC
    2045 aware mail clients.

    charset names the character set to use in the RFC 2046 header.  It
    defaults to iso-8859-1.
    """
    # Return empty headers unchanged
    if not header_bytes:
        return str(header_bytes)
    # Iterate over every byte, encoding if necessary.
    encoded = []
    for character in header_bytes:
        # Space may be represented as _ instead of =20 for readability
        if character == ord(' '):
            encoded.append('_')
        # These characters can be included verbatim.
        elif not header_quopri_check(character):
            encoded.append(chr(character))
        # Otherwise, replace with hex value like =E2
        else:
            encoded.append('=%02X' % character)
    # Now add the RFC chrome to each encoded chunk and glue the chunks
    # together.
    return '=?%s?q?%s?=' % (charset, EMPTYSTRING.join(encoded))



def encode(body, binary=False, maxlinelen=76, eol=NL):
    """Encode with quoted-printable, wrapping at maxlinelen characters.

    If binary is False (the default), end-of-line characters will be converted
    to the canonical email end-of-line sequence \\r\\n.  Otherwise they will
    be left verbatim.

    Each line of encoded text will end with eol, which defaults to "\\n".  Set
    this to "\\r\\n" if you will be using the result of this function directly
    in an email.

    Each line will be wrapped at, at most, maxlinelen characters (defaults to
    76 characters).  Long lines will have the `soft linefeed' quoted-printable
    character "=" appended to them, so the decoded text will be identical to
    the original text.
    """
    if not body:
        return body

    if not binary:
        body = fix_eols(body)

    # BAW: We're accumulating the body text by string concatenation.  That
    # can't be very efficient, but I don't have time now to rewrite it.  It
    # just feels like this algorithm could be more efficient.
    encoded_body = ''
    lineno = -1
    # Preserve line endings here so we can check later to see an eol needs to
    # be added to the output later.
    lines = body.splitlines(1)
    for line in lines:
        # But strip off line-endings for processing this line.
        if line.endswith(CRLF):
            line = line[:-2]
        elif line[-1] in CRLF:
            line = line[:-1]

        lineno += 1
        encoded_line = ''
        prev = None
        linelen = len(line)
        # Now we need to examine every character to see if it needs to be
        # quopri encoded.  BAW: again, string concatenation is inefficient.
        for j in range(linelen):
            c = line[j]
            prev = c
            if body_quopri_check(c):
                c = quote(c)
            elif j+1 == linelen:
                # Check for whitespace at end of line; special case
                if c not in ' \t':
                    encoded_line += c
                prev = c
                continue
            # Check to see to see if the line has reached its maximum length
            if len(encoded_line) + len(c) >= maxlinelen:
                encoded_body += encoded_line + '=' + eol
                encoded_line = ''
            encoded_line += c
        # Now at end of line..
        if prev and prev in ' \t':
            # Special case for whitespace at end of file
            if lineno + 1 == len(lines):
                prev = quote(prev)
                if len(encoded_line) + len(prev) > maxlinelen:
                    encoded_body += encoded_line + '=' + eol + prev
                else:
                    encoded_body += encoded_line + prev
            # Just normal whitespace at end of line
            else:
                encoded_body += encoded_line + prev + '=' + eol
            encoded_line = ''
        # Now look at the line we just finished and it has a line ending, we
        # need to add eol to the end of the line.
        if lines[lineno].endswith(CRLF) or lines[lineno][-1] in CRLF:
            encoded_body += encoded_line + eol
        else:
            encoded_body += encoded_line
        encoded_line = ''
    return encoded_body


# For convenience and backwards compatibility w/ standard base64 module
body_encode = encode
encodestring = encode



# BAW: I'm not sure if the intent was for the signature of this function to be
# the same as base64MIME.decode() or not...
def decode(encoded, eol=NL):
    """Decode a quoted-printable string.

    Lines are separated with eol, which defaults to \\n.
    """
    if not encoded:
        return encoded
    # BAW: see comment in encode() above.  Again, we're building up the
    # decoded string with string concatenation, which could be done much more
    # efficiently.
    decoded = ''

    for line in encoded.splitlines():
        line = line.rstrip()
        if not line:
            decoded += eol
            continue

        i = 0
        n = len(line)
        while i < n:
            c = line[i]
            if c != '=':
                decoded += c
                i += 1
            # Otherwise, c == "=".  Are we at the end of the line?  If so, add
            # a soft line break.
            elif i+1 == n:
                i += 1
                continue
            # Decode if in form =AB
            elif i+2 < n and line[i+1] in hexdigits and line[i+2] in hexdigits:
                decoded += unquote(line[i:i+3])
                i += 3
            # Otherwise, not in form =AB, pass literally
            else:
                decoded += c
                i += 1

            if i == n:
                decoded += eol
    # Special case if original string did not end with eol
    if not encoded.endswith(eol) and decoded.endswith(eol):
        decoded = decoded[:-1]
    return decoded


# For convenience and backwards compatibility w/ standard base64 module
body_decode = decode
decodestring = decode



def _unquote_match(match):
    """Turn a match in the form =AB to the ASCII character with value 0xab"""
    s = match.group(0)
    return unquote(s)


# Header decoding is done a bit differently
def header_decode(s):
    """Decode a string encoded with RFC 2045 MIME header `Q' encoding.

    This function does not parse a full MIME header value encoded with
    quoted-printable (like =?iso-8895-1?q?Hello_World?=) -- please use
    the high level email.Header class for that functionality.
    """
    s = s.replace('_', ' ')
    return re.sub(r'=\w{2}', _unquote_match, s)

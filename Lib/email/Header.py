# Copyright (C) 2002 Python Software Foundation
# Author: che@debian.org (Ben Gertzfield)

"""Header encoding and decoding functionality."""

import re
import email.quopriMIME
import email.base64MIME
from email.Charset import Charset

try:
    from email._compat22 import _intdiv2
except SyntaxError:
    # Python 2.1 spells integer division differently
    from email._compat21 import _intdiv2

CRLFSPACE = '\r\n '
CRLF = '\r\n'
NLSPACE = '\n '

MAXLINELEN = 76

ENCODE = 1
DECODE = 2

# Match encoded-word strings in the form =?charset?q?Hello_World?=
ecre = re.compile(r'''
  =\?                   # literal =?
  (?P<charset>[^?]*?)   # non-greedy up to the next ? is the charset
  \?                    # literal ?
  (?P<encoding>[qb])    # either a "q" or a "b", case insensitive
  \?                    # literal ?
  (?P<encoded>.*?)      # non-greedy up to the next ?= is the encoded string
  \?=                   # literal ?=
  ''', re.VERBOSE | re.IGNORECASE)



# Helpers
_max_append = email.quopriMIME._max_append



def decode_header(header):
    """Decode a message header value without converting charset.

    Returns a list of (decoded_string, charset) pairs containing each of the
    decoded parts of the header.  Charset is None for non-encoded parts of the
    header, otherwise a lower-case string containing the name of the character
    set specified in the encoded string.
    """
    # If no encoding, just return the header
    header = str(header)
    if not ecre.search(header):
        return [(header, None)]

    decoded = []
    dec = ''
    for line in header.splitlines():
        # This line might not have an encoding in it
        if not ecre.search(line):
            decoded.append((line, None))
            continue
        
        parts = ecre.split(line)
        while parts:
            unenc = parts.pop(0).strip()
            if unenc:
                # Should we continue a long line?
                if decoded and decoded[-1][1] is None:
                    decoded[-1] = (decoded[-1][0] + dec, None)
                else:
                    decoded.append((unenc, None))
            if parts:
                charset, encoding = [s.lower() for s in parts[0:2]]
                encoded = parts[2]
                dec = ''
                if encoding == 'q':
                    dec = email.quopriMIME.header_decode(encoded)
                elif encoding == 'b':
                    dec = email.base64MIME.decode(encoded)
                else:
                    dec = encoded

                if decoded and decoded[-1][1] == charset:
                    decoded[-1] = (decoded[-1][0] + dec, decoded[-1][1])
                else:
                    decoded.append((dec, charset))
            del parts[0:3]
    return decoded



class Header:
    def __init__(self, s, charset=None, maxlinelen=None, header_name=None):
        """Create a MIME-compliant header that can contain many languages.

        Specify the initial header value in s.  Specify its character set as a
        Charset object in the charset argument.  If none, a default Charset
        instance will be used.

        You can later append to the header with append(s, charset) below;
        charset does not have to be the same as the one initially specified
        here.  In fact, it's optional, and if not given, defaults to the
        charset specified in the constructor.

        The maximum line length can be specified explicitly via maxlinelen.
        You can also pass None for maxlinelen and the name of a header field
        (e.g. "Subject") to let the constructor guess the best line length to
        use.  The default maxlinelen is 76.
        """
        if charset is None:
            charset = Charset()
        self._charset = charset
        # BAW: I believe `chunks' and `maxlinelen' should be non-public.
        self._chunks = []
        self.append(s, charset)
        if maxlinelen is None:
            if header_name is None:
                self._maxlinelen = MAXLINELEN
            else:
                self.guess_maxlinelen(header_name)
        else:
            self._maxlinelen = maxlinelen

    def __str__(self):
        """A synonym for self.encode()."""
        return self.encode()

    def guess_maxlinelen(self, s=None):
        """Guess the maximum length to make each header line.

        Given a header name (e.g. "Subject"), set this header's maximum line
        length to an appropriate length to avoid line wrapping.  If s is not
        given, return the previous maximum line length and don't set it.

        Returns the new maximum line length.
        """
        # BAW: is this semantic necessary?
        if s is not None:
            self._maxlinelen = MAXLINELEN - len(s) - 2
        return self._maxlinelen

    def append(self, s, charset=None):
        """Append string s with Charset charset to the MIME header.

        charset defaults to the one given in the class constructor.
        """
        if charset is None:
            charset = self._charset
        self._chunks.append((s, charset))
        
    def _split(self, s, charset):
        # Split up a header safely for use with encode_chunks.  BAW: this
        # appears to be a private convenience method.
        splittable = charset.to_splittable(s)
        encoded = charset.from_splittable(splittable)
        elen = charset.encoded_header_len(encoded)
        
        if elen <= self._maxlinelen:
            return [(encoded, charset)]
        # BAW: should we use encoded?
        elif elen == len(s):
            # We can split on _maxlinelen boundaries because we know that the
            # encoding won't change the size of the string
            splitpnt = self._maxlinelen
            first = charset.from_splittable(splittable[:splitpnt], 0)
            last = charset.from_splittable(splittable[splitpnt:], 0)
            return self._split(first, charset) + self._split(last, charset)
        else:
            # Divide and conquer.  BAW: halfway depends on integer division.
            # When porting to Python 2.2, use the // operator.
            halfway = _intdiv2(len(splittable))
            first = charset.from_splittable(splittable[:halfway], 0)
            last = charset.from_splittable(splittable[halfway:], 0)
            return self._split(first, charset) + self._split(last, charset)

    def encode(self):
        """Encode a message header, possibly converting charset and encoding.

        There are many issues involved in converting a given string for use in
        an email header.  Only certain character sets are readable in most
        email clients, and as header strings can only contain a subset of
        7-bit ASCII, care must be taken to properly convert and encode (with
        Base64 or quoted-printable) header strings.  In addition, there is a
        75-character length limit on any given encoded header field, so
        line-wrapping must be performed, even with double-byte character sets.
        
        This method will do its best to convert the string to the correct
        character set used in email, and encode and line wrap it safely with
        the appropriate scheme for that character set.

        If the given charset is not known or an error occurs during
        conversion, this function will return the header untouched.
        """
        newchunks = []
        for s, charset in self._chunks:
            newchunks += self._split(s, charset)
        self._chunks = newchunks
        return self.encode_chunks()

    def encode_chunks(self):
        """MIME-encode a header with many different charsets and/or encodings.

        Given a list of pairs (string, charset), return a MIME-encoded string
        suitable for use in a header field.  Each pair may have different
        charsets and/or encodings, and the resulting header will accurately
        reflect each setting.

        Each encoding can be email.Utils.QP (quoted-printable, for ASCII-like
        character sets like iso-8859-1), email.Utils.BASE64 (Base64, for
        non-ASCII like character sets like KOI8-R and iso-2022-jp), or None
        (no encoding).

        Each pair will be represented on a separate line; the resulting string
        will be in the format:

        "=?charset1?q?Mar=EDa_Gonz=E1lez_Alonso?=\n
          =?charset2?b?SvxyZ2VuIEL2aW5n?="
        """
        chunks = []
        for header, charset in self._chunks:
            if charset is None:
                _max_append(chunks, header, self._maxlinelen, ' ')
            else:
                _max_append(chunks, charset.header_encode(header, 0),
                            self._maxlinelen, ' ')
        return NLSPACE.join(chunks)

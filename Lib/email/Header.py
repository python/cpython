# Copyright (C) 2002 Python Software Foundation
# Author: che@debian.org (Ben Gertzfield), barry@zope.com (Barry Warsaw)

"""Header encoding and decoding functionality."""

import re
from types import StringType, UnicodeType

import email.quopriMIME
import email.base64MIME
from email.Charset import Charset

try:
    from email._compat22 import _floordiv
except SyntaxError:
    # Python 2.1 spells integer division differently
    from email._compat21 import _floordiv

try:
    True, False
except NameError:
    True = 1
    False = 0

CRLFSPACE = '\r\n '
CRLF = '\r\n'
NL = '\n'
SPACE8 = ' ' * 8
EMPTYSTRING = ''

MAXLINELEN = 76

ENCODE = 1
DECODE = 2

USASCII = Charset('us-ascii')
UTF8 = Charset('utf-8')

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



def make_header(decoded_seq, maxlinelen=None, header_name=None,
                continuation_ws=' '):
    """Create a Header from a sequence of pairs as returned by decode_header()

    decode_header() takes a header value string and returns a sequence of
    pairs of the format (decoded_string, charset) where charset is the string
    name of the character set.

    This function takes one of those sequence of pairs and returns a Header
    instance.  Optional maxlinelen, header_name, and continuation_ws are as in
    the Header constructor.
    """
    h = Header(maxlinelen=maxlinelen, header_name=header_name,
               continuation_ws=continuation_ws)
    for s, charset in decoded_seq:
        # None means us-ascii but we can simply pass it on to h.append()
        if charset is not None and not isinstance(charset, Charset):
            charset = Charset(charset)
        h.append(s, charset)
    return h



class Header:
    def __init__(self, s=None, charset=None, maxlinelen=None, header_name=None,
                 continuation_ws=' '):
        """Create a MIME-compliant header that can contain many character sets.

        Optional s is the initial header value.  If None, the initial header
        value is not set.  You can later append to the header with .append()
        method calls.  s may be a byte string or a Unicode string, but see the
        .append() documentation for semantics.

        Optional charset serves two purposes: it has the same meaning as the
        charset argument to the .append() method.  It also sets the default
        character set for all subsequent .append() calls that omit the charset
        argument.  If charset is not provided in the constructor, the us-ascii
        charset is used both as s's initial charset and as the default for
        subsequent .append() calls.

        The maximum line length can be specified explicit via maxlinelen.  For
        splitting the first line to a shorter value (to account for the field
        header which isn't included in s, e.g. `Subject') pass in the name of
        the field in header_name.  The default maxlinelen is 76.

        continuation_ws must be RFC 2822 compliant folding whitespace (usually
        either a space or a hard tab) which will be prepended to continuation
        lines.
        """
        if charset is None:
            charset = USASCII
        if not isinstance(charset, Charset):
            charset = Charset(charset)
        self._charset = charset
        self._continuation_ws = continuation_ws
        cws_expanded_len = len(continuation_ws.replace('\t', SPACE8))
        # BAW: I believe `chunks' and `maxlinelen' should be non-public.
        self._chunks = []
        if s is not None:
            self.append(s, charset)
        if maxlinelen is None:
            maxlinelen = MAXLINELEN
        if header_name is None:
            # We don't know anything about the field header so the first line
            # is the same length as subsequent lines.
            self._firstlinelen = maxlinelen
        else:
            # The first line should be shorter to take into account the field
            # header.  Also subtract off 2 extra for the colon and space.
            self._firstlinelen = maxlinelen - len(header_name) - 2
        # Second and subsequent lines should subtract off the length in
        # columns of the continuation whitespace prefix.
        self._maxlinelen = maxlinelen - cws_expanded_len

    def __str__(self):
        """A synonym for self.encode()."""
        return self.encode()

    def __unicode__(self):
        """Helper for the built-in unicode function."""
        # charset item is a Charset instance so we need to stringify it.
        uchunks = [unicode(s, str(charset)) for s, charset in self._chunks]
        return u''.join(uchunks)

    # Rich comparison operators for equality only.  BAW: does it make sense to
    # have or explicitly disable <, <=, >, >= operators?
    def __eq__(self, other):
        # other may be a Header or a string.  Both are fine so coerce
        # ourselves to a string, swap the args and do another comparison.
        return other == self.encode()

    def __ne__(self, other):
        return not self == other

    def append(self, s, charset=None):
        """Append a string to the MIME header.

        Optional charset, if given, should be a Charset instance or the name
        of a character set (which will be converted to a Charset instance).  A
        value of None (the default) means that the charset given in the
        constructor is used.

        s may be a byte string or a Unicode string.  If it is a byte string
        (i.e. isinstance(s, StringType) is true), then charset is the encoding
        of that byte string, and a UnicodeError will be raised if the string
        cannot be decoded with that charset.  If s is a Unicode string, then
        charset is a hint specifying the character set of the characters in
        the string.  In this case, when producing an RFC 2822 compliant header
        using RFC 2047 rules, the Unicode string will be encoded using the
        following charsets in order: us-ascii, the charset hint, utf-8.  The
        first character set not to provoke a UnicodeError is used.
        """
        if charset is None:
            charset = self._charset
        elif not isinstance(charset, Charset):
            charset = Charset(charset)
        # If the charset is our faux 8bit charset, leave the string unchanged
        if charset <> '8bit':
            # We need to test that the string can be converted to unicode and
            # back to a byte string, given the input and output codecs of the
            # charset.
            if isinstance(s, StringType):
                # Possibly raise UnicodeError if the byte string can't be
                # converted to a unicode with the input codec of the charset.
                incodec = charset.input_codec or 'us-ascii'
                ustr = unicode(s, incodec)
                # Now make sure that the unicode could be converted back to a
                # byte string with the output codec, which may be different
                # than the iput coded.  Still, use the original byte string.
                outcodec = charset.output_codec or 'us-ascii'
                ustr.encode(outcodec)
            elif isinstance(s, UnicodeType):
                # Now we have to be sure the unicode string can be converted
                # to a byte string with a reasonable output codec.  We want to
                # use the byte string in the chunk.
                for charset in USASCII, charset, UTF8:
                    try:
                        outcodec = charset.output_codec or 'us-ascii'
                        s = s.encode(outcodec)
                        break
                    except UnicodeError:
                        pass
                else:
                    assert False, 'utf-8 conversion failed'
        self._chunks.append((s, charset))

    def _split(self, s, charset, firstline=False):
        # Split up a header safely for use with encode_chunks.
        splittable = charset.to_splittable(s)
        encoded = charset.from_splittable(splittable)
        elen = charset.encoded_header_len(encoded)

        if elen <= self._maxlinelen:
            return [(encoded, charset)]
        # If we have undetermined raw 8bit characters sitting in a byte
        # string, we really don't know what the right thing to do is.  We
        # can't really split it because it might be multibyte data which we
        # could break if we split it between pairs.  The least harm seems to
        # be to not split the header at all, but that means they could go out
        # longer than maxlinelen.
        elif charset == '8bit':
            return [(s, charset)]
        # BAW: I'm not sure what the right test here is.  What we're trying to
        # do is be faithful to RFC 2822's recommendation that ($2.2.3):
        #
        # "Note: Though structured field bodies are defined in such a way that
        #  folding can take place between many of the lexical tokens (and even
        #  within some of the lexical tokens), folding SHOULD be limited to
        #  placing the CRLF at higher-level syntactic breaks."
        #
        # For now, I can only imagine doing this when the charset is us-ascii,
        # although it's possible that other charsets may also benefit from the
        # higher-level syntactic breaks.
        #
        elif charset == 'us-ascii':
            return self._ascii_split(s, charset, firstline)
        # BAW: should we use encoded?
        elif elen == len(s):
            # We can split on _maxlinelen boundaries because we know that the
            # encoding won't change the size of the string
            splitpnt = self._maxlinelen
            first = charset.from_splittable(splittable[:splitpnt], False)
            last = charset.from_splittable(splittable[splitpnt:], False)
        else:
            # Divide and conquer.
            halfway = _floordiv(len(splittable), 2)
            first = charset.from_splittable(splittable[:halfway], False)
            last = charset.from_splittable(splittable[halfway:], False)
        # Do the split
        return self._split(first, charset, firstline) + \
               self._split(last, charset)

    def _ascii_split(self, s, charset, firstline):
        # Attempt to split the line at the highest-level syntactic break
        # possible.  Note that we don't have a lot of smarts about field
        # syntax; we just try to break on semi-colons, then whitespace.
        rtn = []
        lines = s.splitlines()
        while lines:
            line = lines.pop(0)
            if firstline:
                maxlinelen = self._firstlinelen
                firstline = False
            else:
                #line = line.lstrip()
                maxlinelen = self._maxlinelen
            # Short lines can remain unchanged
            if len(line.replace('\t', SPACE8)) <= maxlinelen:
                rtn.append(line)
            else:
                oldlen = len(line)
                # Try to break the line on semicolons, but if that doesn't
                # work, try to split on folding whitespace.
                while len(line) > maxlinelen:
                    i = line.rfind(';', 0, maxlinelen)
                    if i < 0:
                        break
                    rtn.append(line[:i] + ';')
                    line = line[i+1:]
                # Is the remaining stuff still longer than maxlinelen?
                if len(line) <= maxlinelen:
                    # Splitting on semis worked
                    rtn.append(line)
                    continue
                # Splitting on semis didn't finish the job.  If it did any
                # work at all, stick the remaining junk on the front of the
                # `lines' sequence and let the next pass do its thing.
                if len(line) <> oldlen:
                    lines.insert(0, line)
                    continue
                # Otherwise, splitting on semis didn't help at all.
                parts = re.split(r'(\s+)', line)
                if len(parts) == 1 or (len(parts) == 3 and
                                       parts[0].endswith(':')):
                    # This line can't be split on whitespace.  There's now
                    # little we can do to get this into maxlinelen.  BAW:
                    # We're still potentially breaking the RFC by possibly
                    # allowing lines longer than the absolute maximum of 998
                    # characters.  For now, let it slide.
                    #
                    # len(parts) will be 1 if this line has no `Field: '
                    # prefix, otherwise it will be len(3).
                    rtn.append(line)
                    continue
                # There is whitespace we can split on.
                first = parts.pop(0)
                sublines = [first]
                acc = len(first)
                while parts:
                    len0 = len(parts[0])
                    len1 = len(parts[1])
                    if acc + len0 + len1 <= maxlinelen:
                        sublines.append(parts.pop(0))
                        sublines.append(parts.pop(0))
                        acc += len0 + len1
                    else:
                        # Split it here, but don't forget to ignore the
                        # next whitespace-only part
                        if first <> '':
                            rtn.append(EMPTYSTRING.join(sublines))
                        del parts[0]
                        first = parts.pop(0)
                        sublines = [first]
                        acc = len(first)
                rtn.append(EMPTYSTRING.join(sublines))
        return [(chunk, charset) for chunk in rtn]

    def _encode_chunks(self, newchunks):
        # MIME-encode a header with many different charsets and/or encodings.
        #
        # Given a list of pairs (string, charset), return a MIME-encoded
        # string suitable for use in a header field.  Each pair may have
        # different charsets and/or encodings, and the resulting header will
        # accurately reflect each setting.
        #
        # Each encoding can be email.Utils.QP (quoted-printable, for
        # ASCII-like character sets like iso-8859-1), email.Utils.BASE64
        # (Base64, for non-ASCII like character sets like KOI8-R and
        # iso-2022-jp), or None (no encoding).
        #
        # Each pair will be represented on a separate line; the resulting
        # string will be in the format:
        #
        # =?charset1?q?Mar=EDa_Gonz=E1lez_Alonso?=\n
        #  =?charset2?b?SvxyZ2VuIEL2aW5n?="
        #
        chunks = []
        for header, charset in newchunks:
            if charset is None or charset.header_encoding is None:
                # There's no encoding for this chunk's charsets
                _max_append(chunks, header, self._maxlinelen)
            else:
                _max_append(chunks, charset.header_encode(header),
                            self._maxlinelen, ' ')
        joiner = NL + self._continuation_ws
        return joiner.join(chunks)

    def encode(self):
        """Encode a message header into an RFC-compliant format.

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
            newchunks += self._split(s, charset, True)
        return self._encode_chunks(newchunks)

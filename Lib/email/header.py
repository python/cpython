# Copyright (C) 2002-2006 Python Software Foundation
# Author: Ben Gertzfield, Barry Warsaw
# Contact: email-sig@python.org

"""Header encoding and decoding functionality."""

__all__ = [
    'Header',
    'decode_header',
    'make_header',
    ]

import re
import binascii

import email.quoprimime
import email.base64mime

from email.errors import HeaderParseError
from email.charset import Charset

NL = '\n'
SPACE = ' '
USPACE = u' '
SPACE8 = ' ' * 8
EMPTYSTRING = ''
UEMPTYSTRING = u''

MAXLINELEN = 76
FWS = ' \t'

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
  (?=[ \t]|$)           # whitespace or the end of the string
  ''', re.VERBOSE | re.IGNORECASE | re.MULTILINE)

# Field name regexp, including trailing colon, but not separating whitespace,
# according to RFC 2822.  Character range is from tilde to exclamation mark.
# For use with .match()
fcre = re.compile(r'[\041-\176]+:$')

# Find a header embedded in a putative header value.  Used to check for
# header injection attack.
_embeded_header = re.compile(r'\n[^ \t]+:')



# Helpers
_max_append = email.quoprimime._max_append



def decode_header(header):
    """Decode a message header value without converting charset.

    Returns a list of (decoded_string, charset) pairs containing each of the
    decoded parts of the header.  Charset is None for non-encoded parts of the
    header, otherwise a lower-case string containing the name of the character
    set specified in the encoded string.

    An email.errors.HeaderParseError may be raised when certain decoding error
    occurs (e.g. a base64 decoding exception).
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
                    decoded[-1] = (decoded[-1][0] + SPACE + unenc, None)
                else:
                    decoded.append((unenc, None))
            if parts:
                charset, encoding = [s.lower() for s in parts[0:2]]
                encoded = parts[2]
                dec = None
                if encoding == 'q':
                    dec = email.quoprimime.header_decode(encoded)
                elif encoding == 'b':
                    paderr = len(encoded) % 4   # Postel's law: add missing padding
                    if paderr:
                        encoded += '==='[:4 - paderr]
                    try:
                        dec = email.base64mime.decode(encoded)
                    except binascii.Error:
                        # Turn this into a higher level exception.  BAW: Right
                        # now we throw the lower level exception away but
                        # when/if we get exception chaining, we'll preserve it.
                        raise HeaderParseError
                if dec is None:
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
    def __init__(self, s=None, charset=None,
                 maxlinelen=None, header_name=None,
                 continuation_ws=' ', errors='strict'):
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

        errors is passed through to the .append() call.
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
            self.append(s, charset, errors)
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
        if header_name is None:
            self._headerlen = 0
        else:
            # Take the separating colon and space into account.
            self._headerlen = len(header_name) + 2

    def __str__(self):
        """A synonym for self.encode()."""
        return self.encode()

    def __unicode__(self):
        """Helper for the built-in unicode function."""
        uchunks = []
        lastcs = None
        for s, charset in self._chunks:
            # We must preserve spaces between encoded and non-encoded word
            # boundaries, which means for us we need to add a space when we go
            # from a charset to None/us-ascii, or from None/us-ascii to a
            # charset.  Only do this for the second and subsequent chunks.
            nextcs = charset
            if uchunks:
                if lastcs not in (None, 'us-ascii'):
                    if nextcs in (None, 'us-ascii'):
                        uchunks.append(USPACE)
                        nextcs = None
                elif nextcs not in (None, 'us-ascii'):
                    uchunks.append(USPACE)
            lastcs = nextcs
            uchunks.append(unicode(s, str(charset)))
        return UEMPTYSTRING.join(uchunks)

    # Rich comparison operators for equality only.  BAW: does it make sense to
    # have or explicitly disable <, <=, >, >= operators?
    def __eq__(self, other):
        # other may be a Header or a string.  Both are fine so coerce
        # ourselves to a string, swap the args and do another comparison.
        return other == self.encode()

    def __ne__(self, other):
        return not self == other

    def append(self, s, charset=None, errors='strict'):
        """Append a string to the MIME header.

        Optional charset, if given, should be a Charset instance or the name
        of a character set (which will be converted to a Charset instance).  A
        value of None (the default) means that the charset given in the
        constructor is used.

        s may be a byte string or a Unicode string.  If it is a byte string
        (i.e. isinstance(s, str) is true), then charset is the encoding of
        that byte string, and a UnicodeError will be raised if the string
        cannot be decoded with that charset.  If s is a Unicode string, then
        charset is a hint specifying the character set of the characters in
        the string.  In this case, when producing an RFC 2822 compliant header
        using RFC 2047 rules, the Unicode string will be encoded using the
        following charsets in order: us-ascii, the charset hint, utf-8.  The
        first character set not to provoke a UnicodeError is used.

        Optional `errors' is passed as the third argument to any unicode() or
        ustr.encode() call.
        """
        if charset is None:
            charset = self._charset
        elif not isinstance(charset, Charset):
            charset = Charset(charset)
        # If the charset is our faux 8bit charset, leave the string unchanged
        if charset != '8bit':
            # We need to test that the string can be converted to unicode and
            # back to a byte string, given the input and output codecs of the
            # charset.
            if isinstance(s, str):
                # Possibly raise UnicodeError if the byte string can't be
                # converted to a unicode with the input codec of the charset.
                incodec = charset.input_codec or 'us-ascii'
                ustr = unicode(s, incodec, errors)
                # Now make sure that the unicode could be converted back to a
                # byte string with the output codec, which may be different
                # than the iput coded.  Still, use the original byte string.
                outcodec = charset.output_codec or 'us-ascii'
                ustr.encode(outcodec, errors)
            elif isinstance(s, unicode):
                # Now we have to be sure the unicode string can be converted
                # to a byte string with a reasonable output codec.  We want to
                # use the byte string in the chunk.
                for charset in USASCII, charset, UTF8:
                    try:
                        outcodec = charset.output_codec or 'us-ascii'
                        s = s.encode(outcodec, errors)
                        break
                    except UnicodeError:
                        pass
                else:
                    assert False, 'utf-8 conversion failed'
        self._chunks.append((s, charset))

    def _nonctext(self, s):
        """True if string s is not a ctext character of RFC822.
        """
        return s.isspace() or s in ('(', ')', '\\')

    def encode(self, splitchars=';, \t'):
        r"""Encode a message header into an RFC-compliant format.

        There are many issues involved in converting a given string for use in
        an email header.  Only certain character sets are readable in most
        email clients, and as header strings can only contain a subset of
        7-bit ASCII, care must be taken to properly convert and encode (with
        Base64 or quoted-printable) header strings.  In addition, there is a
        75-character length limit on any given encoded header field, so
        line-wrapping must be performed, even with double-byte character sets.

        Optional splitchars is a string containing characters which should be
        given extra weight by the splitting algorithm during normal header
        wrapping.  This is in very rough support of RFC 2822's `higher level
        syntactic breaks':  split points preceded by a splitchar are preferred
        during line splitting, with the characters preferred in the order in
        which they appear in the string.  Space and tab may be included in the
        string to indicate whether preference should be given to one over the
        other as a split point when other split chars do not appear in the line
        being split.  Splitchars does not affect RFC 2047 encoded lines.
        """
        linesep = '\n'
        self._normalize()
        maxlinelen = self._maxlinelen
        # A maxlinelen of 0 means don't wrap.  For all practical purposes,
        # choosing a huge number here accomplishes that and makes the
        # _ValueFormatter algorithm much simpler.
        if maxlinelen == 0:
            maxlinelen = 1000000
        formatter = _ValueFormatter(self._headerlen, maxlinelen,
                                    self._continuation_ws, splitchars)
        lastcs = None
        hasspace = lastspace = None
        for string, charset in self._chunks:
            if hasspace is not None:
                hasspace = string and self._nonctext(string[0])
                if lastcs not in (None, 'us-ascii'):
                    if not hasspace or charset not in (None, 'us-ascii'):
                        formatter.add_transition()
                elif charset not in (None, 'us-ascii') and not lastspace:
                    formatter.add_transition()
            lastspace = string and self._nonctext(string[-1])
            lastcs = charset
            hasspace = False
            lines = string.splitlines()
            if lines:
                formatter.feed('', lines[0], charset)
            else:
                formatter.feed('', '', charset)
            for line in lines[1:]:
                formatter.newline()
                if charset.header_encoding is not None:
                    formatter.feed(self._continuation_ws, ' ' + line.lstrip(),
                                   charset)
                else:
                    sline = line.lstrip()
                    fws = line[:len(line)-len(sline)]
                    formatter.feed(fws, sline, charset)
            if len(lines) > 1:
                formatter.newline()
        if self._chunks:
            formatter.add_transition()
        value = formatter._str(linesep)
        if _embeded_header.search(value):
            raise HeaderParseError("header value appears to contain "
                "an embedded header: {!r}".format(value))
        return value

    def _normalize(self):
        # Step 1: Normalize the chunks so that all runs of identical charsets
        # get collapsed into a single unicode string.
        chunks = []
        last_charset = None
        last_chunk = []
        for string, charset in self._chunks:
            if charset == last_charset:
                last_chunk.append(string)
            else:
                if last_charset is not None:
                    chunks.append((SPACE.join(last_chunk), last_charset))
                last_chunk = [string]
                last_charset = charset
        if last_chunk:
            chunks.append((SPACE.join(last_chunk), last_charset))
        self._chunks = chunks



class _ValueFormatter:
    def __init__(self, headerlen, maxlen, continuation_ws, splitchars):
        self._maxlen = maxlen
        self._continuation_ws = continuation_ws
        self._continuation_ws_len = len(continuation_ws)
        self._splitchars = splitchars
        self._lines = []
        self._current_line = _Accumulator(headerlen)

    def _str(self, linesep):
        self.newline()
        return linesep.join(self._lines)

    def __str__(self):
        return self._str(NL)

    def newline(self):
        end_of_line = self._current_line.pop()
        if end_of_line != (' ', ''):
            self._current_line.push(*end_of_line)
        if len(self._current_line) > 0:
            if self._current_line.is_onlyws():
                self._lines[-1] += str(self._current_line)
            else:
                self._lines.append(str(self._current_line))
        self._current_line.reset()

    def add_transition(self):
        self._current_line.push(' ', '')

    def feed(self, fws, string, charset):
        # If the charset has no header encoding (i.e. it is an ASCII encoding)
        # then we must split the header at the "highest level syntactic break"
        # possible. Note that we don't have a lot of smarts about field
        # syntax; we just try to break on semi-colons, then commas, then
        # whitespace.  Eventually, this should be pluggable.
        if charset.header_encoding is None:
            self._ascii_split(fws, string, self._splitchars)
            return
        # Otherwise, we're doing either a Base64 or a quoted-printable
        # encoding which means we don't need to split the line on syntactic
        # breaks.  We can basically just find enough characters to fit on the
        # current line, minus the RFC 2047 chrome.  What makes this trickier
        # though is that we have to split at octet boundaries, not character
        # boundaries but it's only safe to split at character boundaries so at
        # best we can only get close.
        encoded_lines = charset.header_encode_lines(string, self._maxlengths())
        # The first element extends the current line, but if it's None then
        # nothing more fit on the current line so start a new line.
        try:
            first_line = encoded_lines.pop(0)
        except IndexError:
            # There are no encoded lines, so we're done.
            return
        if first_line is not None:
            self._append_chunk(fws, first_line)
        try:
            last_line = encoded_lines.pop()
        except IndexError:
            # There was only one line.
            return
        self.newline()
        self._current_line.push(self._continuation_ws, last_line)
        # Everything else are full lines in themselves.
        for line in encoded_lines:
            self._lines.append(self._continuation_ws + line)

    def _maxlengths(self):
        # The first line's length.
        yield self._maxlen - len(self._current_line)
        while True:
            yield self._maxlen - self._continuation_ws_len

    def _ascii_split(self, fws, string, splitchars):
        # The RFC 2822 header folding algorithm is simple in principle but
        # complex in practice.  Lines may be folded any place where "folding
        # white space" appears by inserting a linesep character in front of the
        # FWS.  The complication is that not all spaces or tabs qualify as FWS,
        # and we are also supposed to prefer to break at "higher level
        # syntactic breaks".  We can't do either of these without intimate
        # knowledge of the structure of structured headers, which we don't have
        # here.  So the best we can do here is prefer to break at the specified
        # splitchars, and hope that we don't choose any spaces or tabs that
        # aren't legal FWS.  (This is at least better than the old algorithm,
        # where we would sometimes *introduce* FWS after a splitchar, or the
        # algorithm before that, where we would turn all white space runs into
        # single spaces or tabs.)
        parts = re.split("(["+FWS+"]+)", fws+string)
        if parts[0]:
            parts[:0] = ['']
        else:
            parts.pop(0)
        for fws, part in zip(*[iter(parts)]*2):
            self._append_chunk(fws, part)

    def _append_chunk(self, fws, string):
        self._current_line.push(fws, string)
        if len(self._current_line) > self._maxlen:
            # Find the best split point, working backward from the end.
            # There might be none, on a long first line.
            for ch in self._splitchars:
                for i in range(self._current_line.part_count()-1, 0, -1):
                    if ch.isspace():
                        fws = self._current_line[i][0]
                        if fws and fws[0]==ch:
                            break
                    prevpart = self._current_line[i-1][1]
                    if prevpart and prevpart[-1]==ch:
                        break
                else:
                    continue
                break
            else:
                fws, part = self._current_line.pop()
                if self._current_line._initial_size > 0:
                    # There will be a header, so leave it on a line by itself.
                    self.newline()
                    if not fws:
                        # We don't use continuation_ws here because the whitespace
                        # after a header should always be a space.
                        fws = ' '
                self._current_line.push(fws, part)
                return
            remainder = self._current_line.pop_from(i)
            self._lines.append(str(self._current_line))
            self._current_line.reset(remainder)


class _Accumulator(list):

    def __init__(self, initial_size=0):
        self._initial_size = initial_size
        super(_Accumulator, self).__init__()

    def push(self, fws, string):
        self.append((fws, string))

    def pop_from(self, i=0):
        popped = self[i:]
        self[i:] = []
        return popped

    def pop(self):
        if self.part_count()==0:
            return ('', '')
        return super(_Accumulator, self).pop()

    def __len__(self):
        return sum((len(fws)+len(part) for fws, part in self),
                   self._initial_size)

    def __str__(self):
        return EMPTYSTRING.join((EMPTYSTRING.join((fws, part))
                                for fws, part in self))

    def reset(self, startval=None):
        if startval is None:
            startval = []
        self[:] = startval
        self._initial_size = 0

    def is_onlyws(self):
        return self._initial_size==0 and (not self or str(self).isspace())

    def part_count(self):
        return super(_Accumulator, self).__len__()

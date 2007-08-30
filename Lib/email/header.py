# Copyright (C) 2002-2007 Python Software Foundation
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
BSPACE = b' '
SPACE8 = ' ' * 8
EMPTYSTRING = ''

MAXLINELEN = 76

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



# Helpers
_max_append = email.quoprimime._max_append



def decode_header(header):
    """Decode a message header value without converting charset.

    Returns a list of (string, charset) pairs containing each of the decoded
    parts of the header.  Charset is None for non-encoded parts of the header,
    otherwise a lower-case string containing the name of the character set
    specified in the encoded string.

    An email.Errors.HeaderParseError may be raised when certain decoding error
    occurs (e.g. a base64 decoding exception).
    """
    # If no encoding, just return the header with no charset.
    if not ecre.search(header):
        return [(header, None)]
    # First step is to parse all the encoded parts into triplets of the form
    # (encoded_string, encoding, charset).  For unencoded strings, the last
    # two parts will be None.
    words = []
    for line in header.splitlines():
        parts = ecre.split(line)
        while parts:
            unencoded = parts.pop(0).strip()
            if unencoded:
                words.append((unencoded, None, None))
            if parts:
                charset = parts.pop(0).lower()
                encoding = parts.pop(0).lower()
                encoded = parts.pop(0)
                words.append((encoded, encoding, charset))
    # The next step is to decode each encoded word by applying the reverse
    # base64 or quopri transformation.  decoded_words is now a list of the
    # form (decoded_word, charset).
    decoded_words = []
    for encoded_string, encoding, charset in words:
        if encoding is None:
            # This is an unencoded word.
            decoded_words.append((encoded_string, charset))
        elif encoding == 'q':
            word = email.quoprimime.header_decode(encoded_string)
            decoded_words.append((word, charset))
        elif encoding == 'b':
            try:
                word = email.base64mime.decode(encoded_string)
            except binascii.Error:
                raise HeaderParseError('Base64 decoding error')
            else:
                decoded_words.append((word, charset))
        else:
            raise AssertionError('Unexpected encoding: ' + encoding)
    # Now convert all words to bytes and collapse consecutive runs of
    # similarly encoded words.
    collapsed = []
    last_word = last_charset = None
    for word, charset in decoded_words:
        if isinstance(word, str):
            word = bytes(ord(c) for c in word)
        if last_word is None:
            last_word = word
            last_charset = charset
        elif charset != last_charset:
            collapsed.append((last_word, last_charset))
            last_word = word
            last_charset = charset
        elif last_charset is None:
            last_word += BSPACE + word
        else:
            last_word += word
    collapsed.append((last_word, last_charset))
    return collapsed



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
        elif not isinstance(charset, Charset):
            charset = Charset(charset)
        self._charset = charset
        self._continuation_ws = continuation_ws
        self._chunks = []
        if s is not None:
            self.append(s, charset, errors)
        if maxlinelen is None:
            maxlinelen = MAXLINELEN
        self._maxlinelen = maxlinelen
        if header_name is None:
            self._headerlen = 0
        else:
            # Take the separating colon and space into account.
            self._headerlen = len(header_name) + 2

    def __str__(self):
        """Return the string value of the header."""
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
                        uchunks.append(SPACE)
                        nextcs = None
                elif nextcs not in (None, 'us-ascii'):
                    uchunks.append(SPACE)
            lastcs = nextcs
            uchunks.append(s)
        return EMPTYSTRING.join(uchunks)

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
        if isinstance(s, str):
            # Convert the string from the input character set to the output
            # character set and store the resulting bytes and the charset for
            # composition later.
            input_charset = charset.input_codec or 'us-ascii'
            input_bytes = s.encode(input_charset, errors)
        else:
            # We already have the bytes we will store internally.
            input_bytes = s
        # Ensure that the bytes we're storing can be decoded to the output
        # character set, otherwise an early error is thrown.
        output_charset = charset.output_codec or 'us-ascii'
        output_string = input_bytes.decode(output_charset, errors)
        self._chunks.append((output_string, charset))

    def encode(self, splitchars=';, \t'):
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

        Optional splitchars is a string containing characters to split long
        ASCII lines on, in rough support of RFC 2822's `highest level
        syntactic breaks'.  This doesn't affect RFC 2047 encoded lines.
        """
        self._normalize()
        formatter = _ValueFormatter(self._headerlen, self._maxlinelen,
                                    self._continuation_ws, splitchars)
        for string, charset in self._chunks:
            lines = string.splitlines()
            for line in lines:
                formatter.feed(line, charset)
                if len(lines) > 1:
                    formatter.newline()
        return str(formatter)

    def _normalize(self):
        # Normalize the chunks so that all runs of identical charsets get
        # collapsed into a single unicode string.  You need a space between
        # encoded words, or between encoded and unencoded words.
        chunks = []
        last_charset = None
        last_chunk = []
        for string, charset in self._chunks:
            if charset == last_charset:
                last_chunk.append(string)
            else:
                if last_charset is not None:
                    chunks.append((SPACE.join(last_chunk), last_charset))
                    if last_charset != USASCII or charset != USASCII:
                        chunks.append((' ', USASCII))
                last_chunk = [string]
                last_charset = charset
        if last_chunk:
            chunks.append((SPACE.join(last_chunk), last_charset))
        self._chunks = chunks



class _ValueFormatter:
    def __init__(self, headerlen, maxlen, continuation_ws, splitchars):
        self._maxlen = maxlen
        self._continuation_ws = continuation_ws
        self._continuation_ws_len = len(continuation_ws.replace('\t', SPACE8))
        self._splitchars = splitchars
        self._lines = []
        self._current_line = _Accumulator(headerlen)

    def __str__(self):
        self.newline()
        return NL.join(self._lines)

    def newline(self):
        if len(self._current_line) > 0:
            self._lines.append(str(self._current_line))
        self._current_line.reset()

    def feed(self, string, charset):
        # If the string itself fits on the current line in its encoded format,
        # then add it now and be done with it.
        encoded_string = charset.header_encode(string)
        if len(encoded_string) + len(self._current_line) <= self._maxlen:
            self._current_line.push(encoded_string)
            return
        # Attempt to split the line at the highest-level syntactic break
        # possible.  Note that we don't have a lot of smarts about field
        # syntax; we just try to break on semi-colons, then commas, then
        # whitespace.  Eventually, we'll allow this to be pluggable.
        for ch in self._splitchars:
            if ch in string:
                break
        else:
            # We can't split the string to fit on the current line, so just
            # put it on a line by itself.
            self._lines.append(str(self._current_line))
            self._current_line.reset(self._continuation_ws)
            self._current_line.push(encoded_string)
            return
        self._spliterate(string, ch, charset)

    def _spliterate(self, string, ch, charset):
        holding = _Accumulator(transformfunc=charset.header_encode)
        # Split the line on the split character, preserving it.  If the split
        # character is whitespace RFC 2822 $2.2.3 requires us to fold on the
        # whitespace, so that the line leads with the original whitespace we
        # split on.  However, if a higher syntactic break is used instead
        # (e.g. comma or semicolon), the folding should happen after the split
        # character.  But then in that case, we need to add our own
        # continuation whitespace -- although won't that break unfolding?
        for part, splitpart, nextpart in _spliterator(ch, string):
            if not splitpart:
                # No splitpart means this is the last chunk.  Put this part
                # either on the current line or the next line depending on
                # whether it fits.
                holding.push(part)
                if len(holding) + len(self._current_line) <= self._maxlen:
                    # It fits, but we're done.
                    self._current_line.push(str(holding))
                else:
                    # It doesn't fit, but we're done.  Before pushing a new
                    # line, watch out for the current line containing only
                    # whitespace.
                    holding.pop()
                    if len(self._current_line) == 0 and (
                        len(holding) == 0 or str(holding).isspace()):
                        # Don't start a new line.
                        holding.push(part)
                        part = None
                    self._current_line.push(str(holding))
                    self._lines.append(str(self._current_line))
                    if part is None:
                        self._current_line.reset()
                    else:
                        holding.reset(part)
                        self._current_line.reset(str(holding))
                return
            elif not nextpart:
                # There must be some trailing split characters because we
                # found a split character but no next part.  In this case we
                # must treat the thing to fit as the part + splitpart because
                # if splitpart is whitespace it's not allowed to be the only
                # thing on the line, and if it's not whitespace we must split
                # after the syntactic break.  In either case, we're done.
                holding_prelen = len(holding)
                holding.push(part + splitpart)
                if len(holding) + len(self._current_line) <= self._maxlen:
                    self._current_line.push(str(holding))
                elif holding_prelen == 0:
                    # This is the only chunk left so it has to go on the
                    # current line.
                    self._current_line.push(str(holding))
                else:
                    save_part = holding.pop()
                    self._current_line.push(str(holding))
                    self._lines.append(str(self._current_line))
                    holding.reset(save_part)
                    self._current_line.reset(str(holding))
                return
            elif not part:
                # We're leading with a split character.  See if the splitpart
                # and nextpart fits on the current line.
                holding.push(splitpart + nextpart)
                holding_len = len(holding)
                # We know we're not leaving the nextpart on the stack.
                holding.pop()
                if holding_len + len(self._current_line) <= self._maxlen:
                    holding.push(splitpart)
                else:
                    # It doesn't fit.  Since there's no current part really
                    # the best we can do is start a new line and push the
                    # split part onto it.
                    self._current_line.push(str(holding))
                    holding.reset()
                    if len(self._current_line) > 0 and self._lines:
                        self._lines.append(str(self._current_line))
                        self._current_line.reset()
                    holding.push(splitpart)
            else:
                # All three parts are present.  First let's see if all three
                # parts will fit on the current line.  If so, we don't need to
                # split it.
                holding.push(part + splitpart + nextpart)
                holding_len = len(holding)
                # Pop the part because we'll push nextpart on the next
                # iteration through the loop.
                holding.pop()
                if holding_len + len(self._current_line) <= self._maxlen:
                    holding.push(part + splitpart)
                else:
                    # The entire thing doesn't fit.  See if we need to split
                    # before or after the split characters.
                    if splitpart.isspace():
                        # Split before whitespace.  Remember that the
                        # whitespace becomes the continuation whitespace of
                        # the next line so it goes to current_line not holding.
                        holding.push(part)
                        self._current_line.push(str(holding))
                        holding.reset()
                        self._lines.append(str(self._current_line))
                        self._current_line.reset(splitpart)
                    else:
                        # Split after non-whitespace.  The continuation
                        # whitespace comes from the instance variable.
                        holding.push(part + splitpart)
                        self._current_line.push(str(holding))
                        holding.reset()
                        self._lines.append(str(self._current_line))
                        if nextpart[0].isspace():
                            self._current_line.reset()
                        else:
                            self._current_line.reset(self._continuation_ws)
        # Get the last of the holding part
        self._current_line.push(str(holding))



def _spliterator(character, string):
    parts = list(reversed(re.split('(%s)' % character, string)))
    while parts:
        part = parts.pop()
        splitparts = (parts.pop() if parts else None)
        nextpart = (parts.pop() if parts else None)
        yield (part, splitparts, nextpart)
        if nextpart is not None:
            parts.append(nextpart)


class _Accumulator:
    def __init__(self, initial_size=0, transformfunc=None):
        self._initial_size = initial_size
        if transformfunc is None:
            self._transformfunc = lambda string: string
        else:
            self._transformfunc = transformfunc
        self._current = []

    def push(self, string):
        self._current.append(string)

    def pop(self):
        return self._current.pop()

    def __len__(self):
        return len(str(self)) + self._initial_size

    def __str__(self):
        return self._transformfunc(EMPTYSTRING.join(self._current))

    def reset(self, string=None):
        self._current = []
        self._current_len = 0
        self._initial_size = 0
        if string is not None:
            self.push(string)

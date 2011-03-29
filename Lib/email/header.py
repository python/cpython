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
from email import charset as _charset
Charset = _charset.Charset

NL = '\n'
SPACE = ' '
BSPACE = b' '
SPACE8 = ' ' * 8
EMPTYSTRING = ''
MAXLINELEN = 78

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

    Returns a list of (string, charset) pairs containing each of the decoded
    parts of the header.  Charset is None for non-encoded parts of the header,
    otherwise a lower-case string containing the name of the character set
    specified in the encoded string.

    header may be a string that may or may not contain RFC2047 encoded words,
    or it may be a Header object.

    An email.errors.HeaderParseError may be raised when certain decoding error
    occurs (e.g. a base64 decoding exception).
    """
    # If it is a Header object, we can just return the chunks.
    if hasattr(header, '_chunks'):
        return list(header._chunks)
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
            paderr = len(encoded_string) % 4   # Postel's law: add missing padding
            if paderr:
                encoded_string += '==='[:4 - paderr]
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
            word = bytes(word, 'raw-unicode-escape')
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

        The maximum line length can be specified explicitly via maxlinelen. For
        splitting the first line to a shorter value (to account for the field
        header which isn't included in s, e.g. `Subject') pass in the name of
        the field in header_name.  The default maxlinelen is 78 as recommended
        by RFC 2822.

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
        self._normalize()
        uchunks = []
        lastcs = None
        for string, charset in self._chunks:
            # We must preserve spaces between encoded and non-encoded word
            # boundaries, which means for us we need to add a space when we go
            # from a charset to None/us-ascii, or from None/us-ascii to a
            # charset.  Only do this for the second and subsequent chunks.
            nextcs = charset
            if nextcs == _charset.UNKNOWN8BIT:
                original_bytes = string.encode('ascii', 'surrogateescape')
                string = original_bytes.decode('ascii', 'replace')
            if uchunks:
                if lastcs not in (None, 'us-ascii'):
                    if nextcs in (None, 'us-ascii'):
                        uchunks.append(SPACE)
                        nextcs = None
                elif nextcs not in (None, 'us-ascii'):
                    uchunks.append(SPACE)
            lastcs = nextcs
            uchunks.append(string)
        return EMPTYSTRING.join(uchunks)

    # Rich comparison operators for equality only.  BAW: does it make sense to
    # have or explicitly disable <, <=, >, >= operators?
    def __eq__(self, other):
        # other may be a Header or a string.  Both are fine so coerce
        # ourselves to a unicode (of the unencoded header value), swap the
        # args and do another comparison.
        return other == str(self)

    def __ne__(self, other):
        return not self == other

    def append(self, s, charset=None, errors='strict'):
        """Append a string to the MIME header.

        Optional charset, if given, should be a Charset instance or the name
        of a character set (which will be converted to a Charset instance).  A
        value of None (the default) means that the charset given in the
        constructor is used.

        s may be a byte string or a Unicode string.  If it is a byte string
        (i.e. isinstance(s, str) is false), then charset is the encoding of
        that byte string, and a UnicodeError will be raised if the string
        cannot be decoded with that charset.  If s is a Unicode string, then
        charset is a hint specifying the character set of the characters in
        the string.  In either case, when producing an RFC 2822 compliant
        header using RFC 2047 rules, the string will be encoded using the
        output codec of the charset.  If the string cannot be encoded to the
        output codec, a UnicodeError will be raised.

        Optional `errors' is passed as the errors argument to the decode
        call if s is a byte string.
        """
        if charset is None:
            charset = self._charset
        elif not isinstance(charset, Charset):
            charset = Charset(charset)
        if not isinstance(s, str):
            input_charset = charset.input_codec or 'us-ascii'
            s = s.decode(input_charset, errors)
        # Ensure that the bytes we're storing can be decoded to the output
        # character set, otherwise an early error is thrown.
        output_charset = charset.output_codec or 'us-ascii'
        if output_charset != _charset.UNKNOWN8BIT:
            s.encode(output_charset, errors)
        self._chunks.append((s, charset))

    def encode(self, splitchars=';, \t', maxlinelen=None, linesep='\n'):
        r"""Encode a message header into an RFC-compliant format.

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

        Optional linesep is a string to be used to separate the lines of
        the value.  The default value is the most useful for typical
        Python applications, but it can be set to \r\n to produce RFC-compliant
        line separators when needed.
        """
        self._normalize()
        if maxlinelen is None:
            maxlinelen = self._maxlinelen
        # A maxlinelen of 0 means don't wrap.  For all practical purposes,
        # choosing a huge number here accomplishes that and makes the
        # _ValueFormatter algorithm much simpler.
        if maxlinelen == 0:
            maxlinelen = 1000000
        formatter = _ValueFormatter(self._headerlen, maxlinelen,
                                    self._continuation_ws, splitchars)
        for string, charset in self._chunks:
            lines = string.splitlines()
            formatter.feed(lines[0] if lines else '', charset)
            for line in lines[1:]:
                formatter.newline()
                if charset.header_encoding is not None:
                    formatter.feed(self._continuation_ws, USASCII)
                    line = ' ' + line.lstrip()
                formatter.feed(line, charset)
            if len(lines) > 1:
                formatter.newline()
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
        self._continuation_ws_len = len(continuation_ws.replace('\t', SPACE8))
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
        if end_of_line is not None:
            self._current_line.push(end_of_line)
        if len(self._current_line) > 0:
            self._lines.append(str(self._current_line))
        self._current_line.reset()

    def add_transition(self):
        self._current_line.push(None)

    def feed(self, string, charset):
        # If the string itself fits on the current line in its encoded format,
        # then add it now and be done with it.
        encoded_string = charset.header_encode(string)
        if len(encoded_string) + len(self._current_line) <= self._maxlen:
            self._current_line.push(encoded_string)
            return
        # If the charset has no header encoding (i.e. it is an ASCII encoding)
        # then we must split the header at the "highest level syntactic break"
        # possible. Note that we don't have a lot of smarts about field
        # syntax; we just try to break on semi-colons, then commas, then
        # whitespace.  Eventually, this should be pluggable.
        if charset.header_encoding is None:
            for ch in self._splitchars:
                if ch in string:
                    break
            else:
                ch = None
            # If there's no available split character then regardless of
            # whether the string fits on the line, we have to put it on a line
            # by itself.
            if ch is None:
                if not self._current_line.is_onlyws():
                    self._lines.append(str(self._current_line))
                    self._current_line.reset(self._continuation_ws)
                self._current_line.push(encoded_string)
            else:
                self._ascii_split(string, ch)
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
            self._current_line.push(first_line)
        self._lines.append(str(self._current_line))
        self._current_line.reset(self._continuation_ws)
        try:
            last_line = encoded_lines.pop()
        except IndexError:
            # There was only one line.
            return
        self._current_line.push(last_line)
        # Everything else are full lines in themselves.
        for line in encoded_lines:
            self._lines.append(self._continuation_ws + line)

    def _maxlengths(self):
        # The first line's length.
        yield self._maxlen - len(self._current_line)
        while True:
            yield self._maxlen - self._continuation_ws_len

    def _ascii_split(self, string, ch):
        holding = _Accumulator()
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
                    if self._current_line.is_onlyws() and holding.is_onlyws():
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
    def __init__(self, initial_size=0):
        self._initial_size = initial_size
        self._current = []

    def push(self, string):
        self._current.append(string)

    def pop(self):
        if not self._current:
            return None
        return self._current.pop()

    def __len__(self):
        return sum(((1 if string is None else len(string))
                    for string in self._current),
                   self._initial_size)

    def __str__(self):
        if self._current and self._current[-1] is None:
            self._current.pop()
        return EMPTYSTRING.join((' ' if string is None else string)
                                for string in self._current)

    def reset(self, string=None):
        self._current = []
        self._initial_size = 0
        if string is not None:
            self.push(string)

    def is_onlyws(self):
        return len(self) == 0 or str(self).isspace()

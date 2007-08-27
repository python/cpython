#! /usr/bin/env python

"""RFC 3548: Base16, Base32, Base64 Data Encodings"""

# Modified 04-Oct-1995 by Jack Jansen to use binascii module
# Modified 30-Dec-2003 by Barry Warsaw to add full RFC 3548 support
# Modified 22-May-2007 by Guido van Rossum to use bytes everywhere

import re
import struct
import binascii


__all__ = [
    # Legacy interface exports traditional RFC 1521 Base64 encodings
    'encode', 'decode', 'encodestring', 'decodestring',
    # Generalized interface for other encodings
    'b64encode', 'b64decode', 'b32encode', 'b32decode',
    'b16encode', 'b16decode',
    # Standard Base64 encoding
    'standard_b64encode', 'standard_b64decode',
    # Some common Base64 alternatives.  As referenced by RFC 3458, see thread
    # starting at:
    #
    # http://zgp.org/pipermail/p2p-hackers/2001-September/000316.html
    'urlsafe_b64encode', 'urlsafe_b64decode',
    ]


def _translate(s, altchars):
    if not isinstance(s, bytes):
        raise TypeError("expected bytes, not %s" % s.__class__.__name__)
    translation = bytes(range(256))
    for k, v in altchars.items():
        translation[ord(k)] = v[0]
    return s.translate(translation)



# Base64 encoding/decoding uses binascii

def b64encode(s, altchars=None):
    """Encode a byte string using Base64.

    s is the byte string to encode.  Optional altchars must be a byte
    string of length 2 which specifies an alternative alphabet for the
    '+' and '/' characters.  This allows an application to
    e.g. generate url or filesystem safe Base64 strings.

    The encoded byte string is returned.
    """
    if not isinstance(s, bytes):
        s = bytes(s)
    # Strip off the trailing newline
    encoded = binascii.b2a_base64(s)[:-1]
    if altchars is not None:
        if not isinstance(altchars, bytes):
            altchars = bytes(altchars, "ascii")
        assert len(altchars) == 2, repr(altchars)
        return _translate(encoded, {'+': altchars[0:1], '/': altchars[1:2]})
    return encoded


def b64decode(s, altchars=None):
    """Decode a Base64 encoded byte string.

    s is the byte string to decode.  Optional altchars must be a
    string of length 2 which specifies the alternative alphabet used
    instead of the '+' and '/' characters.

    The decoded byte string is returned.  binascii.Error is raised if
    s were incorrectly padded or if there are non-alphabet characters
    present in the string.
    """
    if not isinstance(s, bytes):
        s = bytes(s)
    if altchars is not None:
        if not isinstance(altchars, bytes):
            altchars = bytes(altchars, "ascii")
        assert len(altchars) == 2, repr(altchars)
        s = _translate(s, {chr(altchars[0]): b'+', chr(altchars[1]): b'/'})
    return binascii.a2b_base64(s)


def standard_b64encode(s):
    """Encode a byte string using the standard Base64 alphabet.

    s is the byte string to encode.  The encoded byte string is returned.
    """
    return b64encode(s)

def standard_b64decode(s):
    """Decode a byte string encoded with the standard Base64 alphabet.

    s is the byte string to decode.  The decoded byte string is
    returned.  binascii.Error is raised if the input is incorrectly
    padded or if there are non-alphabet characters present in the
    input.
    """
    return b64decode(s)

def urlsafe_b64encode(s):
    """Encode a byte string using a url-safe Base64 alphabet.

    s is the byte string to encode.  The encoded byte string is
    returned.  The alphabet uses '-' instead of '+' and '_' instead of
    '/'.
    """
    return b64encode(s, b'-_')

def urlsafe_b64decode(s):
    """Decode a byte string encoded with the standard Base64 alphabet.

    s is the byte string to decode.  The decoded byte string is
    returned.  binascii.Error is raised if the input is incorrectly
    padded or if there are non-alphabet characters present in the
    input.

    The alphabet uses '-' instead of '+' and '_' instead of '/'.
    """
    return b64decode(s, b'-_')



# Base32 encoding/decoding must be done in Python
_b32alphabet = {
    0: b'A',  9: b'J', 18: b'S', 27: b'3',
    1: b'B', 10: b'K', 19: b'T', 28: b'4',
    2: b'C', 11: b'L', 20: b'U', 29: b'5',
    3: b'D', 12: b'M', 21: b'V', 30: b'6',
    4: b'E', 13: b'N', 22: b'W', 31: b'7',
    5: b'F', 14: b'O', 23: b'X',
    6: b'G', 15: b'P', 24: b'Y',
    7: b'H', 16: b'Q', 25: b'Z',
    8: b'I', 17: b'R', 26: b'2',
    }

_b32tab = [v[0] for k, v in sorted(_b32alphabet.items())]
_b32rev = dict([(v[0], k) for k, v in _b32alphabet.items()])


def b32encode(s):
    """Encode a byte string using Base32.

    s is the byte string to encode.  The encoded byte string is returned.
    """
    if not isinstance(s, bytes):
        s = bytes(s)
    quanta, leftover = divmod(len(s), 5)
    # Pad the last quantum with zero bits if necessary
    if leftover:
        s = s + bytes(5 - leftover)  # Don't use += !
        quanta += 1
    encoded = bytes()
    for i in range(quanta):
        # c1 and c2 are 16 bits wide, c3 is 8 bits wide.  The intent of this
        # code is to process the 40 bits in units of 5 bits.  So we take the 1
        # leftover bit of c1 and tack it onto c2.  Then we take the 2 leftover
        # bits of c2 and tack them onto c3.  The shifts and masks are intended
        # to give us values of exactly 5 bits in width.
        c1, c2, c3 = struct.unpack('!HHB', s[i*5:(i+1)*5])
        c2 += (c1 & 1) << 16 # 17 bits wide
        c3 += (c2 & 3) << 8  # 10 bits wide
        encoded += bytes([_b32tab[c1 >> 11],         # bits 1 - 5
                          _b32tab[(c1 >> 6) & 0x1f], # bits 6 - 10
                          _b32tab[(c1 >> 1) & 0x1f], # bits 11 - 15
                          _b32tab[c2 >> 12],         # bits 16 - 20 (1 - 5)
                          _b32tab[(c2 >> 7) & 0x1f], # bits 21 - 25 (6 - 10)
                          _b32tab[(c2 >> 2) & 0x1f], # bits 26 - 30 (11 - 15)
                          _b32tab[c3 >> 5],          # bits 31 - 35 (1 - 5)
                          _b32tab[c3 & 0x1f],        # bits 36 - 40 (1 - 5)
                          ])
    # Adjust for any leftover partial quanta
    if leftover == 1:
        return encoded[:-6] + b'======'
    elif leftover == 2:
        return encoded[:-4] + b'===='
    elif leftover == 3:
        return encoded[:-3] + b'==='
    elif leftover == 4:
        return encoded[:-1] + b'='
    return encoded


def b32decode(s, casefold=False, map01=None):
    """Decode a Base32 encoded byte string.

    s is the byte string to decode.  Optional casefold is a flag
    specifying whether a lowercase alphabet is acceptable as input.
    For security purposes, the default is False.

    RFC 3548 allows for optional mapping of the digit 0 (zero) to the
    letter O (oh), and for optional mapping of the digit 1 (one) to
    either the letter I (eye) or letter L (el).  The optional argument
    map01 when not None, specifies which letter the digit 1 should be
    mapped to (when map01 is not None, the digit 0 is always mapped to
    the letter O).  For security purposes the default is None, so that
    0 and 1 are not allowed in the input.

    The decoded byte string is returned.  binascii.Error is raised if
    the input is incorrectly padded or if there are non-alphabet
    characters present in the input.
    """
    if not isinstance(s, bytes):
        s = bytes(s)
    quanta, leftover = divmod(len(s), 8)
    if leftover:
        raise binascii.Error('Incorrect padding')
    # Handle section 2.4 zero and one mapping.  The flag map01 will be either
    # False, or the character to map the digit 1 (one) to.  It should be
    # either L (el) or I (eye).
    if map01:
        if not isinstance(map01, bytes):
            map01 = bytes(map01)
        assert len(map01) == 1, repr(map01)
        s = _translate(s, {'0': b'O', '1': map01})
    if casefold:
        s = bytes(str(s, "ascii").upper(), "ascii")
    # Strip off pad characters from the right.  We need to count the pad
    # characters because this will tell us how many null bytes to remove from
    # the end of the decoded string.
    padchars = 0
    mo = re.search('(?P<pad>[=]*)$', s)
    if mo:
        padchars = len(mo.group('pad'))
        if padchars > 0:
            s = s[:-padchars]
    # Now decode the full quanta
    parts = []
    acc = 0
    shift = 35
    for c in s:
        val = _b32rev.get(c)
        if val is None:
            raise TypeError('Non-base32 digit found')
        acc += _b32rev[c] << shift
        shift -= 5
        if shift < 0:
            parts.append(binascii.unhexlify('%010x' % acc))
            acc = 0
            shift = 35
    # Process the last, partial quanta
    last = binascii.unhexlify(bytes('%010x' % acc, "ascii"))
    if padchars == 0:
        last = b''                      # No characters
    elif padchars == 1:
        last = last[:-1]
    elif padchars == 3:
        last = last[:-2]
    elif padchars == 4:
        last = last[:-3]
    elif padchars == 6:
        last = last[:-4]
    else:
        raise binascii.Error('Incorrect padding')
    parts.append(last)
    return b''.join(parts)



# RFC 3548, Base 16 Alphabet specifies uppercase, but hexlify() returns
# lowercase.  The RFC also recommends against accepting input case
# insensitively.
def b16encode(s):
    """Encode a byte string using Base16.

    s is the byte string to encode.  The encoded byte string is returned.
    """
    return bytes(str(binascii.hexlify(s), "ascii").upper(), "ascii")


def b16decode(s, casefold=False):
    """Decode a Base16 encoded byte string.

    s is the byte string to decode.  Optional casefold is a flag
    specifying whether a lowercase alphabet is acceptable as input.
    For security purposes, the default is False.

    The decoded byte string is returned.  binascii.Error is raised if
    s were incorrectly padded or if there are non-alphabet characters
    present in the string.
    """
    if not isinstance(s, bytes):
        s = bytes(s)
    if casefold:
        s = bytes(str(s, "ascii").upper(), "ascii")
    if re.search('[^0-9A-F]', s):
        raise binascii.Error('Non-base16 digit found')
    return binascii.unhexlify(s)



# Legacy interface.  This code could be cleaned up since I don't believe
# binascii has any line length limitations.  It just doesn't seem worth it
# though.  The files should be opened in binary mode.

MAXLINESIZE = 76 # Excluding the CRLF
MAXBINSIZE = (MAXLINESIZE//4)*3

def encode(input, output):
    """Encode a file; input and output are binary files."""
    while True:
        s = input.read(MAXBINSIZE)
        if not s:
            break
        while len(s) < MAXBINSIZE:
            ns = input.read(MAXBINSIZE-len(s))
            if not ns:
                break
            s += ns
        line = binascii.b2a_base64(s)
        output.write(line)


def decode(input, output):
    """Decode a file; input and output are binary files."""
    while True:
        line = input.readline()
        if not line:
            break
        s = binascii.a2b_base64(line)
        output.write(s)


def encodestring(s):
    """Encode a string into multiple lines of base-64 data.

    Argument and return value are bytes.
    """
    if not isinstance(s, bytes):
        raise TypeError("expected bytes, not %s" % s.__class__.__name__)
    pieces = []
    for i in range(0, len(s), MAXBINSIZE):
        chunk = s[i : i + MAXBINSIZE]
        pieces.append(binascii.b2a_base64(chunk))
    return b"".join(pieces)


def decodestring(s):
    """Decode a string.

    Argument and return value are bytes.
    """
    if not isinstance(s, bytes):
        raise TypeError("expected bytes, not %s" % s.__class__.__name__)
    return binascii.a2b_base64(s)



# Usable as a script...
def main():
    """Small main program"""
    import sys, getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'deut')
    except getopt.error as msg:
        sys.stdout = sys.stderr
        print(msg)
        print("""usage: %s [-d|-e|-u|-t] [file|-]
        -d, -u: decode
        -e: encode (default)
        -t: encode and decode string 'Aladdin:open sesame'"""%sys.argv[0])
        sys.exit(2)
    func = encode
    for o, a in opts:
        if o == '-e': func = encode
        if o == '-d': func = decode
        if o == '-u': func = decode
        if o == '-t': test(); return
    if args and args[0] != '-':
        func(open(args[0], 'rb'), sys.stdout)
    else:
        func(sys.stdin, sys.stdout)


def test():
    s0 = b"Aladdin:open sesame"
    print(repr(s0))
    s1 = encodestring(s0)
    print(repr(s1))
    s2 = decodestring(s1)
    print(repr(s2))
    assert s0 == s2


if __name__ == '__main__':
    main()

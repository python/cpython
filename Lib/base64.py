"""Base16, Base32, Base64 (RFC 3548), Base85 and Ascii85 data encodings"""

# Modified 04-Oct-1995 by Jack Jansen to use binascii module
# Modified 30-Dec-2003 by Barry Warsaw to add full RFC 3548 support
# Modified 22-May-2007 by Guido van Rossum to use bytes everywhere

import binascii


__all__ = [
    # Legacy interface exports traditional RFC 2045 Base64 encodings
    'encode', 'decode', 'encodebytes', 'decodebytes',
    # Generalized interface for other encodings
    'b64encode', 'b64decode', 'b32encode', 'b32decode',
    'b32hexencode', 'b32hexdecode', 'b16encode', 'b16decode',
    # Base85 and Ascii85 encodings
    'b85encode', 'b85decode', 'a85encode', 'a85decode', 'z85encode', 'z85decode',
    # Standard Base64 encoding
    'standard_b64encode', 'standard_b64decode',
    # Some common Base64 alternatives.  As referenced by RFC 3458, see thread
    # starting at:
    #
    # http://zgp.org/pipermail/p2p-hackers/2001-September/000316.html
    'urlsafe_b64encode', 'urlsafe_b64decode',
    ]


_NOT_SPECIFIED = ['NOT SPECIFIED']

bytes_types = (bytes, bytearray)  # Types acceptable as binary data

def _bytes_from_decode_data(s):
    if isinstance(s, str):
        try:
            return s.encode('ascii')
        except UnicodeEncodeError:
            raise ValueError('string argument should contain only ASCII characters')
    if isinstance(s, bytes_types):
        return s
    try:
        return memoryview(s).tobytes()
    except TypeError:
        raise TypeError("argument should be a bytes-like object or ASCII "
                        "string, not %r" % s.__class__.__name__) from None


# Base64 encoding/decoding uses binascii

def b64encode(s, altchars=None, *, wrapcol=0):
    """Encode the bytes-like object s using Base64 and return a bytes object.

    Optional altchars should be a byte string of length 2 which specifies an
    alternative alphabet for the '+' and '/' characters.  This allows an
    application to e.g. generate url or filesystem safe Base64 strings.

    If wrapcol is non-zero, insert a newline (b'\\n') character after at most
    every wrapcol characters.
    """
    if altchars is not None:
        if len(altchars) != 2:
            raise ValueError(f'invalid altchars: {altchars!r}')
        alphabet = binascii.BASE64_ALPHABET[:-2] + altchars
        return binascii.b2a_base64(s, wrapcol=wrapcol, newline=False,
                                   alphabet=alphabet)
    return binascii.b2a_base64(s, wrapcol=wrapcol, newline=False)


def b64decode(s, altchars=None, validate=_NOT_SPECIFIED, *, ignorechars=_NOT_SPECIFIED):
    """Decode the Base64 encoded bytes-like object or ASCII string s.

    Optional altchars must be a bytes-like object or ASCII string of length 2
    which specifies the alternative alphabet used instead of the '+' and '/'
    characters.

    The result is returned as a bytes object.  A binascii.Error is raised if
    s is incorrectly padded.

    If ignorechars is specified, it should be a byte string containing
    characters to ignore from the input.  The default value of validate is
    True if ignorechars is specified, False otherwise.

    If validate is false, characters that are neither in the normal base-64
    alphabet nor the alternative alphabet are discarded prior to the
    padding check.  If validate is true, these non-alphabet characters in
    the input result in a binascii.Error if they are not in ignorechars.
    For more information about the strict base64 check, see:

    https://docs.python.org/3.11/library/binascii.html#binascii.a2b_base64
    """
    s = _bytes_from_decode_data(s)
    if validate is _NOT_SPECIFIED:
        validate = ignorechars is not _NOT_SPECIFIED
    badchar = None
    if altchars is not None:
        altchars = _bytes_from_decode_data(altchars)
        if len(altchars) != 2:
            raise ValueError(f'invalid altchars: {altchars!r}')
        if ignorechars is _NOT_SPECIFIED:
            for b in b'+/':
                if b not in altchars and b in s:
                    badchar = b
                    break
            s = s.translate(bytes.maketrans(altchars, b'+/'))
        else:
            alphabet = binascii.BASE64_ALPHABET[:-2] + altchars
            return binascii.a2b_base64(s, strict_mode=validate,
                                       alphabet=alphabet,
                                       ignorechars=ignorechars)
    if ignorechars is _NOT_SPECIFIED:
        ignorechars = b''
    result = binascii.a2b_base64(s, strict_mode=validate,
                                 ignorechars=ignorechars)
    if badchar is not None:
        import warnings
        if validate:
            warnings.warn(f'invalid character {chr(badchar)!a} in Base64 data '
                          f'with altchars={altchars!r} and validate=True '
                          f'will be an error in future Python versions',
                          DeprecationWarning, stacklevel=2)
        else:
            warnings.warn(f'invalid character {chr(badchar)!a} in Base64 data '
                          f'with altchars={altchars!r} and validate=False '
                          f'will be discarded in future Python versions',
                          FutureWarning, stacklevel=2)
    return result


def standard_b64encode(s):
    """Encode bytes-like object s using the standard Base64 alphabet.

    The result is returned as a bytes object.
    """
    return b64encode(s)

def standard_b64decode(s):
    """Decode bytes encoded with the standard Base64 alphabet.

    Argument s is a bytes-like object or ASCII string to decode.  The result
    is returned as a bytes object.  A binascii.Error is raised if the input
    is incorrectly padded.  Characters that are not in the standard alphabet
    are discarded prior to the padding check.
    """
    return b64decode(s)


_urlsafe_decode_translation = bytes.maketrans(b'-_', b'+/')

def urlsafe_b64encode(s):
    """Encode bytes using the URL- and filesystem-safe Base64 alphabet.

    Argument s is a bytes-like object to encode.  The result is returned as a
    bytes object.  The alphabet uses '-' instead of '+' and '_' instead of
    '/'.
    """
    return binascii.b2a_base64(s, newline=False,
                               alphabet=binascii.URLSAFE_BASE64_ALPHABET)

def urlsafe_b64decode(s):
    """Decode bytes using the URL- and filesystem-safe Base64 alphabet.

    Argument s is a bytes-like object or ASCII string to decode.  The result
    is returned as a bytes object.  A binascii.Error is raised if the input
    is incorrectly padded.  Characters that are not in the URL-safe base-64
    alphabet, and are not a plus '+' or slash '/', are discarded prior to the
    padding check.

    The alphabet uses '-' instead of '+' and '_' instead of '/'.
    """
    s = _bytes_from_decode_data(s)
    badchar = None
    for b in b'+/':
        if b in s:
            badchar = b
            break
    s = s.translate(_urlsafe_decode_translation)
    result = binascii.a2b_base64(s, strict_mode=False)
    if badchar is not None:
        import warnings
        warnings.warn(f'invalid character {chr(badchar)!a} in URL-safe Base64 data '
                      f'will be discarded in future Python versions',
                      FutureWarning, stacklevel=2)
    return result



# Base32 encoding/decoding must be done in Python
_B32_ENCODE_DOCSTRING = '''
Encode the bytes-like objects using {encoding} and return a bytes object.

If wrapcol is non-zero, insert a newline (b'\\n') character after at most
every wrapcol characters.
'''
_B32_DECODE_DOCSTRING = '''
Decode the {encoding} encoded bytes-like object or ASCII string s.

Optional casefold is a flag specifying whether a lowercase alphabet is
acceptable as input.  For security purposes, the default is False.

ignorechars should be a byte string containing characters to ignore
from the input.
{extra_args}
The result is returned as a bytes object.  A binascii.Error is raised if
the input is incorrectly padded or if there are non-alphabet
characters present in the input.
'''
_B32_DECODE_MAP01_DOCSTRING = '''
RFC 3548 allows for optional mapping of the digit 0 (zero) to the
letter O (oh), and for optional mapping of the digit 1 (one) to
either the letter I (eye) or letter L (el).  The optional argument
map01 when not None, specifies which letter the digit 1 should be
mapped to (when map01 is not None, the digit 0 is always mapped to
the letter O).  For security purposes the default is None, so that
0 and 1 are not allowed in the input.
'''

def b32encode(s, *, wrapcol=0):
    return binascii.b2a_base32(s, wrapcol=wrapcol)
b32encode.__doc__ = _B32_ENCODE_DOCSTRING.format(encoding='base32')

def b32decode(s, casefold=False, map01=None, *, ignorechars=b''):
    s = _bytes_from_decode_data(s)
    # Handle section 2.4 zero and one mapping.  The flag map01 will be either
    # False, or the character to map the digit 1 (one) to.  It should be
    # either L (el) or I (eye).
    if map01 is not None:
        map01 = _bytes_from_decode_data(map01)
        assert len(map01) == 1, repr(map01)
        s = s.translate(bytes.maketrans(b'01', b'O' + map01))
    if casefold:
        s = s.upper()
    return binascii.a2b_base32(s, ignorechars=ignorechars)
b32decode.__doc__ = _B32_DECODE_DOCSTRING.format(encoding='base32',
                                        extra_args=_B32_DECODE_MAP01_DOCSTRING)

def b32hexencode(s, *, wrapcol=0):
    return binascii.b2a_base32(s, wrapcol=wrapcol,
                               alphabet=binascii.BASE32HEX_ALPHABET)
b32hexencode.__doc__ = _B32_ENCODE_DOCSTRING.format(encoding='base32hex')

def b32hexdecode(s, casefold=False, *, ignorechars=b''):
    s = _bytes_from_decode_data(s)
    # base32hex does not have the 01 mapping
    if casefold:
        s = s.upper()
    return binascii.a2b_base32(s, alphabet=binascii.BASE32HEX_ALPHABET,
                               ignorechars=ignorechars)
b32hexdecode.__doc__ = _B32_DECODE_DOCSTRING.format(encoding='base32hex',
                                                    extra_args='')


# RFC 3548, Base 16 Alphabet specifies uppercase, but hexlify() returns
# lowercase.  The RFC also recommends against accepting input case
# insensitively.
def b16encode(s, *, wrapcol=0):
    """Encode the bytes-like object s using Base16 and return a bytes object.

    If wrapcol is non-zero, insert a newline (b'\\n') character after at most
    every wrapcol characters.
    """
    if not wrapcol:
        return binascii.hexlify(s).upper()
    if wrapcol < 0:
        raise ValueError('Negative wrapcol')
    if wrapcol < 2:
        wrapcol = 2
    return binascii.hexlify(s, bytes_per_sep=-(wrapcol//2), sep=b'\n').upper()


def b16decode(s, casefold=False, *, ignorechars=b''):
    """Decode the Base16 encoded bytes-like object or ASCII string s.

    Optional casefold is a flag specifying whether a lowercase alphabet is
    acceptable as input.  For security purposes, the default is False.

    ignorechars should be a byte string containing characters to ignore
    from the input.

    The result is returned as a bytes object.  A binascii.Error is raised if
    s is incorrectly padded or if there are non-alphabet characters present
    in the input.
    """
    if not casefold:
        s = _bytes_from_decode_data(s)
        if not isinstance(ignorechars, bytes):
            ignorechars = bytes(memoryview(ignorechars))
        for b in b'abcdef':
            if b in s and b not in ignorechars:
                raise binascii.Error('Non-base16 digit found')
        s = s.translate(None, delete=b'abcdef')
    return binascii.unhexlify(s, ignorechars=ignorechars)

#
# Ascii85 encoding/decoding
#
def a85encode(b, *, foldspaces=False, wrapcol=0, pad=False, adobe=False):
    """Encode bytes-like object b using Ascii85 and return a bytes object.

    foldspaces is an optional flag that uses the special short sequence 'y'
    instead of 4 consecutive spaces (ASCII 0x20) as supported by 'btoa'. This
    feature is not supported by the "standard" Adobe encoding.

    If wrapcol is non-zero, insert a newline (b'\\n') character after at most
    every wrapcol characters.

    pad controls whether the input is padded to a multiple of 4 before
    encoding. Note that the btoa implementation always pads.

    adobe controls whether the encoded byte sequence is framed with <~ and ~>,
    which is used by the Adobe implementation.
    """
    return binascii.b2a_ascii85(b, foldspaces=foldspaces,
                                adobe=adobe, wrapcol=wrapcol, pad=pad)

def a85decode(b, *, foldspaces=False, adobe=False, ignorechars=b' \t\n\r\v'):
    """Decode the Ascii85 encoded bytes-like object or ASCII string b.

    foldspaces is a flag that specifies whether the 'y' short sequence should be
    accepted as shorthand for 4 consecutive spaces (ASCII 0x20). This feature is
    not supported by the "standard" Adobe encoding.

    adobe controls whether the input sequence is in Adobe Ascii85 format (i.e.
    is framed with <~ and ~>).

    ignorechars should be a byte string containing characters to ignore from the
    input. This should only contain whitespace characters, and by default
    contains all whitespace characters in ASCII.

    The result is returned as a bytes object.
    """
    return binascii.a2b_ascii85(b, foldspaces=foldspaces,
                                adobe=adobe, ignorechars=ignorechars)

def b85encode(b, pad=False, *, wrapcol=0):
    """Encode bytes-like object b in base85 format and return a bytes object.

    If wrapcol is non-zero, insert a newline (b'\\n') character after at most
    every wrapcol characters.

    If pad is true, the input is padded with b'\\0' so its length is a multiple of
    4 bytes before encoding.
    """
    return binascii.b2a_base85(b, wrapcol=wrapcol, pad=pad)

def b85decode(b, *, ignorechars=b''):
    """Decode the base85-encoded bytes-like object or ASCII string b

    The result is returned as a bytes object.
    """
    return binascii.a2b_base85(b, ignorechars=ignorechars)

def z85encode(s, pad=False, *, wrapcol=0):
    """Encode bytes-like object b in z85 format and return a bytes object.

    If wrapcol is non-zero, insert a newline (b'\\n') character after at most
    every wrapcol characters.

    If pad is true, the input is padded with b'\\0' so its length is a multiple of
    4 bytes before encoding.
    """
    return binascii.b2a_base85(s, wrapcol=wrapcol, pad=pad,
                               alphabet=binascii.Z85_ALPHABET)

def z85decode(s, *, ignorechars=b''):
    """Decode the z85-encoded bytes-like object or ASCII string b

    The result is returned as a bytes object.
    """
    return binascii.a2b_base85(s, alphabet=binascii.Z85_ALPHABET, ignorechars=ignorechars)

# Legacy interface.  This code could be cleaned up since I don't believe
# binascii has any line length limitations.  It just doesn't seem worth it
# though.  The files should be opened in binary mode.

MAXLINESIZE = 76 # Excluding the CRLF
MAXBINSIZE = (MAXLINESIZE//4)*3

def encode(input, output):
    """Encode a file; input and output are binary files."""
    while s := input.read(MAXBINSIZE):
        while len(s) < MAXBINSIZE and (ns := input.read(MAXBINSIZE-len(s))):
            s += ns
        line = binascii.b2a_base64(s)
        output.write(line)


def decode(input, output):
    """Decode a file; input and output are binary files."""
    while line := input.readline():
        s = binascii.a2b_base64(line)
        output.write(s)

def _input_type_check(s):
    try:
        m = memoryview(s)
    except TypeError as err:
        msg = "expected bytes-like object, not %s" % s.__class__.__name__
        raise TypeError(msg) from err
    if m.format not in ('c', 'b', 'B'):
        msg = ("expected single byte elements, not %r from %s" %
                                          (m.format, s.__class__.__name__))
        raise TypeError(msg)
    if m.ndim != 1:
        msg = ("expected 1-D data, not %d-D data from %s" %
                                          (m.ndim, s.__class__.__name__))
        raise TypeError(msg)


def encodebytes(s):
    """Encode a bytestring into a bytes object containing multiple lines
    of base-64 data."""
    _input_type_check(s)
    result = binascii.b2a_base64(s, wrapcol=MAXLINESIZE)
    if result == b'\n':
        return b''
    return result


def decodebytes(s):
    """Decode a bytestring of base-64 data into a bytes object."""
    _input_type_check(s)
    return binascii.a2b_base64(s)


# Usable as a script...
def main():
    """Small main program"""
    import sys, getopt
    usage = f"""usage: {sys.argv[0]} [-h|-d|-e|-u] [file|-]
        -h: print this help message and exit
        -d, -u: decode
        -e: encode (default)"""
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hdeu')
    except getopt.error as msg:
        sys.stdout = sys.stderr
        print(msg)
        print(usage)
        sys.exit(2)
    func = encode
    for o, a in opts:
        if o == '-e': func = encode
        if o == '-d': func = decode
        if o == '-u': func = decode
        if o == '-h': print(usage); return
    if args and args[0] != '-':
        with open(args[0], 'rb') as f:
            func(f, sys.stdout.buffer)
    else:
        if sys.stdin.isatty():
            # gh-138775: read terminal input data all at once to detect EOF
            import io
            data = sys.stdin.buffer.read()
            buffer = io.BytesIO(data)
        else:
            buffer = sys.stdin.buffer
        func(buffer, sys.stdout.buffer)


if __name__ == '__main__':
    main()

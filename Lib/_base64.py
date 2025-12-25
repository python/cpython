"""C accelerator wrappers for originally pure-Python parts of base64."""

from binascii import Error, a2b_ascii85, a2b_base85, b2a_ascii85, b2a_base85


# Base 85 functions in base64 silently convert input to bytes.
# Copy the conversion logic from base64 to avoid circular imports.

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


def _bytes_from_encode_data(b):
    return b if isinstance(b, bytes_types) else memoryview(b).tobytes()


# Functions in binascii raise binascii.Error instead of ValueError.

def a85encode(b, *, foldspaces=False, wrapcol=0, pad=False, adobe=False):
    b = _bytes_from_encode_data(b)
    try:
        return b2a_ascii85(b, fold_spaces=foldspaces,
                           wrap=adobe, width=wrapcol, pad=pad)
    except Error as e:
        raise ValueError(e) from None


def a85decode(b, *, foldspaces=False, adobe=False, ignorechars=b' \t\n\r\v'):
    b = _bytes_from_decode_data(b)
    try:
        return a2b_ascii85(b, fold_spaces=foldspaces,
                           wrap=adobe, ignore=ignorechars)
    except Error as e:
        raise ValueError(e) from None

def b85encode(b, pad=False):
    b = _bytes_from_encode_data(b)
    try:
        return b2a_base85(b, pad=pad, newline=False)
    except Error as e:
        raise ValueError(e) from None


def b85decode(b):
    b = _bytes_from_decode_data(b)
    try:
        return a2b_base85(b, strict_mode=True)
    except Error as e:
        raise ValueError(e) from None


def z85encode(s, pad=False):
    s = _bytes_from_encode_data(s)
    try:
        return b2a_base85(s, pad=pad, newline=False, z85=True)
    except Error as e:
        raise ValueError(e) from None


def z85decode(s):
    s = _bytes_from_decode_data(s)
    try:
        return a2b_base85(s, strict_mode=True, z85=True)
    except Error as e:
        raise ValueError(e) from None

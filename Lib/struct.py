"""
Functions to convert between Python values and C structs.
Python strings are used to hold the data representing the C struct
and also as format strings to describe the layout of data in the C struct.

The optional first format char indicates byte order, size and alignment:
 @: native order, size & alignment (default)
 =: native order, std. size & alignment
 <: little-endian, std. size & alignment
 >: big-endian, std. size & alignment
 !: same as >

The remaining chars indicate types of args and must match exactly;
these can be preceded by a decimal repeat count:
 x: pad byte (no data); c:char; b:signed byte; B:unsigned byte;
 h:short; H:unsigned short; i:int; I:unsigned int;
 l:long; L:unsigned long; f:float; d:double.
Special cases (preceding decimal count indicates length):
 s:string (array of char); p: pascal string (with count byte).
Special case (only available in native format):
 P:an integer type that is wide enough to hold a pointer.
Special case (not in native mode unless 'long long' in platform C):
 q:long long; Q:unsigned long long
Whitespace between formats is ignored.

The variable struct.error is an exception raised on errors.
"""

# XXX Move the bytes and str8 casts into the _struct module

__version__ = '3.0'


from _struct import Struct as _Struct, error

class Struct(_Struct):
    def __init__(self, fmt):
        if isinstance(fmt, str):
            fmt = str8(fmt, 'latin1')
        _Struct.__init__(self, fmt)

_MAXCACHE = 100
_cache = {}

def _compile(fmt):
    # Internal: compile struct pattern
    if len(_cache) >= _MAXCACHE:
        _cache.clear()
    s = Struct(fmt)
    _cache[fmt] = s
    return s

def calcsize(fmt):
    """
    Return size of C struct described by format string fmt.
    See struct.__doc__ for more on format strings.
    """
    try:
        o = _cache[fmt]
    except KeyError:
        o = _compile(fmt)
    return o.size

def pack(fmt, *args):
    """
    Return string containing values v1, v2, ... packed according to fmt.
    See struct.__doc__ for more on format strings.
    """
    try:
        o = _cache[fmt]
    except KeyError:
        o = _compile(fmt)
    return bytes(o.pack(*args))

def pack_into(fmt, buf, offset, *args):
    """
    Pack the values v1, v2, ... according to fmt, write
    the packed bytes into the writable buffer buf starting at offset.
    See struct.__doc__ for more on format strings.
    """
    try:
        o = _cache[fmt]
    except KeyError:
        o = _compile(fmt)
    o.pack_into(buf, offset, *args)

def unpack(fmt, s):
    """
    Unpack the string, containing packed C structure data, according
    to fmt.  Requires len(string)==calcsize(fmt).
    See struct.__doc__ for more on format strings.
    """
    try:
        o = _cache[fmt]
    except KeyError:
        o = _compile(fmt)
    return o.unpack(s)

def unpack_from(fmt, buf, offset=0):
    """
    Unpack the buffer, containing packed C structure data, according to
    fmt starting at offset. Requires len(buffer[offset:]) >= calcsize(fmt).
    See struct.__doc__ for more on format strings.
    """
    try:
        o = _cache[fmt]
    except KeyError:
        o = _compile(fmt)
    return o.unpack_from(buf, offset)

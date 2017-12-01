r"""plistlib.py -- a tool to generate and parse MacOSX .plist files.

The property list (.plist) file format is a simple XML pickle supporting
basic object types, like dictionaries, lists, numbers and strings.
Usually the top level object is a dictionary.

To write out a plist file, use the dump(value, file)
function. 'value' is the top level object, 'file' is
a (writable) file object.

To parse a plist from a file, use the load(file) function,
with a (readable) file object as the only argument. It
returns the top level object (again, usually a dictionary).

To work with plist data in bytes objects, you can use loads()
and dumps().

Values can be strings, integers, floats, booleans, tuples, lists,
dictionaries (but only with string keys), Data, bytes, bytearray, or
datetime.datetime objects.

Generate Plist example:

    pl = dict(
        aString = "Doodah",
        aList = ["A", "B", 12, 32.1, [1, 2, 3]],
        aFloat = 0.1,
        anInt = 728,
        aDict = dict(
            anotherString = "<hello & hi there!>",
            aUnicodeValue = "M\xe4ssig, Ma\xdf",
            aTrueValue = True,
            aFalseValue = False,
        ),
        someData = b"<binary gunk>",
        someMoreData = b"<lots of binary gunk>" * 10,
        aDate = datetime.datetime.fromtimestamp(time.mktime(time.gmtime())),
    )
    with open(fileName, 'wb') as fp:
        dump(pl, fp)

Parse Plist example:

    with open(fileName, 'rb') as fp:
        pl = load(fp)
    print(pl["aKey"])
"""
__all__ = [
    "readPlist", "writePlist", "readPlistFromBytes", "writePlistToBytes",
    "Data", "InvalidFileException", "FMT_XML", "FMT_BINARY",
    "load", "dump", "loads", "dumps"
]

import binascii
import codecs
import contextlib
import datetime
import enum
from io import BytesIO
import os
import re
import struct
from warnings import warn
from xml.parsers.expat import ParserCreate


PlistFormat = enum.Enum('PlistFormat', 'FMT_XML FMT_BINARY', module=__name__)
globals().update(PlistFormat.__members__)


#
#
# Deprecated functionality
#
#


@contextlib.contextmanager
def _maybe_open(pathOrFile, mode):
    if isinstance(pathOrFile, str):
        with open(pathOrFile, mode) as fp:
            yield fp

    else:
        yield pathOrFile


def readPlist(pathOrFile):
    """
    Read a .plist from a path or file. pathOrFile should either
    be a file name, or a readable binary file object.

    This function is deprecated, use load instead.
    """
    warn("The readPlist function is deprecated, use load() instead",
        DeprecationWarning, 2)

    with _maybe_open(pathOrFile, 'rb') as fp:
        return load(fp, fmt=None, use_builtin_types=False)

def writePlist(value, pathOrFile):
    """
    Write 'value' to a .plist file. 'pathOrFile' may either be a
    file name or a (writable) file object.

    This function is deprecated, use dump instead.
    """
    warn("The writePlist function is deprecated, use dump() instead",
        DeprecationWarning, 2)
    with _maybe_open(pathOrFile, 'wb') as fp:
        dump(value, fp, fmt=FMT_XML, sort_keys=True, skipkeys=False)


def readPlistFromBytes(data):
    """
    Read a plist data from a bytes object. Return the root object.

    This function is deprecated, use loads instead.
    """
    warn("The readPlistFromBytes function is deprecated, use loads() instead",
        DeprecationWarning, 2)
    return load(BytesIO(data), fmt=None, use_builtin_types=False)


def writePlistToBytes(value):
    """
    Return 'value' as a plist-formatted bytes object.

    This function is deprecated, use dumps instead.
    """
    warn("The writePlistToBytes function is deprecated, use dumps() instead",
        DeprecationWarning, 2)
    f = BytesIO()
    dump(value, f, fmt=FMT_XML, sort_keys=True, skipkeys=False)
    return f.getvalue()


class Data:
    """
    Wrapper for binary data.

    This class is deprecated, use a bytes object instead.
    """

    def __init__(self, data):
        if not isinstance(data, bytes):
            raise TypeError("data must be as bytes")
        self.data = data

    @classmethod
    def fromBase64(cls, data):
        # base64.decodebytes just calls binascii.a2b_base64;
        # it seems overkill to use both base64 and binascii.
        return cls(_decode_base64(data))

    def asBase64(self, maxlinelength=76):
        return _encode_base64(self.data, maxlinelength)

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.data == other.data
        elif isinstance(other, bytes):
            return self.data == other
        else:
            return NotImplemented

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__, repr(self.data))

#
#
# End of deprecated functionality
#
#


#
# XML support
#


# XML 'header'
PLISTHEADER = b"""\
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
"""


# Regex to find any control chars, except for \t \n and \r
_controlCharPat = re.compile(
    r"[\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0b\x0c\x0e\x0f"
    r"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f]")

def _encode_base64(s, maxlinelength=76):
    # copied from base64.encodebytes(), with added maxlinelength argument
    maxbinsize = (maxlinelength//4)*3
    pieces = []
    for i in range(0, len(s), maxbinsize):
        chunk = s[i : i + maxbinsize]
        pieces.append(binascii.b2a_base64(chunk))
    return b''.join(pieces)

def _decode_base64(s):
    if isinstance(s, str):
        return binascii.a2b_base64(s.encode("utf-8"))

    else:
        return binascii.a2b_base64(s)

# Contents should conform to a subset of ISO 8601
# (in particular, YYYY '-' MM '-' DD 'T' HH ':' MM ':' SS 'Z'.  Smaller units
# may be omitted with #  a loss of precision)
_dateParser = re.compile(r"(?P<year>\d\d\d\d)(?:-(?P<month>\d\d)(?:-(?P<day>\d\d)(?:T(?P<hour>\d\d)(?::(?P<minute>\d\d)(?::(?P<second>\d\d))?)?)?)?)?Z", re.ASCII)


def _date_from_string(s):
    order = ('year', 'month', 'day', 'hour', 'minute', 'second')
    gd = _dateParser.match(s).groupdict()
    lst = []
    for key in order:
        val = gd[key]
        if val is None:
            break
        lst.append(int(val))
    return datetime.datetime(*lst)


def _date_to_string(d):
    return '%04d-%02d-%02dT%02d:%02d:%02dZ' % (
        d.year, d.month, d.day,
        d.hour, d.minute, d.second
    )

def _escape(text):
    m = _controlCharPat.search(text)
    if m is not None:
        raise ValueError("strings can't contains control characters; "
                         "use bytes instead")
    text = text.replace("\r\n", "\n")       # convert DOS line endings
    text = text.replace("\r", "\n")         # convert Mac line endings
    text = text.replace("&", "&amp;")       # escape '&'
    text = text.replace("<", "&lt;")        # escape '<'
    text = text.replace(">", "&gt;")        # escape '>'
    return text


def _parse_xml(fp, use_builtin_types, dict_type):

    stack = []
    data = []
    current_key = None
    root = None

    def handle_begin_element(element, attrs):
        data.clear()
        handler = begin_handlers.get(element)
        if handler is not None:
            handler(attrs)

    def handle_end_element(element):
        handler = end_handlers.get(element)
        if handler is not None:
            handler()

    def add_object(value):
        # this is the root object
        nonlocal root
        root = value

    def add_dict_item(value):
        nonlocal current_key
        if current_key is None:
            raise ValueError("unexpected element at line %d" %
                             parser.CurrentLineNumber)
        stack[-1][current_key] = value
        current_key = None

    def get_data():
        sdata = ''.join(data)
        data.clear()
        return sdata

    # element handlers

    begin_handlers = {}
    end_handlers = {}

    def begin_dict(attrs):
        nonlocal add_object
        d = dict_type()
        add_object(d)
        stack.append(add_object)
        stack.append(d)
        add_object = add_dict_item
    begin_handlers['dict'] = begin_dict

    def end_dict():
        nonlocal add_object
        if current_key is not None:
            raise ValueError("missing value for key '%s' at line %d" %
                             (current_key, parser.CurrentLineNumber))
        stack.pop()
        add_object = stack.pop()
    end_handlers['dict'] = end_dict

    def end_key():
        nonlocal current_key
        if current_key is not None or add_object is not add_dict_item:
            raise ValueError("unexpected key at line %d" %
                             parser.CurrentLineNumber)
        current_key = get_data()
    end_handlers['key'] = end_key

    def begin_array(attrs):
        nonlocal add_object
        a = []
        add_object(a)
        stack.append(add_object)
        add_object = a.append
    begin_handlers['array'] = begin_array

    def end_array():
        nonlocal add_object
        add_object = stack.pop()
    end_handlers['array'] = end_array

    end_handlers['true'] = lambda: add_object(True)
    end_handlers['false'] = lambda: add_object(False)
    end_handlers['integer'] = lambda: add_object(int(get_data()))
    end_handlers['real'] = lambda: add_object(float(get_data()))
    end_handlers['string'] = lambda: add_object(get_data())
    if use_builtin_types:
        end_handlers['data'] = lambda: add_object(_decode_base64(get_data()))
    else:
        end_handlers['data'] = lambda: add_object(Data.fromBase64(get_data()))
    end_handlers['date'] = lambda: add_object(_date_from_string(get_data()))

    parser = ParserCreate()
    parser.StartElementHandler = handle_begin_element
    parser.EndElementHandler = handle_end_element
    parser.CharacterDataHandler = data.append
    parser.ParseFile(fp)
    return root


def _write_xml(value, file, sort_keys, skipkeys):

    stack = []
    indent_level = 0
    indent = b"\t"

    def begin_element(element):
        nonlocal indent_level
        stack.append(element)
        writeln("<%s>" % element)
        indent_level += 1

    def end_element(element):
        nonlocal indent_level
        assert indent_level > 0
        assert stack.pop() == element
        indent_level -= 1
        writeln("</%s>" % element)

    def simple_element(element, value=None):
        if value is not None:
            value = _escape(value)
            writeln("<%s>%s</%s>" % (element, value, element))

        else:
            writeln("<%s/>" % element)

    def writeln(line):
        if line:
            # plist has fixed encoding of utf-8
            line = line.encode('utf-8')
            file.write(indent_level * indent)
            file.write(line)
        file.write(b'\n')

    def writelnbytes(line):
        if line:
            # plist has fixed encoding of utf-8
            file.write(indent_level * indent)
            file.write(line)
        file.write(b'\n')

    def write_value(value):
        if isinstance(value, str):
            simple_element("string", value)

        elif value is True:
            simple_element("true")

        elif value is False:
            simple_element("false")

        elif isinstance(value, int):
            if -1 << 63 <= value < 1 << 64:
                simple_element("integer", "%d" % value)
            else:
                raise OverflowError(value)

        elif isinstance(value, float):
            simple_element("real", repr(value))

        elif isinstance(value, dict):
            write_dict(value)

        elif isinstance(value, Data):
            write_bytes(value.data)

        elif isinstance(value, (bytes, bytearray)):
            write_bytes(value)

        elif isinstance(value, datetime.datetime):
            simple_element("date", _date_to_string(value))

        elif isinstance(value, (tuple, list)):
            write_array(value)

        else:
            raise TypeError("unsupported type: %s" % type(value))

    def write_bytes(data):
        nonlocal indent_level
        begin_element("data")
        indent_level -= 1
        maxlinelength = max(
            16,
            76 - len(indent.replace(b"\t", b" " * 8) * indent_level))

        for line in _encode_base64(data, maxlinelength).split(b"\n"):
            if line:
                writelnbytes(line)
        indent_level += 1
        end_element("data")

    def write_dict(d):
        if d:
            begin_element("dict")
            if sort_keys:
                items = sorted(d.items())
            else:
                items = d.items()

            for key, value in items:
                if not isinstance(key, str):
                    if skipkeys:
                        continue
                    raise TypeError("keys must be strings")
                simple_element("key", key)
                write_value(value)
            end_element("dict")

        else:
            simple_element("dict")

    def write_array(array):
        if array:
            begin_element("array")
            for value in array:
                write_value(value)
            end_element("array")

        else:
            simple_element("array")

    file.write(PLISTHEADER)
    writeln("<plist version=\"1.0\">")
    write_value(value)
    writeln("</plist>")


def _is_fmt_xml(header):
    prefixes = (b'<?xml', b'<plist')

    for pfx in prefixes:
        if header.startswith(pfx):
            return True

    # Also check for alternative XML encodings, this is slightly
    # overkill because the Apple tools (and plistlib) will not
    # generate files with these encodings.
    for bom, encoding in (
                (codecs.BOM_UTF8, "utf-8"),
                (codecs.BOM_UTF16_BE, "utf-16-be"),
                (codecs.BOM_UTF16_LE, "utf-16-le"),
                # expat does not support utf-32
                #(codecs.BOM_UTF32_BE, "utf-32-be"),
                #(codecs.BOM_UTF32_LE, "utf-32-le"),
            ):
        if not header.startswith(bom):
            continue

        for start in prefixes:
            prefix = bom + start.decode('ascii').encode(encoding)
            if header[:len(prefix)] == prefix:
                return True

    return False

#
# Binary Plist
#


class InvalidFileException (ValueError):
    def __init__(self, message="Invalid file"):
        ValueError.__init__(self, message)

_BINARY_FORMAT = {1: 'B', 2: 'H', 4: 'L', 8: 'Q'}

def _parse_binary(fp, use_builtin_types, dict_type):
    """
    Read or write a binary plist file, following the description of the binary
    format.  Raise InvalidFileException in case of error, otherwise return the
    root object.

    see also: http://opensource.apple.com/source/CF/CF-744.18/CFBinaryPList.c
    """

    def get_size(tokenL):
        """ return the size of the next object."""
        if tokenL == 0xF:
            m = fp.read(1)[0] & 0x3
            s = 1 << m
            f = '>' + _BINARY_FORMAT[s]
            return struct.unpack(f, fp.read(s))[0]

        return tokenL

    def read_ints(n, size):
        data = fp.read(size * n)
        if size in _BINARY_FORMAT:
            return struct.unpack('>' + _BINARY_FORMAT[size] * n, data)
        else:
            if not size or len(data) != size * n:
                raise InvalidFileException()
            return tuple(int.from_bytes(data[i: i + size], 'big')
                         for i in range(0, size * n, size))

    def read_refs(n):
        return read_ints(n, ref_size)

    def read_object(ref):
        """
        read the object by reference.

        May recursively read sub-objects (content of an array/dict/set)
        """
        result = objects[ref]
        if result is not undefined:
            return result

        offset = object_offsets[ref]
        fp.seek(offset)
        token = fp.read(1)[0]
        tokenH, tokenL = token & 0xF0, token & 0x0F

        if token == 0x00:
            result = None

        elif token == 0x08:
            result = False

        elif token == 0x09:
            result = True

        # The referenced source code also mentions URL (0x0c, 0x0d) and
        # UUID (0x0e), but neither can be generated using the Cocoa libraries.

        elif token == 0x0f:
            result = b''

        elif tokenH == 0x10:  # int
            result = int.from_bytes(fp.read(1 << tokenL),
                                    'big', signed=tokenL >= 3)

        elif token == 0x22: # real
            result = struct.unpack('>f', fp.read(4))[0]

        elif token == 0x23: # real
            result = struct.unpack('>d', fp.read(8))[0]

        elif token == 0x33:  # date
            f = struct.unpack('>d', fp.read(8))[0]
            # timestamp 0 of binary plists corresponds to 1/1/2001
            # (year of Mac OS X 10.0), instead of 1/1/1970.
            result = _start_datetime + datetime.timedelta(seconds=f)

        elif tokenH == 0x40:  # data
            s = get_size(tokenL)
            if use_builtin_types:
                result = fp.read(s)
            else:
                result = Data(fp.read(s))

        elif tokenH == 0x50:  # ascii string
            s = get_size(tokenL)
            result =  fp.read(s).decode('ascii')

        elif tokenH == 0x60:  # unicode string
            s = get_size(tokenL)
            result = fp.read(s * 2).decode('utf-16be')

        # tokenH == 0x80 is documented as 'UID' and appears to be used for
        # keyed-archiving, not in plists.

        elif tokenH == 0xA0:  # array
            s = get_size(tokenL)
            obj_refs = read_refs(s)
            result = []
            objects[ref] = result
            result.extend(read_object(x) for x in obj_refs)

        # tokenH == 0xB0 is documented as 'ordset', but is not actually
        # implemented in the Apple reference code.

        # tokenH == 0xC0 is documented as 'set', but sets cannot be used in
        # plists.

        elif tokenH == 0xD0:  # dict
            s = get_size(tokenL)
            key_refs = read_refs(s)
            obj_refs = read_refs(s)
            result = dict_type()
            objects[ref] = result
            for k, o in zip(key_refs, obj_refs):
                result[read_object(k)] = read_object(o)

        else:
            raise InvalidFileException()

        objects[ref] = result
        return result

    try:
        # The basic file format:
        # HEADER
        # object...
        # refid->offset...
        # TRAILER
        fp.seek(-32, os.SEEK_END)
        trailer = fp.read(32)
        if len(trailer) != 32:
            raise InvalidFileException()
        (
            offset_size, ref_size, num_objects, top_object,
            offset_table_offset
        ) = struct.unpack('>6xBBQQQ', trailer)
        fp.seek(offset_table_offset)
        object_offsets = read_ints(num_objects, offset_size)
        undefined = object()
        objects = [undefined] * num_objects
        return read_object(top_object)

    except (OSError, IndexError, struct.error):
        raise InvalidFileException()

def _count_to_size(count):
    if count < 1 << 8:
        return 1

    elif count < 1 << 16:
        return 2

    elif count << 1 << 32:
        return 4

    else:
        return 8

_scalars = (str, int, float, datetime.datetime, bytes)
_start_datetime = datetime.datetime(2001, 1, 1)

def _write_binary(value, fp, sort_keys, skipkeys):

    def flatten(value):
        # First check if the object is in the object table, not used for
        # containers to ensure that two subcontainers with the same contents
        # will be serialized as distinct values.
        if isinstance(value, _scalars):
            if (type(value), value) in objtable:
                return

        elif isinstance(value, Data):
            if (type(value.data), value.data) in objtable:
                return

        elif id(value) in objidtable:
            return

        # Add to objectreference map
        refnum = len(objlist)
        objlist.append(value)
        if isinstance(value, _scalars):
            objtable[(type(value), value)] = refnum
        elif isinstance(value, Data):
            objtable[(type(value.data), value.data)] = refnum
        else:
            objidtable[id(value)] = refnum

        # And finally recurse into containers
        if isinstance(value, dict):
            keys = []
            values = []
            items = value.items()
            if sort_keys:
                items = sorted(items)

            for k, v in items:
                if not isinstance(k, str):
                    if skipkeys:
                        continue
                    raise TypeError("keys must be strings")
                keys.append(k)
                values.append(v)

            for o in keys:
                flatten(o)
            for o in values:
                flatten(o)

        elif isinstance(value, (list, tuple)):
            for o in value:
                flatten(o)

    def getrefnum(value):
        if isinstance(value, _scalars):
            return objtable[(type(value), value)]
        elif isinstance(value, Data):
            return objtable[(type(value.data), value.data)]
        else:
            return objidtable[id(value)]

    def write_size(token, size):
        if size < 15:
            fp.write(struct.pack('>B', token | size))

        elif size < 1 << 8:
            fp.write(struct.pack('>BBB', token | 0xF, 0x10, size))

        elif size < 1 << 16:
            fp.write(struct.pack('>BBH', token | 0xF, 0x11, size))

        elif size < 1 << 32:
            fp.write(struct.pack('>BBL', token | 0xF, 0x12, size))

        else:
            fp.write(struct.pack('>BBQ', token | 0xF, 0x13, size))

    def write_object(value):
        ref = getrefnum(value)
        object_offsets[ref] = fp.tell()
        if value is None:
            fp.write(b'\x00')

        elif value is False:
            fp.write(b'\x08')

        elif value is True:
            fp.write(b'\x09')

        elif isinstance(value, int):
            if value < 0:
                try:
                    fp.write(struct.pack('>Bq', 0x13, value))
                except struct.error:
                    raise OverflowError(value) from None
            elif value < 1 << 8:
                fp.write(struct.pack('>BB', 0x10, value))
            elif value < 1 << 16:
                fp.write(struct.pack('>BH', 0x11, value))
            elif value < 1 << 32:
                fp.write(struct.pack('>BL', 0x12, value))
            elif value < 1 << 63:
                fp.write(struct.pack('>BQ', 0x13, value))
            elif value < 1 << 64:
                fp.write(b'\x14' + value.to_bytes(16, 'big', signed=True))
            else:
                raise OverflowError(value)

        elif isinstance(value, float):
            fp.write(struct.pack('>Bd', 0x23, value))

        elif isinstance(value, datetime.datetime):
            f = (value - _start_datetime).total_seconds()
            fp.write(struct.pack('>Bd', 0x33, f))

        elif isinstance(value, Data):
            write_size(0x40, len(value.data))
            fp.write(value.data)

        elif isinstance(value, (bytes, bytearray)):
            write_size(0x40, len(value))
            fp.write(value)

        elif isinstance(value, str):
            try:
                t = value.encode('ascii')
                write_size(0x50, len(value))
            except UnicodeEncodeError:
                t = value.encode('utf-16be')
                write_size(0x60, len(t) // 2)

            fp.write(t)

        elif isinstance(value, (list, tuple)):
            refs = [getrefnum(o) for o in value]
            s = len(refs)
            write_size(0xA0, s)
            fp.write(struct.pack('>' + ref_format * s, *refs))

        elif isinstance(value, dict):
            keyRefs, valRefs = [], []

            if sort_keys:
                rootItems = sorted(value.items())
            else:
                rootItems = value.items()

            for k, v in rootItems:
                if not isinstance(k, str):
                    if skipkeys:
                        continue
                    raise TypeError("keys must be strings")
                keyRefs.append(getrefnum(k))
                valRefs.append(getrefnum(v))

            s = len(keyRefs)
            write_size(0xD0, s)
            fp.write(struct.pack('>' + ref_format * s, *keyRefs))
            fp.write(struct.pack('>' + ref_format * s, *valRefs))

        else:
            raise TypeError(value)

    # Flattened object list:
    objlist = []

    # Mappings from object->objectid
    # First dict has (type(object), object) as the key,
    # second dict is used when object is not hashable and
    # has id(object) as the key.
    objtable = {}
    objidtable = {}

    # Create list of all objects in the plist
    flatten(value)

    # Size of object references in serialized containers
    # depends on the number of objects in the plist.
    num_objects = len(objlist)
    object_offsets = [0]*num_objects
    ref_size = _count_to_size(num_objects)

    ref_format = _BINARY_FORMAT[ref_size]

    # Write file header
    fp.write(b'bplist00')

    # Write object list
    for obj in objlist:
        write_object(obj)

    # Write refnum->object offset table
    top_object = getrefnum(value)
    offset_table_offset = fp.tell()
    offset_size = _count_to_size(offset_table_offset)
    offset_format = '>' + _BINARY_FORMAT[offset_size] * num_objects
    fp.write(struct.pack(offset_format, *object_offsets))

    # Write trailer
    sort_version = 0
    trailer = (
        sort_version, offset_size, ref_size, num_objects,
        top_object, offset_table_offset
    )
    fp.write(struct.pack('>5xBBBQQQ', *trailer))


def _is_fmt_binary(header):
    return header[:8] == b'bplist00'


#
# Generic bits
#

_FORMATS={
    FMT_XML: dict(
        detect=_is_fmt_xml,
        parser=_parse_xml,
        writer=_write_xml,
    ),
    FMT_BINARY: dict(
        detect=_is_fmt_binary,
        parser=_parse_binary,
        writer=_write_binary,
    )
}


def load(fp, *, fmt=None, use_builtin_types=True, dict_type=dict):
    """Read a .plist file. 'fp' should be (readable) file object.
    Return the unpacked root object (which usually is a dictionary).
    """
    if fmt is None:
        header = fp.read(32)
        fp.seek(0)
        for info in _FORMATS.values():
            if info['detect'](header):
                P = info['parser']
                break

        else:
            raise InvalidFileException()

    else:
        P = _FORMATS[fmt]['parser']

    return P(fp, use_builtin_types=use_builtin_types, dict_type=dict_type)


def loads(value, *, fmt=None, use_builtin_types=True, dict_type=dict):
    """Read a .plist file from a bytes object.
    Return the unpacked root object (which usually is a dictionary).
    """
    fp = BytesIO(value)
    return load(
        fp, fmt=fmt, use_builtin_types=use_builtin_types, dict_type=dict_type)


def dump(value, fp, *, fmt=FMT_XML, sort_keys=True, skipkeys=False):
    """Write 'value' to a .plist file. 'fp' should be a (writable)
    file object.
    """
    if fmt not in _FORMATS:
        raise ValueError("Unsupported format: %r"%(fmt,))

    writer = _FORMATS[fmt]["writer"]
    writer(value, fp, sort_keys=sort_keys, skipkeys=skipkeys)


def dumps(value, *, fmt=FMT_XML, skipkeys=False, sort_keys=True):
    """Return a bytes object with the contents for a .plist file.
    """
    fp = BytesIO()
    dump(value, fp, fmt=fmt, skipkeys=skipkeys, sort_keys=sort_keys)
    return fp.getvalue()

"""plistlib.py -- a tool to generate and parse MacOSX .plist files.

The PropertList (.plist) file format is a simple XML pickle supporting
basic object types, like dictionaries, lists, numbers and strings.
Usually the top level object is a dictionary.

To write out a plist file, use the writePlist(rootObject, pathOrFile)
function. 'rootObject' is the top level object, 'pathOrFile' is a
filename or a (writable) file object.

To parse a plist from a file, use the readPlist(pathOrFile)
function, with a file name or a (readable) file object as the only
argument. It returns the top level object (usually a dictionary).
(Warning: you need pyexpat installed for this to work, ie. it doesn't
work with a vanilla Python 2.2 as shipped with MacOS X.2.)

Values can be strings, integers, floats, booleans, tuples, lists,
dictionaries, Data or Date objects. String values (including dictionary
keys) may be unicode strings -- they will be written out as UTF-8.

This module exports a class named Dict(), which allows you to easily
construct (nested) dicts using keyword arguments as well as accessing
values with attribute notation, where d.foo is equivalent to d["foo"].
Regular dictionaries work, too.

To support Boolean values in plists with Python < 2.3, "bool", "True"
and "False" are exported. Use these symbols from this module if you
want to be compatible with Python 2.2.x (strongly recommended).

The <data> plist type is supported through the Data class. This is a
thin wrapper around a Python string.

The <date> plist data has (limited) support through the Date class.
(Warning: Dates are only supported if the PyXML package is installed.)

Generate Plist example:

    pl = Dict(
        aString="Doodah",
        aList=["A", "B", 12, 32.1, [1, 2, 3]],
        aFloat = 0.1,
        anInt = 728,
        aDict=Dict(
            anotherString="<hello & hi there!>",
            aUnicodeValue=u'M\xe4ssig, Ma\xdf',
            aTrueValue=True,
            aFalseValue=False,
        ),
        someData = Data("<binary gunk>"),
        someMoreData = Data("<lots of binary gunk>" * 10),
        aDate = Date(time.mktime(time.gmtime())),
    )
    # unicode keys are possible, but a little awkward to use:
    pl[u'\xc5benraa'] = "That was a unicode key."
    writePlist(pl, fileName)

Parse Plist example:

    pl = readPlist(pathOrFile)
    print pl.aKey  # same as pl["aKey"]


"""


__all__ = ["readPlist", "writePlist", "Plist", "Data", "Date", "Dict",
           "False", "True", "bool"]
# Note: the Plist class has been deprecated, Dict, False, True and bool
# are here only for compatibility with Python 2.2.


def readPlist(pathOrFile):
    """Read a .plist file. 'pathOrFile' may either be a file name or a
    (readable) file object. Return the unpacked root object (which
    usually is a dictionary).
    """
    didOpen = 0
    if isinstance(pathOrFile, (str, unicode)):
        pathOrFile = open(pathOrFile)
        didOpen = 1
    p = PlistParser()
    rootObject = p.parse(pathOrFile)
    if didOpen:
        pathOrFile.close()
    return rootObject


def writePlist(rootObject, pathOrFile):
    """Write 'rootObject' to a .plist file. 'pathOrFile' may either be a
    file name or a (writable) file object.
    """
    didOpen = 0
    if isinstance(pathOrFile, (str, unicode)):
        pathOrFile = open(pathOrFile, "w")
        didOpen = 1
    writer = PlistWriter(pathOrFile)
    writer.writeln("<plist version=\"1.0\">")
    writer.writeValue(rootObject)
    writer.writeln("</plist>")
    if didOpen:
        pathOrFile.close()


class DumbXMLWriter:

    def __init__(self, file, indentLevel=0, indent="\t"):
        self.file = file
        self.stack = []
        self.indentLevel = indentLevel
        self.indent = indent

    def beginElement(self, element):
        self.stack.append(element)
        self.writeln("<%s>" % element)
        self.indentLevel += 1

    def endElement(self, element):
        assert self.indentLevel > 0
        assert self.stack.pop() == element
        self.indentLevel -= 1
        self.writeln("</%s>" % element)

    def simpleElement(self, element, value=None):
        if value is not None:
            value = _escapeAndEncode(value)
            self.writeln("<%s>%s</%s>" % (element, value, element))
        else:
            self.writeln("<%s/>" % element)

    def writeln(self, line):
        if line:
            self.file.write(self.indentLevel * self.indent + line + "\n")
        else:
            self.file.write("\n")


import re
# Regex to strip all control chars, but for \t \n \r and \f
_controlStripper = re.compile(r"[\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0b\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f]")

def _escapeAndEncode(text):
    text = text.replace("\r\n", "\n")       # convert DOS line endings
    text = text.replace("\r", "\n")         # convert Mac line endings
    text = text.replace("&", "&amp;")       # escape '&'
    text = text.replace("<", "&lt;")        # escape '<'
    text = _controlStripper.sub("?", text)  # replace control chars with '?'
    return text.encode("utf-8")             # encode as UTF-8


PLISTHEADER = """\
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
"""

class PlistWriter(DumbXMLWriter):

    def __init__(self, file, indentLevel=0, indent="\t", writeHeader=1):
        if writeHeader:
            file.write(PLISTHEADER)
        DumbXMLWriter.__init__(self, file, indentLevel, indent)

    def writeValue(self, value):
        if isinstance(value, (str, unicode)):
            self.simpleElement("string", value)
        elif isinstance(value, bool):
            # must switch for bool before int, as bool is a
            # subclass of int...
            if value:
                self.simpleElement("true")
            else:
                self.simpleElement("false")
        elif isinstance(value, int):
            self.simpleElement("integer", str(value))
        elif isinstance(value, float):
            # should perhaps use repr() for better precision?
            self.simpleElement("real", str(value))
        elif isinstance(value, dict):
            self.writeDict(value)
        elif isinstance(value, Data):
            self.writeData(value)
        elif isinstance(value, Date):
            self.simpleElement("date", value.toString())
        elif isinstance(value, (tuple, list)):
            self.writeArray(value)
        else:
            raise TypeError("unsuported type: %s" % type(value))

    def writeData(self, data):
        self.beginElement("data")
        for line in data.asBase64().split("\n"):
            if line:
                self.writeln(line)
        self.endElement("data")

    def writeDict(self, d):
        self.beginElement("dict")
        items = d.items()
        items.sort()
        for key, value in items:
            if not isinstance(key, (str, unicode)):
                raise TypeError("keys must be strings")
            self.simpleElement("key", key)
            self.writeValue(value)
        self.endElement("dict")

    def writeArray(self, array):
        self.beginElement("array")
        for value in array:
            self.writeValue(value)
        self.endElement("array")


class Dict(dict):

    """Convenience dictionary subclass: it allows dict construction using
    keyword arguments (just like dict() in 2.3) as well as attribute notation
    to retrieve values, making d.foo more or less uquivalent to d["foo"].
    """

    def __new__(cls, **kwargs):
        self = dict.__new__(cls)
        self.update(kwargs)
        return self

    def __init__(self, **kwargs):
        self.update(kwargs)

    def __getattr__(self, attr):
        try:
            value = self[attr]
        except KeyError:
            raise AttributeError, attr
        return value

    def __setattr__(self, attr, value):
        self[attr] = value

    def __delattr__(self, attr):
        try:
            del self[attr]
        except KeyError:
            raise AttributeError, attr


class Plist(Dict):

    """This class has been deprecated! Use the Dict with readPlist() and
    writePlist() functions instead.
    """

    def fromFile(cls, pathOrFile):
        """Deprecated! Use the readPlist() function instead."""
        rootObject = readPlist(pathOrFile)
        plist = cls()
        plist.update(rootObject)
        return plist
    fromFile = classmethod(fromFile)

    def write(self, pathOrFile):
        """Deprecated! Use the writePlist() function instead."""
        writePlist(self, pathOrFile)


class Data:

    """Wrapper for binary data."""

    def __init__(self, data):
        self.data = data

    def fromBase64(cls, data):
        import base64
        return cls(base64.decodestring(data))
    fromBase64 = classmethod(fromBase64)

    def asBase64(self):
        import base64
        return base64.encodestring(self.data)

    def __cmp__(self, other):
        if isinstance(other, self.__class__):
            return cmp(self.data, other.data)
        elif isinstance(other, str):
            return cmp(self.data, other)
        else:
            return cmp(id(self), id(other))

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__, repr(self.data))


class Date:

    """Primitive date wrapper, uses time floats internally, is agnostic
    about time zones.
    """

    def __init__(self, date):
        if isinstance(date, str):
            from xml.utils.iso8601 import parse
            date = parse(date)
        self.date = date

    def toString(self):
        from xml.utils.iso8601 import tostring
        return tostring(self.date)

    def __cmp__(self, other):
        if isinstance(other, self.__class__):
            return cmp(self.date, other.date)
        elif isinstance(other, (int, float)):
            return cmp(self.date, other)
        else:
            return cmp(id(self), id(other))

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__, repr(self.toString()))


class PlistParser:

    def __init__(self):
        self.stack = []
        self.currentKey = None
        self.root = None

    def parse(self, file):
        from xml.parsers.expat import ParserCreate
        parser = ParserCreate()
        parser.StartElementHandler = self.handleBeginElement
        parser.EndElementHandler = self.handleEndElement
        parser.CharacterDataHandler = self.handleData
        parser.ParseFile(file)
        return self.root

    def handleBeginElement(self, element, attrs):
        self.data = []
        handler = getattr(self, "begin_" + element, None)
        if handler is not None:
            handler(attrs)

    def handleEndElement(self, element):
        handler = getattr(self, "end_" + element, None)
        if handler is not None:
            handler()

    def handleData(self, data):
        self.data.append(data)

    def addObject(self, value):
        if self.currentKey is not None:
            self.stack[-1][self.currentKey] = value
            self.currentKey = None
        elif not self.stack:
            # this is the root object
            self.root = value
        else:
            self.stack[-1].append(value)

    def getData(self):
        data = "".join(self.data)
        try:
            data = data.encode("ascii")
        except UnicodeError:
            pass
        self.data = []
        return data

    # element handlers

    def begin_dict(self, attrs):
        d = Dict()
        self.addObject(d)
        self.stack.append(d)
    def end_dict(self):
        self.stack.pop()

    def end_key(self):
        self.currentKey = self.getData()

    def begin_array(self, attrs):
        a = []
        self.addObject(a)
        self.stack.append(a)
    def end_array(self):
        self.stack.pop()

    def end_true(self):
        self.addObject(True)
    def end_false(self):
        self.addObject(False)
    def end_integer(self):
        self.addObject(int(self.getData()))
    def end_real(self):
        self.addObject(float(self.getData()))
    def end_string(self):
        self.addObject(self.getData())
    def end_data(self):
        self.addObject(Data.fromBase64(self.getData()))
    def end_date(self):
        self.addObject(Date(self.getData()))


# cruft to support booleans in Python <= 2.3
import sys
if sys.version_info[:2] < (2, 3):
    # Python 2.2 and earlier: no booleans
    # Python 2.2.x: booleans are ints
    class bool(int):
        """Imitation of the Python 2.3 bool object."""
        def __new__(cls, value):
            return int.__new__(cls, not not value)
        def __repr__(self):
            if self:
                return "True"
            else:
                return "False"
    True = bool(1)
    False = bool(0)
else:
    # Bind the boolean builtins to local names
    True = True
    False = False
    bool = bool


if __name__ == "__main__":
    from StringIO import StringIO
    import time
    if len(sys.argv) == 1:
        pl = Dict(
            aString="Doodah",
            aList=["A", "B", 12, 32.1, [1, 2, 3]],
            aFloat = 0.1,
            anInt = 728,
            aDict=Dict(
                anotherString="<hello & hi there!>",
                aUnicodeValue=u'M\xe4ssig, Ma\xdf',
                aTrueValue=True,
                aFalseValue=False,
            ),
            someData = Data("<binary gunk>"),
            someMoreData = Data("<lots of binary gunk>" * 10),
#            aDate = Date(time.mktime(time.gmtime())),
        )
    elif len(sys.argv) == 2:
        pl = readPlist(sys.argv[1])
    else:
        print "Too many arguments: at most 1 plist file can be given."
        sys.exit(1)

    # unicode keys are possible, but a little awkward to use:
    pl[u'\xc5benraa'] = "That was a unicode key."
    f = StringIO()
    writePlist(pl, f)
    xml = f.getvalue()
    print xml
    f.seek(0)
    pl2 = readPlist(f)
    assert pl == pl2
    f = StringIO()
    writePlist(pl2, f)
    assert xml == f.getvalue()
    #print repr(pl2)

"""plistlib.py -- a tool to generate and parse MacOSX .plist files.

The main class in this module is Plist. It takes a set of arbitrary
keyword arguments, which will be the top level elements of the plist
dictionary. After instantiation you can add more elements by assigning
new attributes to the Plist instance.

To write out a plist file, call the write() method of the Plist
instance with a filename or a file object.

To parse a plist from a file, use the Plist.fromFile(pathOrFile)
classmethod, with a file name or a file object as the only argument.
(Warning: you need pyexpat installed for this to work, ie. it doesn't
work with a vanilla Python 2.2 as shipped with MacOS X.2.)

Values can be strings, integers, floats, booleans, tuples, lists,
dictionaries, Data or Date objects. String values (including dictionary
keys) may be unicode strings -- they will be written out as UTF-8.

For convenience, this module exports a class named Dict(), which
allows you to easily construct (nested) dicts using keyword arguments.
But regular dicts work, too.

To support Boolean values in plists with Python < 2.3, "bool", "True"
and "False" are exported. Use these symbols from this module if you
want to be compatible with Python 2.2.x (strongly recommended).

The <data> plist type is supported through the Data class. This is a
thin wrapper around a Python string.

The <date> plist data has (limited) support through the Date class.
(Warning: Dates are only supported if the PyXML package is installed.)

Generate Plist example:

    pl = Plist(
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
    pl.write(fileName)

Parse Plist example:

    pl = Plist.fromFile(pathOrFile)
    print pl.aKey


"""

# written by Just van Rossum (just@letterror.com), 2002-11-19


__all__ = ["Plist", "Data", "Date", "Dict", "False", "True", "bool"]


INDENT = "\t"


class DumbXMLWriter:

    def __init__(self, file):
        self.file = file
        self.stack = []
        self.indentLevel = 0

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
        if value:
            value = _encode(value)
            self.writeln("<%s>%s</%s>" % (element, value, element))
        else:
            self.writeln("<%s/>" % element)

    def writeln(self, line):
        if line:
            self.file.write(self.indentLevel * INDENT + line + "\n")
        else:
            self.file.write("\n")


def _encode(text):
    text = text.replace("&", "&amp;")
    text = text.replace("<", "&lt;")
    return text.encode("utf-8")


PLISTHEADER = """\
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
"""

class PlistWriter(DumbXMLWriter):

    def __init__(self, file):
        file.write(PLISTHEADER)
        DumbXMLWriter.__init__(self, file)

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
        elif isinstance(value, (dict, Dict)):
            self.writeDict(value)
        elif isinstance(value, Data):
            self.writeData(value)
        elif isinstance(value, Date):
            self.simpleElement("date", value.toString())
        elif isinstance(value, (tuple, list)):
            self.writeArray(value)
        else:
            assert 0, "unsuported type: %s" % type(value)

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
            assert isinstance(key, (str, unicode)), "keys must be strings"
            self.simpleElement("key", key)
            self.writeValue(value)
        self.endElement("dict")

    def writeArray(self, array):
        self.beginElement("array")
        for value in array:
            self.writeValue(value)
        self.endElement("array")


class Dict:

    """Dict wrapper for convenient access of values through attributes."""

    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

    def __cmp__(self, other):
        if isinstance(other, self.__class__):
            return cmp(self.__dict__, other.__dict__)
        elif isinstance(other, dict):
            return cmp(self.__dict__, other)
        else:
            return cmp(id(self), id(other))

    def __str__(self):
        return "%s(**%s)" % (self.__class__.__name__, self.__dict__)
    __repr__ = __str__

    def copy(self):
        return self.__class__(**self.__dict__)

    def __getattr__(self, attr):
        """Delegate everything else to the dict object."""
        return getattr(self.__dict__, attr)


class Plist(Dict):

    """The main Plist object. Basically a dict (the toplevel object
    of a plist is a dict) with two additional methods to read from
    and write to files.
    """

    def fromFile(cls, pathOrFile):
        didOpen = 0
        if not hasattr(pathOrFile, "write"):
            pathOrFile = open(pathOrFile)
            didOpen = 1
        p = PlistParser()
        plist = p.parse(pathOrFile)
        if didOpen:
            pathOrFile.close()
        return plist
    fromFile = classmethod(fromFile)

    def write(self, pathOrFile):
        if not hasattr(pathOrFile, "write"):
            pathOrFile = open(pathOrFile, "w")
            didOpen = 1
        else:
            didOpen = 0

        writer = PlistWriter(pathOrFile)
        writer.writeln("<plist version=\"1.0\">")
        writer.writeDict(self.__dict__)
        writer.writeln("</plist>")

        if didOpen:
            pathOrFile.close()


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
            assert self.root is value
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
        if self.root is None:
            self.root = d = Plist()
        else:
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
        pl = Plist(
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
    elif len(sys.argv) == 2:
        pl = Plist.fromFile(sys.argv[1])
    else:
        print "Too many arguments: at most 1 plist file can be given."
        sys.exit(1)

    # unicode keys are possible, but a little awkward to use:
    pl[u'\xc5benraa'] = "That was a unicode key."
    f = StringIO()
    pl.write(f)
    xml = f.getvalue()
    print xml
    f.seek(0)
    pl2 = Plist.fromFile(f)
    assert pl == pl2
    f = StringIO()
    pl2.write(f)
    assert xml == f.getvalue()
    #print repr(pl2)

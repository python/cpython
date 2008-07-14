"""aetypes - Python objects representing various AE types."""

from warnings import warnpy3k
warnpy3k("In 3.x, the aetypes module is removed.", stacklevel=2)

from Carbon.AppleEvents import *
import struct
from types import *
import string

#
# convoluted, since there are cyclic dependencies between this file and
# aetools_convert.
#
def pack(*args, **kwargs):
    from aepack import pack
    return pack( *args, **kwargs)

def nice(s):
    """'nice' representation of an object"""
    if type(s) is StringType: return repr(s)
    else: return str(s)

class Unknown:
    """An uninterpreted AE object"""

    def __init__(self, type, data):
        self.type = type
        self.data = data

    def __repr__(self):
        return "Unknown(%r, %r)" % (self.type, self.data)

    def __aepack__(self):
        return pack(self.data, self.type)

class Enum:
    """An AE enumeration value"""

    def __init__(self, enum):
        self.enum = "%-4.4s" % str(enum)

    def __repr__(self):
        return "Enum(%r)" % (self.enum,)

    def __str__(self):
        return string.strip(self.enum)

    def __aepack__(self):
        return pack(self.enum, typeEnumeration)

def IsEnum(x):
    return isinstance(x, Enum)

def mkenum(enum):
    if IsEnum(enum): return enum
    return Enum(enum)

# Jack changed the way this is done
class InsertionLoc:
    def __init__(self, of, pos):
        self.of = of
        self.pos = pos

    def __repr__(self):
        return "InsertionLoc(%r, %r)" % (self.of, self.pos)

    def __aepack__(self):
        rec = {'kobj': self.of, 'kpos': self.pos}
        return pack(rec, forcetype='insl')

# Convenience functions for dsp:
def beginning(of):
    return InsertionLoc(of, Enum('bgng'))

def end(of):
    return InsertionLoc(of, Enum('end '))

class Boolean:
    """An AE boolean value"""

    def __init__(self, bool):
        self.bool = (not not bool)

    def __repr__(self):
        return "Boolean(%r)" % (self.bool,)

    def __str__(self):
        if self.bool:
            return "True"
        else:
            return "False"

    def __aepack__(self):
        return pack(struct.pack('b', self.bool), 'bool')

def IsBoolean(x):
    return isinstance(x, Boolean)

def mkboolean(bool):
    if IsBoolean(bool): return bool
    return Boolean(bool)

class Type:
    """An AE 4-char typename object"""

    def __init__(self, type):
        self.type = "%-4.4s" % str(type)

    def __repr__(self):
        return "Type(%r)" % (self.type,)

    def __str__(self):
        return string.strip(self.type)

    def __aepack__(self):
        return pack(self.type, typeType)

def IsType(x):
    return isinstance(x, Type)

def mktype(type):
    if IsType(type): return type
    return Type(type)


class Keyword:
    """An AE 4-char keyword object"""

    def __init__(self, keyword):
        self.keyword = "%-4.4s" % str(keyword)

    def __repr__(self):
        return "Keyword(%r)" % `self.keyword`

    def __str__(self):
        return string.strip(self.keyword)

    def __aepack__(self):
        return pack(self.keyword, typeKeyword)

def IsKeyword(x):
    return isinstance(x, Keyword)

class Range:
    """An AE range object"""

    def __init__(self, start, stop):
        self.start = start
        self.stop = stop

    def __repr__(self):
        return "Range(%r, %r)" % (self.start, self.stop)

    def __str__(self):
        return "%s thru %s" % (nice(self.start), nice(self.stop))

    def __aepack__(self):
        return pack({'star': self.start, 'stop': self.stop}, 'rang')

def IsRange(x):
    return isinstance(x, Range)

class Comparison:
    """An AE Comparison"""

    def __init__(self, obj1, relo, obj2):
        self.obj1 = obj1
        self.relo = "%-4.4s" % str(relo)
        self.obj2 = obj2

    def __repr__(self):
        return "Comparison(%r, %r, %r)" % (self.obj1, self.relo, self.obj2)

    def __str__(self):
        return "%s %s %s" % (nice(self.obj1), string.strip(self.relo), nice(self.obj2))

    def __aepack__(self):
        return pack({'obj1': self.obj1,
                 'relo': mkenum(self.relo),
                 'obj2': self.obj2},
                'cmpd')

def IsComparison(x):
    return isinstance(x, Comparison)

class NComparison(Comparison):
    # The class attribute 'relo' must be set in a subclass

    def __init__(self, obj1, obj2):
        Comparison.__init__(obj1, self.relo, obj2)

class Ordinal:
    """An AE Ordinal"""

    def __init__(self, abso):
#       self.obj1 = obj1
        self.abso = "%-4.4s" % str(abso)

    def __repr__(self):
        return "Ordinal(%r)" % (self.abso,)

    def __str__(self):
        return "%s" % (string.strip(self.abso))

    def __aepack__(self):
        return pack(self.abso, 'abso')

def IsOrdinal(x):
    return isinstance(x, Ordinal)

class NOrdinal(Ordinal):
    # The class attribute 'abso' must be set in a subclass

    def __init__(self):
        Ordinal.__init__(self, self.abso)

class Logical:
    """An AE logical expression object"""

    def __init__(self, logc, term):
        self.logc = "%-4.4s" % str(logc)
        self.term = term

    def __repr__(self):
        return "Logical(%r, %r)" % (self.logc, self.term)

    def __str__(self):
        if type(self.term) == ListType and len(self.term) == 2:
            return "%s %s %s" % (nice(self.term[0]),
                                 string.strip(self.logc),
                                 nice(self.term[1]))
        else:
            return "%s(%s)" % (string.strip(self.logc), nice(self.term))

    def __aepack__(self):
        return pack({'logc': mkenum(self.logc), 'term': self.term}, 'logi')

def IsLogical(x):
    return isinstance(x, Logical)

class StyledText:
    """An AE object respresenting text in a certain style"""

    def __init__(self, style, text):
        self.style = style
        self.text = text

    def __repr__(self):
        return "StyledText(%r, %r)" % (self.style, self.text)

    def __str__(self):
        return self.text

    def __aepack__(self):
        return pack({'ksty': self.style, 'ktxt': self.text}, 'STXT')

def IsStyledText(x):
    return isinstance(x, StyledText)

class AEText:
    """An AE text object with style, script and language specified"""

    def __init__(self, script, style, text):
        self.script = script
        self.style = style
        self.text = text

    def __repr__(self):
        return "AEText(%r, %r, %r)" % (self.script, self.style, self.text)

    def __str__(self):
        return self.text

    def __aepack__(self):
        return pack({keyAEScriptTag: self.script, keyAEStyles: self.style,
                 keyAEText: self.text}, typeAEText)

def IsAEText(x):
    return isinstance(x, AEText)

class IntlText:
    """A text object with script and language specified"""

    def __init__(self, script, language, text):
        self.script = script
        self.language = language
        self.text = text

    def __repr__(self):
        return "IntlText(%r, %r, %r)" % (self.script, self.language, self.text)

    def __str__(self):
        return self.text

    def __aepack__(self):
        return pack(struct.pack('hh', self.script, self.language)+self.text,
            typeIntlText)

def IsIntlText(x):
    return isinstance(x, IntlText)

class IntlWritingCode:
    """An object representing script and language"""

    def __init__(self, script, language):
        self.script = script
        self.language = language

    def __repr__(self):
        return "IntlWritingCode(%r, %r)" % (self.script, self.language)

    def __str__(self):
        return "script system %d, language %d"%(self.script, self.language)

    def __aepack__(self):
        return pack(struct.pack('hh', self.script, self.language),
            typeIntlWritingCode)

def IsIntlWritingCode(x):
    return isinstance(x, IntlWritingCode)

class QDPoint:
    """A point"""

    def __init__(self, v, h):
        self.v = v
        self.h = h

    def __repr__(self):
        return "QDPoint(%r, %r)" % (self.v, self.h)

    def __str__(self):
        return "(%d, %d)"%(self.v, self.h)

    def __aepack__(self):
        return pack(struct.pack('hh', self.v, self.h),
            typeQDPoint)

def IsQDPoint(x):
    return isinstance(x, QDPoint)

class QDRectangle:
    """A rectangle"""

    def __init__(self, v0, h0, v1, h1):
        self.v0 = v0
        self.h0 = h0
        self.v1 = v1
        self.h1 = h1

    def __repr__(self):
        return "QDRectangle(%r, %r, %r, %r)" % (self.v0, self.h0, self.v1, self.h1)

    def __str__(self):
        return "(%d, %d)-(%d, %d)"%(self.v0, self.h0, self.v1, self.h1)

    def __aepack__(self):
        return pack(struct.pack('hhhh', self.v0, self.h0, self.v1, self.h1),
            typeQDRectangle)

def IsQDRectangle(x):
    return isinstance(x, QDRectangle)

class RGBColor:
    """An RGB color"""

    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

    def __repr__(self):
        return "RGBColor(%r, %r, %r)" % (self.r, self.g, self.b)

    def __str__(self):
        return "0x%x red, 0x%x green, 0x%x blue"% (self.r, self.g, self.b)

    def __aepack__(self):
        return pack(struct.pack('hhh', self.r, self.g, self.b),
            typeRGBColor)

def IsRGBColor(x):
    return isinstance(x, RGBColor)

class ObjectSpecifier:

    """A class for constructing and manipulation AE object specifiers in python.

    An object specifier is actually a record with four fields:

    key type    description
    --- ----    -----------

    'want'  type    4-char class code of thing we want,
            e.g. word, paragraph or property

    'form'  enum    how we specify which 'want' thing(s) we want,
            e.g. by index, by range, by name, or by property specifier

    'seld'  any which thing(s) we want,
            e.g. its index, its name, or its property specifier

    'from'  object  the object in which it is contained,
            or null, meaning look for it in the application

    Note that we don't call this class plain "Object", since that name
    is likely to be used by the application.
    """

    def __init__(self, want, form, seld, fr = None):
        self.want = want
        self.form = form
        self.seld = seld
        self.fr = fr

    def __repr__(self):
        s = "ObjectSpecifier(%r, %r, %r" % (self.want, self.form, self.seld)
        if self.fr:
            s = s + ", %r)" % (self.fr,)
        else:
            s = s + ")"
        return s

    def __aepack__(self):
        return pack({'want': mktype(self.want),
                 'form': mkenum(self.form),
                 'seld': self.seld,
                 'from': self.fr},
                'obj ')

def IsObjectSpecifier(x):
    return isinstance(x, ObjectSpecifier)


# Backwards compatibility, sigh...
class Property(ObjectSpecifier):

    def __init__(self, which, fr = None, want='prop'):
        ObjectSpecifier.__init__(self, want, 'prop', mktype(which), fr)

    def __repr__(self):
        if self.fr:
            return "Property(%r, %r)" % (self.seld.type, self.fr)
        else:
            return "Property(%r)" % (self.seld.type,)

    def __str__(self):
        if self.fr:
            return "Property %s of %s" % (str(self.seld), str(self.fr))
        else:
            return "Property %s" % str(self.seld)


class NProperty(ObjectSpecifier):
    # Subclasses *must* self baseclass attributes:
    # want is the type of this property
    # which is the property name of this property

    def __init__(self, fr = None):
        #try:
        #   dummy = self.want
        #except:
        #   self.want = 'prop'
        self.want = 'prop'
        ObjectSpecifier.__init__(self, self.want, 'prop',
                    mktype(self.which), fr)

    def __repr__(self):
        rv = "Property(%r" % (self.seld.type,)
        if self.fr:
            rv = rv + ", fr=%r" % (self.fr,)
        if self.want != 'prop':
            rv = rv + ", want=%r" % (self.want,)
        return rv + ")"

    def __str__(self):
        if self.fr:
            return "Property %s of %s" % (str(self.seld), str(self.fr))
        else:
            return "Property %s" % str(self.seld)


class SelectableItem(ObjectSpecifier):

    def __init__(self, want, seld, fr = None):
        t = type(seld)
        if t == StringType:
            form = 'name'
        elif IsRange(seld):
            form = 'rang'
        elif IsComparison(seld) or IsLogical(seld):
            form = 'test'
        elif t == TupleType:
            # Breakout: specify both form and seld in a tuple
            # (if you want ID or rele or somesuch)
            form, seld = seld
        else:
            form = 'indx'
        ObjectSpecifier.__init__(self, want, form, seld, fr)


class ComponentItem(SelectableItem):
    # Derived classes *must* set the *class attribute* 'want' to some constant
    # Also, dictionaries _propdict and _elemdict must be set to map property
    # and element names to the correct classes

    _propdict = {}
    _elemdict = {}
    def __init__(self, which, fr = None):
        SelectableItem.__init__(self, self.want, which, fr)

    def __repr__(self):
        if not self.fr:
            return "%s(%r)" % (self.__class__.__name__, self.seld)
        return "%s(%r, %r)" % (self.__class__.__name__, self.seld, self.fr)

    def __str__(self):
        seld = self.seld
        if type(seld) == StringType:
            ss = repr(seld)
        elif IsRange(seld):
            start, stop = seld.start, seld.stop
            if type(start) == InstanceType == type(stop) and \
               start.__class__ == self.__class__ == stop.__class__:
                ss = str(start.seld) + " thru " + str(stop.seld)
            else:
                ss = str(seld)
        else:
            ss = str(seld)
        s = "%s %s" % (self.__class__.__name__, ss)
        if self.fr: s = s + " of %s" % str(self.fr)
        return s

    def __getattr__(self, name):
        if self._elemdict.has_key(name):
            cls = self._elemdict[name]
            return DelayedComponentItem(cls, self)
        if self._propdict.has_key(name):
            cls = self._propdict[name]
            return cls(self)
        raise AttributeError, name


class DelayedComponentItem:
    def __init__(self, compclass, fr):
        self.compclass = compclass
        self.fr = fr

    def __call__(self, which):
        return self.compclass(which, self.fr)

    def __repr__(self):
        return "%s(???, %r)" % (self.__class__.__name__, self.fr)

    def __str__(self):
        return "selector for element %s of %s"%(self.__class__.__name__, str(self.fr))

template = """
class %s(ComponentItem): want = '%s'
"""

exec template % ("Text", 'text')
exec template % ("Character", 'cha ')
exec template % ("Word", 'cwor')
exec template % ("Line", 'clin')
exec template % ("paragraph", 'cpar')
exec template % ("Window", 'cwin')
exec template % ("Document", 'docu')
exec template % ("File", 'file')
exec template % ("InsertionPoint", 'cins')

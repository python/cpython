"""Tools for use in AppleEvent clients and servers:
conversion between AE types and python types

pack(x) converts a Python object to an AEDesc object
unpack(desc) does the reverse
coerce(x, wanted_sample) coerces a python object to another python object
"""

#
# This code was originally written by Guido, and modified/extended by Jack
# to include the various types that were missing. The reference used is
# Apple Event Registry, chapter 9.
#

import struct
import types
from types import *
from Carbon import AE
from Carbon.AppleEvents import *
import MacOS
import Carbon.File
import aetypes
from aetypes import mkenum, ObjectSpecifier

# These ones seem to be missing from AppleEvents
# (they're in AERegistry.h)

#typeColorTable = 'clrt'
#typeDrawingArea = 'cdrw'
#typePixelMap = 'cpix'
#typePixelMapMinus = 'tpmm'
#typeRotation = 'trot'
#typeTextStyles = 'tsty'
#typeStyledText = 'STXT'
#typeAEText = 'tTXT'
#typeEnumeration = 'enum'

#
# Some AE types are immedeately coerced into something
# we like better (and which is equivalent)
#
unpacker_coercions = {
    typeComp : typeFloat,
    typeColorTable : typeAEList,
    typeDrawingArea : typeAERecord,
    typeFixed : typeFloat,
    typeExtended : typeFloat,
    typePixelMap : typeAERecord,
    typeRotation : typeAERecord,
    typeStyledText : typeAERecord,
    typeTextStyles : typeAERecord,
};

#
# Some python types we need in the packer:
#
AEDescType = AE.AEDescType
FSSType = Carbon.File.FSSpecType
FSRefType = Carbon.File.FSRefType
AliasType = Carbon.File.AliasType

def packkey(ae, key, value):
    if hasattr(key, 'which'):
        keystr = key.which
    elif hasattr(key, 'want'):
        keystr = key.want
    else:
        keystr = key
    ae.AEPutParamDesc(keystr, pack(value))

def pack(x, forcetype = None):
    """Pack a python object into an AE descriptor"""

    if forcetype:
        if type(x) is StringType:
            return AE.AECreateDesc(forcetype, x)
        else:
            return pack(x).AECoerceDesc(forcetype)

    if x == None:
        return AE.AECreateDesc('null', '')

    if isinstance(x, AEDescType):
        return x
    if isinstance(x, FSSType):
        return AE.AECreateDesc('fss ', x.data)
    if isinstance(x, FSRefType):
        return AE.AECreateDesc('fsrf', x.data)
    if isinstance(x, AliasType):
        return AE.AECreateDesc('alis', x.data)
    if isinstance(x, IntType):
        return AE.AECreateDesc('long', struct.pack('l', x))
    if isinstance(x, FloatType):
        return AE.AECreateDesc('doub', struct.pack('d', x))
    if isinstance(x, StringType):
        return AE.AECreateDesc('TEXT', x)
    if isinstance(x, UnicodeType):
        data = x.encode('utf16')
        if data[:2] == '\xfe\xff':
            data = data[2:]
        return AE.AECreateDesc('utxt', data)
    if isinstance(x, ListType):
        list = AE.AECreateList('', 0)
        for item in x:
            list.AEPutDesc(0, pack(item))
        return list
    if isinstance(x, DictionaryType):
        record = AE.AECreateList('', 1)
        for key, value in x.items():
            packkey(record, key, value)
            #record.AEPutParamDesc(key, pack(value))
        return record
    if type(x) == types.ClassType and issubclass(x, ObjectSpecifier):
        # Note: we are getting a class object here, not an instance
        return AE.AECreateDesc('type', x.want)
    if hasattr(x, '__aepack__'):
        return x.__aepack__()
    if hasattr(x, 'which'):
        return AE.AECreateDesc('TEXT', x.which)
    if hasattr(x, 'want'):
        return AE.AECreateDesc('TEXT', x.want)
    return AE.AECreateDesc('TEXT', repr(x)) # Copout

def unpack(desc, formodulename=""):
    """Unpack an AE descriptor to a python object"""
    t = desc.type

    if unpacker_coercions.has_key(t):
        desc = desc.AECoerceDesc(unpacker_coercions[t])
        t = desc.type # This is a guess by Jack....

    if t == typeAEList:
        l = []
        for i in range(desc.AECountItems()):
            keyword, item = desc.AEGetNthDesc(i+1, '****')
            l.append(unpack(item, formodulename))
        return l
    if t == typeAERecord:
        d = {}
        for i in range(desc.AECountItems()):
            keyword, item = desc.AEGetNthDesc(i+1, '****')
            d[keyword] = unpack(item, formodulename)
        return d
    if t == typeAEText:
        record = desc.AECoerceDesc('reco')
        return mkaetext(unpack(record, formodulename))
    if t == typeAlias:
        return Carbon.File.Alias(rawdata=desc.data)
    # typeAppleEvent returned as unknown
    if t == typeBoolean:
        return struct.unpack('b', desc.data)[0]
    if t == typeChar:
        return desc.data
    if t == typeUnicodeText:
        return unicode(desc.data, 'utf16')
    # typeColorTable coerced to typeAEList
    # typeComp coerced to extended
    # typeData returned as unknown
    # typeDrawingArea coerced to typeAERecord
    if t == typeEnumeration:
        return mkenum(desc.data)
    # typeEPS returned as unknown
    if t == typeFalse:
        return 0
    if t == typeFloat:
        data = desc.data
        return struct.unpack('d', data)[0]
    if t == typeFSS:
        return Carbon.File.FSSpec(rawdata=desc.data)
    if t == typeFSRef:
        return Carbon.File.FSRef(rawdata=desc.data)
    if t == typeInsertionLoc:
        record = desc.AECoerceDesc('reco')
        return mkinsertionloc(unpack(record, formodulename))
    # typeInteger equal to typeLongInteger
    if t == typeIntlText:
        script, language = struct.unpack('hh', desc.data[:4])
        return aetypes.IntlText(script, language, desc.data[4:])
    if t == typeIntlWritingCode:
        script, language = struct.unpack('hh', desc.data)
        return aetypes.IntlWritingCode(script, language)
    if t == typeKeyword:
        return mkkeyword(desc.data)
    if t == typeLongInteger:
        return struct.unpack('l', desc.data)[0]
    if t == typeLongDateTime:
        a, b = struct.unpack('lL', desc.data)
        return (long(a) << 32) + b
    if t == typeNull:
        return None
    if t == typeMagnitude:
        v = struct.unpack('l', desc.data)
        if v < 0:
            v = 0x100000000L + v
        return v
    if t == typeObjectSpecifier:
        record = desc.AECoerceDesc('reco')
        # If we have been told the name of the module we are unpacking aedescs for,
        # we can attempt to create the right type of python object from that module.
        if formodulename:
            return mkobjectfrommodule(unpack(record, formodulename), formodulename)
        return mkobject(unpack(record, formodulename))
    # typePict returned as unknown
    # typePixelMap coerced to typeAERecord
    # typePixelMapMinus returned as unknown
    # typeProcessSerialNumber returned as unknown
    if t == typeQDPoint:
        v, h = struct.unpack('hh', desc.data)
        return aetypes.QDPoint(v, h)
    if t == typeQDRectangle:
        v0, h0, v1, h1 = struct.unpack('hhhh', desc.data)
        return aetypes.QDRectangle(v0, h0, v1, h1)
    if t == typeRGBColor:
        r, g, b = struct.unpack('hhh', desc.data)
        return aetypes.RGBColor(r, g, b)
    # typeRotation coerced to typeAERecord
    # typeScrapStyles returned as unknown
    # typeSessionID returned as unknown
    if t == typeShortFloat:
        return struct.unpack('f', desc.data)[0]
    if t == typeShortInteger:
        return struct.unpack('h', desc.data)[0]
    # typeSMFloat identical to typeShortFloat
    # typeSMInt indetical to typeShortInt
    # typeStyledText coerced to typeAERecord
    if t == typeTargetID:
        return mktargetid(desc.data)
    # typeTextStyles coerced to typeAERecord
    # typeTIFF returned as unknown
    if t == typeTrue:
        return 1
    if t == typeType:
        return mktype(desc.data, formodulename)
    #
    # The following are special
    #
    if t == 'rang':
        record = desc.AECoerceDesc('reco')
        return mkrange(unpack(record, formodulename))
    if t == 'cmpd':
        record = desc.AECoerceDesc('reco')
        return mkcomparison(unpack(record, formodulename))
    if t == 'logi':
        record = desc.AECoerceDesc('reco')
        return mklogical(unpack(record, formodulename))
    return mkunknown(desc.type, desc.data)

def coerce(data, egdata):
    """Coerce a python object to another type using the AE coercers"""
    pdata = pack(data)
    pegdata = pack(egdata)
    pdata = pdata.AECoerceDesc(pegdata.type)
    return unpack(pdata)

#
# Helper routines for unpack
#
def mktargetid(data):
    sessionID = getlong(data[:4])
    name = mkppcportrec(data[4:4+72])
    location = mklocationnamerec(data[76:76+36])
    rcvrName = mkppcportrec(data[112:112+72])
    return sessionID, name, location, rcvrName

def mkppcportrec(rec):
    namescript = getword(rec[:2])
    name = getpstr(rec[2:2+33])
    portkind = getword(rec[36:38])
    if portkind == 1:
        ctor = rec[38:42]
        type = rec[42:46]
        identity = (ctor, type)
    else:
        identity = getpstr(rec[38:38+33])
    return namescript, name, portkind, identity

def mklocationnamerec(rec):
    kind = getword(rec[:2])
    stuff = rec[2:]
    if kind == 0: stuff = None
    if kind == 2: stuff = getpstr(stuff)
    return kind, stuff

def mkunknown(type, data):
    return aetypes.Unknown(type, data)

def getpstr(s):
    return s[1:1+ord(s[0])]

def getlong(s):
    return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getword(s):
    return (ord(s[0])<<8) | (ord(s[1])<<0)

def mkkeyword(keyword):
    return aetypes.Keyword(keyword)

def mkrange(dict):
    return aetypes.Range(dict['star'], dict['stop'])

def mkcomparison(dict):
    return aetypes.Comparison(dict['obj1'], dict['relo'].enum, dict['obj2'])

def mklogical(dict):
    return aetypes.Logical(dict['logc'], dict['term'])

def mkstyledtext(dict):
    return aetypes.StyledText(dict['ksty'], dict['ktxt'])

def mkaetext(dict):
    return aetypes.AEText(dict[keyAEScriptTag], dict[keyAEStyles], dict[keyAEText])

def mkinsertionloc(dict):
    return aetypes.InsertionLoc(dict[keyAEObject], dict[keyAEPosition])

def mkobject(dict):
    want = dict['want'].type
    form = dict['form'].enum
    seld = dict['seld']
    fr   = dict['from']
    if form in ('name', 'indx', 'rang', 'test'):
        if want == 'text': return aetypes.Text(seld, fr)
        if want == 'cha ': return aetypes.Character(seld, fr)
        if want == 'cwor': return aetypes.Word(seld, fr)
        if want == 'clin': return aetypes.Line(seld, fr)
        if want == 'cpar': return aetypes.Paragraph(seld, fr)
        if want == 'cwin': return aetypes.Window(seld, fr)
        if want == 'docu': return aetypes.Document(seld, fr)
        if want == 'file': return aetypes.File(seld, fr)
        if want == 'cins': return aetypes.InsertionPoint(seld, fr)
    if want == 'prop' and form == 'prop' and aetypes.IsType(seld):
        return aetypes.Property(seld.type, fr)
    return aetypes.ObjectSpecifier(want, form, seld, fr)

# Note by Jack: I'm not 100% sure of the following code. This was
# provided by Donovan Preston, but I wonder whether the assignment
# to __class__ is safe. Moreover, shouldn't there be a better
# initializer for the classes in the suites?
def mkobjectfrommodule(dict, modulename):
    if type(dict['want']) == types.ClassType and issubclass(dict['want'], ObjectSpecifier):
        # The type has already been converted to Python. Convert back:-(
        classtype = dict['want']
        dict['want'] = aetypes.mktype(classtype.want)
    want = dict['want'].type
    module = __import__(modulename)
    codenamemapper = module._classdeclarations
    classtype = codenamemapper.get(want, None)
    newobj = mkobject(dict)
    if classtype:
        assert issubclass(classtype, ObjectSpecifier)
        newobj.__class__ = classtype
    return newobj

def mktype(typecode, modulename=None):
    if modulename:
        module = __import__(modulename)
        codenamemapper = module._classdeclarations
        classtype = codenamemapper.get(typecode, None)
        if classtype:
            return classtype
    return aetypes.mktype(typecode)

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
from Carbon import AE
from Carbon.AppleEvents import *
import MacOS
import Carbon.File
import io
import aetypes
from aetypes import mkenum, ObjectSpecifier
import os

# These ones seem to be missing from AppleEvents
# (they're in AERegistry.h)

#typeColorTable = b'clrt'
#typeDrawingArea = b'cdrw'
#typePixelMap = b'cpix'
#typePixelMapMinus = b'tpmm'
#typeRotation = b'trot'
#typeTextStyles = b'tsty'
#typeStyledText = b'STXT'
#typeAEText = b'tTXT'
#typeEnumeration = b'enum'

def b2i(byte_string):
    result = 0
    for byte in byte_string:
        result <<= 8
        result += byte
    return result
#
# Some AE types are immedeately coerced into something
# we like better (and which is equivalent)
#
unpacker_coercions = {
    b2i(typeComp) : typeFloat,
    b2i(typeColorTable) : typeAEList,
    b2i(typeDrawingArea) : typeAERecord,
    b2i(typeFixed) : typeFloat,
    b2i(typeExtended) : typeFloat,
    b2i(typePixelMap) : typeAERecord,
    b2i(typeRotation) : typeAERecord,
    b2i(typeStyledText) : typeAERecord,
    b2i(typeTextStyles) : typeAERecord,
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
        if isinstance(x, bytes):
            return AE.AECreateDesc(forcetype, x)
        else:
            return pack(x).AECoerceDesc(forcetype)

    if x == None:
        return AE.AECreateDesc(b'null', '')

    if isinstance(x, AEDescType):
        return x
    if isinstance(x, FSSType):
        return AE.AECreateDesc(b'fss ', x.data)
    if isinstance(x, FSRefType):
        return AE.AECreateDesc(b'fsrf', x.data)
    if isinstance(x, AliasType):
        return AE.AECreateDesc(b'alis', x.data)
    if isinstance(x, int):
        return AE.AECreateDesc(b'long', struct.pack('l', x))
    if isinstance(x, float):
        return AE.AECreateDesc(b'doub', struct.pack('d', x))
    if isinstance(x, (bytes, bytearray)):
        return AE.AECreateDesc(b'TEXT', x)
    if isinstance(x, str):
        # See http://developer.apple.com/documentation/Carbon/Reference/Apple_Event_Manager/Reference/reference.html#//apple_ref/doc/constant_group/typeUnicodeText
        # for the possible encodings.
        data = x.encode('utf16')
        if data[:2] == b'\xfe\xff':
            data = data[2:]
        return AE.AECreateDesc(b'utxt', data)
    if isinstance(x, list):
        lst = AE.AECreateList('', 0)
        for item in x:
            lst.AEPutDesc(0, pack(item))
        return lst
    if isinstance(x, dict):
        record = AE.AECreateList('', 1)
        for key, value in x.items():
            packkey(record, key, value)
            #record.AEPutParamDesc(key, pack(value))
        return record
    if isinstance(x, type) and issubclass(x, ObjectSpecifier):
        # Note: we are getting a class object here, not an instance
        return AE.AECreateDesc(b'type', x.want)
    if hasattr(x, '__aepack__'):
        return x.__aepack__()
    if hasattr(x, 'which'):
        return AE.AECreateDesc(b'TEXT', x.which)
    if hasattr(x, 'want'):
        return AE.AECreateDesc(b'TEXT', x.want)
    return AE.AECreateDesc(b'TEXT', repr(x)) # Copout

def unpack(desc, formodulename=""):
    """Unpack an AE descriptor to a python object"""
    t = desc.type

    if b2i(t) in unpacker_coercions:
        desc = desc.AECoerceDesc(unpacker_coercions[b2i(t)])
        t = desc.type # This is a guess by Jack....

    if t == typeAEList:
        l = []
        for i in range(desc.AECountItems()):
            keyword, item = desc.AEGetNthDesc(i+1, b'****')
            l.append(unpack(item, formodulename))
        return l
    if t == typeAERecord:
        d = {}
        for i in range(desc.AECountItems()):
            keyword, item = desc.AEGetNthDesc(i+1, b'****')
            d[b2i(keyword)] = unpack(item, formodulename)
        return d
    if t == typeAEText:
        record = desc.AECoerceDesc(b'reco')
        return mkaetext(unpack(record, formodulename))
    if t == typeAlias:
        return Carbon.File.Alias(rawdata=desc.data)
    # typeAppleEvent returned as unknown
    if t == typeBoolean:
        return struct.unpack('b', desc.data)[0]
    if t == typeChar:
        return desc.data
    if t == typeUnicodeText:
        return str(desc.data, 'utf16')
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
        record = desc.AECoerceDesc(b'reco')
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
        return (int(a) << 32) + b
    if t == typeNull:
        return None
    if t == typeMagnitude:
        v = struct.unpack('l', desc.data)
        if v < 0:
            v = 0x100000000 + v
        return v
    if t == typeObjectSpecifier:
        record = desc.AECoerceDesc(b'reco')
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
    if t == b'rang':
        record = desc.AECoerceDesc(b'reco')
        return mkrange(unpack(record, formodulename))
    if t == b'cmpd':
        record = desc.AECoerceDesc(b'reco')
        return mkcomparison(unpack(record, formodulename))
    if t == b'logi':
        record = desc.AECoerceDesc(b'reco')
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
    return aetypes.Range(dict[b2i(b'star')], dict[b2i(b'stop')])

def mkcomparison(dict):
    return aetypes.Comparison(dict[b2i(b'obj1')],
                              dict[b2i(b'relo')].enum,
                              dict[b2i(b'obj2')])

def mklogical(dict):
    return aetypes.Logical(dict[b2i(b'logc')], dict[b2i(b'term')])

def mkstyledtext(dict):
    return aetypes.StyledText(dict[b2i(b'ksty')], dict[b2i(b'ktxt')])

def mkaetext(dict):
    return aetypes.AEText(dict[b2i(keyAEScriptTag)],
                          dict[b2i(keyAEStyles)],
                          dict[b2i(keyAEText)])

def mkinsertionloc(dict):
    return aetypes.InsertionLoc(dict[b2i(keyAEObject)],
                                dict[b2i(keyAEPosition)])

def mkobject(dict):
    want = dict[b2i(b'want')].type
    form = dict[b2i(b'form')].enum
    seld = dict[b2i(b'seld')]
    fr   = dict[b2i(b'from')]
    if form in (b'name', b'indx', b'rang', b'test'):
        if want == b'text': return aetypes.Text(seld, fr)
        if want == b'cha ': return aetypes.Character(seld, fr)
        if want == b'cwor': return aetypes.Word(seld, fr)
        if want == b'clin': return aetypes.Line(seld, fr)
        if want == b'cpar': return aetypes.Paragraph(seld, fr)
        if want == b'cwin': return aetypes.Window(seld, fr)
        if want == b'docu': return aetypes.Document(seld, fr)
        if want == b'file': return aetypes.File(seld, fr)
        if want == b'cins': return aetypes.InsertionPoint(seld, fr)
    if want == b'prop' and form == b'prop' and aetypes.IsType(seld):
        return aetypes.Property(seld.type, fr)
    return aetypes.ObjectSpecifier(want, form, seld, fr)

# Note by Jack: I'm not 100% sure of the following code. This was
# provided by Donovan Preston, but I wonder whether the assignment
# to __class__ is safe. Moreover, shouldn't there be a better
# initializer for the classes in the suites?
def mkobjectfrommodule(dict, modulename):
    if (isinstance(dict[b2i(b'want')], type) and
        issubclass(dict[b2i(b'want')], ObjectSpecifier)):
        # The type has already been converted to Python. Convert back:-(
        classtype = dict[b2i(b'want')]
        dict[b2i(b'want')] = aetypes.mktype(classtype.want)
    want = dict[b2i(b'want')].type
    module = __import__(modulename)
    codenamemapper = module._classdeclarations
    classtype = codenamemapper.get(b2i(want), None)
    newobj = mkobject(dict)
    if classtype:
        assert issubclass(classtype, ObjectSpecifier)
        newobj.__class__ = classtype
    return newobj

def mktype(typecode, modulename=None):
    if modulename:
        module = __import__(modulename)
        codenamemapper = module._classdeclarations
        classtype = codenamemapper.get(b2i(typecode), None)
        if classtype:
            return classtype
    return aetypes.mktype(typecode)

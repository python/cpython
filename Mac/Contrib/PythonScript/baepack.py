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
import string
import types
from string import strip
from types import *
import AE
from AppleEvents import *
import MacOS
import macfs
import StringIO
import baetypes
from baetypes import mkenum, mktype

import calldll

OSL = calldll.getlibrary('ObjectSupportLib')

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
	typeComp : typeExtended,
	typeColorTable : typeAEList,
	typeDrawingArea : typeAERecord,
	typeFixed : typeExtended,
	typeFloat : typeExtended,
	typePixelMap : typeAERecord,
	typeRotation : typeAERecord,
	typeStyledText : typeAERecord,
	typeTextStyles : typeAERecord,
};

#
# Some python types we need in the packer:
#
AEDescType = type(AE.AECreateDesc('TEXT', ''))
_sample_fss = macfs.FSSpec(':')
_sample_alias = _sample_fss.NewAliasMinimal()
FSSType = type(_sample_fss)
AliasType = type(_sample_alias)

def pack(x, forcetype = None):
	"""Pack a python object into an AE descriptor"""
#	print 'aepack', x, type(x), forcetype
#	if type(x) == TupleType:
#		forcetype, x = x
	if forcetype:
		print x, forcetype
		if type(x) is StringType:
			return AE.AECreateDesc(forcetype, x)
		else:
			return pack(x).AECoerceDesc(forcetype)
			
	if x == None:
		return AE.AECreateDesc('null', '')
		
	t = type(x)
	if t == AEDescType:
		return x
	if t == FSSType:
		return AE.AECreateDesc('fss ', x.data)
	if t == AliasType:
		return AE.AECreateDesc('alis', x.data)
	if t == IntType:
		return AE.AECreateDesc('long', struct.pack('l', x))
	if t == FloatType:
		#
		# XXXX (note by Guido) Weird thing -- Think C's "double" is 10 bytes, but
		# struct.pack('d') return 12 bytes (and struct.unpack requires
		# them, too).  The first 2 bytes seem to be repeated...
		# Probably an alignment problem
		# XXXX (note by Jack) haven't checked this under MW
		#
#		return AE.AECreateDesc('exte', struct.pack('d', x)[2:])
		return AE.AECreateDesc('exte', struct.pack('d', x))
	if t == StringType:
		return AE.AECreateDesc('TEXT', x)
	if t == ListType:
		list = AE.AECreateList('', 0)
		for item in x:
			list.AEPutDesc(0, pack(item))
		return list
	if t == DictionaryType:
		record = AE.AECreateList('', 1)
		for key, value in x.items():
			record.AEPutParamDesc(key, pack(value))
		return record
	if t == InstanceType and hasattr(x, '__aepack__'):
		return x.__aepack__()
	return AE.AECreateDesc('TEXT', repr(x)) # Copout

def unpack(desc):
	"""Unpack an AE descriptor to a python object"""
	t = desc.type
#	print t
	
	if unpacker_coercions.has_key(t):
		desc = desc.AECoerceDesc(unpacker_coercions[t])
		t = desc.type # This is a guess by Jack....
	
	if t == typeAEList:
		l = []
		for i in range(desc.AECountItems()):
			keyword, item = desc.AEGetNthDesc(i+1, '****')
			l.append(unpack(item))
		return l
	if t == typeAERecord:
		d = {}
		for i in range(desc.AECountItems()):
			keyword, item = desc.AEGetNthDesc(i+1, '****')
			d[keyword] = unpack(item)
		return d
	if t == typeAEText:
		record = desc.AECoerceDesc('reco')
		return mkaetext(unpack(record))
	if t == typeAlias:
		return macfs.RawAlias(desc.data)
	# typeAppleEvent returned as unknown
	if t == typeBoolean:
		return struct.unpack('b', desc.data)[0]
	if t == typeChar:
		return desc.data
	# typeColorTable coerced to typeAEList
	# typeComp coerced to extended
	# typeData returned as unknown
	# typeDrawingArea coerced to typeAERecord
	if t == typeEnumeration:
		return mkenum(desc.data)
	# typeEPS returned as unknown
	if t == typeExtended:
#		print desc, type(desc), len(desc)
		data = desc.data
#		print `data[:8]`, type(data), len(data[:8])
#		print struct.unpack('=d', data[:8])[0]
#		print string.atoi(data), type(data), len(data)
#		print struct.calcsize(data)
		# XXX See corresponding note for pack()
#		return struct.unpack('d', data[:2] + data)[0]
		return struct.unpack('d', data[:8])[0]
	if t == typeFalse:
		return 0
	# typeFixed coerced to extended
	# typeFloat coerced to extended
	if t == typeFSS:
		return macfs.RawFSSpec(desc.data)
	if t == typeInsertionLoc:
		record = desc.AECoerceDesc('reco')
		return mkinsertionloc(unpack(record))
	# typeInteger equal to typeLongInteger
	if t == typeIntlText:
		script, language = struct.unpack('hh', desc.data[:4])
		return baetypes.IntlText(script, language, desc.data[4:])
	if t == typeIntlWritingCode:
		script, language = struct.unpack('hh', desc.data)
		return baetypes.IntlWritingCode(script, language)
	if t == typeKeyword:
		return mkkeyword(desc.data)
	# typeLongFloat is equal to typeFloat
	if t == typeLongInteger:
#		print t, struct.unpack('l', desc.data)
		return struct.unpack('l', desc.data)[0]
	if t == typeNull:
		return None
	if t == typeMagnitude:
		v = struct.unpack('l', desc.data)
		if v < 0:
			v = 0x100000000L + v
		return v
	if t == typeObjectSpecifier:
		import Res
#		print desc, type(desc)
#		print desc.__members__
#		print desc.data, desc.type
#		print unpack(desc)
#		getOSL = calldll.newcall(OSL.AEResolve, 'OSErr', 'InHandle', 'InShort')#, 'InString')
#		print 'OSL', getOSL(rdesc, 0)#, desc.data)
		record = desc.AECoerceDesc('reco')
#		print record
		return mkobject(unpack(record))
	# typePict returned as unknown
	# typePixelMap coerced to typeAERecord
	# typePixelMapMinus returned as unknown
	# typeProcessSerialNumber returned as unknown
	if t == typeQDPoint:
		v, h = struct.unpack('hh', desc.data)
		return baetypes.QDPoint(v, h)
	if t == typeQDRectangle:
		v0, h0, v1, h1 = struct.unpack('hhhh', desc.data)
		return baetypes.QDRectangle(v0, h0, v1, h1)
	if t == typeRGBColor:
		r, g, b = struct.unpack('hhh', desc.data)
		return baetypes.RGBColor(r, g, b)
	# typeRotation coerced to typeAERecord
	# typeScrapStyles returned as unknown
	# typeSessionID returned as unknown
	if t == typeShortFloat:
		return struct.unpack('f', desc.data)[0]
	if t == typeShortInteger:
#		print t, desc.data
#		print struct.unpack('h', desc.data)[0]
		return struct.unpack('h', desc.data)[0]
	# typeSMFloat identical to typeShortFloat
	# typeSMInt	indetical to typeShortInt
	# typeStyledText coerced to typeAERecord
	if t == typeTargetID:
		return mktargetid(desc.data)
	# typeTextStyles coerced to typeAERecord
	# typeTIFF returned as unknown
	if t == typeTrue:
		return 1
	if t == typeType:
#		print t, desc.data
		return mktype(desc.data)
	#
	# The following are special
	#
	if t == 'rang':
		record = desc.AECoerceDesc('reco')
		return mkrange(unpack(record))
	if t == 'cmpd':
		record = desc.AECoerceDesc('reco')
		return mkcomparison(unpack(record))
	if t == 'logi':
		record = desc.AECoerceDesc('reco')
		return mklogical(unpack(record))
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
	return baetypes.Unknown(type, data)

def getpstr(s):
	return s[1:1+ord(s[0])]

def getlong(s):
	return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getword(s):
	return (ord(s[0])<<8) | (ord(s[1])<<0)

def mkkeyword(keyword):
	return baetypes.Keyword(keyword)

def mkrange(dict):
	return baetypes.Range(dict['star'], dict['stop'])

def mkcomparison(dict):
	return baetypes.Comparison(dict['obj1'], dict['relo'].enum, dict['obj2'])

def mklogical(dict):
	return baetypes.Logical(dict['logc'], dict['term'])

def mkstyledtext(dict):
	return baetypes.StyledText(dict['ksty'], dict['ktxt'])
	
def mkaetext(dict):
	return baetypes.AEText(dict[keyAEScriptTag], dict[keyAEStyles], dict[keyAEText])
	
def mkinsertionloc(dict):
	return baetypes.InsertionLoc(dict[keyAEObject], dict[keyAEPosition])

def mkobject(dict):
	want = dict['want'].type
	form = dict['form'].enum
	seld = dict['seld']
	fr   = dict['from']
	if form in ('name', 'indx', 'rang', 'test'):
		if want == 'text': return baetypes.Text(seld, fr)
		if want == 'cha ': return baetypes.Character(seld, fr)
		if want == 'cwor': return baetypes.Word(seld, fr)
		if want == 'clin': return baetypes.Line(seld, fr)
		if want == 'cpar': return baetypes.Paragraph(seld, fr)
		if want == 'cwin': return baetypes.Window(seld, fr)
		if want == 'docu': return baetypes.Document(seld, fr)
		if want == 'file': return baetypes.File(seld, fr)
		if want == 'cins': return baetypes.InsertionPoint(seld, fr)
	if want == 'prop' and form == 'prop' and baetypes.IsType(seld):
		return baetypes.Property(seld.type, fr)
	return baetypes.ObjectSpecifier(want, form, seld, fr)

def _test():
	"""Test program. Pack and unpack various things"""
	objs = [
		'a string',
		12,
		12.0,
		None,
		['a', 'list', 'of', 'strings'],
		{'key1': 'value1', 'key2':'value2'},
		macfs.FSSpec(':'),
		macfs.FSSpec(':').NewAliasMinimal(),
		baetypes.Enum('enum'),
		baetypes.Type('type'),
		baetypes.Keyword('kwrd'),
		baetypes.Range(1, 10),
		baetypes.Comparison(1, '<   ', 10),
		baetypes.Logical('not ', 1),
		# Cannot do StyledText
		# Cannot do AEText
		baetypes.IntlText(0, 0, 'international text'),
		baetypes.IntlWritingCode(0,0),
		baetypes.QDPoint(50,100),
		baetypes.QDRectangle(50,100,150,200),
		baetypes.RGBColor(0x7000, 0x6000, 0x5000),
		baetypes.Unknown('xxxx', 'unknown type data'),
		baetypes.Character(1),
		baetypes.Character(2, baetypes.Line(2)),
	]
	for o in objs:
		print 'BEFORE', o, `o`
		print type(o)
		packed = pack(o)
		unpacked = unpack(packed)
		print 'AFTER ', unpacked, `unpacked`
	import sys
	sys.exit(1)
	
if __name__ == '__main__':
	_test()
	

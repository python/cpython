"""Tools for use in AppleEvent clients and servers.

pack(x) converts a Python object to an AEDesc object
unpack(desc) does the reverse

packevent(event, parameters, attributes) sets params and attrs in an AEAppleEvent record
unpackevent(event) returns the parameters and attributes from an AEAppleEvent record

Plus...  Lots of classes and routines that help representing AE objects,
ranges, conditionals, logicals, etc., so you can write, e.g.:

	x = Character(1, Document("foobar"))

and pack(x) will create an AE object reference equivalent to AppleScript's

	character 1 of document "foobar"

"""


import struct
import string
from string import strip
from types import *
import AE
import MacOS
import macfs
import StringIO


AEDescType = type(AE.AECreateDesc('TEXT', ''))

FSSType = type(macfs.FSSpec(':'))


def pack(x, forcetype = None):
	if forcetype:
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
		vol, dir, filename = x.as_tuple()
		fnlen = len(filename)
		header = struct.pack('hlb', vol, dir, fnlen)
		padding = '\0'*(63-fnlen)
		return AE.AECreateDesc('fss ', header + filename + padding)
	if t == IntType:
		return AE.AECreateDesc('long', struct.pack('l', x))
	if t == FloatType:
		# XXX Weird thing -- Think C's "double" is 10 bytes, but
		# struct.pack('d') return 12 bytes (and struct.unpack requires
		# them, too).  The first 2 bytes seem to be repeated...
		# Probably an alignment problem
		return AE.AECreateDesc('exte', struct.pack('d', x)[2:])
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
			record.AEPutKeyDesc(key, pack(value))
		return record
	if t == InstanceType and hasattr(x, '__aepack__'):
		return x.__aepack__()
	return AE.AECreateDesc('TEXT', repr(x)) # Copout


def unpack(desc):
	t = desc.type
	if t == 'TEXT':
		return desc.data
	if t == 'fals':
		return 0
	if t == 'true':
		return 1
	if t == 'enum':
		return mkenum(desc.data)
	if t == 'type':
		return mktype(desc.data)
	if t == 'long':
		return struct.unpack('l', desc.data)[0]
	if t == 'shor':
		return struct.unpack('h', desc.data)[0]
	if t == 'sing':
		return struct.unpack('f', desc.data)[0]
	if t == 'exte':
		data = desc.data
		# XXX See corresponding note for pack()
		return struct.unpack('d', data[:2] + data)[0]
	if t in ('doub', 'comp', 'magn'):
		return unpack(desc.AECoerceDesc('exte'))
	if t == 'null':
		return None
	if t == 'list':
		l = []
		for i in range(desc.AECountItems()):
			keyword, item = desc.AEGetNthDesc(i+1, '****')
			l.append(unpack(item))
		return l
	if t == 'reco':
		d = {}
		for i in range(desc.AECountItems()):
			keyword, item = desc.AEGetNthDesc(i+1, '****')
			d[keyword] = unpack(item)
		return d
	if t == 'obj ':
		record = desc.AECoerceDesc('reco')
		return mkobject(unpack(record))
	if t == 'rang':
		record = desc.AECoerceDesc('reco')
		return mkrange(unpack(record))
	if t == 'cmpd':
		record = desc.AECoerceDesc('reco')
		return mkcomparison(unpack(record))
	if t == 'logi':
		record = desc.AECoerceDesc('reco')
		return mklogical(unpack(record))
	if t == 'targ':
		return mktargetid(desc.data)
	if t == 'alis':
		# XXX Can't handle alias records yet, so coerce to FS spec...
		return unpack(desc.AECoerceDesc('fss '))
	if t == 'fss ':
		return mkfss(desc.data)
	return mkunknown(desc.type, desc.data)


def mkfss(data):
	print "mkfss data =", `data`
	vol, dir, fnlen = struct.unpack('hlb', data[:7])
	filename = data[7:7+fnlen]
	print (vol, dir, fnlen, filename)
	return macfs.FSSpec((vol, dir, filename))


def mktargetid(data):
	sessionID = getlong(data[:4])
	name = mkppcportrec(data[4:4+72])
	print len(name), `name`
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

def getpstr(s):
	return s[1:1+ord(s[0])]

def getlong(s):
	return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getword(s):
	return (ord(s[0])<<8) | (ord(s[1])<<0)


def mkunknown(type, data):
	return Unknown(type, data)

class Unknown:
	
	def __init__(self, type, data):
		self.type = type
		self.data = data
	
	def __repr__(self):
		return "Unknown(%s, %s)" % (`self.type`, `self.data`)
	
	def __aepack__(self):
		return pack(self.data, self.type)


def IsSubclass(cls, base):
	"""Test whether CLASS1 is the same as or a subclass of CLASS2"""
	# Loop to optimize for single inheritance
	while 1:
		if cls is base: return 1
		if len(cls.__bases__) <> 1: break
		cls = cls.__bases__[0]
	# Recurse to cope with multiple inheritance
	for c in cls.__bases__:
		if IsSubclass(c, base): return 1
	return 0

def IsInstance(x, cls):
	"""Test whether OBJECT is an instance of (a subclass of) CLASS"""
	return type(x) is InstanceType and IsSubclass(x.__class__, cls)


def nice(s):
	if type(s) is StringType: return repr(s)
	else: return str(s)


def mkenum(enum):
	if IsEnum(enum): return enum
	return Enum(enum)

class Enum:
	
	def __init__(self, enum):
		self.enum = "%-4.4s" % str(enum)
	
	def __repr__(self):
		return "Enum(%s)" % `self.enum`
	
	def __str__(self):
		return strip(self.enum)
	
	def __aepack__(self):
		return pack(self.enum, 'enum')

def IsEnum(x):
	return IsInstance(x, Enum)


def mktype(type):
	if IsType(type): return type
	return Type(type)

class Type:
	
	def __init__(self, type):
		self.type = "%-4.4s" % str(type)
	
	def __repr__(self):
		return "Type(%s)" % `self.type`
	
	def __str__(self):
		return strip(self.type)
	
	def __aepack__(self):
		return pack(self.type, 'type')

def IsType(x):
	return IsInstance(x, Type)


def mkrange(dict):
	return Range(dict['star'], dict['stop'])

class Range:
	
	def __init__(self, start, stop):
		self.start = start
		self.stop = stop
	
	def __repr__(self):
		return "Range(%s, %s)" % (`self.start`, `self.stop`)
	
	def __str__(self):
		return "%s thru %s" % (nice(self.start), nice(self.stop))
	
	def __aepack__(self):
		return pack({'star': self.start, 'stop': self.stop}, 'rang')

def IsRange(x):
	return IsInstance(x, Range)


def mkcomparison(dict):
	return Comparison(dict['obj1'], dict['relo'].enum, dict['obj2'])

class Comparison:
	
	def __init__(self, obj1, relo, obj2):
		self.obj1 = obj1
		self.relo = "%-4.4s" % str(relo)
		self.obj2 = obj2
	
	def __repr__(self):
		return "Comparison(%s, %s, %s)" % (`self.obj1`, `self.relo`, `self.obj2`)
	
	def __str__(self):
		return "%s %s %s" % (nice(self.obj1), strip(self.relo), nice(self.obj2))
	
	def __aepack__(self):
		return pack({'obj1': self.obj1,
			     'relo': mkenum(self.relo),
			     'obj2': self.obj2},
			    'cmpd')

def IsComparison(x):
	return IsInstance(x, Comparison)


def mklogical(dict):
	return Logical(dict['logc'], dict['term'])

class Logical:
	
	def __init__(self, logc, term):
		self.logc = "%-4.4s" % str(logc)
		self.term = term
	
	def __repr__(self):
		return "Logical(%s, %s)" % (`self.logc`, `self.term`)
	
	def __str__(self):
		if type(self.term) == ListType and len(self.term) == 2:
			return "%s %s %s" % (nice(self.term[0]),
			                     strip(self.logc),
			                     nice(self.term[1]))
		else:
			return "%s(%s)" % (strip(self.logc), nice(self.term))
	
	def __aepack__(self):
		return pack({'logc': mkenum(self.logc), 'term': self.term}, 'logi')

def IsLogical(x):
	return IsInstance(x, Logical)


class ObjectSpecifier:
	
	"""A class for constructing and manipulation AE object specifiers in python.
	
	An object specifier is actually a record with four fields:
	
	key	type	description
	---	----	-----------
	
	'want'	type	what kind of thing we want,
			e.g. word, paragraph or property
	
	'form'	enum	how we specify the thing(s) we want,
			e.g. by index, by range, by name, or by property specifier
	
	'seld'	any	which thing(s) we want,
			e.g. its index, its name, or its property specifier
	
	'from'	object	the object in which it is contained,
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
		s = "ObjectSpecifier(%s, %s, %s" % (`self.want`, `self.form`, `self.seld`)
		if self.fr:
			s = s + ", %s)" % `self.fr`
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
	return IsInstance(x, ObjectSpecifier)


class Property(ObjectSpecifier):

	def __init__(self, which, fr = None):
		ObjectSpecifier.__init__(self, 'prop', 'prop', mkenum(which), fr)

	def __repr__(self):
		if self.fr:
			return "Property(%s, %s)" % (`self.seld.enum`, `self.fr`)
		else:
			return "Property(%s)" % `self.seld.enum`
	
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
		else:
			form = 'indx'
		ObjectSpecifier.__init__(self, want, form, seld, fr)


class ComponentItem(SelectableItem):
	# Derived classes *must* set the *class attribute* 'want' to some constant
	
	def __init__(self, which, fr = None):
		SelectableItem.__init__(self, self.want, which, fr)
	
	def __repr__(self):
		if not self.fr:
			return "%s(%s)" % (self.__class__.__name__, `self.seld`)
		return "%s(%s, %s)" % (self.__class__.__name__, `self.seld`, `self.fr`)
	
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


template = """
class %s(ComponentItem): want = '%s'
"""

exec template % ("Text", 'text')
exec template % ("Character", 'cha ')
exec template % ("Word", 'cwor')
exec template % ("Line", 'clin')
exec template % ("Paragraph", 'cpar')
exec template % ("Window", 'cwin')
exec template % ("Document", 'docu')
exec template % ("File", 'file')
exec template % ("InsertionPoint", 'cins')


def mkobject(dict):
	want = dict['want'].type
	form = dict['form'].enum
	seld = dict['seld']
	fr   = dict['from']
	if form in ('name', 'indx', 'rang', 'test'):
		if want == 'text': return Text(seld, fr)
		if want == 'cha ': return Character(seld, fr)
		if want == 'cwor': return Word(seld, fr)
		if want == 'clin': return Line(seld, fr)
		if want == 'cpar': return Paragraph(seld, fr)
		if want == 'cwin': return Window(seld, fr)
		if want == 'docu': return Document(seld, fr)
		if want == 'file': return File(seld, fr)
		if want == 'cins': return InsertionPoint(seld, fr)
	if want == 'prop' and form == 'prop' and IsType(seld):
		return Property(seld.type, fr)
	return ObjectSpecifier(want, form, seld, fr)


# Special code to unpack an AppleEvent (which is *not* a disguised record!)

aekeywords = [
	'tran',
	'rtid',
	'evcl',
	'evid',
	'addr',
	'optk',
	'timo',
	'inte',	# this attribute is read only - will be set in AESend
	'esrc',	# this attribute is read only
	'miss',	# this attribute is read only
	'from'	# new in 1.0.1
]

def missed(ae):
	try:
		desc = ae.AEGetAttributeDesc('miss', 'keyw')
	except AE.Error, msg:
		return None
	return desc.data

def unpackevent(ae):
	parameters = {}
	while 1:
		key = missed(ae)
		if not key: break
		parameters[key] = unpack(ae.AEGetParamDesc(key, '****'))
	attributes = {}
	for key in aekeywords:
		try:
			desc = ae.AEGetAttributeDesc(key, '****')
		except (AE.Error, MacOS.Error), msg:
			if msg[0] != -1701:
				raise sys.exc_type, sys.exc_value
			continue
		attributes[key] = unpack(desc)
	return parameters, attributes

def packevent(ae, parameters = {}, attributes = {}):
	for key, value in parameters.items():
		ae.AEPutParamDesc(key, pack(value))
	for key, value in attributes.items():
		ae.AEPutAttributeDesc(key, pack(value))


# Test program

def test():
	target = AE.AECreateDesc('sign', 'KAHL')
	ae = AE.AECreateAppleEvent('aevt', 'oapp', target, -1, 0)
	print unpackevent(ae)
	raw_input(":")
	ae = AE.AECreateAppleEvent('core', 'getd', target, -1, 0)
	obj = Character(2, Word(1, Document(1)))
	print obj
	print repr(obj)
	packevent(ae, {'----': obj})
	params, attrs = unpackevent(ae)
	print params['----']
	raw_input(":")

if __name__ == '__main__':
	test()

import struct
import types
import AE
import MacOS
import StringIO

AEDescType = type(AE.AECreateDesc('TEXT', ''))

def pack(x):
	if x == None:
		return AE.AECreateDesc('null', '')
	t = type(x)
	if t == AEDescType:
		return x
	if t == types.IntType:
		return AE.AECreateDesc('long', struct.pack('l', x))
	if t == types.FloatType:
		return AE.AECreateDesc('exte', struct.pack('d', x)[2:])
	if t == types.StringType:
		return AE.AECreateDesc('TEXT', x)
	if t == types.ListType:
		list = AE.AECreateList('', 0)
		for item in x:
			list.AEPutDesc(0, pack(item))
		return list
	if t == types.TupleType:
		t, d = x
		return AE.AECreateDesc(t, d)
	if t == types.DictionaryType:
		record = AE.AECreateList('', 1)
		for key, value in x.items():
			record.AEPutKeyDesc(key, pack(value))
	if t == types.InstanceType and hasattr(x, '__aepack__'):
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
	if t == 'long':
		return struct.unpack('l', desc.data)[0]
	if t == 'shor':
		return struct.unpack('h', desc.data)[0]
	if t == 'sing':
		return struct.unpack('f', desc.data)[0]
	if t == 'exte':
		data = desc.data
		return struct.unpack('d', data[:2] + data)[0]
	if t in ('doub', 'comp', 'magn'):
		return unpack(desc.AECoerceDesc('exte'))
	if t == 'enum':
		return ('enum', desc.data)
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
		return unpackobject(desc.data)
	return desc.type, desc.data # Copout

class Object:
	def __init__(self, dict = {}):
		self.dict = dict
		for key, value in dict.items():
			self.dict[key] = value
	def __repr__(self):
		return "Object(%s)" % `self.dict`
	def __str__(self):
		want = self.dict['want']
		form = self.dict['form']
		seld = self.dict['seld']
		s = "%s %s %s" % (nicewant(want), niceform(form), niceseld(seld))
		fr   = self.dict['from']
		if fr:
			s = s + " of " + str(fr)
		return s
	def __aepack__(self):
		f = StringIO.StringIO()
		putlong(f, len(self.dict))
		putlong(f, 0)
		for key, value in self.dict.items():
			putcode(f, key)
			desc = pack(value)
			putcode(f, desc.type)
			data = desc.data
			putlong(f, len(data))
			f.write(data)
		return AE.AECreateDesc('obj ', f.getvalue())

def nicewant(want):
	if type(want) == types.TupleType and len(want) == 2:
		return reallynicewant(want)
	else:
		return `want`

def reallynicewant((t, w)):
	if t != 'type': return `t, w`
	# These should be taken from the "elements" of the 'aete' resource
	if w == 'cins': return 'insertion point'
	if w == 'cha ': return 'character'
	if w == 'word': return 'word'
	if w == 'para': return 'paragraph'
	if w == 'ccel': return 'cell'
	if w == 'ccol': return 'column'
	if w == 'crow': return 'row'
	if w == 'crng': return 'range'
	if w == 'wind': return 'window'
	if w == 'docu': return 'document'
	return `w`

def niceform(form):
	if type(form) == types.TupleType and len(form) == 2:
		return reallyniceform(form)
	else:
		return `form`

def reallyniceform((t, f)):
	if t <> 'enum': return `t, f`
	if f == 'indx': return ''
	if f == 'name': return ''
	if f == 'rele': return ''
	return `f`

def niceseld(seld):
	if type(seld) == types.TupleType and len(seld) == 2:
		return reallyniceseld(seld)
	else:
		return `seld`

def reallyniceseld((t, s)):
	if t == 'long': return `s`
	if t == 'TEXT': return `s`
	if t == 'enum':
		if s == 'next': return 'after'
		if s == 'prev': return 'before'
	return `t, s`

def unpackobject(data):
	f = StringIO.StringIO(data)
	nkey = getlong(f)
	dumm = getlong(f)
	dict = {}
	for i in range(nkey):
		keyw = getcode(f)
		type = getcode(f)
		size = getlong(f)
		if size:
			data = f.read(size)
		else:
			data = ''
		desc = AE.AECreateDesc(type, data)
		dict[keyw] = unpack(desc)
	return Object(dict)


# --- get various data types from a "file"

def getword(f, *args):
	getalgn(f)
	s = f.read(2)
	if len(s) < 2:
		raise EOFError, 'in getword' + str(args)
	return (ord(s[0])<<8) | ord(s[1])

def getlong(f, *args):
	getalgn(f)
	s = f.read(4)
	if len(s) < 4:
		raise EOFError, 'in getlong' + str(args)
	return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getcode(f, *args):
	getalgn(f)
	s = f.read(4)
	if len(s) < 4:
		raise EOFError, 'in getcode' + str(args)
	return s

def getpstr(f, *args):
	c = f.read(1)
	if len(c) < 1:
		raise EOFError, 'in getpstr[1]' + str(args)
	nbytes = ord(c)
	if nbytes == 0: return ''
	s = f.read(nbytes)
	if len(s) < nbytes:
		raise EOFError, 'in getpstr[2]' + str(args)
	return s

def getalgn(f):
	if f.tell() & 1:
		c = f.read(1)
		##if c <> '\0':
		##	print 'align:', `c`

# ---- end get routines


# ---- put various data types to a "file"

def putlong(f, value):
	putalgn(f)
	f.write(chr((value>>24)&0xff))
	f.write(chr((value>>16)&0xff))
	f.write(chr((value>>8)&0xff))
	f.write(chr(value&0xff))

def putword(f, value):
	putalgn(f)
	f.write(chr((value>>8)&0xff))
	f.write(chr(value&0xff))

def putcode(f, value):
	if type(value) != types.StringType or len(value) != 4:
		raise TypeError, "ostype must be 4-char string"
	putalgn(f)
	f.write(value)

def putpstr(f, value):
	if type(value) != types.StringType or len(value) > 255:
		raise TypeError, "pstr must be string <= 255 chars"
	f.write(chr(len(value)) + value)

def putalgn(f):
	if f.tell() & 1:
		f.write('\0')

# ---- end put routines


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

def test():
	target = AE.AECreateDesc('sign', 'KAHL')
	ae = AE.AECreateAppleEvent('aevt', 'oapp', target, -1, 0)
	print unpackevent(ae)

if __name__ == '__main__':
	test()

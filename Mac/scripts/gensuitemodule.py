"""
gensuitemodule - Generate an AE suite module from an aete/aeut resource

Based on aete.py
"""

import MacOS
import os
import string
import sys
import types
import StringIO
import macfs

from Res import *

def main():
	fss, ok = macfs.PromptGetFile('Select file with aeut/aete resource:')
	if not ok:
		sys.exit(0)
	process(fss.as_pathname())

def process(fullname):
	"""Process all resources in a single file"""
	cur = CurResFile()
	print fullname
	rf = OpenRFPerm(fullname, 0, 1)
	try:
		UseResFile(rf)
		resources = []
		for i in range(Count1Resources('aete')):
			res = Get1IndResource('aete', 1+i)
			resources.append(res)
		for i in range(Count1Resources('aeut')):
			res = Get1IndResource('aeut', 1+i)
			resources.append(res)
		print "\nLISTING aete+aeut RESOURCES IN", `fullname`
		for res in resources:
			print "decoding", res.GetResInfo(), "..."
			data = res.data
			aete = decode(data)
			# switch back (needed for dialogs in Python)
			UseResFile(cur)
			compileaete(aete, fullname)
			UseResFile(rf)
	finally:
		if rf <> cur:
			CloseResFile(rf)
			UseResFile(cur)

def decode(data):
	"""Decode a resource into a python data structure"""
	f = StringIO.StringIO(data)
	aete = generic(getaete, f)
	aete = simplify(aete)
	processed = f.tell()
	unprocessed = len(f.read())
	total = f.tell()
	if unprocessed:
		sys.stderr.write("%d processed + %d unprocessed = %d total\n" %
		                 (processed, unprocessed, total))
	return aete

def simplify(item):
	"""Recursively replace singleton tuples by their constituent item"""
	if type(item) is types.ListType:
		return map(simplify, item)
	elif type(item) == types.TupleType and len(item) == 2:
		return simplify(item[1])
	else:
		return item


# Here follows the aete resource decoder.
# It is presented bottom-up instead of top-down because there are  direct
# references to the lower-level part-decoders from the high-level part-decoders.

def getbyte(f, *args):
	c = f.read(1)
	if not c:
		raise EOFError, 'in getbyte' + str(args)
	return ord(c)

def getword(f, *args):
	getalign(f)
	s = f.read(2)
	if len(s) < 2:
		raise EOFError, 'in getword' + str(args)
	return (ord(s[0])<<8) | ord(s[1])

def getlong(f, *args):
	getalign(f)
	s = f.read(4)
	if len(s) < 4:
		raise EOFError, 'in getlong' + str(args)
	return (ord(s[0])<<24) | (ord(s[1])<<16) | (ord(s[2])<<8) | ord(s[3])

def getostype(f, *args):
	getalign(f)
	s = f.read(4)
	if len(s) < 4:
		raise EOFError, 'in getostype' + str(args)
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

def getalign(f):
	if f.tell() & 1:
		c = f.read(1)
		##if c <> '\0':
		##	print 'align:', `c`

def getlist(f, description, getitem):
	count = getword(f)
	list = []
	for i in range(count):
		list.append(generic(getitem, f))
		getalign(f)
	return list

def alt_generic(what, f, *args):
	print "generic", `what`, args
	res = vageneric(what, f, args)
	print '->', `res`
	return res

def generic(what, f, *args):
	if type(what) == types.FunctionType:
		return apply(what, (f,) + args)
	if type(what) == types.ListType:
		record = []
		for thing in what:
			item = apply(generic, thing[:1] + (f,) + thing[1:])
			record.append((thing[1], item))
		return record
	return "BAD GENERIC ARGS: %s" % `what`

getdata = [
	(getostype, "type"),
	(getpstr, "description"),
	(getword, "flags")
	]
getargument = [
	(getpstr, "name"),
	(getostype, "keyword"),
	(getdata, "what")
	]
getevent = [
	(getpstr, "name"),
	(getpstr, "description"),
	(getostype, "suite code"),
	(getostype, "event code"),
	(getdata, "returns"),
	(getdata, "accepts"),
	(getlist, "optional arguments", getargument)
	]
getproperty = [
	(getpstr, "name"),
	(getostype, "code"),
	(getdata, "what")
	]
getelement = [
	(getostype, "type"),
	(getlist, "keyform", getostype)
	]
getclass = [
	(getpstr, "name"),
	(getostype, "class code"),
	(getpstr, "description"),
	(getlist, "properties", getproperty),
	(getlist, "elements", getelement)
	]
getcomparison = [
	(getpstr, "operator name"),
	(getostype, "operator ID"),
	(getpstr, "operator comment"),
	]
getenumerator = [
	(getpstr, "enumerator name"),
	(getostype, "enumerator ID"),
	(getpstr, "enumerator comment")
	]
getenumeration = [
	(getostype, "enumeration ID"),
	(getlist, "enumerator", getenumerator)
	]
getsuite = [
	(getpstr, "suite name"),
	(getpstr, "suite description"),
	(getostype, "suite ID"),
	(getword, "suite level"),
	(getword, "suite version"),
	(getlist, "events", getevent),
	(getlist, "classes", getclass),
	(getlist, "comparisons", getcomparison),
	(getlist, "enumerations", getenumeration)
	]
getaete = [
	(getword, "major/minor version in BCD"),
	(getword, "language code"),
	(getword, "script code"),
	(getlist, "suites", getsuite)
	]

def compileaete(aete, fname):
	"""Generate code for a full aete resource. fname passed for doc purposes"""
	[version, language, script, suites] = aete
	major, minor = divmod(version, 256)
	for suite in suites:
		compilesuite(suite, major, minor, language, script, fname)

def compilesuite(suite, major, minor, language, script, fname):
	"""Generate code for a single suite"""
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	
	modname = identify(name)
	fss, ok = macfs.StandardPutFile('Python output file', modname+'.py')
	if not ok:
		return
	fp = open(fss.as_pathname(), 'w')
	fss.SetCreatorType('Pyth', 'TEXT')
	
	fp.write('"""Suite %s: %s\n' % (name, desc))
	fp.write("Level %d, version %d\n\n" % (level, version))
	fp.write("Generated from %s\n"%fname)
	fp.write("AETE/AEUT resource version %d/%d, language %d, script %d\n" % \
		(major, minor, language, script))
	fp.write('"""\n\n')
	
	fp.write('import aetools\n')
	fp.write('import MacOS\n\n')
	fp.write("_code = %s\n\n"% `code`)
	
	compileclassheader(fp, modname)

	enumsneeded = {}
	if events:
		for event in events:
			compileevent(fp, event, enumsneeded)
	else:
		fp.write("\tpass\n\n")

	objc = ObjectCompiler(fp)
	for cls in classes:
		objc.compileclass(cls)
	for cls in classes:
		objc.fillclasspropsandelems(cls)
	for comp in comps:
		objc.compilecomparison(comp)
	for enum in enums:
		objc.compileenumeration(enum)
	
	for enum in enumsneeded.keys():
		objc.checkforenum(enum)
		
	objc.dumpindex()

def compileclassheader(fp, name):
	"""Generate class boilerplate"""
	fp.write("class %s:\n\n"%name)
	
def compileevent(fp, event, enumsneeded):
	"""Generate code for a single event"""
	[name, desc, code, subcode, returns, accepts, arguments] = event
	funcname = identify(name)
	#
	# generate name->keyword map
	#
	if arguments:
		fp.write("\t_argmap_%s = {\n"%funcname)
		for a in arguments:
			fp.write("\t\t%s : %s,\n"%(`identify(a[0])`, `a[1]`))
		fp.write("\t}\n\n")
		
	#
	# Generate function header
	#
	has_arg = (not is_null(accepts))
	opt_arg = (has_arg and is_optional(accepts))
	
	fp.write("\tdef %s(self, "%funcname)
	if has_arg:
		if not opt_arg:
			fp.write("_object, ")		# Include direct object, if it has one
		else:
			fp.write("_object=None, ")	# Also include if it is optional
	else:
		fp.write("_no_object=None, ")	# For argument checking
	fp.write("_attributes={}, **_arguments):\n")	# include attribute dict and args
	#
	# Generate doc string (important, since it may be the only
	# available documentation, due to our name-remaping)
	#
	fp.write('\t\t"""%s: %s\n'%(name, desc))
	if has_arg:
		fp.write("\t\tRequired argument: %s\n"%getdatadoc(accepts))
	elif opt_arg:
		fp.write("\t\tOptional argument: %s\n"%getdatadoc(accepts))
	for arg in arguments:
		fp.write("\t\tKeyword argument %s: %s\n"%(identify(arg[0]),
				getdatadoc(arg[2])))
	fp.write("\t\tKeyword argument _attributes: AppleEvent attribute dictionary\n")
	if not is_null(returns):
		fp.write("\t\tReturns: %s\n"%getdatadoc(returns))
	fp.write('\t\t"""\n')
	#
	# Fiddle the args so everything ends up in 'arguments' dictionary
	#
	fp.write("\t\t_code = %s\n"% `code`)
	fp.write("\t\t_subcode = %s\n\n"% `subcode`)
	#
	# Do keyword name substitution
	#
	if arguments:
		fp.write("\t\taetools.keysubst(_arguments, self._argmap_%s)\n"%funcname)
	else:
		fp.write("\t\tif _arguments: raise TypeError, 'No optional args expected'\n")
	#
	# Stuff required arg (if there is one) into arguments
	#
	if has_arg:
		fp.write("\t\t_arguments['----'] = _object\n")
	elif opt_arg:
		fp.write("\t\tif _object:\n")
		fp.write("\t\t\t_arguments['----'] = _object\n")
	else:
		fp.write("\t\tif _no_object != None: raise TypeError, 'No direct arg expected'\n")
	fp.write("\n")
	#
	# Do enum-name substitution
	#
	for a in arguments:
		if is_enum(a[2]):
			kname = a[1]
			ename = a[2][0]
			if ename <> '****':
				fp.write("\t\taetools.enumsubst(_arguments, %s, _Enum_%s)\n" %
					(`kname`, ename))
				enumsneeded[ename] = 1
	fp.write("\n")
	#
	# Do the transaction
	#
	fp.write("\t\t_reply, _arguments, _attributes = self.send(_code, _subcode,\n")
	fp.write("\t\t\t\t_arguments, _attributes)\n")
	#
	# Error handling
	#
	fp.write("\t\tif _arguments.has_key('errn'):\n")
	fp.write("\t\t\traise aetools.Error, aetools.decodeerror(_arguments)\n")
	fp.write("\t\t# XXXX Optionally decode result\n")
	#
	# Decode result
	#
	fp.write("\t\tif _arguments.has_key('----'):\n")
	if is_enum(returns):
		fp.write("\t\t\t# XXXX Should do enum remapping here...\n")
	fp.write("\t\t\treturn _arguments['----']\n")
	fp.write("\n")
	
#	print "\n#    Command %s -- %s (%s, %s)" % (`name`, `desc`, `code`, `subcode`)
#	print "#        returns", compiledata(returns)
#	print "#        accepts", compiledata(accepts)
#	for arg in arguments:
#		compileargument(arg)

def compileargument(arg):
	[name, keyword, what] = arg
	print "#        %s (%s)" % (name, `keyword`), compiledata(what)

class ObjectCompiler:
	def __init__(self, fp):
		self.fp = fp
		self.propnames = {}
		self.classnames = {}
		self.propcodes = {}
		self.classcodes = {}
		self.compcodes = {}
		self.enumcodes = {}
		self.othersuites = []
		
	def findcodename(self, type, code):
		while 1:
			if type == 'property':
				if self.propcodes.has_key(code):
					return self.propcodes[code], self.propcodes[code], None
				for s in self.othersuites:
					if s._propdeclarations.has_key(code):
						name = s._propdeclarations[code].__name__
						return name, '%s.%s' % (s.__name__, name), s.__name__
			if type == 'class':
				if self.classcodes.has_key(code):
					return self.classcodes[code], self.classcodes[code], None
				for s in self.othersuites:
					if s._classdeclarations.has_key(code):
						name = s._classdeclarations[code].__name__
						return name, '%s.%s' % (s.__name__, name), s.__name__
			if type == 'enum':
				if self.enumcodes.has_key(code):
					return self.enumcodes[code], self.enumcodes[code], None
				for s in self.othersuites:
					if s._enumdeclarations.has_key(code):
						name = '_Enum_' + identify(code)
						return name, '%s.%s' % (s.__name__, name), s.__name__
			if type == 'comparison':
				if self.compcodes.has_key(code):
					return self.compcodes[code], self.compcodes[code], None
				for s in self.othersuites:
					if s._compdeclarations.has_key(code):
						name = s._compdeclarations[code].__name__
						return name, '%s.%s' % (s.__name__, name), s.__name__
						
			m = self.askdefinitionmodule(type, code)
			if not m: return None, None, None
			self.othersuites.append(m)
	
	def askdefinitionmodule(self, type, code):
		fss, ok = macfs.PromptGetFile('Where is %s %s declared?'%(type, code))
		if not ok: return
		path, file = os.path.split(fss.as_pathname())
		modname = os.path.splitext(file)[0]
		if not path in sys.path:
			sys.path.insert(0, path)
		m = __import__(modname)
		self.fp.write("import %s\n"%modname)
		return m
		
	def compileclass(self, cls):
		[name, code, desc, properties, elements] = cls
		pname = identify(name)
		if self.classcodes.has_key(code):
			# plural forms and such
			self.fp.write("\n%s = %s\n"%(pname, self.classcodes[code]))
			self.classnames[pname] = code
		else:
			self.fp.write('\nclass %s(aetools.ComponentItem):\n' % pname)
			self.fp.write('\t"""%s - %s"""\n' % (name, desc))
			self.fp.write('\twant = %s\n' % `code`)
			self.classnames[pname] = code
			self.classcodes[code] = pname
		for prop in properties:
			self.compileproperty(prop)
		for elem in elements:
			self.compileelement(elem)
	
	def compileproperty(self, prop):
		[name, code, what] = prop
		if code == 'c@#!':
			# Something silly with plurals. Skip it.
			return
		pname = identify(name)
		if self.propcodes.has_key(code):
			self.fp.write("# repeated property %s %s\n"%(pname, what[1]))
		else:
			self.fp.write("class %s(aetools.NProperty):\n" % pname)
			self.fp.write('\t"""%s - %s"""\n' % (name, what[1]))
			self.fp.write("\twhich = %s\n" % `code`)
			self.fp.write("\twant = %s\n" % `what[0]`)
			self.propnames[pname] = code
			self.propcodes[code] = pname
	
	def compileelement(self, elem):
		[code, keyform] = elem
		self.fp.write("#        element %s as %s\n" % (`code`, keyform))

	def fillclasspropsandelems(self, cls):
		[name, code, desc, properties, elements] = cls
		cname = identify(name)
		if self.classcodes[code] != cname:
			# This is an other name (plural or so) for something else. Skip.
			return
		plist = []
		elist = []
		for prop in properties:
			[pname, pcode, what] = prop
			if pcode == 'c@#!':
				continue
			pname = identify(pname)
			plist.append(pname)
		for elem in elements:
			[ecode, keyform] = elem
			if ecode == 'c@#!':
				continue
			name, ename, module = self.findcodename('class', ecode)
			if not name:
				self.fp.write("# XXXX %s element %s not found!!\n"%(cname, `ecode`))
			else:
				elist.append(name, ename)
			
		self.fp.write("%s._propdict = {\n"%cname)
		for n in plist:
			self.fp.write("\t'%s' : %s,\n"%(n, n))
		self.fp.write("}\n")
		self.fp.write("%s._elemdict = {\n"%cname)
		for n, fulln in elist:
			self.fp.write("\t'%s' : %s,\n"%(n, fulln))
		self.fp.write("}\n")
	
	def compilecomparison(self, comp):
		[name, code, comment] = comp
		iname = identify(name)
		self.compcodes[code] = iname
		self.fp.write("class %s(aetools.NComparison):\n" % iname)
		self.fp.write('\t"""%s - %s"""\n' % (name, comment))
		
	def compileenumeration(self, enum):
		[code, items] = enum
		name = "_Enum_%s" % identify(code)
		self.fp.write("%s = {\n" % name)
		for item in items:
			self.compileenumerator(item)
		self.fp.write("}\n\n")
		self.enumcodes[code] = name
		return code
	
	def compileenumerator(self, item):
		[name, code, desc] = item
		self.fp.write("\t%s : %s,\t# %s\n" % (`identify(name)`, `code`, desc))
		
	def checkforenum(self, enum):
		"""This enum code is used by an event. Make sure it's available"""
		name, fullname, module = self.findcodename('enum', enum)
		if not name:
			self.fp.write("# XXXX enum %s not found!!\n"%(enum))
			return
		if module:
			self.fp.write("from %s import %s\n"%(module, name))
		
	def dumpindex(self):
		self.fp.write("\n#\n# Indices of types declared in this module\n#\n")
		self.fp.write("_classdeclarations = {\n")
		for k in self.classcodes.keys():
			self.fp.write("\t%s : %s,\n" % (`k`, self.classcodes[k]))
		self.fp.write("}\n")
		self.fp.write("\n_propdeclarations = {\n")
		for k in self.propcodes.keys():
			self.fp.write("\t%s : %s,\n" % (`k`, self.propcodes[k]))
		self.fp.write("}\n")
		self.fp.write("\n_compdeclarations = {\n")
		for k in self.compcodes.keys():
			self.fp.write("\t%s : %s,\n" % (`k`, self.compcodes[k]))
		self.fp.write("}\n")
		self.fp.write("\n_enumdeclarations = {\n")
		for k in self.enumcodes.keys():
			self.fp.write("\t%s : %s,\n" % (`k`, self.enumcodes[k]))
		self.fp.write("}\n")

def compiledata(data):
	[type, description, flags] = data
	return "%s -- %s %s" % (`type`, `description`, compiledataflags(flags))
	
def is_null(data):
	return data[0] == 'null'
	
def is_optional(data):
	return (data[2] & 0x8000)
	
def is_enum(data):
	return (data[2] & 0x2000)
	
def getdatadoc(data):
	[type, descr, flags] = data
	if descr:
		return descr
	if type == '****':
		return 'anything'
	if type == 'obj ':
		return 'an AE object reference'
	return "undocumented, typecode %s"%`type`

dataflagdict = {15: "optional", 14: "list", 13: "enum", 12: "mutable"}
def compiledataflags(flags):
	bits = []
	for i in range(16):
		if flags & (1<<i):
			if i in dataflagdict.keys():
				bits.append(dataflagdict[i])
			else:
				bits.append(`i`)
	return '[%s]' % string.join(bits)
	
# XXXX Do we have a set of python keywords somewhere?
illegal_ids = [ "for", "in", "from", "and", "or", "not", "print", "class", "return",
	"def" ]

def identify(str):
	"""Turn any string into an identifier:
	- replace space by _
	- replace other illegal chars by _xx_ (hex code)
	- prepend _ if the result is a python keyword
	"""
	if not str:
		return "_empty_ae_name"
	rv = ''
	ok = string.letters  + '_'
	ok2 = ok + string.digits
	for c in str:
		if c in ok:
			rv = rv + c
		elif c == ' ':
			rv = rv + '_'
		else:
			rv = rv + '_%02.2x_'%ord(c)
		ok = ok2
	if rv in illegal_ids:
		rv = '_' + rv
	return rv

# Call the main program

if __name__ == '__main__':
	main()
	sys.exit(1)

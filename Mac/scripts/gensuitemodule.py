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
	fss, ok = macfs.StandardGetFile()
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
			compileaete(aete, fullname)
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
	fss.SetCreatorType('PYTH', 'TEXT')
	
	fp.write('"""Suite %s: %s\n' % (name, desc))
	fp.write("Level %d, version %d\n\n" % (level, version))
	fp.write("Generated from %s\n"%fname)
	fp.write("AETE/AEUT resource version %d/%d, language %d, script %d\n" % \
		(major, minor, language, script))
	fp.write('"""\n\n')
	# XXXX Temp?
	fp.write("import addpack\n")
	fp.write("addpack.addpack('Tools')\n")
	fp.write("addpack.addpack('bgen')\n")
	fp.write("addpack.addpack('ae')\n\n")
	
	fp.write('import aetools\n')
	fp.write('import MacOS\n\n')
	fp.write("_code = %s\n\n"% `code`)
	
	enum_names = []
	for enum in enums:
		name = compileenumeration(fp, enum)
		enum_names.append(enum)
		
	compileclassheader(fp, modname)
	if events:
		for event in events:
			compileevent(fp, event)
	else:
		fp.write("\tpass\n\n")
	for cls in classes:
		compileclass(fp, cls)
	for comp in comps:
		compilecomparison(fp, comp)

def compileclassheader(fp, name):
	"""Generate class boilerplate"""
	fp.write("class %s:\n\n"%name)
	
def compileevent(fp, event):
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
	
	if has_arg:
		fp.write("\tdef %s(self, object, *arguments):\n"%funcname)
	else:
		fp.write("\tdef %s(self, *arguments):\n"%funcname)
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
	if opt_arg:
		fp.write("\t\tif len(arguments):\n")
		fp.write("\t\t\tobject = arguments[0]\n")
		fp.write("\t\t\targuments = arguments[1:]\n")
		fp.write("\t\telse:\n")
		fp.write("\t\t\tobject = None\n")
	fp.write("\t\tif len(arguments) > 1:\n")
	fp.write("\t\t\traise TypeError, 'Too many arguments'\n")
	fp.write("\t\tif arguments:\n")
	fp.write("\t\t\targuments = arguments[0]\n")
	fp.write("\t\t\tif type(arguments) <> type({}):\n")
	fp.write("\t\t\t\traise TypeError, 'Must be a mapping'\n")
	fp.write("\t\telse:\n")
	fp.write("\t\t\targuments = {}\n")
	if has_arg:
		fp.write("\t\targuments['----'] = object\n")
	elif opt_arg:
		fp.write("\t\tif object:\n")
		fp.write("\t\t\targuments['----'] = object\n")
	fp.write("\n")
	#
	# Extract attributes
	#
	fp.write("\t\tif arguments.has_key('_attributes'):\n")
	fp.write("\t\t\tattributes = arguments['_attributes']\n")
	fp.write("\t\t\tdel arguments['_attributes']\n")
	fp.write("\t\telse:\n");
	fp.write("\t\t\tattributes = {}\n")
	fp.write("\n")
	#
	# Do key substitution
	#
	if arguments:
		fp.write("\t\taetools.keysubst(arguments, self._argmap_%s)\n"%funcname)
	for a in arguments:
		if is_enum(a[2]):
			kname = a[1]
			ename = a[2][0]
			fp.write("\t\taetools.enumsubst(arguments, %s, _Enum_%s)\n" %
					(`kname`, ename))
	fp.write("\n")
	#
	# Do the transaction
	#
	fp.write("\t\treply, arguments, attributes = self.send(_code, _subcode,\n")
	fp.write("\t\t\t\targuments, attributes)\n")
	#
	# Error handling
	#
	fp.write("\t\tif arguments.has_key('errn'):\n")
	fp.write("\t\t\traise MacOS.Error, aetools.decodeerror(arguments)\n")
	fp.write("\t\t# XXXX Optionally decode result\n")
	#
	# Decode result
	#
	fp.write("\t\tif arguments.has_key('----'):\n")
	if is_enum(returns):
		fp.write("\t\t\t# XXXX Should do enum remapping here...\n")
	fp.write("\t\t\treturn arguments['----']\n")
	fp.write("\n")
	
#	print "\n#    Command %s -- %s (%s, %s)" % (`name`, `desc`, `code`, `subcode`)
#	print "#        returns", compiledata(returns)
#	print "#        accepts", compiledata(accepts)
#	for arg in arguments:
#		compileargument(arg)

def compileargument(arg):
	[name, keyword, what] = arg
	print "#        %s (%s)" % (name, `keyword`), compiledata(what)

def compileclass(fp, cls):
	[name, code, desc, properties, elements] = cls
	fp.write("\n#    Class %s (%s) -- %s\n" % (`name`, `code`, `desc`))
	for prop in properties:
		compileproperty(fp, prop)
	for elem in elements:
		compileelement(fp, elem)

def compileproperty(fp, prop):
	[name, code, what] = prop
	fp.write("#        property %s (%s) %s\n" % (`name`, `code`, compiledata(what)))

def compileelement(fp, elem):
	[code, keyform] = elem
	fp.write("#        element %s as %s\n" % (`code`, keyform))

def compilecomparison(fp, comp):
	[name, code, comment] = comp
	fp.write("#    comparison  %s (%s) -- %s\n" % (`name`, `code`, comment))

def compileenumeration(fp, enum):
	[code, items] = enum
	fp.write("_Enum_%s = {\n" % identify(code))
	for item in items:
		compileenumerator(fp, item)
	fp.write("}\n\n")
	return code

def compileenumerator(fp, item):
	[name, code, desc] = item
	fp.write("\t%s : %s,\t# %s\n" % (`identify(name)`, `code`, desc))

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
illegal_ids = [ "for", "in", "from", "and", "or", "not", "print" ]

def identify(str):
	"""Turn any string into an identifier:
	- replace space by _
	- replace other illegal chars by _xx_ (hex code)
	- prepend _ if the result is a python keyword
	"""
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

"""
gensuitemodule - Generate an AE suite module from an aete/aeut resource

Based on aete.py.

Reading and understanding this code is left as an exercise to the reader.
"""

import MacOS
import os
import string
import sys
import types
import StringIO
import macfs

from Carbon.Res import *

def main():
	fss, ok = macfs.PromptGetFile('Select file with aeut/aete resource:')
	if not ok:
		sys.exit(0)
	processfile(fss.as_pathname())

def processfile(fullname):
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
		aetelist = []
		for res in resources:
			print "decoding", res.GetResInfo(), "..."
			data = res.data
			aete = decode(data)
			aetelist.append((aete, res.GetResInfo()))
	finally:
		if rf <> cur:
			CloseResFile(rf)
			UseResFile(cur)
	# switch back (needed for dialogs in Python)
	UseResFile(cur)
	compileaetelist(aetelist, fullname)

def compileaetelist(aetelist, fullname):
	for aete, resinfo in aetelist:
		compileaete(aete, resinfo, fullname)
		
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

def compileaete(aete, resinfo, fname):
	"""Generate code for a full aete resource. fname passed for doc purposes"""
	[version, language, script, suites] = aete
	major, minor = divmod(version, 256)
	fss = macfs.FSSpec(fname)
	creatorsignature, dummy = fss.GetCreatorType()
	packagename = identify(os.path.basename(fname))
	if language:
		packagename = packagename+'_lang%d'%language
	if script:
		packagename = packagename+'_script%d'%script
	if len(packagename) > 27:
		packagename = packagename[:27]
	macfs.SetFolder(os.path.join(sys.prefix, ':Mac:Lib:lib-scriptpackages'))
	fss, ok = macfs.GetDirectory('Package folder for %s'%packagename)
	if not ok:
		return
	pathname = fss.as_pathname()
	packagename = os.path.split(os.path.normpath(pathname))[1]
	fss, ok = macfs.GetDirectory('Package folder for base suite (usually StdSuites)')
	if ok:
		dirname, basepkgname = os.path.split(os.path.normpath(fss.as_pathname()))
		if not dirname in sys.path:
			sys.path.insert(0, dirname)
		basepackage = __import__(basepkgname)
	else:
		basepackage = None
	macfs.SetFolder(pathname)
	suitelist = []
	allprecompinfo = []
	allsuites = []
	for suite in suites:
		code, suite, fss, modname, precompinfo = precompilesuite(suite, basepackage)
		if not code:
			continue
		allprecompinfo = allprecompinfo + precompinfo
		suiteinfo = suite, fss, modname
		suitelist.append((code, modname))
		allsuites.append(suiteinfo)
	for suiteinfo in allsuites:
		compilesuite(suiteinfo, major, minor, language, script, fname, basepackage, allprecompinfo)
	fss, ok = macfs.StandardPutFile('Package module', '__init__.py')
	if not ok:
		return
	fp = open(fss.as_pathname(), 'w')
	fss.SetCreatorType('Pyth', 'TEXT')
	fp.write('"""\n')
	fp.write("Package generated from %s\n"%fname)
	fp.write("Resource %s resid %d %s\n"%(resinfo[1], resinfo[0], resinfo[2]))
	fp.write('"""\n')
	fp.write('import aetools\n')
	fp.write('Error = aetools.Error\n')
	for code, modname in suitelist:
		fp.write("import %s\n" % modname)
	fp.write("\n\n_code_to_module = {\n")
	for code, modname in suitelist:
		fp.write("\t'%s' : %s,\n"%(code, modname))
	fp.write("}\n\n")
	fp.write("\n\n_code_to_fullname = {\n")
	for code, modname in suitelist:
		fp.write("\t'%s' : ('%s.%s', '%s'),\n"%(code, packagename, modname, modname))
	fp.write("}\n\n")
	for code, modname in suitelist:
		fp.write("from %s import *\n"%modname)
	if suitelist:
		fp.write("\n\nclass %s(%s_Events"%(packagename, suitelist[0][1]))
		for code, modname in suitelist[1:]:
			fp.write(",\n\t\t%s_Events"%modname)
		fp.write(",\n\t\taetools.TalkTo):\n")
		fp.write("\t_signature = '%s'\n\n"%creatorsignature)
	fp.close()
	
def precompilesuite(suite, basepackage=None):
	"""Parse a single suite without generating the output. This step is needed
	so we can resolve recursive references by suites to enums/comps/etc declared
	in other suites"""
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	
	modname = identify(name)
	if len(modname) > 28:
		modname = modname[:27]
	fss, ok = macfs.StandardPutFile('Python output file', modname+'.py')
	if not ok:
		return None, None, None, None, None

	pathname = fss.as_pathname()
	modname = os.path.splitext(os.path.split(pathname)[1])[0]
	
	if basepackage and basepackage._code_to_module.has_key(code):
		# We are an extension of a baseclass (usually an application extending
		# Standard_Suite or so). Import everything from our base module
		basemodule = basepackage._code_to_module[code]
	else:
		# We are not an extension.
		basemodule = None

	enumsneeded = {}
	for event in events:
		findenumsinevent(event, enumsneeded)

	objc = ObjectCompiler(None, basemodule)
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
	
	precompinfo = objc.getprecompinfo(modname)
	
	return code, suite, fss, modname, precompinfo

def compilesuite((suite, fss, modname), major, minor, language, script, fname, basepackage, precompinfo):
	"""Generate code for a single suite"""
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	
	pathname = fss.as_pathname()
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
	if basepackage and basepackage._code_to_module.has_key(code):
		# We are an extension of a baseclass (usually an application extending
		# Standard_Suite or so). Import everything from our base module
		fp.write('from %s import *\n'%basepackage._code_to_fullname[code][0])
		basemodule = basepackage._code_to_module[code]
	elif basepackage and basepackage._code_to_module.has_key(code.lower()):
		# This is needed by CodeWarrior and some others.
		fp.write('from %s import *\n'%basepackage._code_to_fullname[code.lower()][0])
		basemodule = basepackage._code_to_module[code.lower()]
	else:
		# We are not an extension.
		basemodule = None
	compileclassheader(fp, modname, basemodule)

	enumsneeded = {}
	if events:
		for event in events:
			compileevent(fp, event, enumsneeded)
	else:
		fp.write("\tpass\n\n")

	objc = ObjectCompiler(fp, basemodule, precompinfo)
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
	
	return code, modname

def compileclassheader(fp, name, module=None):
	"""Generate class boilerplate"""
	classname = '%s_Events'%name
	if module:
		modshortname = string.split(module.__name__, '.')[-1]
		baseclassname = '%s_Events'%modshortname
		fp.write("class %s(%s):\n\n"%(classname, baseclassname))
	else:
		fp.write("class %s:\n\n"%classname)
	
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
					(`kname`, identify(ename)))
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

def findenumsinevent(event, enumsneeded):
	"""Find all enums for a single event"""
	[name, desc, code, subcode, returns, accepts, arguments] = event
	for a in arguments:
		if is_enum(a[2]):
			ename = a[2][0]
			if ename <> '****':
				enumsneeded[ename] = 1

#
# This class stores the code<->name translations for a single module. It is used
# to keep the information while we're compiling the module, but we also keep these objects
# around so if one suite refers to, say, an enum in another suite we know where to
# find it. Finally, if we really can't find a code, the user can add modules by
# hand.
#
class CodeNameMapper:
	
	def __init__(self):
		self.code2name = {
			"property" : {},
			"class" : {},
			"enum" : {},
			"comparison" : {},
		}
		self.name2code =  {
			"property" : {},
			"class" : {},
			"enum" : {},
			"comparison" : {},
		}
		self.modulename = None
		self.star_imported = 0
		
	def addnamecode(self, type, name, code):
		self.name2code[type][name] = code
		if not self.code2name[type].has_key(code):
			self.code2name[type][code] = name
		
	def hasname(self, type, name):
		return self.name2code[type].has_key(name)
		
	def hascode(self, type, code):
		return self.code2name[type].has_key(code)
		
	def findcodename(self, type, code):
		if not self.hascode(type, code):
			return None, None, None
		name = self.code2name[type][code]
		if self.modulename and not self.star_imported:
			qualname = '%s.%s'%(self.modulename, name)
		else:
			qualname = name
		return name, qualname, self.modulename
		
	def getall(self, type):
		return self.code2name[type].items()
			
	def addmodule(self, module, name, star_imported):
		self.modulename = name
		self.star_imported = star_imported
		for code, name in module._propdeclarations.items():
			self.addnamecode('property', name, code)
		for code, name in module._classdeclarations.items():
			self.addnamecode('class', name, code)
		for code in module._enumdeclarations.keys():
			self.addnamecode('enum', '_Enum_'+identify(code), code)
		for code, name in module._compdeclarations.items():
			self.addnamecode('comparison', name, code)
		
	def prepareforexport(self, name=None):
		if not self.modulename:
			self.modulename = name
		return self
			
class ObjectCompiler:
	def __init__(self, fp, basesuite=None, othernamemappers=None):
		self.fp = fp
		self.basesuite = basesuite
		self.namemappers = [CodeNameMapper()]
		if othernamemappers:
			self.othernamemappers = othernamemappers[:]
		else:
			self.othernamemappers = []
		if basesuite:
			basemapper = CodeNameMapper()
			basemapper.addmodule(basesuite, '', 1)
			self.namemappers.append(basemapper)
		
	def getprecompinfo(self, modname):
		list = []
		for mapper in self.namemappers:
			emapper = mapper.prepareforexport(modname)
			if emapper:
				list.append(emapper)
		return list
		
	def findcodename(self, type, code):
		while 1:
			# First try: check whether we already know about this code.
			for mapper in self.namemappers:
				if mapper.hascode(type, code):
					return mapper.findcodename(type, code)
			# Second try: maybe one of the other modules knows about it.
			for mapper in self.othernamemappers:
				if mapper.hascode(type, code):
					self.othernamemappers.remove(mapper)
					self.namemappers.append(mapper)
					if self.fp:
						self.fp.write("import %s\n"%mapper.modulename)
					break
			else:
				# If all this has failed we ask the user for a guess on where it could
				# be and retry.
				if self.fp:
					m = self.askdefinitionmodule(type, code)
				else:
					m = None
				if not m: return None, None, None
				mapper = CodeNameMapper()
				mapper.addmodule(m, m.__name__, 0)
				self.namemappers.append(mapper)
	
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
		if self.namemappers[0].hascode('class', code):
			# plural forms and such
			othername, dummy, dummy = self.namemappers[0].findcodename('class', code)
			if self.fp:
				self.fp.write("\n%s = %s\n"%(pname, othername))
		else:
			if self.fp:
				self.fp.write('\nclass %s(aetools.ComponentItem):\n' % pname)
				self.fp.write('\t"""%s - %s """\n' % (name, desc))
				self.fp.write('\twant = %s\n' % `code`)
		self.namemappers[0].addnamecode('class', pname, code)
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
		if self.namemappers[0].hascode('property', code):
			# plural forms and such
			othername, dummy, dummy = self.namemappers[0].findcodename('property', code)
			if pname == othername:
				return
			if self.fp:
				self.fp.write("\n%s = %s\n"%(pname, othername))
		else:
			if self.fp:
				self.fp.write("class %s(aetools.NProperty):\n" % pname)
				self.fp.write('\t"""%s - %s """\n' % (name, what[1]))
				self.fp.write("\twhich = %s\n" % `code`)
				self.fp.write("\twant = %s\n" % `what[0]`)
		self.namemappers[0].addnamecode('property', pname, code)
	
	def compileelement(self, elem):
		[code, keyform] = elem
		if self.fp:
			self.fp.write("#        element %s as %s\n" % (`code`, keyform))

	def fillclasspropsandelems(self, cls):
		[name, code, desc, properties, elements] = cls
		cname = identify(name)
		if self.namemappers[0].hascode('class', code) and \
				self.namemappers[0].findcodename('class', code)[0] != cname:
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
				if self.fp:
					self.fp.write("# XXXX %s element %s not found!!\n"%(cname, `ecode`))
			else:
				elist.append((name, ename))
		
		if self.fp:
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
		self.namemappers[0].addnamecode('comparison', iname, code)
		if self.fp:
			self.fp.write("class %s(aetools.NComparison):\n" % iname)
			self.fp.write('\t"""%s - %s """\n' % (name, comment))
		
	def compileenumeration(self, enum):
		[code, items] = enum
		name = "_Enum_%s" % identify(code)
		if self.fp:
			self.fp.write("%s = {\n" % name)
			for item in items:
				self.compileenumerator(item)
			self.fp.write("}\n\n")
		self.namemappers[0].addnamecode('enum', name, code)
		return code
	
	def compileenumerator(self, item):
		[name, code, desc] = item
		self.fp.write("\t%s : %s,\t# %s\n" % (`identify(name)`, `code`, desc))
		
	def checkforenum(self, enum):
		"""This enum code is used by an event. Make sure it's available"""
		name, fullname, module = self.findcodename('enum', enum)
		if not name:
			if self.fp:
				self.fp.write("_Enum_%s = None # XXXX enum %s not found!!\n"%(identify(enum), enum))
			return
		if module:
			if self.fp:
				self.fp.write("from %s import %s\n"%(module, name))
		
	def dumpindex(self):
		if not self.fp:
			return
		self.fp.write("\n#\n# Indices of types declared in this module\n#\n")
		self.fp.write("_classdeclarations = {\n")
		for k, v in self.namemappers[0].getall('class'):
			self.fp.write("\t%s : %s,\n" % (`k`, v))
		self.fp.write("}\n")
		self.fp.write("\n_propdeclarations = {\n")
		for k, v in self.namemappers[0].getall('property'):
			self.fp.write("\t%s : %s,\n" % (`k`, v))
		self.fp.write("}\n")
		self.fp.write("\n_compdeclarations = {\n")
		for k, v in self.namemappers[0].getall('comparison'):
			self.fp.write("\t%s : %s,\n" % (`k`, v))
		self.fp.write("}\n")
		self.fp.write("\n_enumdeclarations = {\n")
		for k, v in self.namemappers[0].getall('enum'):
			self.fp.write("\t%s : %s,\n" % (`k`, v))
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
	ok = string.ascii_letters + '_'
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

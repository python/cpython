"""
Produces a human readable file of an application's aete

v.02 january 29, 1998 bug fix Main()
v.03 january 31, 1998 added support for inheriting suites from aeut
v.04 april 16, 1998 Changed identify to match getaete
"""

import aetools
import sys
import MacOS
import StringIO
import types
import macfs
import string
import macpath
from Res import *

# for testing only
app = 'MACS'#CSOm'#'nwSP'#'ezVu'#

#Dialect file hard coded as a tempoary measure
DIALECT = 'Hermit:System Folder:Scripting Additions:Dialects:English Dialect'

#Restrict the application suites to the dialect we want to use.
LANG = 0 # 0 = English, 1 = French, 11 = Japanese

#The following are neaded to open the application aete
kASAppleScriptSuite	= 'ascr'
kGetAETE			= 'gdte'
attributes = {}
arguments = {}

class AETE(aetools.TalkTo):
	pass
	
def Main(appl):
	fss, ok = macfs.PromptGetFile('Application to work on', 'FNDR', 'APPL')#
	if not ok:
		return
	app = fss.GetCreatorType()[0]
	path = macpath.split(sys.argv[0])[0]
	appname =  macpath.split(fss.as_pathname())[1]
	appname = appname + '.aete'
	appname = macpath.join(path, appname)
	try:
		data = Getaete(app)
	except MacOS.Error, msg:
		if msg[0] == -609:
			_launch(app)
			data = Getaete(app)
#	print data
	data = decode(data['----'].data)
	data = compileaete(data, appname)
	
	
def decode(data):
	"""Decode an aete into a python data structure"""
	f = StringIO.StringIO(data)
	aete = generic(getaete, f)
	aete = simplify(aete)
	return aete

def simplify(item):
	"""Recursively replace singleton tuples by their constituent item"""
	if type(item) is types.ListType:
		return map(simplify, item)
	elif type(item) == types.TupleType and len(item) == 2:
		return simplify(item[1])
	else:
		return item


## Here follows the aete resource decoder.
## It is presented bottom-up instead of top-down because there are  direct
## references to the lower-level part-decoders from the high-level part-decoders.
#
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
#			print thing
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

def compileaete(aete, appname):
	"""Generate dictionary file for a full aete resource."""
	[version, language, script, suites] = aete
	major, minor = divmod(version, 256)
	fp = open(appname, 'w')
	
	fp.write('%s:\n' % (appname))
	fp.write("AETE resource version %d/%d, language %d, script %d\n" % \
		(major, minor, language, script))
	fp.write('\n\n')
	gsuites = openaeut()
	for suite in suites:
		if language == LANG:
			suitecode = suite[2]
			if suite[5] == []:
				for gsuite in gsuites:
					if suitecode == gsuite[2]:
						suite = gsuite
			[name, desc, code, level, version, events, classes, comps, enums] = suite
			fp.write('\n%s Suite: %s\n' % (name, desc))
			fp.write('\n\tEvents:\n')
			for event in events:
				fp.write('\n\t%s: %s\n' % (identify(event[0]), event[1]))
				fp.write('\t\t%s: %s -- %s\n' % (identify(event[0]), event[5][1], event[5][0]))
				fp.write('\t\tResult: %s -- %s\n' % (event[4][1], event[4][0]))
				for ev in event[6]:
					fp.write('\t\t\t%s: %s -- %s\n' % (identify(ev[0]), ev[2][0], ev[2][1]))
			fp.write('\n\tClasses')
			for klass in classes:
				fp.write('\n\t%s: %s\n' % (identify(klass[0]), klass[2]))
				if klass[3]:
					if not klass[3][0][0]: continue
					fp.write('\t\tProperties\n')
					for cl in klass[3]:
						fp.write('\t\t\t%s: %s -- %s\n' % (identify(cl[0]), cl[2][1], cl[2][0]))#,, cl[3][3][1]))
				if klass[4]:
					fp.write('\n\t\t\tElements\n')
					for cl in klass[4]:
						fp.write('\t\t\t\t%s: %s\n' % (identify(cl[0]), cl[1]))
						
				
					
		
	

illegal_ids = [ "for", "in", "from", "and", "or", "not", "print", "class", "return",
	"def", "name" ]

def identify(str):
	"""Turn any string into an identifier:
	- replace space by _
	- prepend _ if the result is a python keyword
	"""

	rv = string.replace(str, ' ', '_')
	rv = string.replace(rv, '-', '')
	rv = string.replace(rv, ',', '')
	rv = string.capitalize(rv)
	return rv


def Getaete(app):
	'''Read the target aete'''
	arguments['----'] = LANG
	_aete = AETE(app)
	_reply, _arguments, _attributes = _aete.send('ascr', 'gdte', arguments, attributes)
	if _arguments.has_key('errn'):
		raise aetools.Error, aetools.decodeerror(_arguments)
	return  _arguments

def openaeut():
	"""Open and read a aeut file.
	XXXXX This has been temporarily hard coded until a Python aeut is written XXXX"""

	fullname = DIALECT
	
	rf = OpenRFPerm(fullname, 0, 1)
	try:
		UseResFile(rf)
		resources = []
		for i in range(Count1Resources('aeut')):
			res = Get1IndResource('aeut', 1+i)
			resources.append(res)
		for res in resources:
			data = res.data
			data = decode(data)[3]
	finally:
		CloseResFile(rf)
	return data


#The following should be replaced by direct access to a python 'aeut'

class _miniFinder(aetools.TalkTo):
	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open the specified object(s)
		Required argument: list of objects to open
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'odoc'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']
	
_finder = _miniFinder('MACS')

def _launch(appfile):
	"""Open a file thru the finder. Specify file by name or fsspec"""
	_finder.open(_application_file(('ID  ', appfile)))


class _application_file(aetools.ComponentItem):
	"""application file - An application's file on disk"""
	want = 'appf'
	
_application_file._propdict = {
}
_application_file._elemdict = {
}

Main(app)
sys.exit(1)

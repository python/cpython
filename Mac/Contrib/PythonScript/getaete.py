"""
Produces a 3 dictionaries from application aete's 
to be read by PythonScript

v.02 january 31, 1998 added support for inheriting suites from aeut
v.03 february 16, 1998 changes to identify
v.04 february 26, 1998 simplified decode
v.05 23/04/98 simplified _launch

"""
import baetools
import macpath
import sys
import os
import MacOS
import StringIO
import types
from MACFS import *
import macfs
import string
from Carbon.Res import *
import struct

# for testing only
app ='CSOm' #'ezVu'# 'nwSP'#MACS'#

#Restrict the application suites to the dialect we want to use.
LANG = 0 # 0 = English, 1 = French, 11 = Japanese
lang = {0:'English', 1:'French', 11:'Japanese'}

#The following are neaded to open the application aete
kASAppleScriptSuite	= 'ascr'
kGetAETE			= 'gdte'
attributes = {}
arguments = {}

class AETE(baetools.TalkTo):
	pass
	
def Getaete(app):
	try:
		data = openaete(app)
	except MacOS.Error, msg:
		if msg[0] == -609:
			_launch(app)
			data = openaete(app)
	data = decode(data['----'].data)
	data = compileaete(data)
	return data
	
	
def decode(data):
	"""Decode an aete into a python data structure"""
	f = StringIO.StringIO(data)
	aete = generic(getaete, f)
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
def getflag(f, *args):
	m = ''
	c = f.read(2)
	print `c`
	if not c:
		raise EOFError, 'in getflag' + str(args)
	for n in c:
		m = m + `ord(n)`

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
			record.append(item)
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
	(getbyte, "major version in BCD"),
	(getbyte, "minor version in BCD"),
	(getword, "language code"),
	(getword, "script code"),
	(getlist, "suites", getsuite)
	]

def compileaete(aete):
	"""Generate dictionary for a full aete resource."""
	[major, minor, language, script, suites] = aete
	suitedict = {}
	gsuites = openaeut()
	for gsuite in gsuites:
		if gsuite[0] == 'AppleScript Suite':
			suite = gsuite
			suite = compilesuite(suite)
			suitedict[identify(suite[0])] = suite[1:]
	for suite in suites:
		if language == LANG:
			suitecode = suite[2]
			if suite[5] == []:
				for gsuite in gsuites:
					if suitecode == gsuite[2]:
						suite = gsuite
			suite = compilesuite(suite)
			suitedict[identify(suite[0])] = suite[1:]
	suitedict = combinesuite(suitedict)
	return suitedict
			
def compilesuite(suite):
	"""Generate dictionary for a single suite"""
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	eventdict ={}
	classdict = {}
	enumdict ={}
	for event in events:
		if event[6]:
			for ev in event[6]:
				ev[0] = identify(ev[:2])
		eventdict[identify(event[:2])] = event[1:]
	for klass in classes:
		if klass[3]:
			for kl in klass[3]:
				kl[0] = identify(kl[:2])
		classdict[identify(klass[:2])] = klass[1:]
	for enum in enums:
		enumdict[enum[0]] = enum[1]
	return name, eventdict, classdict, enumdict
		
def combinesuite(suite):
	"""Combines suite dictionaries to seperate event, class, enumeration dictionaries
	"""
	
	suitelist = []
	eventDict ={}
	classDict ={}
	enumDict ={}
	for value in suite.values():
		for key in value[0].keys():
			val = value[0][key]
			eventDict[key] = val
		for key in value[1].keys():
			val = value[1][key]
			if key in classDict.keys():
				nval = classDict[key][2]
				val[2] = val[2] + nval
			classDict[key] = val
		for key in value[2].keys():
			val = value[2][key]
			enumDict[key] = val
	return 	eventDict, classDict, enumDict
		

illegal_ids = [ "for", "in", "from", "and", "or", "not", "print", "class", "return",
	"def", "name", 'data' ]

def identify(str):
	"""Turn any string into an identifier:
	- replace space by _
	- remove ',' and '-'
	capitalise
	"""
	if not str[0]:
		if str[1] == 'c@#!':
			return "Every"
		else:
			return 'Any'
	rv = string.replace(str[0], ' ', '_')
	rv = string.replace(rv, '-', '')
	rv = string.replace(rv, ',', '')
	rv = string.capitalize(rv)
	return rv


def openaete(app):
	"""open and read the aete of the target application"""
	arguments['----'] = LANG
	_aete = AETE(app)
	_reply, _arguments, _attributes = _aete.send(kASAppleScriptSuite, kGetAETE, arguments, attributes)
	if _arguments.has_key('errn'):
		raise baetools.Error, baetools.decodeerror(_arguments)
	return  _arguments

def openaeut():
	"""Open and read a aeut file.
	XXXXX This has been temporarily hard coded until a Python aeut is written XXXX"""

	fullname = dialect
	rf = OpenRFPerm(fullname, 0, 1)
	try:
		UseResFile(rf)
		resources = []
		for i in range(Count1Resources('aeut')):
			res = Get1IndResource('aeut', 1+i)
			resources.append(res)
		for res in resources:
			data = res.data
			data = decode(data)[4]
	finally:
		CloseResFile(rf)
	return data
	
def dialect():
	"""find the correct Dialect file"""

	dialect = lang[LANG] + " Dialect"
	try:
		##System 8
		vRefNum, dirID = macfs.FindFolder(kOnSystemDisk, kScriptingAdditionsFolderType, 0)
		fss = macfs.FSSpec((vRefNum, dirID, ''))
		fss = fss.as_pathname()
	except macfs.error:
		##Sytem 7
		vRefNum, dirID = macfs.FindFolder(kOnSystemDisk, kExtensionFolderType, 0)
		fss = macfs.FSSpec((vRefNum, dirID, ''))
		fss = fss.as_pathname()
		fss = macpath.join(fss, "Scripting Additions")
	fss = macpath.join(fss, "Dialect")
	fss = macpath.join(fss, dialect)
	return fss
	
	
#def openosax():
#	"""Open and read the aetes of osaxen in the scripting additions folder"""
#
#	# System 7.x
#	aete = []
#	vRefNum, dirID = macfs.FindFolder(kOnSystemDisk, kExtensionFolderType, 0)
#	fss = macfs.FSSpec((vRefNum, dirID, ''))
#	fss = fss.as_pathname()
#	osax = macpath.join(fss, "Scripting Additions")
#	for file in os.listdir(osax):
#		fullname = macpath.join(osax, file)
#		print fullname
#		rf = OpenRFPerm(fullname, 0, 1)
#		try:
#			UseResFile(rf)
#			resources = []
#			for i in range(Count1Resources('aete')):
#				res = Get1IndResource('aete', 1+i)
#				resources.append(res)
#			for res in resources:
#				data = res.data
#				data = decode(data)[4]
#		finally:
#			CloseResFile(rf)
#		aete.append(data)
#	print data


#The following should be replaced by direct access to a python 'aeut'

def _launch(appfile):
	"""Open a file thru the finder. Specify file by name or fsspec"""

#	from PythonScript import PyScript
	import baetypes
	_finder = AETE('MACS')
	parameters ={}
	parameters['----'] = eval("baetypes.ObjectSpecifier('%s', '%s', %s)" % ('appf', 'ID  ', `appfile`))
	_reply, _arguments, _attributes = _finder.send( 'aevt', 'odoc', parameters , attributes = {})
	if _arguments.has_key('errn'):
		raise baetools.Error, baetools.decodeerror(_arguments)
	# XXXX Optionally decode result
	if _arguments.has_key('----'):
		return _arguments['----']



if __name__ == '__main__':
#	import profile
#	profile.run('Getaete(app)', 'Getaeteprof')
	Getaete(app)
#	openosax()
#	openaete('ascr')
#	sys.exit(1)

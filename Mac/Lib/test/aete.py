# Look for scriptable applications -- that is, applications with an 'aete' resource
# Also contains (partially) reverse engineered 'aete' resource decoding

import MacOS
import os
import string
import sys
import types
import StringIO

from Res import *

def main():
	filename = raw_input("Listing file? (default stdout): ")
	redirect(filename, realmain)

def redirect(filename, func, *args):
	f = filename and open(filename, 'w')
	save_stdout = sys.stdout
	try:
		if f: sys.stdout = f
		return apply(func, args)
	finally:
		sys.stdout = save_stdout
		if f: f.close()

def realmain():
	#list('C:System Folder:Extensions:AppleScript\252')
	#list('C:Tao AppleScript:Finder Liaison:Finder Liaison 1.0')
	#list('C:Tao AppleScript:Scriptable Text Editor')
	#list('C:Internet:Eudora 1.4.2:Eudora1.4.2')
	#list('E:Excel 4.0:Microsoft Excel')
	#list('C:Internet:Netscape 1.0N:Netscape 1.0N')
	#find('C:')
	find('D:')
	find('E:')
	find('F:')

def find(dir, maxlevel = 5):
	hits = []
	cur = CurResFile()
	names = os.listdir(dir)
	tuples = map(lambda x: (os.path.normcase(x), x), names)
	tuples.sort()
	names = map(lambda (x, y): y, tuples)
	for name in names:
		if name in (os.curdir, os.pardir): continue
		fullname = os.path.join(dir, name)
		if os.path.islink(fullname):
			pass
		if os.path.isdir(fullname):
			if maxlevel > 0:
				sys.stderr.write("        %s\n" % `fullname`)
				hits = hits + find(fullname, maxlevel-1)
		else:
			ctor, type = MacOS.GetCreatorAndType(fullname)
			if type in ('APPL', 'FNDR', 'zsys', 'INIT', 'scri', 'cdev'):
				sys.stderr.write("    %s\n" % `fullname`)
				try:
					rf = OpenRFPerm(fullname, 0, '\1')
				except MacOS.Error, msg:
					print "Error:", fullname, msg
					continue
				UseResFile(rf)
				n = Count1Resources('aete')
				if rf <> cur:
					CloseResFile(rf)
					UseResFile(cur)
				if n > 1:
					hits.append(fullname)
					sys.stderr.write("YES!  %d in %s\n" % (n, `fullname`))
					list(fullname)
	return hits

def list(fullname):
	cur = CurResFile()
	rf = OpenRFPerm(fullname, 0, '\1')
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
			try:
				aete = decode(data)
				showaete(aete)
				print "Checking putaete..."
				f = StringIO.StringIO()
				putaete(f, aete)
				newdata = f.getvalue()
				if len(newdata) == len(data):
					if newdata == data:
						print "putaete created identical data"
					else:
						newaete = decode(newdata)
						if newaete == aete:
							print "putaete created equivalent data"
						else:
							print "putaete failed the test:"
							showaete(newaete)
				else:
					print "putaete created different data:"
					print `newdata`
			except:
				import traceback
				traceback.print_exc()
			sys.stdout.flush()
	finally:
		if rf <> cur:
			CloseResFile(rf)
			UseResFile(cur)

def decode(data):
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


# Display 'aete' resources in a friendly manner.
# This one's done top-down again...

def showaete(aete):
	[version, language, script, suites] = aete
	major, minor = divmod(version, 256)
	print "\nVersion %d/%d, language %d, script %d" % \
		(major, minor, language, script)
	for suite in suites:
		showsuite(suite)

def showsuite(suite):
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	print "\nSuite %s -- %s (%s)" % (`name`, `desc`, `code`)
	print "Level %d, version %d" % (level, version)
	for event in events:
		showevent(event)
	for cls in classes:
		showclass(cls)
	for comp in comps:
		showcomparison(comp)
	for enum in enums:
		showenumeration(enum)

def showevent(event):
	[name, desc, code, subcode, returns, accepts, arguments] = event
	print "\n    Command %s -- %s (%s, %s)" % (`name`, `desc`, `code`, `subcode`)
	print "        returns", showdata(returns)
	print "        accepts", showdata(accepts)
	for arg in arguments:
		showargument(arg)

def showargument(arg):
	[name, keyword, what] = arg
	print "        %s (%s)" % (name, `keyword`), showdata(what)

def showclass(cls):
	[name, code, desc, properties, elements] = cls
	print "\n    Class %s (%s) -- %s" % (`name`, `code`, `desc`)
	for prop in properties:
		showproperty(prop)
	for elem in elements:
		showelement(elem)

def showproperty(prop):
	[name, code, what] = prop
	print "        property %s (%s)" % (`name`, `code`), showdata(what)

def showelement(elem):
	[code, keyform] = elem
	print "        element %s" % `code`, "as", keyform

def showcomparison(comp):
	[name, code, comment] = comp
	print "    comparison  %s (%s) -- %s" % (`name`, `code`, comment)

def showenumeration(enum):
	[code, items] = enum
	print "\n    Enum %s" % `code`
	for item in items:
		showenumerator(item)

def showenumerator(item):
	[name, code, desc] = item
	print "        %s (%s) -- %s" % (`name`, `code`, `desc`)

def showdata(data):
	[type, description, flags] = data
	return "%s -- %s %s" % (`type`, `description`, showdataflags(flags))

dataflagdict = {15: "optional", 14: "list", 13: "enum", 12: "mutable"}
def showdataflags(flags):
	bits = []
	for i in range(16):
		if flags & (1<<i):
			if i in dataflagdict.keys():
				bits.append(dataflagdict[i])
			else:
				bits.append(`i`)
	return '[%s]' % string.join(bits)


# Write an 'aete' resource.
# Closedly modelled after showaete()...

def putaete(f, aete):
	[version, language, script, suites] = aete
	putword(f, version)
	putword(f, language)
	putword(f, script)
	putlist(f, suites, putsuite)

def putsuite(f, suite):
	[name, desc, code, level, version, events, classes, comps, enums] = suite
	putpstr(f, name)
	putpstr(f, desc)
	putostype(f, code)
	putword(f, level)
	putword(f, version)
	putlist(f, events, putevent)
	putlist(f, classes, putclass)
	putlist(f, comps, putcomparison)
	putlist(f, enums, putenumeration)

def putevent(f, event):
	[name, desc, eventclass, eventid, returns, accepts, arguments] = event
	putpstr(f, name)
	putpstr(f, desc)
	putostype(f, eventclass)
	putostype(f, eventid)
	putdata(f, returns)
	putdata(f, accepts)
	putlist(f, arguments, putargument)

def putargument(f, arg):
	[name, keyword, what] = arg
	putpstr(f, name)
	putostype(f, keyword)
	putdata(f, what)

def putclass(f, cls):
	[name, code, desc, properties, elements] = cls
	putpstr(f, name)
	putostype(f, code)
	putpstr(f, desc)
	putlist(f, properties, putproperty)
	putlist(f, elements, putelement)

putproperty = putargument

def putelement(f, elem):
	[code, parts] = elem
	putostype(f, code)
	putlist(f, parts, putostype)

def putcomparison(f, comp):
	[name, id, comment] = comp
	putpstr(f, name)
	putostype(f, id)
	putpstr(f, comment)

def putenumeration(f, enum):
	[code, items] = enum
	putostype(f, code)
	putlist(f, items, putenumerator)

def putenumerator(f, item):
	[name, code, desc] = item
	putpstr(f, name)
	putostype(f, code)
	putpstr(f, desc)

def putdata(f, data):
	[type, description, flags] = data
	putostype(f, type)
	putpstr(f, description)
	putword(f, flags)

def putlist(f, list, putitem):
	putword(f, len(list))
	for item in list:
		putitem(f, item)
		putalign(f)

def putalign(f):
	if f.tell() & 1:
		f.write('\0')

def putbyte(f, value):
	f.write(chr(value))

def putword(f, value):
	putalign(f)
	f.write(chr((value>>8)&0xff))
	f.write(chr(value&0xff))

def putostype(f, value):
	putalign(f)
	if type(value) != types.StringType or len(value) != 4:
		raise TypeError, "ostype must be 4-char string"
	f.write(value)

def putpstr(f, value):
	if type(value) != types.StringType or len(value) > 255:
		raise TypeError, "pstr must be string <= 255 chars"
	f.write(chr(len(value)) + value)


# Call the main program

if __name__ == '__main__':
	main()
else:
	realmain()

# persist.py
#
# Implement limited persistence.
#
# Simple interface:
#	persist.save()		save __main__ module on file (overwrite)
#	persist.load()		load __main__ module from file (merge)
#
# These use the filename persist.defaultfile, initialized to 'wsrestore.py'.
#
# A raw interface also exists:
#	persist.writedict(dict, fp)	save dictionary to open file
#	persist.readdict(dict, fp)	read (merge) dictionary from open file
#
# Internally, the function dump() and a whole bunch of support of functions
# traverse a graph of objects and print them in a restorable form
# (which happens to be a Python module).
#
# XXX Limitations:
# - Volatile objects are dumped as strings:
#   - open files, windows etc.
# - Other 'obscure' objects are dumped as strings:
#   - classes, instances and methods
#   - compiled regular expressions
#   - anything else reasonably obscure (e.g., capabilities)
#   - type objects for obscure objects
# - It's slow when there are many of lists or dictionaries
#   (This could be fixed if there were a quick way to compute a hash
#   function of any object, even if recursive)

defaultfile = 'wsrestore.py'

def save():
	import __main__
	import os
	# XXX On SYSV, if len(defaultfile) >= 14, this is wrong!
	backup = defaultfile + '~'
	try:
		os.unlink(backup)
	except os.error:
		pass
	try:
		os.rename(defaultfile, backup)
	except os.error:
		pass
	fp = open(defaultfile, 'w')
	writedict(__main__.__dict__, fp)
	fp.close()

def load():
	import __main__
	fp = open(defaultfile, 'r')
	readdict(__main__.__dict__, fp)

def writedict(dict, fp):
	import sys
	savestdout = sys.stdout
	try:
		sys.stdout = fp
		dump(dict)	# Writes to sys.stdout
	finally:
		sys.stdout = savestdout

def readdict(dict, fp):
	contents = fp.read()
	globals = {}
	exec(contents, globals)
	top = globals['top']
	for key in top.keys():
		if dict.has_key(key):
			print 'warning:', key, 'not overwritten'
		else:
			dict[key] = top[key]


# Function dump(x) prints (on sys.stdout!) a sequence of Python statements
# that, when executed in an empty environment, will reconstruct the
# contents of an arbitrary dictionary.

import sys

# Name used for objects dict on output.
#
FUNNYNAME = FN = 'A'

# Top-level function.  Call with the object you want to dump.
#
def dump(x):
	types = {}
	stack = []			# Used by test for recursive objects
	print FN, '= {}'
	topuid = dumpobject(x, types, stack)
	print 'top =', FN, '[', `topuid`, ']'

# Generic function to dump any object.
#
dumpswitch = {}
#
def dumpobject(x, types, stack):
	typerepr = `type(x)`
	if not types.has_key(typerepr):
		types[typerepr] = {}
	typedict = types[typerepr]
	if dumpswitch.has_key(typerepr):
		return dumpswitch[typerepr](x, typedict, types, stack)
	else:
		return dumpbadvalue(x, typedict, types, stack)

# Generic function to dump unknown values.
# This assumes that the Python interpreter prints such values as
# <foo object at xxxxxxxx>.
# The object will be read back as a string: '<foo object at xxxxxxxx>'.
# In some cases it may be possible to fix the dump manually;
# to ease the editing, these cases are labeled with an XXX comment.
#
def dumpbadvalue(x, typedict, types, stack):
	xrepr = `x`
	if typedict.has_key(xrepr):
		return typedict[xrepr]
	uid = genuid()
	typedict[xrepr] = uid
	print FN, '[', `uid`, '] =', `xrepr`, '# XXX'
	return uid

# Generic function to dump pure, simple values, except strings
#
def dumpvalue(x, typedict, types, stack):
	xrepr = `x`
	if typedict.has_key(xrepr):
		return typedict[xrepr]
	uid = genuid()
	typedict[xrepr] = uid
	print FN, '[', `uid`, '] =', `x`
	return uid

# Functions to dump string objects
#
def dumpstring(x, typedict, types, stack):
	# XXX This can break if strings have embedded '\0' bytes
	# XXX because of a bug in the dictionary module
	if typedict.has_key(x):
		return typedict[x]
	uid = genuid()
	typedict[x] = uid
	print FN, '[', `uid`, '] =', `x`
	return uid

# Function to dump type objects
#
typeswitch = {}
class some_class:
	def method(self): pass
some_instance = some_class()
#
def dumptype(x, typedict, types, stack):
	xrepr = `x`
	if typedict.has_key(xrepr):
		return typedict[xrepr]
	uid = genuid()
	typedict[xrepr] = uid
	if typeswitch.has_key(xrepr):
		print FN, '[', `uid`, '] =', typeswitch[xrepr]
	elif x == type(sys):
		print 'import sys'
		print FN, '[', `uid`, '] = type(sys)'
	elif x == type(sys.stderr):
		print 'import sys'
		print FN, '[', `uid`, '] = type(sys.stderr)'
	elif x == type(dumptype):
		print 'def some_function(): pass'
		print FN, '[', `uid`, '] = type(some_function)'
	elif x == type(some_class):
		print 'class some_class(): pass'
		print FN, '[', `uid`, '] = type(some_class)'
	elif x == type(some_instance):
		print 'class another_class(): pass'
		print 'some_instance = another_class()'
		print FN, '[', `uid`, '] = type(some_instance)'
	elif x == type(some_instance.method):
		print 'class yet_another_class():'
		print '    def method(): pass'
		print 'another_instance = yet_another_class()'
		print FN, '[', `uid`, '] = type(another_instance.method)'
	else:
		# Unknown type
		print FN, '[', `uid`, '] =', `xrepr`, '# XXX'
	return uid

# Initialize the typeswitch
#
for x in None, 0, 0.0, '', (), [], {}:
	typeswitch[`type(x)`] = 'type(' + `x` + ')'
for s in 'type(0)', 'abs', '[].append':
	typeswitch[`type(eval(s))`] = 'type(' + s + ')'

# Dump a tuple object
#
def dumptuple(x, typedict, types, stack):
	item_uids = []
	xrepr = ''
	for item in x:
		item_uid = dumpobject(item, types, stack)
		item_uids.append(item_uid)
		xrepr = xrepr + ' ' + item_uid
	del stack[-1:]
	if typedict.has_key(xrepr):
		return typedict[xrepr]
	uid = genuid()
	typedict[xrepr] = uid
	print FN, '[', `uid`, '] = (',
	for item_uid in item_uids:
		print FN, '[', `item_uid`, '],',
	print ')'
	return uid

# Dump a list object
#
def dumplist(x, typedict, types, stack):
	# Check for recursion
	for x1, uid1 in stack:
		if x is x1: return uid1
	# Check for occurrence elsewhere in the typedict
	for uid1 in typedict.keys():
		if x is typedict[uid1]: return uid1
	# This uses typedict differently!
	uid = genuid()
	typedict[uid] = x
	print FN, '[', `uid`, '] = []'
	stack.append(x, uid)
	item_uids = []
	for item in x:
		item_uid = dumpobject(item, types, stack)
		item_uids.append(item_uid)
	del stack[-1:]
	for item_uid in item_uids:
		print FN, '[', `uid`, '].append(', FN, '[', `item_uid`, '])'
	return uid

# Dump a dictionary object
#
def dumpdict(x, typedict, types, stack):
	# Check for recursion
	for x1, uid1 in stack:
		if x is x1: return uid1
	# Check for occurrence elsewhere in the typedict
	for uid1 in typedict.keys():
		if x is typedict[uid1]: return uid1
	# This uses typedict differently!
	uid = genuid()
	typedict[uid] = x
	print FN, '[', `uid`, '] = {}'
	stack.append(x, uid)
	item_uids = []
	for key in x.keys():
		val_uid = dumpobject(x[key], types, stack)
		item_uids.append(key, val_uid)
	del stack[-1:]
	for key, val_uid in item_uids:
		print FN, '[', `uid`, '][', `key`, '] =',
		print FN, '[', `val_uid`, ']'
	return uid

# Dump a module object
#
def dumpmodule(x, typedict, types, stack):
	xrepr = `x`
	if typedict.has_key(xrepr):
		return typedict[xrepr]
	from string import split
	# `x` has the form <module 'foo'>
	name = xrepr[9:-2]
	uid = genuid()
	typedict[xrepr] = uid
	print 'import', name
	print FN, '[', `uid`, '] =', name
	return uid


# Initialize dumpswitch, a table of functions to dump various objects,
# indexed by `type(x)`.
#
for x in None, 0, 0.0:
	dumpswitch[`type(x)`] = dumpvalue
for x, f in ('', dumpstring), (type(0), dumptype), ((), dumptuple), \
		([], dumplist), ({}, dumpdict), (sys, dumpmodule):
	dumpswitch[`type(x)`] = f


# Generate the next unique id; a string consisting of digits.
# The seed is stored as seed[0].
#
seed = [0]
#
def genuid():
	x = seed[0]
	seed[0] = seed[0] + 1
	return `x`

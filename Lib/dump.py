# Module 'dump'
#
# Print python code that reconstructs a variable.
# This only works in certain cases.
#
# It works fine for:
# - ints and floats (except NaNs and other weird things)
# - strings
# - compounds and lists, provided it works for all their elements
# - imported modules, provided their name is the module name
#
# It works for top-level dictionaries but not for dictionaries
# contained in other objects (could be made to work with some hassle
# though).
#
# It does not work for functions (all sorts), classes, class objects,
# windows, files etc.
#
# Finally, objects referenced by more than one name or contained in more
# than one other object lose their sharing property (this is bad for
# strings used as exception identifiers, for instance).

# Dump a whole symbol table
#
def dumpsymtab(dict):
	for key in dict.keys():
		dumpvar(key, dict[key])

# Dump a single variable
#
def dumpvar(name, x):
	import sys
	t = type(x)
	if t == type({}):
		print name, '= {}'
		for key in x.keys():
			item = x[key]
			if not printable(item):
				print '#',
			print name, '[', `key`, '] =', `item`
	elif t in (type(''), type(0), type(0.0), type([]), type(())):
		if not printable(x):
			print '#',
		print name, '=', `x`
	elif t == type(sys):
		print 'import', name, '#', x
	else:
		print '#', name, '=', x

# check if a value is printable in a way that can be read back with input()
#
def printable(x):
	t = type(x)
	if t in (type(''), type(0), type(0.0)):
		return 1
	if t in (type([]), type(())):
		for item in x:
			if not printable(item):
				return 0
		return 1
	if x == {}:
		return 1
	return 0

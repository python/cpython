'''Parse a Python file and retrieve classes and methods.

Parse enough of a Python file to recognize class and method
definitions and to find out the superclasses of a class.

The interface consists of a single function:
	readmodule(module, path)
module is the name of a Python module, path is an optional list of
directories where the module is to be searched.  If present, path is
prepended to the system search path sys.path.
The return value is a dictionary.  The keys of the dictionary are
the names of the classes defined in the module (including classes
that are defined via the from XXX import YYY construct).  The values
are class instances of the class Class defined here.

A class is described by the class Class in this module.  Instances
of this class have the following instance variables:
	name -- the name of the class
	super -- a list of super classes (Class instances)
	methods -- a dictionary of methods
	file -- the file in which the class was defined
	lineno -- the line in the file on which the class statement occurred
The dictionary of methods uses the method names as keys and the line
numbers on which the method was defined as values.
If the name of a super class is not recognized, the corresponding
entry in the list of super classes is not a class instance but a
string giving the name of the super class.  Since import statements
are recognized and imported modules are scanned as well, this
shouldn't happen often.

BUGS
Continuation lines are not dealt with at all and strings may confuse
the hell out of the parser, but it usually works.'''

import os
import sys
import imp
import re
import string

id = '(?P<id>[A-Za-z_][A-Za-z0-9_]*)'	# match identifier
blank_line = re.compile('^[ \t]*($|#)')
is_class = re.compile('^class[ \t]+'+id+'[ \t]*(?P<sup>\([^)]*\))?[ \t]*:')
is_method = re.compile('^[ \t]+def[ \t]+'+id+'[ \t]*\(')
is_import = re.compile('^import[ \t]*(?P<imp>[^#]+)')
is_from = re.compile('^from[ \t]+'+id+'[ \t]+import[ \t]+(?P<imp>[^#]+)')
dedent = re.compile('^[^ \t]')
indent = re.compile('^[^ \t]*')

_modules = {}				# cache of modules we've seen

# each Python class is represented by an instance of this class
class Class:
	'''Class to represent a Python class.'''
	def __init__(self, module, name, super, file, lineno):
		self.module = module
		self.name = name
		if super is None:
			super = []
		self.super = super
		self.methods = {}
		self.file = file
		self.lineno = lineno

	def _addmethod(self, name, lineno):
		self.methods[name] = lineno

def readmodule(module, path=[], inpackage=0):
	'''Read a module file and return a dictionary of classes.

	Search for MODULE in PATH and sys.path, read and parse the
	module and return a dictionary with one entry for each class
	found in the module.'''

	i = string.rfind(module, '.')
	if i >= 0:
		# Dotted module name
		package = module[:i]
		submodule = module[i+1:]
		parent = readmodule(package, path, inpackage)
		child = readmodule(submodule, parent['__path__'], 1)
		return child

	if _modules.has_key(module):
		# we've seen this module before...
		return _modules[module]
	if module in sys.builtin_module_names:
		# this is a built-in module
		dict = {}
		_modules[module] = dict
		return dict

	# search the path for the module
	f = None
	if inpackage:
		try:
			f, file, (suff, mode, type) = \
				imp.find_module(module, path)
		except ImportError:
			f = None
	if f is None:
		fullpath = path + sys.path
		f, file, (suff, mode, type) = imp.find_module(module, fullpath)
	if type == imp.PKG_DIRECTORY:
		dict = {'__path__': [file]}
		_modules[module] = dict
		# XXX Should we recursively look for submodules?
		return dict
	if type != imp.PY_SOURCE:
		# not Python source, can't do anything with this module
		f.close()
		dict = {}
		_modules[module] = dict
		return dict

	cur_class = None
	dict = {}
	_modules[module] = dict
	imports = []
	lineno = 0
	while 1:
		line = f.readline()
		if not line:
			break
		lineno = lineno + 1	# count lines
		line = line[:-1]	# remove line feed
		if blank_line.match(line):
			# ignore blank (and comment only) lines
			continue
## 		res = indent.match(line)
## 		if res:
## 			indentation = len(string.expandtabs(res.group(0), 8))
		res = is_import.match(line)
		if res:
			# import module
			for n in string.splitfields(res.group('imp'), ','):
				n = string.strip(n)
				try:
					# recursively read the
					# imported module
					d = readmodule(n, path, inpackage)
				except:
					print 'module',n,'not found'
					pass
			continue
		res = is_from.match(line)
		if res:
			# from module import stuff
			mod = res.group('id')
			names = string.splitfields(res.group('imp'), ',')
			try:
				# recursively read the imported module
				d = readmodule(mod, path, inpackage)
			except:
				print 'module',mod,'not found'
				continue
			# add any classes that were defined in the
			# imported module to our name space if they
			# were mentioned in the list
			for n in names:
				n = string.strip(n)
				if d.has_key(n):
					dict[n] = d[n]
				elif n == '*':
					# only add a name if not
					# already there (to mimic what
					# Python does internally)
					# also don't add names that
					# start with _
					for n in d.keys():
						if n[0] != '_' and \
						   not dict.has_key(n):
							dict[n] = d[n]
			continue
		res = is_class.match(line)
		if res:
			# we found a class definition
			class_name = res.group('id')
			inherit = res.group('sup')
			if inherit:
				# the class inherits from other classes
				inherit = string.strip(inherit[1:-1])
				names = []
				for n in string.splitfields(inherit, ','):
					n = string.strip(n)
					if dict.has_key(n):
						# we know this super class
						n = dict[n]
					else:
						c = string.splitfields(n, '.')
						if len(c) > 1:
							# super class
							# is of the
							# form module.class:
							# look in
							# module for class
							m = c[-2]
							c = c[-1]
							if _modules.has_key(m):
								d = _modules[m]
								if d.has_key(c):
									n = d[c]
					names.append(n)
				inherit = names
			# remember this class
			cur_class = Class(module, class_name, inherit, file, lineno)
			dict[class_name] = cur_class
			continue
		res = is_method.match(line)
		if res:
			# found a method definition
			if cur_class:
				# and we know the class it belongs to
				meth_name = res.group('id')
				cur_class._addmethod(meth_name, lineno)
			continue
		if dedent.match(line):
			# end of class definition
			cur_class = None
	f.close()
	return dict


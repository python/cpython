"""Parse a Python file and retrieve classes and methods.

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
- Continuation lines are not dealt with at all.
- While triple-quoted strings won't confuse it, lines that look like
  def, class, import or "from ... import" stmts inside backslash-continued
  single-quoted strings are treated like code.  The expense of stopping
  that isn't worth it.
- Code that doesn't pass tabnanny or python -t will confuse it, unless
  you set the module TABWIDTH vrbl (default 8) to the correct tab width
  for the file.

PACKAGE RELATED BUGS
- If you have a package and a module inside that or another package
  with the same name, module caching doesn't work properly since the
  key is the base name of the module/package.
- The only entry that is returned when you readmodule a package is a
  __path__ whose value is a list which confuses certain class browsers.
- When code does:
  from package import subpackage
  class MyClass(subpackage.SuperClass):
    ...
  It can't locate the parent.  It probably needs to have the same
  hairy logic that the import locator already does.  (This logic
  exists coded in Python in the freeze package.)
"""

import os
import sys
import imp
import re
import string

TABWIDTH = 8

_getnext = re.compile(r"""
    (?P<String>
       \""" [^"\\]* (?:
			(?: \\. | "(?!"") )
			[^"\\]*
		    )*
       \"""

    |   ''' [^'\\]* (?:
			(?: \\. | '(?!'') )
			[^'\\]*
		    )*
	'''
    )

|   (?P<Method>
	^
	(?P<MethodIndent> [ \t]* )
	def [ \t]+
	(?P<MethodName> [a-zA-Z_] \w* )
	[ \t]* \(
    )

|   (?P<Class>
	^
	(?P<ClassIndent> [ \t]* )
	class [ \t]+
	(?P<ClassName> [a-zA-Z_] \w* )
	[ \t]*
	(?P<ClassSupers> \( [^)\n]* \) )?
	[ \t]* :
    )

|   (?P<Import>
	^ import [ \t]+
	(?P<ImportList> [^#;\n]+ )
    )

|   (?P<ImportFrom>
	^ from [ \t]+
	(?P<ImportFromPath>
	    [a-zA-Z_] \w*
	    (?:
		[ \t]* \. [ \t]* [a-zA-Z_] \w*
	    )*
	)
	[ \t]+
	import [ \t]+
	(?P<ImportFromList> [^#;\n]+ )
    )
""", re.VERBOSE | re.DOTALL | re.MULTILINE).search

_modules = {}                           # cache of modules we've seen

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

class Function(Class):
	'''Class to represent a top-level Python function'''
	def __init__(self, module, name, file, lineno):
		Class.__init__(self, module, name, None, file, lineno)
	def _addmethod(self, name, lineno):
		assert 0, "Function._addmethod() shouldn't be called"

def readmodule(module, path=[], inpackage=0):
	'''Backwards compatible interface.

	Like readmodule_ex() but strips Function objects from the
	resulting dictionary.'''

	dict = readmodule_ex(module, path, inpackage)
	res = {}
	for key, value in dict.items():
		if not isinstance(value, Function):
			res[key] = value
	return res

def readmodule_ex(module, path=[], inpackage=0):
	'''Read a module file and return a dictionary of classes.

	Search for MODULE in PATH and sys.path, read and parse the
	module and return a dictionary with one entry for each class
	found in the module.'''

	dict = {}

	i = string.rfind(module, '.')
	if i >= 0:
		# Dotted module name
		package = string.strip(module[:i])
		submodule = string.strip(module[i+1:])
		parent = readmodule(package, path, inpackage)
		child = readmodule(submodule, parent['__path__'], 1)
		return child

	if _modules.has_key(module):
		# we've seen this module before...
		return _modules[module]
	if module in sys.builtin_module_names:
		# this is a built-in module
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
		fullpath = list(path) + sys.path
		f, file, (suff, mode, type) = imp.find_module(module, fullpath)
	if type == imp.PKG_DIRECTORY:
		dict['__path__'] = [file]
		_modules[module] = dict
		path = [file] + path
		f, file, (suff, mode, type) = \
				imp.find_module('__init__', [file])
	if type != imp.PY_SOURCE:
		# not Python source, can't do anything with this module
		f.close()
		_modules[module] = dict
		return dict

	_modules[module] = dict
	imports = []
	classstack = []	# stack of (class, indent) pairs
	src = f.read()
	f.close()

	# To avoid having to stop the regexp at each newline, instead
	# when we need a line number we simply string.count the number of
	# newlines in the string since the last time we did this; i.e.,
	#    lineno = lineno + \
	#             string.count(src, '\n', last_lineno_pos, here)
	#    last_lineno_pos = here
	countnl = string.count
	lineno, last_lineno_pos = 1, 0
	i = 0
	while 1:
		m = _getnext(src, i)
		if not m:
			break
		start, i = m.span()

		if m.start("Method") >= 0:
			# found a method definition or function
			thisindent = _indent(m.group("MethodIndent"))
			meth_name = m.group("MethodName")
			lineno = lineno + \
				 countnl(src, '\n',
					 last_lineno_pos, start)
			last_lineno_pos = start
			# close all classes indented at least as much
			while classstack and \
			      classstack[-1][1] >= thisindent:
				del classstack[-1]
			if classstack:
				# it's a class method
				cur_class = classstack[-1][0]
				cur_class._addmethod(meth_name, lineno)
			else:
				# it's a function
				f = Function(module, meth_name,
					     file, lineno)
				dict[meth_name] = f

		elif m.start("String") >= 0:
			pass

		elif m.start("Class") >= 0:
			# we found a class definition
			thisindent = _indent(m.group("ClassIndent"))
			# close all classes indented at least as much
			while classstack and \
			      classstack[-1][1] >= thisindent:
				del classstack[-1]
			lineno = lineno + \
				 countnl(src, '\n', last_lineno_pos, start)
			last_lineno_pos = start
			class_name = m.group("ClassName")
			inherit = m.group("ClassSupers")
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
			cur_class = Class(module, class_name, inherit,
					  file, lineno)
			dict[class_name] = cur_class
			classstack.append((cur_class, thisindent))

		elif m.start("Import") >= 0:
			# import module
			for n in string.split(m.group("ImportList"), ','):
				n = string.strip(n)
				try:
					# recursively read the imported module
					d = readmodule(n, path, inpackage)
				except:
					##print 'module', n, 'not found'
					pass

		elif m.start("ImportFrom") >= 0:
			# from module import stuff
			mod = m.group("ImportFromPath")
			names = string.split(m.group("ImportFromList"), ',')
			try:
				# recursively read the imported module
				d = readmodule(mod, path, inpackage)
			except:
				##print 'module', mod, 'not found'
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
		else:
			assert 0, "regexp _getnext found something unexpected"

	return dict

def _indent(ws, _expandtabs=string.expandtabs):
	return len(_expandtabs(ws, TABWIDTH))

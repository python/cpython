"""Findmodulefiles - Find out where modules are loaded from.
Run findmodulefiles() after running a program with "python -i". The
resulting file list can be given to mkfrozenresources or to a
(non-existent) freeze-like application.

findmodulefiles will create a file listing all modules and where they have
been imported from.

findunusedbuiltins takes a list of (modules, file) and prints all builtin modules
that are _not_ in the list. The list is created by opening the findmodulefiles
output, reading it and eval()ing that.

mkpycresource takes a list of (module, file) and creates a resourcefile with all those
modules and (optionally) a main module.
"""

def findmodulefiles(output=None):
	"""Produce a file containing a list of (modulename, filename-or-None)
	tuples mapping module names to source files"""
	# Immedeately grab the names
	import sys
	module_names = sys.modules.keys()[:]
	import os
	if not output:
		if os.name == 'mac':
			import macfs
			
			output, ok = macfs.StandardPutFile('Module file listing output:')
			if not ok: sys.exit(0)
			output = output.as_pathname()
	if not output:
		output = sys.stdout
	elif type(output) == type(''):
		output = open(output, 'w')
	output.write('[\n')
	for name in module_names:
		try:
			source = sys.modules[name].__file__
		except AttributeError:
			source = None
		else:
			source = `source`
		output.write('\t(%s,\t%s),\n' % (`name`, source))
	output.write(']\n')
	del output
	
def findunusedbuiltins(list):
	"""Produce (on stdout) a list of unused builtin modules"""
	import sys
	dict = {}
	for name, location in list:
		if location == None:
			dict[name] = 1
	for name in sys.builtin_module_names:
		if not dict.has_key(name):
			print 'Unused builtin module:', name
			

def mkpycresourcefile(list, main='', dst=None):
	"""Copy list-of-modules to resource file dst."""
	import py_resource
	import Res
	import sys
	
	if dst == None:
		import macfs
		fss, ok = macfs.StandardPutFile("PYC Resource output file")
		if not ok: sys.exit(0)
		dst = fss.as_pathname()
	if main == '':
		import macfs
		fss, ok = macfs.PromptGetFile("Main program:", "TEXT")
		if ok:
			main = fss.as_pathname()
	
	fsid = py_resource.create(dst)

	if main:
		id, name = py_resource.frompyfile(main, '__main__', preload=1)
		print '%5d\t%s\t%s'%(id, name, main)
	for name, location in list:
		if not location: continue
		if location[-4:] == '.pyc':
			# Attempt corresponding .py
			location = location[:-1]
		if location[-3:] != '.py':
			print '*** skipping', location
			continue
		id, name = py_resource.frompyfile(location, name, preload=1)
		print '%5d\t%s\t%s'%(id, name, location)

	Res.CloseResFile(fsid)

			

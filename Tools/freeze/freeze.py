#! /usr/local/bin/python

# Given a Python script, create a binary that runs the script.
# The binary is 100% independent of Python libraries and binaries.
# It will not contain any Python source code -- only "compiled" Python
# (as initialized static variables containing marshalled code objects).
# It even does the right thing for dynamically loaded modules!
# The module search path of the binary is set to the current directory.
#
# Some problems remain:
# - You need to have the Python source tree lying around as well as
#   the various libraries used to generate the Python binary.
# - For scripts that use many modules it generates absurdly large
#   files (frozen.c and config.o as well as the final binary),
#   and is consequently rather slow.
#
# Caveats:
# - The search for modules sometimes finds modules that are never
#   actually imported since the code importing them is never executed.
# - If an imported module isn't found, you get a warning but the
#   process of freezing continues.  The binary will fail if it
#   actually tries to import one of these modules.
# - This often happens with the module 'mac', which module 'os' tries
#   to import (to determine whether it is running on a Macintosh).
#   You can ignore the warning about this.
# - If the program dynamically reads or generates Python code and
#   executes it, this code may reference built-in or library modules
#   that aren't present in the frozen binary, and this will fail.
# - Your program may be using external data files, e.g. compiled
#   forms definitions (*.fd).  These aren't incorporated. By default,
#   the sys.path in the resulting binary is only '.' (but you can override
#   that with the -P option).
#
# Usage hints:
# - If you have a bunch of scripts that you want to freeze, instead
#   of freezing each of them separately, you might consider writing
#   a tiny main script that looks at sys.argv[0] and then imports
#   the corresponding module.  You can then make links to the
#   frozen binary named after the various scripts you support.
#   Pass the additional scripts as arguments after the main script.
#   A minimal script to do this is the following.
#       import sys, posixpath
#       exec('import ' + posixpath.basename(sys.argv[0]) + '\n')
#
# Mods by Jack, August 94:
# - Removed all static configuration stuff. Now, Setup and Makefile files
#   are parsed to obtain the linking info for the libraries. You have to
#   supply the -B option, though.
# - Added -P (set sys.path) and -I/-D/-L/-l options (passed on to cc and
#   ld).

import os
import sys
import regex
import getopt
import regsub
import string
import marshal

# Exception used when scanfile fails
NoSuchFile = 'NoSuchFile'

# Global options
builddir = ''				# -B dir
quiet = 0				# -q
verbose = 0				# -v
noexec = 0				# -n
nowrite = 0				# -N
ofile = 'a.out'				# -o file
path = '\'"."\''			# -P path

cc_options = []				# Collects cc options
ld_options = []				# Collects ld options
module_libraries = {}			# ld options for each module
global_libraries = []			# Libraries we always need
include_path = ''			# Include path, from Makefile 
lib_path = ''				# and lib path, ditto
compiler = 'cc'				# and compiler

# Main program -- argument parsing etc.
def main():
	global quiet, verbose, noexec, nowrite, ofile, builddir, path
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'B:nNo:P:qvI:D:L:l:')
	except getopt.error, msg:
		usage(str(msg))
		sys.exit(2)
	for o, a in opts:
	        if o == '-B': builddir = a
		if o == '-n': noexec = 1
		if o == '-N': nowrite = 1
		if o == '-o': ofile = a
		if o == '-P':
		    if '"' in a:
			usage('sorry, cannot have " in -P option')
			sys.exit(2)
		    path = `'"' + a + '"'`
		if o == '-q': verbose = 0; quiet = 1
		if o == '-v': verbose = verbose + 1; quiet = 0
		if o in ('-I', '-D'): cc_options.append(o+a)
		if o in ('-L', '-l'): ld_options.append(o+a)
	if not builddir:
	    usage('sorry, you have to pass a -B option')
	    sys.exit(2)
	if len(args) < 1:
		usage('please pass at least one file argument')
		sys.exit(2)
	process(args[0], args[1:])

# Print usage message to stderr
def usage(*msgs):
	sys.stdout = sys.stderr
	for msg in msgs: print msg
	print 'Usage: freeze [options] scriptfile [modulefile ...]'
	print '-B dir  : name of python build dir (no default)'
	print '-n      : generate the files but don\'t compile and link'
	print '-N      : don\'t write frozen.c (do compile unless -n given)'
	print '-o file : binary output file (default a.out)'
	print '-P path : set sys.path for program (default ".")'
	print '-q      : quiet (no messages at all except errors)'
	print '-v      : verbose (lots of extra messages)'
	print '-D and -I options are passed to cc, -L and -l to ld'

# Process the script file
def process(filename, addmodules):
	global noexec
	#
	if not quiet: print 'Computing needed modules ...'
	todo = {}
	todo['__main__'] = filename
	for name in addmodules:
		mod = os.path.basename(name)
		if mod[-3:] == '.py': mod = mod[:-3]
		todo[mod] = name
	try:
		dict = closure(todo)
	except NoSuchFile, filename:
		sys.stderr.write('Can\'t open file %s\n' % filename)
		sys.exit(1)
	#
	mods = dict.keys()
	mods.sort()
	#
	if verbose:
		print '%-15s %s' % ('Module', 'Filename')
		for mod in mods:
			print '%-15s %s' % (`mod`, dict[mod])
	#
	if not quiet: print 'Looking for dynamically linked modules ...'
	dlmodules = []
	objs = []
	libs = []
	for mod in mods:
		if dict[mod][-2:] == '.o':
			if verbose: print 'Found', mod, dict[mod]
			dlmodules.append(mod)
			objs.append(dict[mod])
			libsname = dict[mod][:-2] + '.libs'
			try:
				f = open(libsname, 'r')
			except IOError:
				f = None
			if f:
				libtext = f.read()
				f.close()
				for lib in string.split(libtext):
					if lib in libs: libs.remove(lib)
					libs.append(lib)
	#
	if not nowrite:
		if not quiet: print 'Writing frozen.c ...'
		writefrozen('frozen.c', dict)
	else:
		if not quiet: print 'NOT writing frozen.c ...'
	#
	if not quiet:
	    print 'Deducing compile/link options from', builddir
	#
	# Parse the config info
	#
	parse(builddir)
	CONFIG_IN = lib_path + '/config.c.in'
	FMAIN = lib_path + '/frozenmain.c'
	CC = compiler
	#
##	if not dlmodules:
	if 0:
		config = CONFIG
		if not quiet: print 'Using existing', config, '...'
	else:
		config = 'tmpconfig.c'
		if nowrite:
			if not quiet: print 'NOT writing config.c ...'
		else:
			if not quiet:
				print 'Writing config.c with dl modules ...'
			f = open(CONFIG_IN, 'r')
			g = open(config, 'w')
			m1 = regex.compile('-- ADDMODULE MARKER 1 --')
			m2 = regex.compile('-- ADDMODULE MARKER 2 --')
			builtinmodules = []
			stdmodules = ('sys', '__main__', '__builtin__',
				      'marshal')
			for mod in dict.keys():
				if dict[mod] == '<builtin>' and \
					  mod not in stdmodules:
					builtinmodules.append(mod)
			todomodules = builtinmodules + dlmodules
			while 1:
				line = f.readline()
				if not line: break
				g.write(line)
				if m1.search(line) >= 0:
					if verbose: print 'Marker 1 ...'
					for mod in todomodules:
						g.write('extern void init' + \
						  mod + '();\n')
				if m2.search(line) >= 0:
					if verbose: print 'Marker 2 ...'
					for mod in todomodules:
						g.write('{"' + mod + \
						  '", init' + mod + '},\n')
			g.close()
	#
	if not quiet:
		if noexec: print 'Generating compilation commands ...'
		else: print 'Starting compilation ...'
	defs = ['-DNO_MAIN', '-DUSE_FROZEN']
	defs.append('-DPYTHONPATH='+path)
	#
	incs = ['-I.', '-I' + include_path]
#	if dict.has_key('stdwin'):
#		incs.append('-I' + j(STDWIN, 'H'))
	#
	srcs = [config, FMAIN]
	#
	modlibs = module_libraries
	
	for mod in dict.keys():
	    if modlibs.has_key(mod):
		libs = libs + modlibs[mod]

	libs = libs + global_libraries
	#
	# remove dups:
	# XXXX Not well tested...
	nskip = 0
	newlibs = []
	while libs:
	    l = libs[0]
	    del libs[0]
	    if l[:2] == '-L' and l in newlibs:
		nskip = nskip + 1
		continue
	    if (l[:2] == '-l' or l[-2:] == '.a') and l in libs:
		nskip = nskip + 1
		continue
	    newlibs.append(l)
	libs = newlibs
	if nskip and not quiet:
	    print 'Removed %d duplicate libraries'%nskip
	#
	sts = 0
	#
	cmd = CC + ' -c'
	if cc_options:
	    cmd = cmd + ' ' + string.join(cc_options)
	cmd = cmd + ' ' + string.join(defs)
	cmd = cmd + ' ' + string.join(incs)
	cmd = cmd + ' ' + string.join(srcs)
	print cmd
	#
	if not noexec:
		sts = os.system(cmd)
		if sts:
			print 'Exit status', sts, '-- turning on -n'
			noexec = 1
	#
	for s in srcs:
		s = os.path.basename(s)
		if s[-2:] == '.c': s = s[:-2]
		o = s + '.o'
		objs.insert(0, o)
	#
	cmd = CC
	cmd = cmd + ' ' + string.join(objs)
	cmd = cmd + ' ' + string.join(libs)
	if ld_options:
	    cmd = cmd + ' ' + string.join(ld_options)
	cmd = cmd + ' -o ' + ofile
	print cmd
	#
	if not noexec:
		sts = os.system(cmd)
		if sts:
			print 'Exit status', sts
		else:
			print 'Done.'
	#
	if not quiet and not noexec and sts == 0:
		print 'Note: consider this:'; print '\tstrip', ofile
	#
	sys.exit(sts)


# Generate code for a given module
def makecode(filename):
	if filename[-2:] == '.o':
		return None
	try:
		f = open(filename, 'r')
	except IOError:
		return None
	if verbose: print 'Making code from', filename, '...'
	text = f.read()
	code = compile(text, filename, 'exec')
	f.close()
	return marshal.dumps(code)


# Write the C source file containing the frozen Python code
def writefrozen(filename, dict):
	f = open(filename, 'w')
	codelist = []
	for mod in dict.keys():
		codestring = makecode(dict[mod])
		if codestring is not None:
			codelist.append((mod, codestring))
	write = sys.stdout.write
	save_stdout = sys.stdout
	try:
		sys.stdout = f
		for mod, codestring in codelist:
			if verbose:
				write('Writing initializer for %s\n'%mod)
			print 'static unsigned char M_' + mod + '[' + \
				  str(len(codestring)) + '+1] = {'
			for i in range(0, len(codestring), 16):
				for c in codestring[i:i+16]:
					print str(ord(c)) + ',',
				print
			print '};'
		print 'struct frozen {'
		print '  char *name;'
		print '  unsigned char *code;'
		print '  int size;'
		print '} frozen_modules[] = {'
		for mod, codestring in codelist:
			print '  {"' + mod + '",',
			print 'M_' + mod + ',',
			print str(len(codestring)) + '},'
		print '  {0, 0, 0} /* sentinel */'
		print '};'
	finally:
		sys.stdout = save_stdout
	f.close()


# Determine the names and filenames of the modules imported by the
# script, recursively.  This is done by scanning for lines containing
# import statements.  (The scanning has only superficial knowledge of
# Python syntax and no knowledge of semantics, so in theory the result
# may be incorrect -- however this is quite unlikely if you don't
# intentionally obscure your Python code.)

# Compute the closure of scanfile() -- special first file because of script
def closure(todo):
	done = {}
	while todo:
		newtodo = {}
		for modname in todo.keys():
			if not done.has_key(modname):
				filename = todo[modname]
				if filename is None:
					filename = findmodule(modname)
				done[modname] = filename
				if filename in ('<builtin>', '<unknown>'):
					continue
				modules = scanfile(filename)
				for m in modules:
					if not done.has_key(m):
						newtodo[m] = None
		todo = newtodo
	return done

# Scan a file looking for import statements
importstr = '\(^\|:\)[ \t]*import[ \t]+\([a-zA-Z0-9_, \t]+\)'
fromstr   = '\(^\|:\)[ \t]*from[ \t]+\([a-zA-Z0-9_]+\)[ \t]+import[ \t]+'
isimport = regex.compile(importstr)
isfrom = regex.compile(fromstr)
def scanfile(filename):
	allmodules = {}
	try:
		f = open(filename, 'r')
	except IOError, msg:
		raise NoSuchFile, filename
	while 1:
		line = f.readline()
		if not line: break # EOF
		while line[-2:] == '\\\n': # Continuation line
			line = line[:-2] + ' '
			line = line + f.readline()
		if isimport.search(line) >= 0:
			rawmodules = isimport.group(2)
			modules = string.splitfields(rawmodules, ',')
			for i in range(len(modules)):
				modules[i] = string.strip(modules[i])
		elif isfrom.search(line) >= 0:
			modules = [isfrom.group(2)]
		else:
			continue
		for mod in modules:
			allmodules[mod] = None
	f.close()
	return allmodules.keys()

# Find the file containing a module, given its name; None if not found
builtins = sys.builtin_module_names + ['sys']
def findmodule(modname):
	if modname in builtins: return '<builtin>'
	for dirname in sys.path:
		dlfullname = os.path.join(dirname, modname + 'module.o')
		try:
			f = open(dlfullname, 'r')
		except IOError:
			f = None
		if f:
			f.close()
			return dlfullname
		fullname = os.path.join(dirname, modname + '.py')
		try:
			f = open(fullname, 'r')
		except IOError:
			continue
		f.close()
		return fullname
	if not quiet:
		sys.stderr.write('Warning: module %s not found\n' % modname)
	return '<unknown>'
#
# Parse a setup file. Returns two dictionaries, one containing variables
# defined with their values and one containing module definitions
#
def parse_setup(fp):
    modules = {}
    variables = {}
    for line in fp.readlines():
	if '#' in line:				# Strip comments
	    line = string.splitfields(line, '#')[0]
	line = string.strip(line[:-1])		# Strip whitespace
	if not line:
	    continue
	words = string.split(line)
	if '=' in words[0]:
	    #
	    # equal sign before first space. Definition
	    #
	    pos = string.index(line, '=')
	    name = line[:pos]
	    value = string.strip(line[pos+1:])
	    variables[name] = value
	else:
	    modules[words[0]] = words[1:]
    return modules, variables
#
# Parse a makefile. Returns a list of the variables defined.
#
def parse_makefile(fp):
    variables = {}
    for line in fp.readlines():
	if '#' in line:				# Strip comments
	    line = string.splitfields(line, '#')[0]
	if not line:
	    continue
	if line[0] in string.whitespace:
	    continue
	line = string.strip(line[:-1])		# Strip whitespace
	if not line:
	    continue
	if '=' in string.splitfields(line, ':')[0]: 
	    #
	    # equal sign before first colon. Definition
	    #
	    pos = string.index(line, '=')
	    name = line[:pos]
	    value = string.strip(line[pos+1:])
	    variables[name] = value
    return variables

#
# Recursively add loader options from Setup files in extension
# directories.
#
def add_extension_directory(name, isinstalldir):
    if verbose:
	print 'Adding extension directory', name
    fp = open(name + '/Setup', 'r')
    modules, variables = parse_setup(fp)
    #
    # Locate all new modules and remember the ld flags needed for them
    #
    for m in modules.keys():
	if module_libraries.has_key(m):
	    continue
	options = modules[m]
	if isinstalldir:
	    ld_options = []
	else:
	    ld_options = [name + '/lib.a']
	for o in options:
	    # ld options are all capital except DUIC and l
	    if o[:-2] == '.a':
		ld_options.append(o)
	    elif o[0] == '-':
		if o[1] == 'l':
		    ld_options.append(o)
		elif o[1] in string.uppercase and not o[1] in 'DUIC':
		    ld_options.append(o)
	module_libraries[m] = ld_options
    #
    # See if we have to bother with base setups
    #
    if variables.has_key('BASESETUP'):
	if isinstalldir:
	    raise 'installdir has base setup'
	setupfiles = string.split(variables['BASESETUP'])
	for s in setupfiles:
	    if s[-6:] <> '/Setup':
		raise 'Incorrect BASESETUP', s
	    s = s[:-6]
	    if s[0] <> '/':
		s = name + '/' + s
		s = os.path.normpath(s)
	    add_extension_directory(s, 0)
#
# Main routine for this module: given a build directory, get all
# information needed for the linker.
#
def parse(dir):
    global include_path
    global lib_path
    global compiler
    
    fp = open(dir + '/Makefile', 'r')
    #
    # First find the global libraries and the base python
    #
    vars = parse_makefile(fp)
    if vars.has_key('CC'):
	compiler = vars['CC']
    if not vars.has_key('installdir'):
	raise 'No $installdir in Makefile'
    include_path = vars['installdir'] + '/include/Py'
    lib_path = vars['installdir'] + '/lib/python/lib'
    global_libraries.append('-L' + lib_path)
    global_libraries.append('-lPython')
    global_libraries.append('-lParser')
    global_libraries.append('-lObjects')
    global_libraries.append('-lModules')
    for name in ('LIBS', 'LIBM', 'LIBC'):
	if not vars.has_key(name):
	    raise 'Missing required def in Makefile', name
	for lib in string.split(vars[name]):
	    global_libraries.append(lib)
    #
    # Next, parse the modules from the base python
    #
    add_extension_directory(lib_path, 1)
    #
    # Finally, parse the modules from the extension python
    #
    if dir <> lib_path:
	add_extension_directory(dir, 0)

# Call the main program
main()

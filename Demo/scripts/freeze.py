#! /usr/local/bin/python

# Given a Python script, create a binary that runs the script.
# The binary is 100% independent of Python libraries and binaries.
# It will not contain any Python source code -- only "compiled" Python
# (as initialized static variables containing marshalled code objects).
# It even does the right thing for dynamically loaded modules!
# The module search path of the binary is set to the current directory.
#
# Some problems remain:
# - It's highly non-portable, since it knows about paths and libraries
#   (there's a customization section though, and it knows how to
#   distinguish an SGI from a Sun SPARC system -- adding knowledge
#   about more systems is left as an exercise for the reader).
# - You need to have the Python source tree lying around as well as
#   the "libpython.a" used to generate the Python binary.
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
#   forms definitions (*.fd).  These aren't incorporated.  Since
#   sys.path in the resulting binary only contains '.', if your
#   program searches its data files along sys.path (as the 'flp'
#   modules does to find its forms definitions), you may need to
#   change the program to extend the search path or instruct its users
#   to set the environment variable PYTHONPATH to point to your data
#   files.
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


import os
import sys
import regex
import getopt
import regsub
import string
import marshal

# Function to join two pathnames with a slash in between
j = os.path.join

##################################
# START OF CONFIGURATION SECTION #
##################################

# Attempt to guess machine architecture
if os.path.exists('/usr/lib/libgl_s'): ARCH = 'sgi'
elif os.path.exists('/etc/issue'): ARCH = 'sequent'
else: ARCH = 'sun4'

# Site parametrizations (change to match your site)
CC = 'cc'				# C compiler
TOP = '/ufs/guido/src'			# Parent of all source trees
PYTHON = j(TOP, 'python')		# Top of the Python source tree
SRC = j(PYTHON, 'src')			# Python source directory
BLD = j(PYTHON, 'build.' + ARCH)	# Python build directory
#BLD = SRC				# Use this if you build in SRC

LIBINST = '/ufs/guido/src/python/irix4/tmp/lib/python/lib' # installed libraries
INCLINST = '/ufs/guido/src/python/irix4/tmp/include/Py' # installed include files

# Other packages (change to match your site)
DL = j(TOP, 'dl')			# Top of the dl source tree
DL_DLD = j(TOP, 'dl-dld')		# The dl-dld source directory
DLD = j(TOP, 'dld-3.2.3')		# The dld source directory
FORMS = j(TOP, 'forms')			# Top of the FORMS source tree
STDWIN = j(TOP, 'stdwin')		# Top of the STDWIN source tree
READLINE = j(TOP, 'readline.' + ARCH)	# Top of the GNU Readline source tree
SUN_X11 = '/usr/local/X11R5/lib/libX11.a'

# File names (usually no need to change)
LIBP = [				# Main Python libraries
        j(LIBINST, 'libPython.a'),
        j(LIBINST, 'libParser.a'),
        j(LIBINST, 'libObjects.a'),
        j(LIBINST, 'libModules.a')
       ]
CONFIG_IN = j(LIBINST, 'config.c.in')	# Configuration source file
FMAIN = j(LIBINST, 'frozenmain.c')	# Special main source file

# Libraries needed when linking.  First tuple item is built-in module
# for which it is needed (or '*' for always), rest are ld arguments.
# There is a separate list per architecture.
libdeps_sgi = [ \
	  ('stdwin',	j(STDWIN, 'Build/' + ARCH + '/x11/lib/lib.a')), \
	  ('fl',	j(FORMS, 'FORMS/libforms.a'), '-lfm_s'), \
	  ('*',		j(READLINE, 'libreadline.a'), '-ltermcap'), \
	  ('al',	'-laudio'), \
	  ('sv',	'-lsvideo', '-lXext'), \
	  ('cd',	'-lcdaudio', '-lds'), \
	  ('cl',	'-lcl'), \
	  ('imgfile',	'-limage', '-lgutil', '-lm'), \
	  ('mpz',	'/ufs/guido/src/gmp/libgmp.a'), \
	  ('*',		'-lsun'), \
	  ('*',		j(DL, 'libdl.a'), '-lmld'), \
	  ('*',		'-lmpc'), \
	  ('fm',	'-lfm_s'), \
	  ('gl',	'-lgl_s', '-lX11_s'), \
	  ('stdwin',	'-lX11_s'), \
	  ('*',		'-lm'), \
	  ('*',		'-lc_s'), \
	  ]
libdeps_sun4 = [ \
	  ('*',		'-Bstatic'), \
	  ('stdwin',	j(STDWIN, 'Build/' + ARCH + '/x11/lib/lib.a')), \
	  ('*',		j(READLINE, 'libreadline.a')), \
	  ('*',		'-lm'), \
	  ('*',		j(DL_DLD,'libdl.a'), j(DLD,'libdld.a')), \
	  ('*',		SUN_X11), \
	  ('*',		'-ltermcap'), \
	  ('*',		'-lc'), \
	  ]
libdeps_sequent = [ \
	  ('*',		j(LIBINST, 'libreadline.a'), '-ltermcap'), \
	  ('*',		'-lsocket'), \
	  ('*',		'-linet'), \
	  ('*',		'-lnsl'), \
	  ('*',		'-lm'), \
	  ('*',		'-lc'), \
	  ]
libdeps = eval('libdeps_' + ARCH)

################################
# END OF CONFIGURATION SECTION #
################################

# Exception used when scanfile fails
NoSuchFile = 'NoSuchFile'

# Global options
quiet = 0				# -q
verbose = 0				# -v
noexec = 0				# -n
nowrite = 0				# -N
ofile = 'a.out'				# -o file

# Main program -- argument parsing etc.
def main():
	global quiet, verbose, noexec, nowrite, ofile
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'nNo:qv')
	except getopt.error, msg:
		usage(str(msg))
		sys.exit(2)
	for o, a in opts:
		if o == '-n': noexec = 1
		if o == '-N': nowrite = 1
		if o == '-o': ofile = a
		if o == '-q': verbose = 0; quiet = 1
		if o == '-v': verbose = verbose + 1; quiet = 0
	if len(args) < 1:
		usage('please pass at least one file argument')
		sys.exit(2)
	process(args[0], args[1:])

# Print usage message to stderr
def usage(*msgs):
	sys.stdout = sys.stderr
	for msg in msgs: print msg
	print 'Usage: freeze [options] scriptfile [modulefile ...]'
	print '-n      : generate the files but don\'t compile and link'
	print '-N      : don\'t write frozen.c (do compile unless -n given)'
	print '-o file : binary output file (default a.out)'
	print '-q      : quiet (no messages at all except errors)'
	print '-v      : verbose (lots of extra messages)'

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
			todomodules = builtinmodules + dlmodules
			for mod in dict.keys():
				if dict[mod] == '<builtin>' and \
					  mod not in stdmodules:
					builtinmodules.append(mod)
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
	defs = ['-DNO_MAIN', '-DUSE_FROZEN', '-DPYTHONPATH=\'"."\'']
	#
	incs = ['-I.', '-I' + INCLINST]
	if dict.has_key('stdwin'):
		incs.append('-I' + j(STDWIN, 'H'))
	#
	srcs = [config, FMAIN]
	#
	if type(LIBP) == type(''):
		libs.append(LIBP)
	else:
		for lib in LIBP:
			libs.append(lib)
	for item in libdeps:
		m = item[0]
		if m == '*' or dict.has_key(m):
			for l in item[1:]:
				if l in libs: libs.remove(l)
				libs.append(l)
	#
	sts = 0
	#
	cmd = CC + ' -c'
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


# Call the main program
main()

#! /usr/local/bin/python

# "Freeze" a Python script into a binary.
# Usage: see first function below (before the imports!)

# HINTS:
# - Edit the line at XXX below before running!
# - You must have done "make inclinstall libainstall" in the Python
#   build directory.
# - The script should not use dynamically loaded modules
#   (*.so on most systems).


# XXX Change the following line to point to your Demo/freeze directory!
pack = '/ufs/guido/src/python/Demo/freeze'


# Print usage message and exit

def usage(msg = None):
	if msg:
		sys.stderr.write(str(msg) + '\n')
	sys.stderr.write('usage: freeze [-p prefix] script [module] ...\n')
	sys.exit(2)


# Import standard modules

import getopt
import os
import string
import sys
import addpack


# Set the directory to look for the freeze-private modules

dir = os.path.dirname(sys.argv[0])
if dir:
	pack = dir
addpack.addpack(pack)


# Import the freeze-private modules

import findmodules
import makeconfig
import makefreeze
import makemakefile
import parsesetup

hint = """
Use the '-p prefix' command line option to specify the prefix used
when you ran 'Make inclinstall libainstall' in the Python build directory.
(Please specify an absolute path.)
"""


# Main program

def main():
	# overridable context
	prefix = '/usr/local'		# settable with -p option
	path = sys.path

	# output files
	frozen_c = 'frozen.c'
	config_c = 'config.c'
	target = 'a.out'		# normally derived from script name
	makefile = 'Makefile'

	# parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'p:')
		if not args:
			raise getopt.error, 'not enough arguments'
	except getopt.error, msg:
		usage('getopt error: ' + str(msg))

	# proces option arguments
	for o, a in opts:
		if o == '-p':
			prefix = a

	# locations derived from options
	binlib = os.path.join(prefix, 'lib/python/lib')
	incldir = os.path.join(prefix, 'include/Py')
	config_c_in = os.path.join(binlib, 'config.c.in')
	frozenmain_c = os.path.join(binlib, 'frozenmain.c')
	makefile_in = os.path.join(binlib, 'Makefile')
	defines = ['-DHAVE_CONFIG_H', '-DUSE_FROZEN', '-DNO_MAIN',
		   '-DPTHONPATH=\\"$(PYTHONPATH)\\"']
	includes = ['-I' + incldir, '-I' + binlib]

	# sanity check of locations
	for dir in prefix, binlib, incldir:
		if not os.path.exists(dir):
			usage('needed directory %s not found' % dir + hint)
		if not os.path.isdir(dir):
			usage('%s: not a directory' % dir)
	for file in config_c_in, makefile_in, frozenmain_c:
		if not os.path.exists(file):
			usage('needed file %s not found' % file)
		if not os.path.isfile(file):
			usage('%s: not a plain file' % file)

	# check that file arguments exist
	for arg in args:
		if not os.path.exists(arg):
			usage('argument %s not found' % arg)
		if not os.path.isfile(arg):
			usage('%s: not a plain file' % arg)

	# process non-option arguments
	scriptfile = args[0]
	modules = args[1:]

	# derive target name from script name
	base = os.path.basename(scriptfile)
	base, ext = os.path.splitext(base)
	if base:
		if base != scriptfile:
			target = base
		else:
			target = base + '.bin'

	# Actual work starts here...

	dict = findmodules.findmodules(scriptfile, modules, path)

	builtins = []
	mods = dict.keys()
	mods.sort()
	for mod in mods:
		if dict[mod] == '<builtin>':
			builtins.append(mod)
		elif dict[mod] == '<unknown>':
			sys.stderr.write(
				'Warning: module %s not found anywhere\n' %
				mod)

	outfp = open(frozen_c, 'w')
	try:
		makefreeze.makefreeze(outfp, dict)
	finally:
		outfp.close()

	infp = open(config_c_in)
	outfp = open(config_c, 'w')
	try:
		makeconfig.makeconfig(infp, outfp, builtins)
	finally:
		outfp.close()
	infp.close()

	cflags = defines + includes + ['$(OPT)']
	libs = []
	for n in 'Modules', 'Python', 'Objects', 'Parser':
		n = 'lib%s.a' % n
		n = os.path.join(binlib, n)
		libs.append(n)

	makevars = parsesetup.getmakevars(makefile_in)
	somevars = {}
	for key in makevars.keys():
		somevars[key] = makevars[key]

	somevars['CFLAGS'] = string.join(cflags) # override
	files = ['$(OPT)', config_c, frozenmain_c] + libs + \
		['$(MODLIBS)', '$(LIBS)', '$(SYSLIBS)']

	outfp = open(makefile, 'w')
	try:
		makemakefile.makemakefile(outfp, somevars, files, target)
	finally:
		outfp.close()

	# Done!

	print 'Now run make to build the target:', target

main()

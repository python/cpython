#! /usr/local/bin/python

# "Freeze" a Python script into a binary.
# Usage: see variable usage_msg below (before the imports!)

# HINTS:
# - Edit the lines marked XXX below to localize.
# - You must have done "make inclinstall libainstall" in the Python
#   build directory.
# - The script should not use dynamically loaded modules
#   (*.so on most systems).


# Usage message

usage_msg = """
usage: freeze [-p prefix] [-e extension] ... script [module] ...

-p prefix:    This is the prefix used when you ran
              'Make inclinstall libainstall' in the Python build directory.
              (If you never ran this, freeze won't work.)
               The default is /usr/local.

-e extension: A directory containing additional .o files that
              may be used to resolve modules.  This directory
              should also have a Setup file describing the .o files.
              More than one -e option may be given.

script:       The Python script to be executed by the resulting binary.

module ...:   Additional Python modules (referenced by pathname)
              that will be included in the resulting binary.  These
              may be .py or .pyc files.
"""


# XXX Change the following line to point to your Demo/freeze directory
PACK = '/ufs/guido/src/python/Demo/freeze'

# XXX Change the following line to point to your install prefix
PREFIX = '/usr/local'


# Import standard modules

import cmp
import getopt
import os
import string
import sys
import addpack


# Set the directory to look for the freeze-private modules

dir = os.path.dirname(sys.argv[0])
if dir:
	pack = dir
else:
	pack = PACK
addpack.addpack(pack)


# Import the freeze-private modules

import checkextensions
import findmodules
import makeconfig
import makefreeze
import makemakefile
import parsesetup


# Main program

def main():
	# overridable context
	prefix = PREFIX			# settable with -p option
	extensions = []
	path = sys.path

	# output files
	frozen_c = 'frozen.c'
	config_c = 'config.c'
	target = 'a.out'		# normally derived from script name
	makefile = 'Makefile'

	# parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'e:p:')
	except getopt.error, msg:
		usage('getopt error: ' + str(msg))

	# proces option arguments
	for o, a in opts:
		if o == '-e':
			extensions.append(a)
		if o == '-p':
			prefix = a

	# locations derived from options
	binlib = os.path.join(prefix, 'lib/python/lib')
	incldir = os.path.join(prefix, 'include/Py')
	config_c_in = os.path.join(binlib, 'config.c.in')
	frozenmain_c = os.path.join(binlib, 'frozenmain.c')
	makefile_in = os.path.join(binlib, 'Makefile')
	defines = ['-DHAVE_CONFIG_H', '-DUSE_FROZEN', '-DNO_MAIN',
		   '-DPYTHONPATH=\\"$(PYTHONPATH)\\"']
	includes = ['-I' + incldir, '-I' + binlib]

	# sanity check of directories and files
	for dir in [prefix, binlib, incldir] + extensions:
		if not os.path.exists(dir):
			usage('needed directory %s not found' % dir)
		if not os.path.isdir(dir):
			usage('%s: not a directory' % dir)
	for file in config_c_in, makefile_in, frozenmain_c:
		if not os.path.exists(file):
			usage('needed file %s not found' % file)
		if not os.path.isfile(file):
			usage('%s: not a plain file' % file)
	for dir in extensions:
		setup = os.path.join(dir, 'Setup')
		if not os.path.exists(setup):
			usage('needed file %s not found' % setup)
		if not os.path.isfile(setup):
			usage('%s: not a plain file' % setup)

	# check that enough arguments are passed
	if not args:
		usage('at least one filename argument required')

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

	backup = frozen_c + '~'
	try:
		os.rename(frozen_c, backup)
	except os.error:
		backup = None
	outfp = open(frozen_c, 'w')
	try:
		makefreeze.makefreeze(outfp, dict)
	finally:
		outfp.close()
	if backup:
		if cmp.cmp(backup, frozen_c):
			sys.stderr.write('%s not changed, not written\n' %
					 frozen_c)
			os.rename(backup, frozen_c)

	builtins = []
	unknown = []
	mods = dict.keys()
	mods.sort()
	for mod in mods:
		if dict[mod] == '<builtin>':
			builtins.append(mod)
		elif dict[mod] == '<unknown>':
			unknown.append(mod)

	addfiles = []
	if unknown:
		addfiles, addmods = \
			  checkextensions.checkextensions(unknown, extensions)
		for mod in addmods:
			unknown.remove(mod)
		builtins = builtins + addmods
	if unknown:
		sys.stderr.write('Warning: unknown modules remain: %s\n' %
				 string.join(unknown))

	builtins.sort()
	infp = open(config_c_in)
	backup = config_c + '~'
	try:
		os.rename(config_c, backup)
	except os.error:
		backup = None
	outfp = open(config_c, 'w')
	try:
		makeconfig.makeconfig(infp, outfp, builtins)
	finally:
		outfp.close()
	infp.close()
	if backup:
		if cmp.cmp(backup, config_c):
			sys.stderr.write('%s not changed, not written\n' %
					 config_c)
			os.rename(backup, config_c)

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
	files = ['$(OPT)', config_c, frozen_c, frozenmain_c] + \
		addfiles + libs + \
		['$(MODLIBS)', '$(LIBS)', '$(SYSLIBS)']

	outfp = open(makefile, 'w')
	try:
		makemakefile.makemakefile(outfp, somevars, files, target)
	finally:
		outfp.close()

	# Done!

	print 'Now run make to build the target:', target


# Print usage message and exit

def usage(msg = None):
	if msg:
		sys.stderr.write(str(msg) + '\n')
	sys.stderr.write(usage_msg)
	sys.exit(2)


main()

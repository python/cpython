#! /usr/bin/env python

"""Freeze a Python script into a binary.

usage: freeze [options...] script.py [module]...

Options:

-p prefix:    This is the prefix used when you ran ``name install''
              in the Python build directory.
              (If you never ran this, freeze won't work.)
              The default is whatever sys.prefix evaluates to.

-P exec_prefix: Like -p but this is the 'exec_prefix', used to
		install objects etc.  The default is whatever sys.exec_prefix
		evaluates to, or the -p argument if given.

-e extension: A directory containing additional .o files that
              may be used to resolve modules.  This directory
              should also have a Setup file describing the .o files.
              More than one -e option may be given.

-o dir:       Directory where the output files are created; default '.'.

Arguments:

script.py:    The Python script to be executed by the resulting binary.
	      It *must* end with a .py suffix!

module ...:   Additional Python modules (referenced by pathname)
              that will be included in the resulting binary.  These
              may be .py or .pyc files.

NOTES:

In order to use freeze successfully, you must have built Python and
installed it ("make install").

The script should not use modules provided only as shared libraries;
if it does, the resulting binary is not self-contained.
"""


# Import standard modules

import cmp
import getopt
import os
import string
import sys
import addpack


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
	prefix = None			# settable with -p option
	exec_prefix = None		# settable with -P option
	extensions = []
	path = sys.path
	odir = ''

	# output files
	frozen_c = 'frozen.c'
	config_c = 'config.c'
	target = 'a.out'		# normally derived from script name
	makefile = 'Makefile'

	# parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'e:o:p:P:')
	except getopt.error, msg:
		usage('getopt error: ' + str(msg))

	# proces option arguments
	for o, a in opts:
		if o == '-e':
			extensions.append(a)
		if o == '-o':
			odir = a
		if o == '-p':
			prefix = a
		if o == '-P':
			exec_prefix = a
	
	# default prefix and exec_prefix
	if not exec_prefix:
		if prefix:
			exec_prefix = prefix
		else:
			exec_prefix = sys.exec_prefix
	if not prefix:
		prefix = sys.prefix

	# locations derived from options
	version = sys.version[:3]
	binlib = os.path.join(exec_prefix, 'lib/python%s/config' % version)
	incldir = os.path.join(prefix, 'include/python%s' % version)
	config_c_in = os.path.join(binlib, 'config.c.in')
	frozenmain_c = os.path.join(binlib, 'frozenmain.c')
	supp_sources = []
	makefile_in = os.path.join(binlib, 'Makefile')
	defines = []
	includes = ['-I' + incldir, '-I' + binlib]

	# sanity check of directories and files
	for dir in [prefix, exec_prefix, binlib, incldir] + extensions:
		if not os.path.exists(dir):
			usage('needed directory %s not found' % dir)
		if not os.path.isdir(dir):
			usage('%s: not a directory' % dir)
	for file in [config_c_in, makefile_in] + supp_sources:
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

	# check that the script name ends in ".py"
	if args[0][-3:] != ".py":
		usage('the script name must have a .py suffix')

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
	
	# handle -o option
	base_frozen_c = frozen_c
	base_config_c = config_c
	base_target = target
	if odir and not os.path.isdir(odir):
		try:
			os.mkdir(odir)
			print "Created output directory", odir
		except os.error, msg:
			usage('%s: mkdir failed (%s)' % (odir, str(msg)))
	if odir:
		frozen_c = os.path.join(odir, frozen_c)
		config_c = os.path.join(odir, config_c)
		target = os.path.join(odir, target)
		makefile = os.path.join(odir,makefile)

	# Actual work starts here...

	dict = findmodules.findmodules(scriptfile, modules, path)
	names = dict.keys()
	names.sort()
	print "Modules being frozen:"
	for name in names:
	    print '\t', name

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
	libs = [os.path.join(binlib, 'libpython$(VERSION).a')]

	makevars = parsesetup.getmakevars(makefile_in)
	somevars = {}
	for key in makevars.keys():
		somevars[key] = makevars[key]

	somevars['CFLAGS'] = string.join(cflags) # override
	files = ['$(OPT)', '$(LDFLAGS)', base_config_c, base_frozen_c] + \
		supp_sources +  addfiles + libs + \
		['$(MODLIBS)', '$(LIBS)', '$(SYSLIBS)']

	backup = makefile + '~'
	try:
		os.rename(makefile, backup)
	except os.error:
		backup = None
	outfp = open(makefile, 'w')
	try:
		makemakefile.makemakefile(outfp, somevars, files, base_target)
	finally:
		outfp.close()
	if backup:
		if not cmp.cmp(backup, makefile):
			print 'previous Makefile saved as', backup
		else:
			sys.stderr.write('%s not changed, not written\n' %
					 makefile)
			os.rename(backup, makefile)

	# Done!

	if odir:
		print 'Now run "make" in', odir,
		print 'to build the target:', base_target
	else:
		print 'Now run "make" to build the target:', base_target


# Print usage message and exit

def usage(msg = None):
	sys.stderr.write(__doc__)
	# Put the error last since the usage message scrolls off the screen
	if msg:
		sys.stderr.write('\nError: ' + str(msg) + '\n')
	sys.exit(2)


main()

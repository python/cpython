#! /usr/bin/env python

# XXX This will be replaced by a main program in Mac/Lib/bundlebuilder.py,
# but for now this is kept so Jack won't need to change his scripts...


"""\
buildappbundle creates an application bundle
Usage:
  buildappbundle [options] executable
Options:
  --output o    Output file; default executable with .app appended, short -o
  --link        Symlink the executable instead of copying it, short -l
  --plist file  Plist file (default: generate one), short -p
  --nib file    Main nib file or lproj folder for Cocoa program, short -n
  --resource r  Extra resource file to be copied to Resources, short -r
  --creator c   4-char creator code (default: '????'), short -c
  --verbose     increase verbosity level (default: quiet), short -v
  --help        This message, short -? or -h
"""


import sys
import os
import getopt
from bundlebuilder import AppBuilder
from plistlib import Plist


def usage():
	print __doc__
	sys.exit(1)


def main():
	output = None
	symlink = 0
	creator = "????"
	plist = None
	nib = None
	resources = []
	verbosity = 0
	SHORTOPTS = "o:ln:r:p:c:v?h"
	LONGOPTS=("output=", "link", "nib=", "resource=", "plist=", "creator=", "help",
			"verbose")
	try:
		options, args = getopt.getopt(sys.argv[1:], SHORTOPTS, LONGOPTS)
	except getopt.error:
		usage()
	if len(args) != 1:
		usage()
	executable = args[0]
	for opt, arg in options:
		if opt in ('-o', '--output'):
			output = arg
		elif opt in ('-l', '--link'):
			symlink = 1
		elif opt in ('-n', '--nib'):
			nib = arg
		elif opt in ('-r', '--resource'):
			resources.append(arg)
		elif opt in ('-c', '--creator'):
			creator = arg
		elif opt in ('-p', '--plist'):
			plist = arg
		elif opt in ('-v', '--verbose'):
			verbosity += 1
		elif opt in ('-?', '-h', '--help'):
			usage()
	if output is not None:
		builddir, bundlename = os.path.split(output)
	else:
		builddir = os.curdir
		bundlename = None  # will be derived from executable
	if plist is not None:
		plist = Plist.fromFile(plist)

	builder = AppBuilder(name=bundlename, executable=executable,
		builddir=builddir, creator=creator, plist=plist, resources=resources,
		symlink_exec=symlink, verbosity=verbosity)

	if nib is not None:
		resources.append(nib)
		nibname, ext = os.path.splitext(os.path.basename(nib))
		if ext == '.lproj':
			# Special case: if the main nib is a .lproj we assum a directory
			# and use the first nib from there. XXX Look: an arbitrary pick ;-)
			files = os.listdir(nib)
			for f in files:
				if f[-4:] == '.nib':
					nibname = os.path.split(f)[1][:-4]
					break
			else:
				nibname = ""
		if nibname:
			builder.plist.NSMainNibFile = nibname
			if not hasattr(builder.plist, "NSPrincipalClass"):
				builder.plist.NSPrincipalClass = "NSApplication"
	builder.setup()
	builder.build()


if __name__ == '__main__':
	main()

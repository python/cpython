#! /bin/env python
""" Dump data about a Metrowerks object file.

Based on reverse-engineering the library file format, since the docs are
wrong.

Copyright (C) 1997 Chris Herborth (chrish@qnx.com)
"""

# ----------------------------------------------------------------------
# Standard modules
import sys, getopt, string, time

# ----------------------------------------------------------------------
# Extra goodies
from dumpar import mk_long, str2hex, print_offset, get_string

# ----------------------------------------------------------------------
def mk_short( str ):
	""" convert a 2-byte string into a number

	Assumes big-endian!
	"""
	if len( str ) < 2:
		raise ValueError, "str must be 2 bytes long"

	num = ord( str[1] )
	num = num + ord( str[0] ) * 0x100

	return num

# ----------------------------------------------------------------------
def usage():
	""" Display a usage message and exit.
	"""
	print "dumpo [-v] object1 [object2 ... objectn]"
	print
	print "Attempt to display some useful information about the contents"
	print "of the given Metrowerks object file(s)."
	print
	print "-v        Be verbose (displays offsets along with the data)"
	raise SystemExit

# ----------------------------------------------------------------------
def dump_o( file, verbose ):
	""" dump information about a Metrowerks object file

	Note that there is more info there, 6 more quads before the file name.
	"""
	offset = 0

	print "Dumping object:", file

	# Attempt to read the data.
	try:
		data = open( file ).read()
	except IOError, retval:
		print "*** Unable to open file %s: %s" % ( file, retval[1] )
		return

	# Check the magic number.
	if verbose:
		print_offset( offset )
	print "Magic:",
	magic = data[offset:offset + 8]
	print "'%s'" % ( magic )
	if magic != "MWOBPPC ":
		print "*** Invalid magic  number!"
		return

	offset = offset + 8

	# version
	if verbose:
		print_offset( offset )
	print "version:", mk_long( data[offset:offset + 4] )
	offset = offset + 4

	# flags
	if verbose:
		print_offset( offset )
	print "flags:", str2hex( data[offset:offset + 4] )
	offset = offset + 4

	# code size
	if verbose:
		print_offset( offset )
	print "code size:", mk_long( data[offset:offset + 4] )
	offset = offset + 4

	# data size
	if verbose:
		print_offset( offset )
	print "data size:", mk_long( data[offset:offset + 4] )
	offset = offset + 4

# ----------------------------------------------------------------------
def main():
	""" mainline
	"""

	# Set up some defaults
	be_verbose = 0

	# First, check the command-line arguments
	try:
		opt, args = getopt.getopt( sys.argv[1:], "vh?" )
	except getopt.error:
		print "*** Error parsing command-line options!"
		usage()

	for o in opt:
		if o[0] == "-h" or o[0] == "-?":
			usage()
		elif o[0] == "-v":
			be_verbose = 1
		else:
			print "*** Unknown command-line option!"
			usage()

	# Now we can attempt to dump info about the arguments.
	for obj in args:
		dump_o( obj, be_verbose )

if __name__ == "__main__":
	main()

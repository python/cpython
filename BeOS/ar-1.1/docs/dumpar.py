#! /bin/env python
""" Dump data about a Metrowerks archive file.

$Id$

Based on reverse-engineering the library file format.

Copyright (C) 1997 Chris Herborth (chrish@qnx.com)
"""

# ----------------------------------------------------------------------
# Standard modules
import sys
import getopt
import string
import time

# ----------------------------------------------------------------------
def usage():
	""" Display a usage message and exit.
	"""
	print "dumpar [-v] library1 [library2 ... libraryn]"
	print
	print "Attempt to display some useful information about the contents"
	print "of the given Metrowerks library file(s)."
	print
	print "-v        Be verbose (displays offsets along with the data)"
	raise SystemExit

# ----------------------------------------------------------------------
def mk_long( str ):
	""" convert a 4-byte string into a number

	Assumes big-endian!
	"""
	if len( str ) < 4:
		raise ValueError, "str must be 4 bytes long"

	num = ord( str[3] )
	num = num + ord( str[2] ) * 0x100
	num = num + ord( str[1] ) * 0x10000
	num = num + ord( str[0] ) * 0x1000000

	return num

# ----------------------------------------------------------------------
def str2hex( str ):
	""" convert a string into a string of hex numbers
	"""
	ret = []
	for c in str:
		h = hex( ord( c ) )
		ret.append( string.zfill( "%s" % ( h[2:] ), 2 ) )

	return string.join( ret )

# ----------------------------------------------------------------------
def print_offset( offset ):
	""" print the offset nicely
	"""

	# Turn the offset into a hex number and strip off the leading "0x".
	val = "%s" % ( hex( offset ) )
	val = val[2:]

	out = "0x" + string.zfill( val, 8 )

	print out,

# ----------------------------------------------------------------------
def get_string( data ):
	""" dig a C string out of a data stream

	returns the string
	"""
	len = 0
	while data[len] != '\0':
		len = len + 1

	return data[:len]

# ----------------------------------------------------------------------
def dump_lib( file, verbose ):
	""" dump information about a Metrowerks library file
	"""
	offset = 0

	print "Dumping library:", file

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
		print "*** Invalid magic number!"
		return

	offset = offset + 8

	# File flags
	if verbose:
		print_offset( offset )
	print "file flags:",
	print mk_long( data[offset:offset + 4] )
	offset = offset + 4

	if verbose:
		print_offset( offset )
	print "file version:",
	print mk_long( data[offset:offset + 4] )
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

	# number of objects
	if verbose:
		print_offset( offset )
	print "number of objects:",
	num_objs = mk_long( data[offset:offset + 4] )
	print num_objs

	offset = offset + 4

	print

	# Now loop through the objects.
	obj_sizes = [ 0, ] * num_objs
	obj_data_offsets = [ 0, ] * num_objs

	for obj in range( num_objs ):
		# Magic?
		if verbose:
			print_offset( offset )
		print "modification time:",
		modtime = mk_long( data[offset:offset + 4] )
		print "[%s]" % ( ( time.localtime( modtime ), ) )

		offset = offset + 4

		# Offsets?
		if verbose:
			print_offset( offset )
		print "file name offset 1:",
		file_offset1 = mk_long( data[offset:offset + 4] )
		unknown = "%s" % ( hex( file_offset1 ) )
		print "%s (%s)" % ( unknown, str2hex( data[offset:offset + 4] ) )

		offset = offset + 4

		if verbose:
			print_offset( offset )
		print "file name offset 2:",
		file_offset2 = mk_long( data[offset:offset + 4] )
		unknown = "%s" % ( hex( file_offset2 ) )
		print "%s (%s)" % ( unknown, str2hex( data[offset:offset + 4] ) )

		offset = offset + 4

		# Extra -1 for NUL character.
		print "           >>>> File name should be %s characters." % \
			  ( file_offset2 - file_offset1 - 1)

		if verbose:
			print_offset( offset )
		print "object data offset:",
		file_data_offset = mk_long( data[offset:offset + 4] )
		unknown = "%s" % ( hex( file_data_offset ) )
		print "%s (%s)" % ( unknown, str2hex( data[offset:offset + 4] ) )

		obj_data_offsets[obj] = file_data_offset

		offset = offset + 4

		# object size
		if verbose:
			print_offset( offset )
		print "object size:",
		obj_sizes[obj] = mk_long( data[offset:offset + 4] )
		print "%s bytes" % ( obj_sizes[obj] )

		offset = offset + 4

		print

	# Now loop through the object names.
	for obj in range( num_objs ):
		# First name
		if verbose:
			print_offset( offset )
		print "object",
		print obj,
		print "name 1:",
		name1 = get_string( data[offset:] )
		print "[%s] %s chars" % ( name1, len( name1 ) )

		offset = offset + len( name1 ) + 1

		# Second name
		if verbose:
			print_offset( offset )
		print "object",
		print obj,
		print "name 2:",
		name2 = get_string( data[offset:] )
		print "[%s] %s chars" % ( name2, len( name1 ) )

		offset = offset + len( name2 ) + 1

		# See if we've got a magic cookie in the object data
		if verbose:
			print_offset( obj_data_offsets[obj] )

		cookie = data[obj_data_offsets[obj]:obj_data_offsets[obj] + 8]
		print "object",
		print obj,
		print "cookie: '%s'" % ( cookie )

		print

	# Now loop through the data and check for magic numbers there.
	return

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
	for lib in args:
		dump_lib( lib, be_verbose )

if __name__ == "__main__":
	main()

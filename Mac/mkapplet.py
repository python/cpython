"""Add a 'PYC ' resource named "__main__" to a resource file.

This first puts up a dialog asking for a Python source file ('TEXT'),
then one asking for an existing destination file ('APPL' or 'rsrc').

It compiles the Python source into a code object, marshals it into a string,
prefixes the string with a .pyc header, and turns it into a resource,
which is then written to the destination.

If the destination already contains a resource with the same name and type,
it is overwritten.
"""

import marshal
import imp
import macfs
from Res import *

# .pyc file (and 'PYC ' resource magic number)
MAGIC = imp.get_magic()

# Complete specification of our resource
RESTYPE = 'PYC '
RESID = 128
RESNAME = '__main__'

# OpenResFile mode parameters
READ = 1
WRITE = 2

def main():
	
	# Ask for source text
	
	srcfss, ok = macfs.StandardGetFile('TEXT')
	if not ok: return
	
	# Read the source and compile it
	# (there's no point asking for a destination if it has a syntax error)
	
	filename = srcfss.as_pathname()
	fp = open(filename)
	text = fp.read()
	fp.close()
	code = compile(text, filename, "exec")
	
	# Ask for destination file
	
	dstfss, ok = macfs.StandardGetFile('APPL', 'rsrc')
	if not ok: return
	
	# Create the raw data for the resource from the code object
	
	data = marshal.dumps(code)
	del code
	data = (MAGIC + '\0\0\0\0') + data
	
	# Open the resource file
	
	fnum = FSpOpenResFile(dstfss.as_pathname(), WRITE)
	
	# Delete any existing resource with name __main__ or number 128
	
	try:
		res = Get1NamedResource(RESTYPE, RESNAME)
		res.RmveResource()
	except Error:
		pass
	
	try:
		res = Get1Resource(RESTYPE, RESID)
		res.RmveResource()
	except Error:
		pass
	
	# Create the resource and write it
	
	res = Resource(data)
	res.AddResource(RESTYPE, RESID, RESNAME)
	# XXX Are the following two calls needed?
	res.WriteResource()
	res.ReleaseResource()
	
	# Close the resource file
	
	CloseResFile(fnum)

if __name__ == '__main__':
	main()

"""Create an applet from a Python script.

This puts up a dialog asking for a Python source file ('TEXT').
The output is a file with the same name but its ".py" suffix dropped.
It is created by copying an applet template and then adding a 'PYC '
resource named __main__ containing the compiled, marshalled script.
"""

import sys
import string
import os
import marshal
import imp
import macfs
import MacOS
from Res import *

# .pyc file (and 'PYC ' resource magic number)
MAGIC = imp.get_magic()

# Template file (searched on sys.path)
TEMPLATE = "PythonApplet"

# Specification of our resource
RESTYPE = 'PYC '
RESNAME = '__main__'

# OpenResFile mode parameters
READ = 1
WRITE = 2

def main():
	
	# Find the template
	# (there's no point in proceeding if we can't find it)
	
	for p in sys.path:
		template = os.path.join(p, TEMPLATE)
		try:
			tmpl = open(template, "rb")
			break
		except IOError:
			continue
	else:
		# XXX Ought to put up a dialog
		print "Template not found.  Exit."
		return
	
	# Ask for source text
	
	srcfss, ok = macfs.StandardGetFile('TEXT')
	if not ok:
		tmpl.close()
		return
	filename = srcfss.as_pathname()
	
	# Read the source and compile it
	# (there's no point overwriting the destination if it has a syntax error)
	
	fp = open(filename)
	text = fp.read()
	fp.close()
	try:
		code = compile(text, filename, "exec")
	except (SyntaxError, EOFError):
		print "Syntax error in script"
		tmpl.close()
		return
	
	# Set the destination file name
	
	if string.lower(filename[-3:]) == ".py":
		destname = filename[:-3]
	else:
		destname = filename + ".applet"
	
	# Copy the data from the template (creating the file as well)
	
	dest = open(destname, "wb")
	data = tmpl.read()
	if data:
		dest.write(data)
	dest.close()
	tmpl.close()
	
	# Copy the creator and type of the template to the destination
	
	ctor, type = MacOS.GetCreatorAndType(template)
	MacOS.SetCreatorAndType(destname, ctor, type)
	
	# Open the input and output resource forks
	
	input = FSpOpenResFile(template, READ)
	try:
		output = FSpOpenResFile(destname, WRITE)
	except MacOS.Error:
		print "Creating resource fork..."
		CreateResFile(destname)
		output = FSpOpenResFile(destname, WRITE)
	
	# Copy the resources from the template,
	# except a 'PYC ' resource named __main__
	
	UseResFile(input)
	ntypes = Count1Types()
	for itype in range(1, 1+ntypes):
		type = Get1IndType(itype)
		nresources = Count1Resources(type)
		for ires in range(1, 1+nresources):
			res = Get1IndResource(type, ires)
			id, type, name = res.GetResInfo()
			if (type, name) == (RESTYPE, RESNAME):
				continue # Don't copy __main__ from template
			size = res.SizeResource()
			attrs = res.GetResAttrs()
			print id, type, name, size, hex(attrs)
			res.LoadResource()
			res.DetachResource()
			UseResFile(output)
			try:
				res2 = Get1Resource(type, id)
			except MacOS.Error:
				res2 = None
			if res2:
				print "Overwriting..."
				res2.RmveResource()
			res.AddResource(type, id, name)
			#res.SetResAttrs(attrs)
			res.WriteResource()
			UseResFile(input)
	CloseResFile(input)
	
	# Make sure we're manipulating the output resource file now
	
	UseResFile(output)
	
	# Delete any existing 'PYC 'resource named __main__
	
	try:
		res = Get1NamedResource(RESTYPE, RESNAME)
		res.RmveResource()
	except Error:
		pass
	
	# Create the raw data for the resource from the code object
	
	data = marshal.dumps(code)
	del code
	data = (MAGIC + '\0\0\0\0') + data
	
	# Create the resource and write it
	
	id = 0
	while id < 128:
		id = Unique1ID(RESTYPE)
	res = Resource(data)
	res.AddResource(RESTYPE, id, RESNAME)
	res.WriteResource()
	res.ReleaseResource()
	
	# Close the resource file
	
	CloseResFile(output)

if __name__ == '__main__':
	main()

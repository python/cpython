"""Create an applet from a Python script.

This puts up a dialog asking for a Python source file ('TEXT').
The output is a file with the same name but its ".py" suffix dropped.
It is created by copying an applet template and then adding a 'PYC '
resource named __main__ containing the compiled, marshalled script.
"""

DEBUG=0

import sys
sys.stdout = sys.stderr

import string
import os
import marshal
import imp
import macfs
import MACFS
import MacOS
from Res import *

# .pyc file (and 'PYC ' resource magic number)
MAGIC = imp.get_magic()

# Template file (searched on sys.path)
TEMPLATE = "PythonApplet"

# Specification of our resource
RESTYPE = 'PYC '
RESNAME = '__main__'

# A resource with this name sets the "owner" (creator) of the destination
OWNERNAME = "owner resource"

# OpenResFile mode parameters
READ = 1
WRITE = 2

def findtemplate():
	"""Locate the applet template along sys.path"""
	for p in sys.path:
		template = os.path.join(p, TEMPLATE)
		try:
			template, d1, d2 = macfs.ResolveAliasFile(template)
			break
		except (macfs.error, ValueError):
			continue
	else:
		die("Template %s not found on sys.path" % `TEMPLATE`)
		return
	template = template.as_pathname()
	return template

def main():
	global DEBUG
	DEBUG=1
	
	# Find the template
	# (there's no point in proceeding if we can't find it)
	
	template = findtemplate()
	print 'Using template', template
			
	# Ask for source text if not specified in sys.argv[1:]
	
	if not sys.argv[1:]:
		srcfss, ok = macfs.PromptGetFile('Select Python source file:', 'TEXT')
		if not ok:
			return
		filename = srcfss.as_pathname()
		tp, tf = os.path.split(filename)
		if tf[-3:] == '.py':
			tf = tf[:-3]
		else:
			tf = tf + '.applet'
		dstfss, ok = macfs.StandardPutFile('Save application as:', tf)
		if not ok: return
		process(template, filename, dstfss.as_pathname())
	else:
		
		# Loop over all files to be processed
		for filename in sys.argv[1:]:
			process(template, filename, '')

undefs = ('Atmp', '????', '    ', '\0\0\0\0', 'BINA')

def process(template, filename, output):
	
	if DEBUG:
		print "Processing", `filename`, "..."
	
	# Read the source and compile it
	# (there's no point overwriting the destination if it has a syntax error)
	
	fp = open(filename)
	text = fp.read()
	fp.close()
	try:
		code = compile(text, filename, "exec")
	except (SyntaxError, EOFError):
		die("Syntax error in script %s" % `filename`)
		return
	
	# Set the destination file name
	
	if string.lower(filename[-3:]) == ".py":
		destname = filename[:-3]
		rsrcname = destname + '.rsrc'
	else:
		destname = filename + ".applet"
		rsrcname = filename + '.rsrc'
	
	if output:
		destname = output
	# Copy the data from the template (creating the file as well)
	
	template_fss = macfs.FSSpec(template)
	template_fss, d1, d2 = macfs.ResolveAliasFile(template_fss)
	dest_fss = macfs.FSSpec(destname)
	
	tmpl = open(template, "rb")
	dest = open(destname, "wb")
	data = tmpl.read()
	if data:
		dest.write(data)
	dest.close()
	tmpl.close()
	
	# Copy the creator of the template to the destination
	# unless it already got one.  Set type to APPL
	
	tctor, ttype = template_fss.GetCreatorType()
	ctor, type = dest_fss.GetCreatorType()
	if type in undefs: type = 'APPL'
	if ctor in undefs: ctor = tctor
	
	# Open the output resource fork
	
	try:
		output = FSpOpenResFile(dest_fss, WRITE)
	except MacOS.Error:
		if DEBUG:
			print "Creating resource fork..."
		CreateResFile(destname)
		output = FSpOpenResFile(dest_fss, WRITE)
	
	# Copy the resources from the template
	
	input = FSpOpenResFile(template_fss, READ)
	newctor = copyres(input, output)
	CloseResFile(input)
	if newctor: ctor = newctor
	
	# Copy the resources from the target specific resource template, if any
	
	try:
		input = FSpOpenResFile(rsrcname, READ)
	except (MacOS.Error, ValueError):
		print 'No resource file', rsrcname
		pass
	else:
		newctor = copyres(input, output)
		CloseResFile(input)
		if newctor: ctor = newctor
	
	# Now set the creator, type and bundle bit of the destination
	dest_finfo = dest_fss.GetFInfo()
	dest_finfo.Creator = ctor
	dest_finfo.Type = type
	dest_finfo.Flags = dest_finfo.Flags | MACFS.kHasBundle
	dest_finfo.Flags = dest_finfo.Flags & ~MACFS.kHasBeenInited
	dest_fss.SetFInfo(dest_finfo)
	
	# Make sure we're manipulating the output resource file now
	
	UseResFile(output)
	
	# Delete any existing 'PYC 'resource named __main__
	
	try:
		res = Get1NamedResource(RESTYPE, RESNAME)
		res.RemoveResource()
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
	
	# Close the output file
	
	CloseResFile(output)
	
	# Give positive feedback
	
	message("Applet %s created." % `destname`)


# Copy resources between two resource file descriptors.
# Exception: don't copy a __main__ resource.
# If a resource's name is "owner resource", its type is returned
# (so the caller can use it to set the destination's creator)

def copyres(input, output):
	ctor = None
	UseResFile(input)
	ntypes = Count1Types()
	for itype in range(1, 1+ntypes):
		type = Get1IndType(itype)
		nresources = Count1Resources(type)
		for ires in range(1, 1+nresources):
			res = Get1IndResource(type, ires)
			id, type, name = res.GetResInfo()
			lcname = string.lower(name)
			if (type, lcname) == (RESTYPE, RESNAME):
				continue # Don't copy __main__ from template
			if lcname == OWNERNAME: ctor = type
			size = res.size
			attrs = res.GetResAttrs()
			if DEBUG:
				print id, type, name, size, hex(attrs)
			res.LoadResource()
			res.DetachResource()
			UseResFile(output)
			try:
				res2 = Get1Resource(type, id)
			except MacOS.Error:
				res2 = None
			if res2:
				if DEBUG:
					print "Overwriting..."
				res2.RemoveResource()
			res.AddResource(type, id, name)
			res.WriteResource()
			attrs = attrs | res.GetResAttrs()
			if DEBUG:
				print "New attrs =", hex(attrs)
			res.SetResAttrs(attrs)
			UseResFile(input)
	return ctor


# Show a message and exit

def die(str):
	message(str)
	sys.exit(1)


# Show a message

def message(str, id = 256):
	from Dlg import *
	d = GetNewDialog(id, -1)
	if not d:
		print "Error:", `str`
		print "DLOG id =", id, "not found."
		return
	tp, h, rect = d.GetDialogItem(2)
	SetDialogItemText(h, str)
	while 1:
		n = ModalDialog(None)
		if n == 1: break
	del d


if __name__ == '__main__':
	main()


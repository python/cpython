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
# XXXX Should look for id=0
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
	if DEBUG:
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
		
	# Try removing the output file
	try:
		os.unlink(output)
	except os.error:
		pass
		

	# Create FSSpecs for the various files
		
	template_fss = macfs.FSSpec(template)
	template_fss, d1, d2 = macfs.ResolveAliasFile(template_fss)
	dest_fss = macfs.FSSpec(destname)
	
	# Copy data (not resources, yet) from the template
	
	tmpl = open(template, "rb")
	dest = open(destname, "wb")
	data = tmpl.read()
	if data:
		dest.write(data)
	dest.close()
	tmpl.close()
	
	# Open the output resource fork
	
	try:
		output = FSpOpenResFile(dest_fss, WRITE)
	except MacOS.Error:
		if DEBUG:
			print "Creating resource fork..."
		CreateResFile(destname)
		output = FSpOpenResFile(dest_fss, WRITE)
		
	# Copy the resources from the target specific resource template, if any
	typesfound, ownertype = [], None
	try:
		input = FSpOpenResFile(rsrcname, READ)
	except (MacOS.Error, ValueError):
		pass
	else:
		typesfound, ownertype = copyres(input, output, [], 0)
		CloseResFile(input)
		
	# Check which resource-types we should not copy from the template
	skiptypes = []
	if 'SIZE' in typesfound: skiptypes.append('SIZE')
	if 'BNDL' in typesfound: skiptypes = skiptypes + ['BNDL', 'FREF', 'icl4', 
			'icl8', 'ics4', 'ics8', 'ICN#', 'ics#']
	skipowner = (ownertype <> None)
	
	# Copy the resources from the template
	
	input = FSpOpenResFile(template_fss, READ)
	dummy, tmplowner = copyres(input, output, skiptypes, skipowner)
	if ownertype == None:
		ownertype = tmplowner
	CloseResFile(input)
	if ownertype == None:
		die("No owner resource found in either resource file or template")	
	
	# Now set the creator, type and bundle bit of the destination
	dest_finfo = dest_fss.GetFInfo()
	dest_finfo.Creator = ownertype
	dest_finfo.Type = 'APPL'
	dest_finfo.Flags = dest_finfo.Flags | MACFS.kHasBundle
	dest_finfo.Flags = dest_finfo.Flags & ~MACFS.kHasBeenInited
	dest_fss.SetFInfo(dest_finfo)
	
	# Make sure we're manipulating the output resource file now
	
	UseResFile(output)
	
	# Delete any existing 'PYC ' resource named __main__
	
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
# skip a resource named '__main__' or (if skipowner is set) 'Owner resource'.
# Also skip resources with a type listed in skiptypes.
#
def copyres(input, output, skiptypes, skipowner):
	ctor = None
	alltypes = []
	UseResFile(input)
	ntypes = Count1Types()
	for itype in range(1, 1+ntypes):
		type = Get1IndType(itype)
		if type in skiptypes:
			continue
		alltypes.append(type)
		nresources = Count1Resources(type)
		for ires in range(1, 1+nresources):
			res = Get1IndResource(type, ires)
			id, type, name = res.GetResInfo()
			lcname = string.lower(name)
			if (type, lcname) == (RESTYPE, RESNAME):
				continue # Don't copy __main__ from template
			# XXXX should look for id=0
			if lcname == OWNERNAME:
				if skipowner:
					continue # Skip this one
				else:
					ctor = type
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
	return alltypes, ctor


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

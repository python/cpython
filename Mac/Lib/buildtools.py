"""tools for BuildApplet and BuildApplication"""

import sys
import os
import string
import imp
import marshal
import macfs
import Res
import MACFS
import MacOS
import macostools
import EasyDialogs


BuildError = "BuildError"

DEBUG=1


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
		raise BuildError, "Template %s not found on sys.path" % `TEMPLATE`
	template = template.as_pathname()
	return template


def process(template, filename, output, copy_codefragment):
	
	if DEBUG:
		progress = EasyDialogs.ProgressBar("Processing %s..."%os.path.split(filename)[1], 120)
		progress.label("Compiling...")
	else:
		progress = None
	
	# Read the source and compile it
	# (there's no point overwriting the destination if it has a syntax error)
	
	fp = open(filename)
	text = fp.read()
	fp.close()
	try:
		code = compile(text, filename, "exec")
	except (SyntaxError, EOFError):
		raise BuildError, "Syntax error in script %s" % `filename`
	
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
		os.remove(destname)
	except os.error:
		pass
	process_common(template, progress, code, rsrcname, destname, 0, copy_codefragment)
	

def update(template, filename, output):
	if DEBUG:
		progress = EasyDialogs.ProgressBar("Updating %s..."%os.path.split(filename)[1], 120)
	else:
		progress = None
	if not output:
		output = filename + ' (updated)'
	
	# Try removing the output file
	try:
		os.remove(output)
	except os.error:
		pass
	process_common(template, progress, None, filename, output, 1, 1)


def process_common(template, progress, code, rsrcname, destname, is_update, copy_codefragment):
	# Create FSSpecs for the various files
	template_fss = macfs.FSSpec(template)
	template_fss, d1, d2 = macfs.ResolveAliasFile(template_fss)
	dest_fss = macfs.FSSpec(destname)
	
	# Copy data (not resources, yet) from the template
	if DEBUG:
		progress.label("Copy data fork...")
		progress.set(10)
	
	if copy_codefragment:
		tmpl = open(template, "rb")
		dest = open(destname, "wb")
		data = tmpl.read()
		if data:
			dest.write(data)
		dest.close()
		tmpl.close()
		del dest
		del tmpl
	
	# Open the output resource fork
	
	if DEBUG:
		progress.label("Copy resources...")
		progress.set(20)
	try:
		output = Res.FSpOpenResFile(dest_fss, WRITE)
	except MacOS.Error:
		Res.CreateResFile(destname)
		output = Res.FSpOpenResFile(dest_fss, WRITE)
	
	# Copy the resources from the target specific resource template, if any
	typesfound, ownertype = [], None
	try:
		input = Res.FSpOpenResFile(rsrcname, READ)
	except (MacOS.Error, ValueError):
		pass
		if DEBUG:
			progress.inc(50)
	else:
		if is_update:
			skip_oldfile = ['cfrg']
		else:
			skip_oldfile = []
		typesfound, ownertype = copyres(input, output, skip_oldfile, 0, progress)
		Res.CloseResFile(input)
	
	# Check which resource-types we should not copy from the template
	skiptypes = []
	if 'SIZE' in typesfound: skiptypes.append('SIZE')
	if 'BNDL' in typesfound: skiptypes = skiptypes + ['BNDL', 'FREF', 'icl4', 
			'icl8', 'ics4', 'ics8', 'ICN#', 'ics#']
	if not copy_codefragment:
		skiptypes.append('cfrg')
	skipowner = (ownertype <> None)
	
	# Copy the resources from the template
	
	input = Res.FSpOpenResFile(template_fss, READ)
	dummy, tmplowner = copyres(input, output, skiptypes, skipowner, progress)
	if ownertype == None:
		ownertype = tmplowner
	Res.CloseResFile(input)
	if ownertype == None:
		raise BuildError, "No owner resource found in either resource file or template"
	
	# Make sure we're manipulating the output resource file now
	
	Res.UseResFile(output)
	
	if code:
		# Delete any existing 'PYC ' resource named __main__
		
		try:
			res = Res.Get1NamedResource(RESTYPE, RESNAME)
			res.RemoveResource()
		except Res.Error:
			pass
		
		# Create the raw data for the resource from the code object
		if DEBUG:
			progress.label("Write PYC resource...")
			progress.set(120)
		
		data = marshal.dumps(code)
		del code
		data = (MAGIC + '\0\0\0\0') + data
		
		# Create the resource and write it
		
		id = 0
		while id < 128:
			id = Res.Unique1ID(RESTYPE)
		res = Res.Resource(data)
		res.AddResource(RESTYPE, id, RESNAME)
		res.WriteResource()
		res.ReleaseResource()
	
	# Close the output file
	
	Res.CloseResFile(output)
	
	# Now set the creator, type and bundle bit of the destination
	dest_finfo = dest_fss.GetFInfo()
	dest_finfo.Creator = ownertype
	dest_finfo.Type = 'APPL'
	dest_finfo.Flags = dest_finfo.Flags | MACFS.kHasBundle
	dest_finfo.Flags = dest_finfo.Flags & ~MACFS.kHasBeenInited
	dest_fss.SetFInfo(dest_finfo)
	
	macostools.touched(dest_fss)
	if DEBUG:
		progress.label("Done.")


# Copy resources between two resource file descriptors.
# skip a resource named '__main__' or (if skipowner is set) 'Owner resource'.
# Also skip resources with a type listed in skiptypes.
#
def copyres(input, output, skiptypes, skipowner, progress=None):
	ctor = None
	alltypes = []
	Res.UseResFile(input)
	ntypes = Res.Count1Types()
	progress_type_inc = 50/ntypes
	for itype in range(1, 1+ntypes):
		type = Res.Get1IndType(itype)
		if type in skiptypes:
			continue
		alltypes.append(type)
		nresources = Res.Count1Resources(type)
		progress_cur_inc = progress_type_inc/nresources
		for ires in range(1, 1+nresources):
			res = Res.Get1IndResource(type, ires)
			id, type, name = res.GetResInfo()
			lcname = string.lower(name)
##			if (type, lcname) == (RESTYPE, RESNAME):
##				continue # Don't copy __main__ from template
			# XXXX should look for id=0
			if lcname == OWNERNAME:
				if skipowner:
					continue # Skip this one
				else:
					ctor = type
			size = res.size
			attrs = res.GetResAttrs()
			if DEBUG and progress:
				progress.label("Copy %s %d %s"%(type, id, name))
				progress.inc(progress_cur_inc)
			res.LoadResource()
			res.DetachResource()
			Res.UseResFile(output)
			try:
				res2 = Res.Get1Resource(type, id)
			except MacOS.Error:
				res2 = None
			if res2:
				if DEBUG and progress:
					progress.label("Overwrite %s %d %s"%(type, id, name))
				res2.RemoveResource()
			res.AddResource(type, id, name)
			res.WriteResource()
			attrs = attrs | res.GetResAttrs()
			res.SetResAttrs(attrs)
			Res.UseResFile(input)
	return alltypes, ctor



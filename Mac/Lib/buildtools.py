"""tools for BuildApplet and BuildApplication"""

import sys
import os
import string
import imp
import marshal
import macfs
from Carbon import Res
import MACFS
import MacOS
import macostools
import macresource
import EasyDialogs
import shutil


BuildError = "BuildError"

# .pyc file (and 'PYC ' resource magic number)
MAGIC = imp.get_magic()

# Template file (searched on sys.path)
TEMPLATE = "PythonInterpreter"

# Specification of our resource
RESTYPE = 'PYC '
RESNAME = '__main__'

# A resource with this name sets the "owner" (creator) of the destination
# It should also have ID=0. Either of these alone is not enough.
OWNERNAME = "owner resource"

# Default applet creator code
DEFAULT_APPLET_CREATOR="Pyta"

# OpenResFile mode parameters
READ = 1
WRITE = 2


def findtemplate(template=None):
	"""Locate the applet template along sys.path"""
	if MacOS.runtimemodel == 'macho':
		if template:
			return template
		return findtemplate_macho()
	if not template:
		template=TEMPLATE
	for p in sys.path:
		file = os.path.join(p, template)
		try:
			file, d1, d2 = macfs.ResolveAliasFile(file)
			break
		except (macfs.error, ValueError):
			continue
	else:
		raise BuildError, "Template %s not found on sys.path" % `template`
	file = file.as_pathname()
	return file
	
def findtemplate_macho():
	execpath = sys.executable.split('/')
	if not 'Contents' in execpath:
		raise BuildError, "Not running from a .app bundle: %s" % sys.executable
	i = execpath.index('Contents')
	return '/'.join(execpath[:i])


def process(template, filename, destname, copy_codefragment, 
		rsrcname=None, others=[], raw=0, progress="default"):
	
	if progress == "default":
		progress = EasyDialogs.ProgressBar("Processing %s..."%os.path.split(filename)[1], 120)
		progress.label("Compiling...")
		progress.inc(0)
	# check for the script name being longer than 32 chars. This may trigger a bug
	# on OSX that can destroy your sourcefile.
	if '#' in os.path.split(filename)[1]:
		raise BuildError, "BuildApplet could destroy your sourcefile on OSX, please rename: %s" % filename
	# Read the source and compile it
	# (there's no point overwriting the destination if it has a syntax error)
	
	fp = open(filename, 'rU')
	text = fp.read()
	fp.close()
	try:
		code = compile(text, filename, "exec")
	except SyntaxError, arg:
		raise BuildError, "Syntax error in script %s: %s" % (filename, arg)
	except EOFError:
		raise BuildError, "End-of-file in script %s" % (filename,)
	
	# Set the destination file name. Note that basename
	# does contain the whole filepath, only a .py is stripped.
	
	if string.lower(filename[-3:]) == ".py":
		basename = filename[:-3]
		if MacOS.runtimemodel != 'macho' and not destname:
			destname = basename
	else:
		basename = filename
		
	if not destname:
		if MacOS.runtimemodel == 'macho':
			destname = basename + '.app'
		else:
			destname = basename + '.applet'
	if not rsrcname:
		rsrcname = basename + '.rsrc'
		
	# Try removing the output file. This fails in MachO, but it should
	# do any harm.
	try:
		os.remove(destname)
	except os.error:
		pass
	process_common(template, progress, code, rsrcname, destname, 0, 
		copy_codefragment, raw, others)
	

def update(template, filename, output):
	if MacOS.runtimemodel == 'macho':
		raise BuildError, "No updating yet for MachO applets"
	if progress:
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


def process_common(template, progress, code, rsrcname, destname, is_update, 
		copy_codefragment, raw=0, others=[]):
	if MacOS.runtimemodel == 'macho':
		return process_common_macho(template, progress, code, rsrcname, destname,
			is_update, raw, others)
	if others:
		raise BuildError, "Extra files only allowed for MachoPython applets"
	# Create FSSpecs for the various files
	template_fss = macfs.FSSpec(template)
	template_fss, d1, d2 = macfs.ResolveAliasFile(template_fss)
	dest_fss = macfs.FSSpec(destname)
	
	# Copy data (not resources, yet) from the template
	if progress:
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
	
	if progress:
		progress.label("Copy resources...")
		progress.set(20)
	try:
		output = Res.FSpOpenResFile(dest_fss, WRITE)
	except MacOS.Error:
		Res.FSpCreateResFile(destname, '????', 'APPL', MACFS.smAllScripts)
		output = Res.FSpOpenResFile(dest_fss, WRITE)
	
	# Copy the resources from the target specific resource template, if any
	typesfound, ownertype = [], None
	try:
		input = Res.FSpOpenResFile(rsrcname, READ)
	except (MacOS.Error, ValueError):
		pass
		if progress:
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
	if 'vers' in typesfound: skiptypes.append('vers')
	if 'SIZE' in typesfound: skiptypes.append('SIZE')
	if 'BNDL' in typesfound: skiptypes = skiptypes + ['BNDL', 'FREF', 'icl4', 
			'icl8', 'ics4', 'ics8', 'ICN#', 'ics#']
	if not copy_codefragment:
		skiptypes.append('cfrg')
##	skipowner = (ownertype <> None)
	
	# Copy the resources from the template
	
	input = Res.FSpOpenResFile(template_fss, READ)
	dummy, tmplowner = copyres(input, output, skiptypes, 1, progress)
		
	Res.CloseResFile(input)
##	if ownertype == None:
##		raise BuildError, "No owner resource found in either resource file or template"
	# Make sure we're manipulating the output resource file now
	
	Res.UseResFile(output)

	if ownertype == None:
		# No owner resource in the template. We have skipped the
		# Python owner resource, so we have to add our own. The relevant
		# bundle stuff is already included in the interpret/applet template.
		newres = Res.Resource('\0')
		newres.AddResource(DEFAULT_APPLET_CREATOR, 0, "Owner resource")
		ownertype = DEFAULT_APPLET_CREATOR
	
	if code:
		# Delete any existing 'PYC ' resource named __main__
		
		try:
			res = Res.Get1NamedResource(RESTYPE, RESNAME)
			res.RemoveResource()
		except Res.Error:
			pass
		
		# Create the raw data for the resource from the code object
		if progress:
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
		attrs = res.GetResAttrs()
		attrs = attrs | 0x04	# set preload
		res.SetResAttrs(attrs)
		res.WriteResource()
		res.ReleaseResource()
	
	# Close the output file
	
	Res.CloseResFile(output)
	
	# Now set the creator, type and bundle bit of the destination
	dest_finfo = dest_fss.GetFInfo()
	dest_finfo.Creator = ownertype
	dest_finfo.Type = 'APPL'
	dest_finfo.Flags = dest_finfo.Flags | MACFS.kHasBundle | MACFS.kIsShared
	dest_finfo.Flags = dest_finfo.Flags & ~MACFS.kHasBeenInited
	dest_fss.SetFInfo(dest_finfo)
	
	macostools.touched(dest_fss)
	if progress:
		progress.label("Done.")
		progress.inc(0)

def process_common_macho(template, progress, code, rsrcname, destname, is_update, raw=0, others=[]):
	# First make sure the name ends in ".app"
	if destname[-4:] != '.app':
		destname = destname + '.app'
	# Now deduce the short name
	shortname = os.path.split(destname)[1]
	if shortname[-4:] == '.app':
		# Strip the .app suffix
		shortname = shortname[:-4]
	# And deduce the .plist and .icns names
	plistname = None
	icnsname = None
	if rsrcname and rsrcname[-5:] == '.rsrc':
		tmp = rsrcname[:-5]
		plistname = tmp + '.plist'
		if os.path.exists(plistname):
			icnsname = tmp + '.icns'
			if not os.path.exists(icnsname):
				icnsname = None
		else:
			plistname = None
	# Start with copying the .app framework
	if not is_update:
		exceptlist = ["Contents/Info.plist", 
				"Contents/Resources/English.lproj/InfoPlist.strings", 
				"Contents/Resources/python.rsrc",
				]
		copyapptree(template, destname, exceptlist, progress)
	# Now either use the .plist file or the default
	if progress:
		progress.label('Create info.plist')
		progress.inc(0)
	if plistname:
		shutil.copy2(plistname, os.path.join(destname, 'Contents', 'Info.plist'))
		if icnsname:
			icnsdest = os.path.split(icnsname)[1]
			icnsdest = os.path.join(destname, 
				os.path.join('Contents', 'Resources', icnsdest))
			shutil.copy2(icnsname, icnsdest)
		# XXXX Wrong. This should be parsed from plist file. Also a big hack:-)
		if shortname == 'PythonIDE':
			ownertype = 'Pide'
		else:
			ownertype = 'PytA'
		# XXXX Should copy .icns file
	else:
		cocoainfo = ''
		for o in others:
			if o[-4:] == '.nib':
				nibname = os.path.split(o)[1][:-4]
				cocoainfo = """
        <key>NSMainNibFile</key>
        <string>%s</string>
        <key>NSPrincipalClass</key>
        <string>NSApplication</string>""" % nibname
			elif o[-6:] == '.lproj':
				files = os.listdir(o)
				for f in files:
					if f[-4:] == '.nib':
						nibname = os.path.split(f)[1][:-4]
						cocoainfo = """
        <key>NSMainNibFile</key>
        <string>%s</string>
        <key>NSPrincipalClass</key>
        <string>NSApplication</string>""" % nibname

		plistname = os.path.join(template, 'Contents', 'Resources', 'Applet-Info.plist')
		plistdata = open(plistname).read()
		plistdata = plistdata % {'appletname':shortname, 'cocoainfo':cocoainfo}
		ofp = open(os.path.join(destname, 'Contents', 'Info.plist'), 'w')
		ofp.write(plistdata)
		ofp.close()
		ownertype = 'PytA'
	# Create the PkgInfo file
	if progress:
		progress.label('Create PkgInfo')
		progress.inc(0)
	ofp = open(os.path.join(destname, 'Contents', 'PkgInfo'), 'wb')
	ofp.write('APPL' + ownertype)
	ofp.close()
		
	
	if progress:
		progress.label("Copy resources...")
		progress.set(20)
	resfilename = 'python.rsrc'  # XXXX later: '%s.rsrc' % shortname
	try:
		output = Res.FSOpenResourceFile(
				os.path.join(destname, 'Contents', 'Resources', resfilename), 
				u'', WRITE)
	except MacOS.Error:
		fsr, dummy = Res.FSCreateResourceFile(
				os.path.join(destname, 'Contents', 'Resources'), 
				unicode(resfilename), '')
		output = Res.FSOpenResourceFile(fsr, u'', WRITE)
	
	# Copy the resources from the target specific resource template, if any
	typesfound, ownertype = [], None
	try:
		input = macresource.open_pathname(rsrcname)
	except (MacOS.Error, ValueError):
		pass
		if progress:
			progress.inc(50)
	else:
		typesfound, ownertype = copyres(input, output, [], 0, progress)
		Res.CloseResFile(input)
	
	# Check which resource-types we should not copy from the template
	skiptypes = []
##	if 'vers' in typesfound: skiptypes.append('vers')
##	if 'SIZE' in typesfound: skiptypes.append('SIZE')
##	if 'BNDL' in typesfound: skiptypes = skiptypes + ['BNDL', 'FREF', 'icl4', 
##			'icl8', 'ics4', 'ics8', 'ICN#', 'ics#']
##	if not copy_codefragment:
##		skiptypes.append('cfrg')
##	skipowner = (ownertype <> None)
	
	# Copy the resources from the template
	
	input = Res.FSOpenResourceFile(
			os.path.join(template, 'Contents', 'Resources', 'python.rsrc'), u'', READ)
	if progress:
		progress.label("Copy standard resources...")
		progress.inc(0)
##	dummy, tmplowner = copyres(input, output, skiptypes, 1, progress)
	dummy, tmplowner = copyres(input, output, skiptypes, 1, None)
		
	Res.CloseResFile(input)
##	if ownertype == None:
##		raise BuildError, "No owner resource found in either resource file or template"
	# Make sure we're manipulating the output resource file now
	
	Res.CloseResFile(output)

	if code:
		if raw:
			pycname = '__rawmain__.pyc'
		else:
			pycname = '__main__.pyc'
			# And we also create __rawmain__.pyc
			outputfilename = os.path.join(destname, 'Contents', 'Resources', '__rawmain__.pyc')
			if progress:
				progress.label('Creating __rawmain__.pyc')
				progress.inc(0)
			rawsourcefile = os.path.join(sys.prefix, 'Mac', 'Lib', 'appletrawmain.py')
			rawsource = open(rawsourcefile, 'rU').read()
			rawcode = compile(rawsource, rawsourcefile, 'exec')
			writepycfile(rawcode, outputfilename)
			
		outputfilename = os.path.join(destname, 'Contents', 'Resources', pycname)
		if progress:
			progress.label('Creating '+pycname)
			progress.inc(0)
		writepycfile(code, outputfilename)
	# Copy other files the user asked for
	for osrc in others:
		oname = os.path.split(osrc)[1]
		odst = os.path.join(destname, 'Contents', 'Resources', oname)
		if progress: 
			progress.label('Copy ' + oname)
			progress.inc(0)
		if os.path.isdir(osrc):
			copyapptree(osrc, odst)
		else:
			shutil.copy2(osrc, odst)
	if progress: 
		progress.label('Done.')
		progress.inc(0)
	
##	macostools.touched(dest_fss)

# Copy resources between two resource file descriptors.
# skip a resource named '__main__' or (if skipowner is set) with ID zero.
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

			if lcname == OWNERNAME and id == 0:
				if skipowner:
					continue # Skip this one
				else:
					ctor = type
			size = res.size
			attrs = res.GetResAttrs()
			if progress:
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
				if progress:
					progress.label("Overwrite %s %d %s"%(type, id, name))
					progress.inc(0)
				res2.RemoveResource()
			res.AddResource(type, id, name)
			res.WriteResource()
			attrs = attrs | res.GetResAttrs()
			res.SetResAttrs(attrs)
			Res.UseResFile(input)
	return alltypes, ctor

def copyapptree(srctree, dsttree, exceptlist=[], progress=None):
	names = []
	if os.path.exists(dsttree):
		shutil.rmtree(dsttree)
	os.mkdir(dsttree)
	todo = os.listdir(srctree)
	while todo:
		this, todo = todo[0], todo[1:]
		if this in exceptlist:
			continue
		thispath = os.path.join(srctree, this)
		if os.path.isdir(thispath):
			thiscontent = os.listdir(thispath)
			for t in thiscontent:
				todo.append(os.path.join(this, t))
		names.append(this)
	for this in names:
		srcpath = os.path.join(srctree, this)
		dstpath = os.path.join(dsttree, this)
		if os.path.isdir(srcpath):
			os.mkdir(dstpath)
		else:
			if progress:
				progress.label('Copy '+this)
				progress.inc(0)
			shutil.copy2(srcpath, dstpath)
			
def writepycfile(codeobject, cfile):
	import marshal
	fc = open(cfile, 'wb')
	fc.write('\0\0\0\0') # MAGIC placeholder, written later
	fc.write('\0\0\0\0') # Timestap placeholder, not needed
	marshal.dump(codeobject, fc)
	fc.flush()
	fc.seek(0, 0)
	fc.write(MAGIC)
	fc.close()


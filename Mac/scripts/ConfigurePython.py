# This python script creates Finder aliases for all the
# dynamically-loaded modules that "live in" in a single
# shared library.
#
# This is sort-of a merger between Jack's MkPluginAliases
# and Guido's mkaliases.
#
# Jack Jansen, CWI, August 1996

import sys
import os
import macfs

ppc_goals = [
	("AE.ppc.slb", "toolboxmodules.ppc.slb"),
	("Ctl.ppc.slb", "toolboxmodules.ppc.slb"),
	("Dlg.ppc.slb", "toolboxmodules.ppc.slb"),
	("Evt.ppc.slb", "toolboxmodules.ppc.slb"),
	("Fm.ppc.slb", "toolboxmodules.ppc.slb"),
	("Menu.ppc.slb", "toolboxmodules.ppc.slb"),
	("List.ppc.slb", "toolboxmodules.ppc.slb"),
	("Qd.ppc.slb", "toolboxmodules.ppc.slb"),
	("Res.ppc.slb", "toolboxmodules.ppc.slb"),
	("Scrap.ppc.slb", "toolboxmodules.ppc.slb"),
	("Snd.ppc.slb", "toolboxmodules.ppc.slb"),
	("TE.ppc.slb", "toolboxmodules.ppc.slb"),
	("Win.ppc.slb", "toolboxmodules.ppc.slb"),

	("Cm.ppc.slb", "qtmodules.ppc.slb"),
	("Qt.ppc.slb", "qtmodules.ppc.slb"),

	("imgcolormap.ppc.slb", "imgmodules.ppc.slb"),
	("imgformat.ppc.slb", "imgmodules.ppc.slb"),
	("imggif.ppc.slb", "imgmodules.ppc.slb"),
	("imgjpeg.ppc.slb", "imgmodules.ppc.slb"),
	("imgop.ppc.slb", "imgmodules.ppc.slb"),
	("imgpbm.ppc.slb", "imgmodules.ppc.slb"),
	("imgpgm.ppc.slb", "imgmodules.ppc.slb"),
	("imgppm.ppc.slb", "imgmodules.ppc.slb"),
	("imgtiff.ppc.slb", "imgmodules.ppc.slb"),
	("imgsgi.ppc.slb", "imgmodules.ppc.slb")
]

cfm68k_goals = [
	("AE.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Ctl.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Dlg.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Evt.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Fm.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Menu.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("List.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Qd.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Res.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Scrap.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Snd.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("TE.CFM68K.slb", "toolboxmodules.CFM68K.slb"),
	("Win.CFM68K.slb", "toolboxmodules.CFM68K.slb"),

	("Cm.CFM68K.slb", "qtmodules.CFM68K.slb"),
	("Qt.CFM68K.slb", "qtmodules.CFM68K.slb"),

	("imgcolormap.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgformat.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imggif.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgjpeg.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgop.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgpbm.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgpgm.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgppm.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgtiff.CFM68K.slb", "imgmodules.CFM68K.slb"),
	("imgsgi.CFM68K.slb", "imgmodules.CFM68K.slb")
]

def gotopluginfolder():
	"""Go to the plugin folder, assuming we are somewhere in the Python tree"""
	import os
	
	while not os.path.isdir(":Plugins"):
		os.chdir("::")
	os.chdir(":Plugins")
	print "current directory is", os.getcwd()
	
def loadtoolboxmodules():
	"""Attempt to load the Res module"""
	try:
		import Res
	except ImportError, arg:
		err1 = arg
		pass
	else:
		print 'imported Res the standard way.'
		return
	
	# We cannot import it. First attempt to load the cfm68k version
	import imp
	try:
		dummy = imp.load_dynamic('Res', 'toolboxmodules.CFM68K.slb')
	except ImportError, arg:
		err2 = arg
		pass
	else:
		print 'Loaded Res from toolboxmodules.CFM68K.slb.'
		return
		
	# Ok, try the ppc version
	try:
		dummy = imp.load_dynamic('Res', 'toolboxmodules.ppc.slb')
	except ImportError, arg:
		err3 = arg
		pass
	else:
		print 'Loaded Res from toolboxmodules.ppc.slb.'
		return
	
	# Tough luck....
	print "I cannot import the Res module, nor load it from either of"
	print "toolboxmodules shared libraries. The errors encountered were:"
	print "import Res:", err1
	print "load from toolboxmodules.CFM68K.slb:", err2
	print "load from toolboxmodules.ppc.slb:", err3
	sys.exit(1)
	
def getextensiondirfile(fname):
	import macfs
	import MACFS
	vrefnum, dirid = macfs.FindFolder(MACFS.kOnSystemDisk, MACFS.kExtensionFolderType, 0)
	fss = macfs.FSSpec((vrefnum, dirid, fname))
	return fss.as_pathname()
	
def mkcorealias(src, altsrc):
	import string
	import macostools
	version = string.split(sys.version)[0]
	dst = getextensiondirfile(src+ ' ' + version)
	if not os.path.exists(src):
		if not os.path.exists(altsrc):
			print '*', src, 'not found'
			return 0
		src = altsrc
	try:
		os.unlink(dst)
	except os.error:
		pass
	macostools.mkalias(src, dst)
	print ' ', dst, '->', src
	return 1
	

def main():
	gotopluginfolder()
	
	loadtoolboxmodules()
	
	import macostools
		
	# Remove old .slb aliases and collect a list of .slb files
	LibFiles = []
	allfiles = os.listdir(':')
	print 'Removing old aliases...'
	for f in allfiles:
		if f[-4:] == '.slb':
			finfo = macfs.FSSpec(f).GetFInfo()
			if finfo.Flags & 0x8000:
				print '  Removing', f
				os.unlink(f)
			else:
				LibFiles.append(f)
				print '  Found', f
	print
	
	# Create the new PPC aliases.
	print 'Creating PPC aliases...'
	for dst, src in ppc_goals:
		if src in LibFiles:
			macostools.mkalias(src, dst)
			print ' ', dst, '->', src
		else:
			print '*', dst, 'not created:', src, 'not found'
	print
	
	# Create the CFM68K aliases.
	print 'Creating CFM68K aliases...'
	for dst, src in cfm68k_goals:
		if src in LibFiles:
			macostools.mkalias(src, dst)
			print ' ', dst, '->', src
		else:
			print '*', dst, 'not created:', src, 'not found'
	print
			
	# Create the PythonCore alias(es)
	print 'Creating PythonCore aliases in Extensions folder...'
	os.chdir('::')
	n = 0
	n = n + mkcorealias('PythonCore', 'PythonCore')
	n = n + mkcorealias('PythonCorePPC', ':build.macppc.shared:PythonCorePPC')
	n = n + mkcorealias('PythonCoreCFM68K', ':build.mac68k.shared:PythonCoreCFM68K')
	if n == 0:
		sys.exit(1)
			
if __name__ == '__main__':
	main()

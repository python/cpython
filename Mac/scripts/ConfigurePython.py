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
import MacOS
verbose=0

SPLASH_LOCATE=512
SPLASH_REMOVE=513
SPLASH_CFM68K=514
SPLASH_PPC=515
SPLASH_NUMPY=516
ALERT_NONBOOT=517
ALERT_NONBOOT_COPY=1
ALERT_NONBOOT_ALIAS=2

ppc_goals = [
]

def gotopluginfolder():
	"""Go to the plugin folder, assuming we are somewhere in the Python tree"""
	import os
	
	while not os.path.isdir(":Mac:PlugIns"):
		os.chdir("::")
	os.chdir(":Mac:PlugIns")
	if verbose: print "current directory is", os.getcwd()
	
def loadtoolboxmodules():
	"""Attempt to load the Res module"""
	try:
		import Res
	except ImportError, arg:
		err1 = arg
		pass
	else:
		if verbose: print 'imported Res the standard way.'
		return
	
	# We cannot import it. Try a manual load
	try:
		dummy = imp.load_dynamic('Res', 'toolboxmodules.ppc.slb')
	except ImportError, arg:
		err3 = arg
		pass
	else:
		if verbose:  print 'Loaded Res from toolboxmodules.ppc.slb.'
		return
	
	# Tough luck....
	print "I cannot import the Res module, nor load it from either of"
	print "toolboxmodules shared libraries. The errors encountered were:"
	print "import Res:", err1
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
	if not os.path.exists(os.path.join(sys.exec_prefix, src)):
		if not os.path.exists(os.path.join(sys.exec_prefix, altsrc)):
			if verbose:  print '*', src, 'not found'
			return 0
		src = altsrc
	try:
		os.unlink(dst)
	except os.error:
		pass
	do_copy = 0
	if macfs.FSSpec(sys.exec_prefix).as_tuple()[0] != -1: # XXXX
		try:
			import Dlg
			rv = Dlg.CautionAlert(ALERT_NONBOOT, None)
			if rv == ALERT_NONBOOT_COPY:
				do_copy = 1
		except ImportError:
			pass
	if do_copy:
		macostools.copy(os.path.join(sys.exec_prefix, src), dst)
	else:
		macostools.mkalias(os.path.join(sys.exec_prefix, src), dst)
	if verbose:  print ' ', dst, '->', src
	return 1
	

def main():
	MacOS.splash(SPLASH_LOCATE)
	gotopluginfolder()
	
	loadtoolboxmodules()
	
	sys.path.append('::Mac:Lib')
	import macostools
		
	# Remove old .slb aliases and collect a list of .slb files
	didsplash = 0
	LibFiles = []
	allfiles = os.listdir(':')
	if verbose:  print 'Removing old aliases...'
	for f in allfiles:
		if f[-4:] == '.slb':
			finfo = macfs.FSSpec(f).GetFInfo()
			if finfo.Flags & 0x8000:
				if not didsplash:
					MacOS.splash(SPLASH_REMOVE)
					didsplash = 1
				if verbose:  print '  Removing', f
				os.unlink(f)
			else:
				LibFiles.append(f)
				if verbose:  print '  Found', f
	if verbose:  print
	
	# Create the new PPC aliases.
	didsplash = 0
	if verbose:  print 'Creating PPC aliases...'
	for dst, src in ppc_goals:
		if src in LibFiles:
			if not didsplash:
				MacOS.splash(SPLASH_PPC)
				didsplash = 1
			macostools.mkalias(src, dst)
			if verbose:  print ' ', dst, '->', src
		else:
			if verbose:  print '*', dst, 'not created:', src, 'not found'
	if verbose:  print
				
	# Create the PythonCore alias(es)
	if verbose:  print 'Creating PythonCore aliases in Extensions folder...'
	os.chdir('::')
	n = 0
	n = n + mkcorealias('PythonCore', 'PythonCore')
	n = n + mkcorealias('PythonCoreCarbon', 'PythonCoreCarbon')
	
	if verbose and n == 0:
		sys.exit(1)
			
if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == '-v':
		verbose = 1
	main()

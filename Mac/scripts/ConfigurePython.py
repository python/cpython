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

SPLASH_COPYCORE=512
SPLASH_COPYCARBON=513
SPLASH_COPYCLASSIC=514
SPLASH_BUILDAPPLETS=515

ALERT_NOCORE=516
ALERT_NONBOOT=517
ALERT_NONBOOT_COPY=1
ALERT_NONBOOT_ALIAS=2

APPLET_LIST=[
		(":Mac:scripts:EditPythonPrefs.py", "EditPythonPrefs", None),
		(":Mac:scripts:BuildApplet.py", "BuildApplet", None),
		(":Mac:scripts:BuildApplication.py", "BuildApplication", None),
##		(":Mac:scripts:ConfigurePython.py", "ConfigurePython", None),
##		(":Mac:scripts:ConfigurePython.py", "ConfigurePythonCarbon", "PythonInterpreterCarbon"),
##		(":Mac:scripts:ConfigurePython.py", "ConfigurePythonClassic", "PythonInterpreterClassic"),
		(":Mac:Tools:IDE:PythonIDE.py", "Python IDE", None),
		(":Mac:Tools:CGI:PythonCGISlave.py", ":Mac:Tools:CGI:PythonCGISlave", None),
		(":Mac:Tools:CGI:BuildCGIApplet.py", ":Mac:Tools:CGI:BuildCGIApplet", None),
]

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
	return 1

# Copied from fullbuild, should probably go to buildtools
def buildapplet(top, dummy, list):
	"""Create python applets"""
	import buildtools
	for src, dst, tmpl in list:
		template = buildtools.findtemplate(tmpl)
		if src[-3:] != '.py':
			raise 'Should end in .py', src
		base = os.path.basename(src)
		src = os.path.join(top, src)
		dst = os.path.join(top, dst)
		try:
			os.unlink(dst)
		except os.error:
			pass
		try:
			buildtools.process(template, src, dst, 1)
		except buildtools.BuildError, arg:
			print '**', dst, arg
		
def buildcopy(top, dummy, list):
	import macostools
	for src, dst in list:
		src = os.path.join(top, src)
		dst = os.path.join(top, dst)
		macostools.copy(src, dst)

def main():
	os.chdir(sys.prefix)
	
	sys.path.append('::Mac:Lib')
	import macostools
				
	# Create the PythonCore alias(es)
	MacOS.splash(SPLASH_COPYCORE)
	n = 0
	n = n + mkcorealias('PythonCore', 'PythonCore')
	n = n + mkcorealias('PythonCoreCarbon', 'PythonCoreCarbon')
	if n == 0:
		Dlg.CautionAlert(ALERT_NOCORE, None)
		return
	if sys.argv[0][-7:] == 'Classic':
		do_classic = 1
	elif sys.argv[0][-6:] == 'Carbon':
		do_classic = 0
	elif sys.argv[0][-15:] == 'ConfigurePython' or sys.argv[0][-18:] == 'ConfigurePython.py':
		return
	else:
		print "I don't know the sys.argv[0] function", sys.argv[0]
		sys.exit(1)
	if do_classic:
		MacOS.splash(SPLASH_COPYCLASSIC)
		buildcopy(sys.prefix, None, [("PythonInterpreterClassic", "PythonInterpreter")])
	else:
		MacOS.splash(SPLASH_COPYCARBON)
		buildcopy(sys.prefix, None, [("PythonInterpreterCarbon", "PythonInterpreter")])
	MacOS.splash(SPLASH_BUILDAPPLETS)
	buildapplet(sys.prefix, None, APPLET_LIST)

if __name__ == '__main__':
	main()

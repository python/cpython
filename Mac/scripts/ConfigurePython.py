# This python script creates Finder aliases for all the
# dynamically-loaded modules that "live in" in a single
# shared library.
# It needs a fully functional non-dynamic python to work
# (since it creates aliases to stuff it needs itself),
# you should probably drag it onto your non-dynamic python.
# 
# If you compare it to MkPluginAliases.as it also serves
# as a comparison between python and AppleScript:-)
#
# Jack Jansen, CWI, August 1995

import sys

def help():
	print"""
Try the following:
1. Remove any old "Python Preferences" files from the system folder.
2. Remove any old "PythonCore" files from the system folder.
3. Make sure this script, PythonPPC and PythonCore are all located in the
   python home folder (where the Lib and PlugIns folders are)
4. Run this script again, by dropping it on PythonPPC.

If this fails try removing starting afresh from the distribution archive.
"""
	sys.exit(1)
	
try:
	import os
except ImportError:
	print """
I cannot import the 'os' module, so something is wrong with sys.path
"""
	help()
	
try:
	import Res
except ImportError:
	#
	# Check that we are actually in the main python directory
	#
	try:
		os.chdir(':PlugIns')
	except IOError:
		print """
I cannot find the 'PlugIns' folder, so I am obviously not run from the Python
home folder.
"""
		help()
	import imp
	cwd = os.getcwd()
	tblibname = os.path.join(cwd, "toolboxmodules.slb")
	if not os.exists(tblibname):
		print """
I cannot find the 'toolboxmodules.slb' file in the PlugIns directory.
Start afresh from a clean distribution. 
"""
		sys.exit(1)
	try:
		for wtd in ["Ctl", "Dlg", "Evt", "Qd", "Res", "Win"]:
			imp.load_dynamic_module(wtd, tblibname, None)
	except ImportError:
		print """
I cannot load the toolbox modules by hand. Are you sure you are
using a PowerPC mac?
"""
		sys.exit(1)
		

import macfs
import EasyDialogs
import macostools

goals = [
	("mactcp.slb", "mactcpmodules.slb"),
	("macdnr.slb", "mactcpmodules.slb"),
	("AE.slb", "toolboxmodules.slb"),
	("Ctl.slb", "toolboxmodules.slb"),
	("Dlg.slb", "toolboxmodules.slb"),
	("Evt.slb", "toolboxmodules.slb"),
	("Menu.slb", "toolboxmodules.slb"),
	("List.slb", "toolboxmodules.slb"),
	("Qd.slb", "toolboxmodules.slb"),
	("Res.slb", "toolboxmodules.slb"),
	("Snd.slb", "toolboxmodules.slb"),
	("Win.slb", "toolboxmodules.slb"),
	("imgcolormap.slb", "imgmodules.slb"),
	("imgformat.slb", "imgmodules.slb"),
	("imggif.slb", "imgmodules.slb"),
	("imgjpeg.slb", "imgmodules.slb"),
	("imgop.slb", "imgmodules.slb"),
	("imgpgm.slb", "imgmodules.slb"),
	("imgppm.slb", "imgmodules.slb"),
	("imgtiff.slb", "imgmodules.slb")
]


def main():
	# Ask the user for the plugins directory
	dir, ok = macfs.GetDirectory('Where is the PlugIns folder?')
	if not ok: sys.exit(0)
	os.chdir(dir.as_pathname())
	
	# Remove old .slb aliases and collect a list of .slb files
	if EasyDialogs.AskYesNoCancel('Proceed with removing old aliases?') <= 0:
		sys.exit(0)
	LibFiles = []
	allfiles = os.listdir(':')
	for f in allfiles:
		if f[-4:] == '.slb':
			finfo = macfs.FSSpec(f).GetFInfo()
			if finfo.Flags & 0x8000:
				os.unlink(f)
			else:
				LibFiles.append(f)
	
	print LibFiles
	# Create the new aliases.
	if EasyDialogs.AskYesNoCancel('Proceed with creating new ones?') <= 0:
		sys.exit(0)
	for dst, src in goals:
		if src in LibFiles:
			macostools.mkalias(src, dst)
		else:
			EasyDialogs.Message(dst+' not created: '+src+' not found')
			
	EasyDialogs.Message('All done!')
			
if __name__ == '__main__':
	main()

# This python script creates Finder aliases for all the
# dynamically-loaded modules that "live in" in a single
# shared library.
# It needs a fully functional non-dynamic python to work
# but you can run it in a shared python as long as you can point
# it to toolboxmodules.slb
#
# Jack Jansen, CWI, August 1995

import sys

def help():
	print"""
Try the following:
1. Remove any old "Python Preferences" files from the system folder.
2. Remove any old "PythonCore" or "PythonCoreCFM68K" files from the system folder.
3. Make sure this script, your interpreter and your PythonCore are all located in the
   same folder.
4. Run this script again, by dropping it on your interpreter.

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
	import macfs
	#
	# Check that we are actually in the main python directory
	#
	fss, ok = macfs.StandardGetFile('Where are the toolbox modules?', 'shlb')
	tblibname = fss.as_pathname()
	try:
		for wtd in ["Ctl", "Dlg", "Evt", "Qd", "Res", "Win"]:
			imp.load_dynamic(wtd, tblibname)
	except ImportError:
		print """
I cannot load the toolbox modules by hand. Are you sure you are
using a PowerPC mac?
"""
		sys.exit(1)
		

import macfs
import EasyDialogs
import macostools

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
	if EasyDialogs.AskYesNoCancel('Proceed with creating PPC aliases?') > 0:
		for dst, src in ppc_goals:
			if src in LibFiles:
				macostools.mkalias(src, dst)
			else:
				EasyDialogs.Message(dst+' not created: '+src+' not found')
	if EasyDialogs.AskYesNoCancel('Proceed with creating CFM68K aliases?') > 0:
		for dst, src in cfm68k_goals:
			if src in LibFiles:
				macostools.mkalias(src, dst)
			else:
				EasyDialogs.Message(dst+' not created: '+src+' not found')
			
	EasyDialogs.Message('All done!')
			
if __name__ == '__main__':
	main()

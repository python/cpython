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

import os
import macfs
import sys

try:
	import Res
except ImportError:
	print """
Res module not found, which probably means that you are trying
to execute this script with a dynamically-linked python. This will
not work, since the whole point of the script is to create aliases
for dynamically-linked python to use. Do one of the following:
- Run this script using a non-dynamic python
- Use MkPluginAliases.as (an AppleScript)
- Create the aliases by hand (see the source for a list)."""
	sys.exit(1)

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

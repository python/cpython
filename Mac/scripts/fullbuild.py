#
# fullbuild creates everything that needs to be created before a
# distribution can be made, and puts it all in the right place.
#
# It expects the projects to be in the places where Jack likes them:
# in directories named like 'build.macppc.shared'. That is fixable,
# however.
#
# NOTE: You should proably make a copy of python with which to execute this
# script, rebuilding running programs does not work...

import os
import sys
import macfs

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('AE')
import aetools
from Metrowerks_Shell_Suite import Metrowerks_Shell_Suite
from Required_Suite import Required_Suite 

addpack.addpack('Mac')
addpack.addpack('scripts')
import mkapplet

class MwShell(aetools.TalkTo, Metrowerks_Shell_Suite, Required_Suite):
	pass


def buildmwproject(top, creator, projects):
	"""Build projects with an MW compiler"""
	print 'Please start project mgr with signature', creator,'-'
	sys.stdin.readline()
	try:
		mgr = MwShell(creator)
	except 'foo':
		print 'Not handled:', creator
		return
	for file in projects:
		file = os.path.join(top, file)
		fss = macfs.FSSpec(file)
		print 'Building', file
		mgr.open(fss)
		mgr.Make_Project()
		mgr.Close_Project()
	mgr.quit()
	
def buildapplet(top, dummy, list):
	"""Create a PPC python applet"""
	template = mkapplet.findtemplate()
	for src in list:
		if src[-3:] != '.py':
			raise 'Should end in .py', src
		base = os.path.basename(src)
		dst = os.path.join(top, base)[:-3]
		src = os.path.join(top, src)
		try:
			os.unlink(dst)
		except os.error:
			pass
		print 'Building applet', dst
		mkapplet.process(template, src, dst)

#
# The build instructions. Entries are (routine, arg, list-of-files)
# XXXX We could also include the builds for stdwin and such here...
INSTRUCTIONS=[
	(buildmwproject, "MPCC", [
		":build.macppc.shared:PythonCore.µ",
		":build.macppc.shared:PythonPPC.µ",
		":build.macppc.shared:PythonApplet.µ",

		":PlugIns:ctbmodule.µ",
		":PlugIns:imgmodules.µ",
		":PlugIns:macspeechmodule.µ",
		":PlugIns:mactcpmodules.µ",
		":PlugIns:stdwinmodule.µ",
		":PlugIns:toolboxmodules.µ",
	]),
	(buildmwproject, "MMCC", [
		":build.mac68k.stand:Python68K.µ",
	]),
	(buildapplet, None, [
		":Mac:scripts:EditPythonPrefs.py",
		":Mac:scripts:mkapplet.py",
		":Mac:scripts:RunLibScript.py"
	])
]
				
def main():
	dir, ok = macfs.GetDirectory('Python source folder:')
	if not ok:
		sys.exit(0)
	dir = dir.as_pathname()
	for routine, arg, list in INSTRUCTIONS:
		routine(dir, arg, list)
	print "All done!"
	sys.exit(1)	
	
if __name__ == '__main__':
	main()
	

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
import MacOS
import EasyDialogs

import addpack
import aetools
import AppleEvents
from Metrowerks_Shell_Suite import Metrowerks_Shell_Suite
from Required_Suite import Required_Suite 

addpack.addpack('Mac')
addpack.addpack('scripts')
import mkapplet

class MwShell(aetools.TalkTo, Metrowerks_Shell_Suite, Required_Suite):
	pass

RUNNING=[]

def buildmwproject(top, creator, projects):
	"""Build projects with an MW compiler"""
	if not creator in RUNNING:
		print 'Please start project mgr with signature', creator,'-'
		sys.stdin.readline()
		RUNNING.append(creator)
	try:
		mgr = MwShell(creator)
	except 'foo':
		print 'Not handled:', creator
		return
	mgr.send_timeout = AppleEvents.kNoTimeOut
	
	for file in projects:
		file = os.path.join(top, file)
		fss = macfs.FSSpec(file)
		print 'Building', file
		mgr.open(fss)
		try:
			mgr.Make_Project()
		except MacOS.Error, arg:
			print '** Failed. Possible error:', arg
		mgr.Close_Project()
##	mgr.quit()
	
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
PPC_INSTRUCTIONS=[
	(buildmwproject, "CWIE", [
		":build.macppc.shared:PythonCore.µ",
		":build.macppc.shared:PythonPPC.µ",
		":build.macppc.shared:PythonApplet.µ",
	])
]
PLUGIN_INSTRUCTIONS=[
	(buildmwproject, "CWIE", [
		":PlugIns:ctbmodule.µ",
		":PlugIns:imgmodules.µ",
		":PlugIns:macspeechmodule.µ",
		":PlugIns:toolboxmodules.µ",
		":PlugIns:wastemodule.µ",
		":PlugIns:_tkintermodule.µ",
	])
]
M68K_INSTRUCTIONS=[
	(buildmwproject, "CWIE", [
		":build.mac68k.stand:Python68K.µ",
	])
]
APPLET_INSTRUCTIONS=[
	(buildapplet, None, [
		":Mac:scripts:EditPythonPrefs.py",
		":Mac:scripts:mkapplet.py",
		":Mac:scripts:RunLibScript.py",
		":Mac:scripts:MkPluginAliases.py"
	])
]

ALLINST=[
	("PPC shared executable", PPC_INSTRUCTIONS),
	("PPC plugin modules", PLUGIN_INSTRUCTIONS),
	("68K executable", M68K_INSTRUCTIONS),
	("PPC applets", APPLET_INSTRUCTIONS)
]
				
def main():
	dir, ok = macfs.GetDirectory('Python source folder:')
	if not ok:
		sys.exit(0)
	dir = dir.as_pathname()
	INSTRUCTIONS = []
	for string, inst in ALLINST:
		answer = EasyDialogs.AskYesNoCancel("Build %s?"%string, 1)
		if answer < 0:
			sys.exit(0)
		if answer:
			INSTRUCTIONS = INSTRUCTIONS + inst
	for routine, arg, list in INSTRUCTIONS:
		routine(dir, arg, list)
	print "All done!"
	sys.exit(1)	
	
if __name__ == '__main__':
	main()
	

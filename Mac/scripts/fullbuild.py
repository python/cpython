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

import Res
import Dlg

import mkapplet
import cfmfile

# Dialog resource. Note that the item numbers should correspond
# to those in the DITL resource. Also note that the order is important:
# things are built in this order, so there should be no forward dependencies.
DIALOG_ID = 512

I_OK=1
I_CANCEL=2

I_PPC_CORE=3
I_PPC_PLUGINS=4
I_PPC_EXTENSIONS=5
I_68K_CORE=6
I_68K_PLUGINS=7
I_68K_EXTENSIONS=8
I_PPC_FULL=9
I_PPC_SMALL=10
I_68K_FULL=11
I_68K_SMALL=12
I_FAT=13
I_APPLETS=14

N_BUTTONS=15

class MwShell(aetools.TalkTo, Metrowerks_Shell_Suite, Required_Suite):
	pass

RUNNING=[]

def buildmwproject(top, creator, projects):
	"""Build projects with an MW compiler"""
	mgr = MwShell(creator, start=1)
	mgr.send_timeout = AppleEvents.kNoTimeOut
	
	for file in projects:
		file = os.path.join(top, file)
		fss = macfs.FSSpec(file)
		print 'Building', file
		mgr.open(fss)
		try:
			mgr.Make_Project()
		except aetools.Error, arg:
			print '** Failed:', arg
		mgr.Close_Project()
##	mgr.quit()
	
def buildapplet(top, dummy, list):
	"""Create python applets"""
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
		
def buildfat(top, dummy, list):
	"""Build fat binaries"""
	for dst, src1, src2 in list:
		dst = os.path.join(top, dst)
		src1 = os.path.join(top, src1)
		src2 = os.path.join(top, src2)
		print 'Building fat binary', dst
		cfmfile.mergecfmfiles((src1, src2), dst)
		
def handle_dialog():
	"""Handle selection dialog, return list of selected items"""
	d = Dlg.GetNewDialog(DIALOG_ID, -1)
	d.SetDialogDefaultItem(I_OK)
	d.SetDialogCancelItem(I_CANCEL)
	results = [0]*N_BUTTONS
	while 1:
		n = Dlg.ModalDialog(None)
		if n == I_OK:
			break
		if n == I_CANCEL:
			return []
		if n < len(results):
			results[n] = (not results[n])
			tp, h, rect = d.GetDialogItem(n)
			h.as_Control().SetControlValue(results[n])
	rv = []
	for i in range(len(results)):
		if results[i]:
			rv.append(i)
	return rv

#
# The build instructions. Entries are (routine, arg, list-of-files)
# XXXX We could also include the builds for stdwin and such here...
BUILD_DICT = {
I_PPC_CORE : (buildmwproject, "CWIE", [
		":build.macppc.shared:PythonCorePPC.µ",
		":build.macppc.shared:PythonPPC.µ",
		":build.macppc.shared:PythonAppletPPC.µ",
	]),

I_68K_CORE : (buildmwproject, "CWIE", [
		":build.mac68k.shared:PythonCoreCFM68K.µ",
		":build.mac68k.shared:PythonCFM68K.µ",
		":build.mac68k.shared:PythonAppletCFM68K.µ",
	]),

I_PPC_PLUGINS : (buildmwproject, "CWIE", [
		":PlugIns:ctb.ppc.µ",
		":PlugIns:gdbm.ppc.µ",
		":PlugIns:icglue.ppc.µ",
		":PlugIns:imgmodules.ppc.µ",
		":PlugIns:macspeech.ppc.µ",
		":PlugIns:toolboxmodules.ppc.µ",
		":PlugIns:qtmodules.ppc.µ",
		":PlugIns:waste.ppc.µ",
		":PlugIns:_tkinter.ppc.µ",
		":PlugIns:calldll.ppc.µ",
	]),

I_68K_PLUGINS : (buildmwproject, "CWIE", [
		":PlugIns:ctb.CFM68K.µ",
		":PlugIns:gdbm.CFM68K.µ",
		":PlugIns:icglue.CFM68K.µ",
		":PlugIns:imgmodules.CFM68K.µ",
		":PlugIns:toolboxmodules.CFM68K.µ",
		":PlugIns:qtmodules.CFM68K.µ",
		":PlugIns:waste.CFM68K.µ",
		":PlugIns:_tkinter.CFM68K.µ",
	]),

I_68K_FULL : (buildmwproject, "CWIE", [
		":build.mac68k.stand:Python68K.µ",
	]),
	
I_68K_SMALL : (buildmwproject, "CWIE", [
		":build.mac68k.stand:Python68Ksmall.µ",
	]),

I_PPC_FULL : (buildmwproject, "CWIE", [
		":build.macppc.stand:PythonStandalone.µ",
	]),

I_PPC_SMALL : (buildmwproject, "CWIE", [
		":build.macppc.stand:PythonStandSmall.µ",
	]),

I_PPC_EXTENSIONS : (buildmwproject, "CWIE", [
		":Extensions:Imaging:_imaging.ppc.µ",
		":Extensions:Imaging:_tkinter.ppc.µ",
		":Extensions:NumPy:numpymodules.ppc.µ",
	]),

I_68K_EXTENSIONS : (buildmwproject, "CWIE", [
		":Extensions:Imaging:_imaging.CFM68K.µ",
		":Extensions:Imaging:_tkinter.CFM68K.µ",
		":Extensions:NumPy:numpymodules.CFM68K.µ",
	]),

I_APPLETS : (buildapplet, None, [
		":Mac:scripts:EditPythonPrefs.py",
		":Mac:scripts:mkapplet.py",
		":Mac:scripts:MkPluginAliases.py"
	]),

I_FAT : (buildfat, None, [
			(":Python", ":build.macppc.shared:PythonPPC", 
						":build.mac68k.shared:PythonCFM68K"),
			(":PythonApplet", ":build.macppc.shared:PythonAppletPPC",
							  ":build.mac68k.shared:PythonAppletCFM68K")
	])
}
				
def main():
	try:
		h = Res.OpenResFile('fullbuild.rsrc')
	except Res.Error:
		pass	# Assume we already have acces to our own resource

	dir, ok = macfs.GetDirectory('Python source folder:')
	if not ok:
		sys.exit(0)
	dir = dir.as_pathname()
	
	todo = handle_dialog()
	
	instructions = []
	for i in todo:
		instructions.append(BUILD_DICT[i])
		
	for routine, arg, list in instructions:
		#routine(dir, arg, list)
		print routine, dir, arg, list # DBG
		
	print "All done!"
	sys.exit(1)	
	
if __name__ == '__main__':
	main()
	

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
from CodeWarrior_Standard_Suite import CodeWarrior_Standard_Suite
from Required_Suite import Required_Suite

import Res
import Dlg

import BuildApplet
import cfmfile

# Dialog resource. Note that the item numbers should correspond
# to those in the DITL resource. Also note that the order is important:
# things are built in this order, so there should be no forward dependencies.
DIALOG_ID = 512

I_OK=1
I_CANCEL=2

I_CORE=3
I_PPC_PLUGINS=4
I_PPC_EXTENSIONS=5
I_68K_PLUGINS=6
I_68K_EXTENSIONS=7
I_PPC_FULL=8
I_PPC_SMALL=9
I_68K_FULL=10
I_68K_SMALL=11
I_APPLETS=12

N_BUTTONS=13

class MwShell(Metrowerks_Shell_Suite, CodeWarrior_Standard_Suite,
				Required_Suite, aetools.TalkTo):
	pass

RUNNING=[]

def buildmwproject(top, creator, projects):
	"""Build projects with an MW compiler"""
	mgr = MwShell(creator, start=1)
	mgr.send_timeout = AppleEvents.kNoTimeOut
	
	for file in projects:
		if type(file) == type(()):
			file, target = file
		else:
			target = ''
		file = os.path.join(top, file)
		fss = macfs.FSSpec(file)
		print 'Building', file, target
		mgr.open(fss)
		if target:
			try:
				mgr.Set_Current_Target(target)
			except aetools.Error, arg:
				print '**', file, target, 'Cannot select:', arg
		try:
			mgr.Make_Project()
		except aetools.Error, arg:
			print '**', file, target, 'Failed:', arg
		mgr.Close_Project()
##	mgr.quit()
	
def buildapplet(top, dummy, list):
	"""Create python applets"""
	template = BuildApplet.findtemplate()
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
		BuildApplet.process(template, src, dst)
		
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
I_CORE : (buildmwproject, "CWIE", [
		(":build.mac:PythonCore.prj", "PythonCore"),
		(":build.mac:Python.prj", "PythonFAT"),
		(":build.mac:PythonApplet.prj", "PythonAppletFAT"),
	]),

I_PPC_PLUGINS : (buildmwproject, "CWIE", [
		(":PlugIns:PlugIns.prj",	"PlugIns.ppc"),
	]),

I_68K_PLUGINS : (buildmwproject, "CWIE", [
		(":PlugIns:PlugIns.prj",	"PlugIns.CFM68K"),
	]),

I_68K_FULL : (buildmwproject, "CWIE", [
		(":build.macstand:PythonStandalone.prj", "Python68K"),
	]),
	
I_68K_SMALL : (buildmwproject, "CWIE", [
		(":build.macstand:PythonStandSmall.prj", "PythonSmall68K"),
	]),

I_PPC_FULL : (buildmwproject, "CWIE", [
		(":build.macstand:PythonStandalone.prj", "PythonStandalone"),
	]),

I_PPC_SMALL : (buildmwproject, "CWIE", [
		(":build.macstand:PythonStandSmall.prj", "PythonStandSmall"),
	]),

I_PPC_EXTENSIONS : (buildmwproject, "CWIE", [
		(":Extensions:Imaging:_imaging.prj", "_imaging.ppc"),
		(":Extensions:Imaging:_tkinter.prj", "_tkinter.ppc"),
		(":Extensions:NumPy:numpymodules.prj", "numpymodules.ppc"),
	]),

I_68K_EXTENSIONS : (buildmwproject, "CWIE", [
		(":Extensions:Imaging:_imaging.prj", "_imaging.CFM68K"),
		(":Extensions:Imaging:_tkinter.prj", "_tkinter.CFM68K"),
		(":Extensions:NumPy:numpymodules.prj", "numpymodules.CFM68K"),
	]),

I_APPLETS : (buildapplet, None, [
		":Mac:scripts:EditPythonPrefs.py",
		":Mac:scripts:BuildApplet.py",
		":Mac:scripts:ConfigurePython.py"
	]),
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
		routine(dir, arg, list)
		
	print "All done!"
	sys.exit(1)	
	
if __name__ == '__main__':
	main()
	

# Scan <Controls.h>, generating ctlgen.py.
import addpack
addpack.addpack(':Tools:bgen:bgen')

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

def main():
	input = "Controls.h"
	output = "ctlgen.py"
	defsoutput = TOOLBOXDIR + "Controls.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now doing 'import ctlsupport' ==="
	import ctlsupport
	print "=== Done.  It's up to you to compile Ctlmodule.c ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("ControlHandle", "ControlRef") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("from TextEdit import *\n")
		self.defsfile.write("from QuickDraw import *\n")
		self.defsfile.write("\n")

	def makeblacklistnames(self):
		return [
			'DisposeControl', # Generated manually
			'KillControls', # Implied by close of dialog
			'SetCtlAction',
			'kControlBevelButtonCenterPopupGlyphTag', # Constant with funny definition
			'kControlProgressBarIndeterminateTag', # ditto
			# The following are unavailable for static 68k (appearance manager)
			'GetBevelButtonMenuValue',
			'SetBevelButtonMenuValue',
			'GetBevelButtonMenuHandle',
			'SetBevelButtonTransform',
			'SetBevelButtonGraphicAlignment',
			'SetBevelButtonTextAlignment',
			'SetBevelButtonTextPlacement',
			'SetImageWellTransform',
			'GetTabContentRect',
			'SetTabEnabled',
			'SetDisclosureTriangleLastValue',
			]

	def makeblacklisttypes(self):
		return [
			'ProcPtr',
			'ControlActionUPP',
			'ControlButtonContentInfoPtr',
			'Ptr',
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			
			# For TrackControl
			([("ProcPtr", "actionProc", "InMode")],
			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),
			([("ControlActionUPP", "actionProc", "InMode")],
			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),
			
			([("ControlHandle", "*", "OutMode")],
			 [("ExistingControlHandle", "*", "*")]),
			([("ControlRef", "*", "OutMode")],	# Ditto, for Universal Headers
			 [("ExistingControlHandle", "*", "*")]),
			]

if __name__ == "__main__":
	main()

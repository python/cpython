# Scan <Controls.h>, generating ctlgen.py.
import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

def main():
#	input = "Controls.h" # Universal Headers < 3.3
	input = ["Controls.h", "ControlDefinitions.h"] # Universal Headers >= 3.3
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
		self.defsfile.write("from Dragconst import *\n")
		self.defsfile.write("\n")

	def makeblacklistnames(self):
		return [
			'DisposeControl', # Generated manually
			'KillControls', # Implied by close of dialog
			'SetCtlAction',
			'TrackControl',	# Generated manually
			'HandleControlClick',	# Generated manually
			'SetControlData',	# Generated manually
			'GetControlData',	# Generated manually
			'kControlBevelButtonCenterPopupGlyphTag', # Constant with funny definition
			'kControlProgressBarIndeterminateTag', # ditto
			# The following are unavailable for static 68k (appearance manager)
##			'GetBevelButtonMenuValue',
##			'SetBevelButtonMenuValue',
##			'GetBevelButtonMenuHandle',
##			'SetBevelButtonTransform',
			'SetBevelButtonGraphicAlignment',
			'SetBevelButtonTextAlignment',
			'SetBevelButtonTextPlacement',
##			'SetImageWellTransform',
##			'GetTabContentRect',
##			'SetTabEnabled',
##			'SetDisclosureTriangleLastValue',
## 			# Unavailable in CW Pro 3 libraries
## 			'SetUpControlTextColor',
## 			# Unavailable in Jack's CW Pro 5.1 libraries
## 			'GetControlRegion',
## 			'RemoveControlProperty',
## 			'IsValidControlHandle',
## 			'SetControl32BitMinimum',
## 			'GetControl32BitMinimum',
## 			'SetControl32BitMaximum',
## 			'GetControl32BitMaximum',
## 			'SetControl32BitValue',
## 			'GetControl32BitValue',
## 			'SetControlViewSize',
## 			'GetControlViewSize',
			# Generally Bad News
			'GetControlProperty',
			'SetControlProperty',
			'GetControlPropertySize',
			'SendControlMessage', # Parameter changed from long to void* from UH3.3 to UH3.4
			]

	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'GetAuxiliaryControlRecord',
				'SetControlColor',
				# These have suddenly disappeared in UH 3.3.2...
##				'GetBevelButtonMenuValue',
##				'SetBevelButtonMenuValue',
##				'GetBevelButtonMenuHandle',
##				'SetBevelButtonTransform',
##				'SetImageWellTransform',
##				'GetTabContentRect',
##				'SetTabEnabled',
##				'SetDisclosureTriangleLastValue',
			]),
			('#if TARGET_API_MAC_CARBON', [
				'IsAutomaticControlDragTrackingEnabledForWindow',
				'SetAutomaticControlDragTrackingEnabledForWindow',
				'HandleControlDragReceive',
				'HandleControlDragTracking',
				'GetControlByID',
				'IsControlDragTrackingEnabled',
				'SetControlDragTrackingEnabled',
				'GetControlPropertyAttributes',
				'ChangeControlPropertyAttributes',
				'GetControlID',
				'SetControlID',
				'HandleControlSetCursor',
				'GetControlClickActivation',
				'HandleControlContextualMenuClick',
			]),
			('#if ACCESSOR_CALLS_ARE_FUNCTIONS', [
				# XXX These are silly, they should be #defined to access the fields
				# directly. Later...
				'GetControlBounds',
				'IsControlHilited',
				'GetControlHilite',
				'GetControlOwner',
				'GetControlDataHandle',
				'GetControlPopupMenuHandle',
				'GetControlPopupMenuID',
				'SetControlDataHandle',
				'SetControlBounds',
				'SetControlPopupMenuHandle',
				'SetControlPopupMenuID',
			])]
			
	def makeblacklisttypes(self):
		return [
			'ProcPtr',
			'ControlActionUPP',
			'ControlButtonContentInfoPtr',
			'Ptr',
			'ControlDefSpec', # Don't know how to do this yet
			'ControlDefSpec_ptr', # ditto
			'Collection', # Ditto
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),

			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),

##			# For TrackControl
##			([("ProcPtr", "actionProc", "InMode")],
##			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),
##			([("ControlActionUPP", "actionProc", "InMode")],
##			 [("FakeType('(ControlActionUPP)0')", "*", "*")]),

			# For GetControlTitle
			([('Str255', 'title', 'InMode')],
			 [('Str255', 'title', 'OutMode')]),

			([("ControlHandle", "*", "OutMode")],
			 [("ExistingControlHandle", "*", "*")]),
			([("ControlRef", "*", "OutMode")],	# Ditto, for Universal Headers
			 [("ExistingControlHandle", "*", "*")]),
			 
			([("Rect_ptr", "*", "ReturnMode")], # GetControlBounds
			 [("void", "*", "ReturnMode")]),
			]

if __name__ == "__main__":
	main()

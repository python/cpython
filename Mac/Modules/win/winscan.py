# Scan an Apple header file, generating a Python file of generator calls.
import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

def main():
	input = "MacWindows.h"
	output = "wingen.py"
	defsoutput = TOOLBOXDIR + "Windows.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Testing definitions output code ==="
	execfile(defsoutput, {}, {})
	print "=== Done scanning and generating, now importing the generated code... ==="
	import winsupport
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("WindowPtr", "WindowPeek", "WindowRef") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("false = 0\n")
		self.defsfile.write("true = 1\n")

	def makeblacklistnames(self):
		return [
			'DisposeWindow', # Implied when the object is deleted
			'CloseWindow',
			'SetWindowProperty',	# For the moment
			'GetWindowProperty',
			'GetWindowPropertySize',
			'RemoveWindowProperty',
			'MacCloseWindow',
			'GetWindowList', # Don't know whether this is safe...
			# Constants with funny definitions
			'kMouseUpOutOfSlop',
			'kAllWindowClasses',
			]
			
	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'GetAuxWin',
				'GetWindowDataHandle',
				'SaveOld',
				'DrawNew',
				'SetWinColor',
				'SetDeskCPat',
				'InitWindows',
				'InitFloatingWindows',
				'GetWMgrPort',
				'GetCWMgrPort',
				'ValidRgn',		# Use versions with Window in their name
				'ValidRect',
				'InvalRgn',
				'InvalRect',
				'IsValidWindowPtr', # I think this is useless for Python, but not sure...
				'GetWindowZoomFlag',	# Not available in Carbon
				'GetWindowTitleWidth',	# Ditto
				'GetWindowGoAwayFlag',
				'GetWindowSpareFlag',
			]),
			('#if !TARGET_API_MAC_OS8', [
				'IsWindowUpdatePending',
				'FindWindowOfClass',
				'GetFrontWindowOfClass',
				'ChangeWindowPropertyAttributes',
				'GetWindowPropertyAttributes',
				'GetNextWindowOfClass',
				'ScrollWindowRegion',
				'ScrollWindowRect',
				'ChangeWindowAttributes',
				'ReshapeCustomWindow',
				'EnableScreenUpdates',
				'DisableScreenUpdates',
				'GetAvailableWindowPositioningBounds',
				'CreateStandardWindowMenu',
				'GetSheetWindowParent',
				'HideSheetWindow',
				'ShowSheetWindow',
				'ConstrainWindowToScreen',
				'GetWindowGreatestAreaDevice',
				'CopyWindowTitleAsCFString',
				'SetWindowTitleWithCFString',
				'CopyWindowAlternateTitle',
				'SetWindowAlternateTitle',
				'GetWindowModality',
				'SetWindowModality',
				'SetWindowClass',
				'ReleaseWindow',
				'RetainWindow',
				'GetWindowRetainCount',
			]),
			('#if TARGET_API_MAC_OSX', [
				'TransitionWindowAndParent',
			])]
			
	def makeblacklisttypes(self):
		return [
			'ProcPtr',
			'DragGrayRgnUPP',
			'WindowPaintUPP',
			'Collection',		# For now, to be done later
			'WindowDefSpec',	# Too difficult for now
			'WindowDefSpec_ptr',
			'EventRef', #TBD
			]

	def makerepairinstructions(self):
		return [
			
			# GetWTitle
			([("Str255", "*", "InMode")],
			 [("*", "*", "OutMode")]),
			
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			
			([("void", "wStorage", "OutMode")],
			 [("NullStorage", "*", "InMode")]),
			
			# match FindWindowOfClass
			([("WindowRef", "outWindow", "OutMode"), ("WindowPartCode", "outWindowPart", "OutMode")],
			 [("ExistingWindowPtr", "*", "OutMode"), ("WindowPartCode", "outWindowPart", "OutMode")]),
		    # then match CreateNewWindow and CreateWindowFromResource
			([("WindowRef", "outWindow", "OutMode")],
			 [("WindowRef", "*", "*")]),
			
			([("WindowPtr", "*", "OutMode")],
			 [("ExistingWindowPtr", "*", "*")]),
			([("WindowRef", "*", "OutMode")],	# Same, but other style headerfiles
			 [("ExistingWindowPtr", "*", "*")]),
			
			([("WindowPtr", "FrontWindow", "ReturnMode")],
			 [("ExistingWindowPtr", "*", "*")]),
			([("WindowRef", "FrontWindow", "ReturnMode")],	# Ditto
			 [("ExistingWindowPtr", "*", "*")]),
			([("WindowPtr", "FrontNonFloatingWindow", "ReturnMode")],
			 [("ExistingWindowPtr", "*", "*")]),
			([("WindowRef", "FrontNonFloatingWindow", "ReturnMode")],	# Ditto
			 [("ExistingWindowPtr", "*", "*")]),
			 
			([("Rect_ptr", "*", "ReturnMode")], # GetWindowXXXState accessors
			 [("void", "*", "ReturnMode")]),
			]

if __name__ == "__main__":
	main()


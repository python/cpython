# Scan <Controls.h>, generating ctlgen.py.
import addpack
addpack.addpack(':Tools:bgen:bgen')

from scantools import Scanner

def main():
	input = "Controls.h"
	output = "ctlgen.py"
	defsoutput = "Controls.py"
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

	def makeblacklistnames(self):
		return [
			'DisposeControl' # Implied by deletion of control object
			'KillControls', # Implied by close of dialog
			'SetCtlAction',
			]

	def makeblacklisttypes(self):
		return [
			'ProcPtr',
			'ControlActionUPP',
			'CCTabHandle',
			'AuxCtlHandle',
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

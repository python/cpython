# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack(':Tools:bgen:bgen')

from scantools import Scanner

LONG = "Dialogs"
SHORT = "dlg"
OBJECT = "DialogPtr"

def main():
	input = LONG + ".h"
	output = SHORT + "gen.py"
	defsoutput = LONG + ".py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("DialogPtr", "DialogRef") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			'InitDialogs',
			'ErrorSound',
			# Dialogs are disposed when the object is deleted
			'CloseDialog',
			'DisposDialog',
			'DisposeDialog',
			'UpdtDialog',
			'CouldAlert',
			'FreeAlert',
			'CouldDialog',
			'FreeDialog',
			'GetStdFilterProc',
			]

	def makeblacklisttypes(self):
		return [
			]

	def makerepairinstructions(self):
		return [
			([("Str255", "*", "InMode")],
			 [("*", "*", "OutMode")]),
			
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			
			# NewDialog ETC.
			([("void", "*", "OutMode")],
			 [("NullStorage", "*", "InMode")]),
			
			([("DialogPtr", "*", "OutMode")],
			 [("ExistingDialogPtr", "*", "*")]),
			([("DialogRef", "*", "OutMode")],
			 [("ExistingDialogPtr", "*", "*")]),
			]

if __name__ == "__main__":
	main()

# Scan <Menus.h>, generating menugen.py.
import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

def main():
	input = "Menus.h"
	output = "menugen.py"
	defsoutput = TOOLBOXDIR + "Menus.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now doing 'import menusupport' ==="
	import menusupport
	print "=== Done.  It's up to you to compile Menumodule.c ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("MenuHandle", "MenuRef") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			]

	def makeblacklisttypes(self):
		return [
			'MCTableHandle',
			'MCEntryPtr',
			'MCTablePtr',
			]

	def makerepairinstructions(self):
		return [
			([("Str255", "itemString", "InMode")],
			 [("*", "*", "OutMode")]),
			
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			]

if __name__ == "__main__":
	main()

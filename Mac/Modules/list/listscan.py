# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner
from bgenlocations import TOOLBOXDIR

LONG = "Lists"
SHORT = "list"
OBJECT = "ListHandle"

def main():
	input = LONG + ".h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + LONG + ".py"
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
			t, n, m = arglist[-1]
			# This is non-functional today
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"LDispose",		# Done by removing the object
			"LSearch",		# We don't want to handle procs just yet
			"LGetCellDataLocation",		# What does this do??
			]

	def makeblacklisttypes(self):
		return [
			]

	def makerepairinstructions(self):
		return [
			([('ListBounds_ptr', '*', 'InMode')],
			 [('Rect_ptr', '*', 'InMode')]),

			([("Cell", "theCell", "OutMode")],
			 [("Cell", "theCell", "InOutMode")]),
			 
			([("void_ptr", "*", "InMode"), ("short", "*", "InMode")],
			 [("InBufferShortsize", "*", "*")]),
			
			([("void", "*", "OutMode"), ("short", "*", "OutMode")],
			 [("VarOutBufferShortsize", "*", "InOutMode")]),
			
#			([("void", "wStorage", "OutMode")],
#			 [("NullStorage", "*", "InMode")]),
#			
#			# GetKeys
#			([('KeyMap', 'theKeys', 'InMode')],
#			 [('*', '*', 'OutMode')]),
#			 
#			# GetTicker
#			([('unsigned long', '*', '*')],
#			 [('unsigned_long', '*', '*')]),
			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

LONG = "HtmlRendering"
SHORT = "html"
OBJECT = "HRReference"

def main():
##	input = LONG + ".h"
	input = "Macintosh HD:ufs:jack:SWdev:Universal:Interfaces:CIncludes:HTMLRendering.h"
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
			t, n, m = arglist[0]
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"HRDisposeReference",
			]

	def makeblacklisttypes(self):
		return [
			"HRNewURLUPP",
			"HRURLToFSSpecUPP",
			"HRWasURLVisitedUPP",
		]

	def makerepairinstructions(self):
		return [
 			([('char', '*', 'OutMode'), ('UInt32', '*', 'InMode')],
 			 [('InBuffer', '*', 'InMode')]),
			]

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")


if __name__ == "__main__":
	main()

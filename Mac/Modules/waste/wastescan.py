# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner_PreUH3
from bgenlocations import MWERKSDIR, TOOLBOXDIR

WASTEDIR=":::::Waste 1.3 Distribution:WASTE C/C++ Headers:"

OBJECT = "TEHandle"
SHORT = "waste"
OBJECT = "WEReference"
OBJECT2 = "WEObjectReference"

def main():
	input = WASTEDIR + "WASTE.h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + "WASTEconst.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
##	scanner.gentypetest(SHORT+"typetest.py")
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner_PreUH3):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[-1]
			# This is non-functional today
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
			else:
				t, n, m = arglist[0]
				if t == OBJECT2 and m == "InMode":
					classname = "Method2"
					listname = "methods2"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("kPascalStackBased = None # workaround for header parsing\n")
	def makeblacklistnames(self):
		return [
			"WEDispose",
			"WESetInfo", # Argument type unknown...
			"WEGetInfo",
			"WEVersion", # Unfortunately...
			]

	def makeblacklisttypes(self):
		return [
			"DragReference",	# For now...
			"UniversalProcPtr",
			"WEFontIDToNameUPP",
			"WEFontNameToIDUPP",
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("SInt32", "*", "InMode")],
			 [("InBuffer", "*", "*")]),

			# WEContinuousStyle
			([("WEStyleMode", "mode", "OutMode"), ("TextStyle", "ts", "OutMode")],
			 [("WEStyleMode", "mode", "InOutMode"), ("TextStyle", "ts", "OutMode")]),
			 
			# WECopyRange
			([('Handle', 'hText', 'InMode'), ('StScrpHandle', 'hStyles', 'InMode'),
    			('WESoupHandle', 'hSoup', 'InMode')],
    		 [('OptHandle', 'hText', 'InMode'), ('OptStScrpHandle', 'hStyles', 'InMode'),
    			('OptSoupHandle', 'hSoup', 'InMode')]),
			 
			# WEInsert
			([('StScrpHandle', 'hStyles', 'InMode'), ('WESoupHandle', 'hSoup', 'InMode')],
    		 [('OptStScrpHandle', 'hStyles', 'InMode'), ('OptSoupHandle', 'hSoup', 'InMode')]),
    		 
    		# WEGetObjectOwner
    		("WEGetObjectOwner",
    		 [('WEReference', '*', 'ReturnMode')],
    		 [('ExistingWEReference', '*', 'ReturnMode')])

			]
			
if __name__ == "__main__":
	main()

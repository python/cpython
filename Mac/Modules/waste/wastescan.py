# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack(':tools:bgen:bgen')
from scantools import Scanner
from bgenlocations import TOOLBOXDIR

WASTEDIR=":::::Waste 1.2a5:"

OBJECT = "TEHandle"
SHORT = "waste"
OBJECT = "WEReference"
OBJECT2 = "WEObjectReference"

def main():
	input = WASTEDIR + "WASTE C/C++ Headers:WASTE.h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + "WASTEconst.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
##	scanner.gentypetest(SHORT+"typetest.py")
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def initpatterns(self):
		# Waste doesn't use 'extern':
		Scanner.initpatterns(self)
		self.head_pat = "^pascal[ \t]+"

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

	def makeblacklistnames(self):
		return [
			"WEDispose",
			"WESetInfo", # Argument type unknown...
			"WEGetInfo",
			"WEGetObjectOwner", # Returns ref to existing WE
			]

	def makeblacklisttypes(self):
		return [
			"DragReference",	# For now...
			"UniversalProcPtr",
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
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
    		 [('OptStScrpHandle', 'hStyles', 'InMode'), ('OptSoupHandle', 'hSoup', 'InMode')])

			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner_OSX
from bgenlocations import TOOLBOXDIR

LONG = "CoreFoundation"
SHORT = "cf"
OBJECTS = ("CFTypeRef", "CFStringRef")

def main():
	input = [
		"CFBase.h",
###		"CFArray.h",
##		"CFBag.h",
##		"CFBundle.h",
##		"CFCharacterSet.h",
###		"CFData.h",
##		"CFDate.h",
###		"CFDictionary.h",
##		"CFNumber.h",
##		"CFPlugIn.h",
##		"CFPreferences.h",
##		"CFPropertyList.h",
##		"CFSet.h",
###		"CFString.h",
##		"CFStringEncodingExt.h",
##		"CFTimeZone.h",
##		"CFURL.h",
		]
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + LONG + ".py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.gentypetest(SHORT+"typetest.py")
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner_OSX):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in OBJECTS and m == "InMode":
				classname = "Method"
				listname = t + "_methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

	def makeblacklistnames(self):
		return [
			# Memory allocator functions
			"CFAllocatorGetDefault",
			"CFAllocatorSetDefault",
			"CFAllocatorAllocate",
			"CFAllocatorReallocate",
			"CFAllocatorDeallocate",
			"CFGetAllocator",
			]

	def makegreylist(self):
		return []

	def makeblacklisttypes(self):
		return [
			"CFAllocatorContext",
			]

	def makerepairinstructions(self):
		return [
			]
			
if __name__ == "__main__":
	main()

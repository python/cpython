# Scan AppleEvents.h header file, generate aegen.py and AppleEvents.py files.
# Then run aesupport to generate AEmodule.c.
# (Should learn how to tell the compiler to compile it as well.)

import sys
import os
import string
import regex
import regsub
import MacOS

BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from bgenlocations import TOOLBOXDIR

from scantools import Scanner

def main():
	print "=== Scanning AERegistry.h for defines ==="
	input = "AERegistry.h"
	output = "@dummy-registry.py"
	defsoutput = TOOLBOXDIR + "AERegistry.py"
	scanner = AppleEventsRegScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Scanning AEObjects.h for defines ==="
	# XXXX This isn't correct. We only scan AEObjects.h for defines, but there
	# are some functions in there that are probably useful (the accessor stuff)
	# once we start writing servers in python.
	input = "AEObjects.h"
	output = "@dummy-objects.py"
	defsoutput = TOOLBOXDIR + "AEObjects.py"
	scanner = AppleEventsScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Scanning AEDataModel.h ==="
	input = "AEDataModel.h"
	output = "aedatamodelgen.py"
	defsoutput = TOOLBOXDIR + "AEDataModel.py"
	scanner = AppleEventsScanner(input, output, defsoutput)
	
	scanner.scan()
	scanner.close()
	print "=== Scanning AppleEvents.h ==="
	input = "AppleEvents.h"
	output = "aegen.py"
	defsoutput = TOOLBOXDIR + "AppleEvents.py"
	scanner = AppleEventsRegScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done Scanning and Generating, now doing 'import aesupport' ==="
	import aesupport
	print "=== Done 'import aesupport'.  It's up to you to compile AEmodule.c ==="

class AppleEventsScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "AEFunction"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t[-4:] == "_ptr" and m == "InMode" and \
			   t[:-4] in ("AEDesc", "AEAddressDesc", "AEDescList",
			         "AERecord", "AppleEvent"):
				classname = "AEMethod"
				listname = "aedescmethods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"AEDisposeDesc",
#			"AEGetEventHandler",
			]

	def makeblacklisttypes(self):
		return [
			"ProcPtr",
			"AEArrayType",
			"AECoercionHandlerUPP",
			"UniversalProcPtr",
			]

	def makerepairinstructions(self):
		return [
			([("Boolean", "isSysHandler", "InMode")],
			 [("AlwaysFalse", "*", "*")]),
			
			([("void_ptr", "*", "InMode"), ("Size", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("EventHandlerProcPtr", "*", "InMode"), ("long", "*", "InMode")],
			 [("EventHandler", "*", "*")]),
			
			([("EventHandlerProcPtr", "*", "OutMode"), ("long", "*", "OutMode")],
			 [("EventHandler", "*", "*")]),
			
			([("AEEventHandlerUPP", "*", "InMode"), ("long", "*", "InMode")],
			 [("EventHandler", "*", "*")]),
			
			([("AEEventHandlerUPP", "*", "OutMode"), ("long", "*", "OutMode")],
			 [("EventHandler", "*", "*")]),
			
			([("void", "*", "OutMode"), ("Size", "*", "InMode"),
			                            ("Size", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			 
			([("AppleEvent", "theAppleEvent", "OutMode")],
			 [("AppleEvent_ptr", "*", "InMode")]),
			 
			([("AEDescList", "theAEDescList", "OutMode")],
			 [("AEDescList_ptr", "*", "InMode")]),
			]

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

class AppleEventsRegScanner(AppleEventsScanner):
	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("from AEDataModel import *\n")

if __name__ == "__main__":
	main()

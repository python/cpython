# Scan Resources.h header file, generate resgen.py and Resources.py files.
# Then run ressupport to generate Resmodule.c.
# (Should learn how to tell the compiler to compile it as well.)

import sys
import os
import string
import MacOS

BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from bgenlocations import TOOLBOXDIR

from scantools import Scanner

def main():
	input = "Resources.h"
	output = "resgen.py"
	defsoutput = TOOLBOXDIR + "Resources.py"
	scanner = ResourcesScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now doing 'import ressupport' ==="
	import ressupport
	print "=== Done 'import ressupport'.  It's up to you to compile Resmodule.c ==="

class ResourcesScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "ResFunction"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t == "Handle" and m == "InMode":
				classname = "ResMethod"
				listname = "resmethods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"ReadPartialResource",
			"WritePartialResource",
			"TempInsertROMMap",
##			"RmveResource",		# RemoveResource
##			"SizeResource",		# GetResourceSizeOnDisk
##			"MaxSizeRsrc",		# GetMaxResourceSize
			]
			
	def makegreylist(self):
		return [
			('#if TARGET_API_MAC_OS8', [
				'RGetResource',
				'OpenResFile',
				'CreateResFile',
				'RsrcZoneInit',
				'InitResources',
				'RsrcMapEntry',
			]),
			('#if TARGET_API_MAC_CARBON', [
				'GetNextResourceFile',
				'GetTopResourceFile',
				'FSpOpenOrphanResFile',
				'DetachResourceFile',
				'InsertResourceFile',
				'FSpResourceFileAlreadyOpen',
			])]

	def makerepairinstructions(self):
		return [
			([("Str255", "*", "InMode")],
			 [("*", "*", "OutMode")]),
			
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode")],
			 [("InOutBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("OutBuffer", "*", "InOutMode")]),
			 
			([("SInt8", "*", "*")],
			 [("SignedByte", "*", "*")])
			]

if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.
import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from bgenlocations import TOOLBOXDIR

from scantools import Scanner

def main():
	input = "QDOffscreen.h"
	output = "qdoffsgen.py"
	defsoutput = TOOLBOXDIR + "QDOffscreen.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	import qdoffssupport
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t == "GWorldPtr" and m in ("InMode", "InOutMode"):
				classname = "Method"
				listname = "methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

	def makeblacklistnames(self):
		return [
			'DisposeGWorld', # Implied when the object is deleted
			'NewGWorldFromHBITMAP', # Don't know what the args do
			'GetGDeviceAttributes', # Windows-only
			]

	def makeblacklisttypes(self):
		return [
			"void_ptr",		# GetGDeviceSurface, blacklisted for now
			"Ptr",			# Again, for now (array is probably ok here)
			]

	def makerepairinstructions(self):
		return [
			
## 			("UpdateGWorld",
## 			 [("GWorldPtr", "*", "OutMode")],
## 			 [("*", "*", "InOutMode")]),
			 
			# This one is incorrect: we say that all input gdevices are
			# optional, but some are not. Most are, however, so users passing
			# None for non-optional gdevices will get a qd error, I guess, in
			# stead of a python argument error.
			([("GDHandle", "*", "InMode")],
			 [("OptGDHandle", "*", "InMode")]),
			]

if __name__ == "__main__":
	main()


# IBCarbonscan.py

import sys
import os
import string
import MacOS

BGENDIR= '/Users/jack/src/python/Tools/bgen/bgen'
sys.path.append(BGENDIR)
print sys.path, sys.prefix
from bgenlocations import TOOLBOXDIR

from scantools import Scanner_OSX

def main():
	print "---Scanning IBCarbonRuntime.h---"
	input = ["IBCarbonRuntime.h"]
	output = "IBCarbongen.py"
	defsoutput = TOOLBOXDIR + "IBCarbonRuntime.py"
	scanner = IBCarbon_Scanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "--done scanning, importing--"
	import IBCarbonsupport
	print "done"

class IBCarbon_Scanner(Scanner_OSX):

	def destination(self, type, name, arglist):
		classname = "IBCarbonFunction"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t == "IBNibRef" and m == "InMode":
				classname = "IBCarbonMethod"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"DisposeNibReference",				# taken care of by destructor
			"CreateNibReferenceWithCFBundle",  ## need to wrap CFBundle.h properly first
			]
			
	def makerepairinstructions(self):
		return []


if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "LaunchServices"
SHORT = "launch"
OBJECT = "NOTUSED"

def main():
	input = LONG + ".h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + LONG + ".py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	scanner.gentypetest(SHORT+"typetest.py")
	print "=== Testing definitions output code ==="
	execfile(defsoutput, {}, {})
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			# This is non-functional today
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("kLSRequestAllInfo = -1\n")
		self.defsfile.write("kLSRolesAll = -1\n")

	def makeblacklistnames(self):
		return [
			"kLSRequestAllInfo",
			"kLSRolesAll",
			]

	def makeblacklisttypes(self):
		return [
			"LSLaunchFSRefSpec_ptr",
			"LSLaunchURLSpec_ptr",
			]

	def makerepairinstructions(self):
		return [
			# LSGetApplicationForInfo
			([('CFStringRef', 'inExtension', 'InMode')],
    		 [('OptCFStringRef', 'inExtension', 'InMode')]),
    		 
			# LSFindApplicationForInfo
			([('CFStringRef', 'inBundleID', 'InMode')],
    		 [('OptCFStringRef', 'inBundleID', 'InMode')]),
			([('CFStringRef', 'inName', 'InMode')],
    		 [('OptCFStringRef', 'inName', 'InMode')]),
			]
			
if __name__ == "__main__":
	main()

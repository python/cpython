# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "Events"
SHORT = "evt"
OBJECT = "NOTUSED"

def main():
	input = LONG + ".h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + LONG + ".py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
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

	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'SystemEvent',
				'SystemTask',
				'SystemClick',
				'GetOSEvent',
				'OSEventAvail',
			]),
			('#if TARGET_API_MAC_CARBON', [
				'CheckEventQueueForUserCancel',
				'GetCurrentKeyModifiers',
				'GetGlobalMouse',
			])]

	def makeblacklistnames(self):
		return [
			"KeyTranslate",
			"GetEventMask",		# I cannot seem to find this routine...
			"WaitNextEvent",	# Manually generated because of optional region
			# Constants with funny definitions
			"osEvtMessageMask",
			]

	def makeblacklisttypes(self):
		return [
			"EvQElPtr", "QHdrPtr"
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			
			([("void", "wStorage", "OutMode")],
			 [("NullStorage", "*", "InMode")]),
			
			# GetKeys
			([('KeyMap', 'theKeys', 'InMode')],
			 [('*', '*', 'OutMode')]),
			 
			# GetTicker
			([('unsigned long', '*', '*')],
			 [('unsigned_long', '*', '*')]),
			]
			
if __name__ == "__main__":
	main()

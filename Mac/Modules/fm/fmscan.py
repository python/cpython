# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner
from bgenlocations import TOOLBOXDIR

LONG = "Fonts"
SHORT = "fm"

def main():
	input = "Fonts.h"
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
		return classname, listname

	def makeblacklistnames(self):
		return [
			"OutlineMetrics",	# Too complicated
			"AntiTextIsAntiAliased",	# XXXX Missing from library...
			"AntiTextGetEnabled",
			"AntiTextSetEnabled",
			"AntiTextGetApplicationAware",
			"AntiTextSetApplicationAware",
			]

	def makeblacklisttypes(self):
		return [
			"FMInput_ptr",	# Not needed for now
			"FMOutPtr",		# Ditto
			]

	def makerepairinstructions(self):
		return [
			([('Str255', '*', 'InMode')], [('Str255', '*', 'OutMode')]),
			([('FMetricRecPtr', 'theMetrics', 'InMode')], [('FMetricRecPtr', 'theMetrics', 'OutMode')]),
			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.
#
# Note that the scrap-manager include file is so weird that this
# generates a boilerplate to be edited by hand.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner
from bgenlocations import TOOLBOXDIR

LONG = "Scrap"
SHORT = "scrap"

def main():
	input = "Scrap.h"
	output = SHORT + "gen.py"
	defsoutput = "@Scrap.py"
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
			]

	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'InfoScrap',
				'GetScrap',
				'ZeroScrap',
				'PutScrap',
			]),
			('#if TARGET_API_MAC_CARBON', [
				'CallInScrapPromises',
				'ClearCurrentScrap',
			])]

	def makeblacklisttypes(self):
		return [
			"ScrapRef",		# For now -- This is the Carbon scrap main object
			]

	def makerepairinstructions(self):
		return [
			([('void', '*', 'OutMode')], [('putscrapbuffer', '*', 'InMode')]),
			([('void_ptr', '*', 'InMode')], [('putscrapbuffer', '*', 'InMode')]),
			]
			
if __name__ == "__main__":
	main()

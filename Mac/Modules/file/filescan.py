# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner_OSX

LONG = "Files"
SHORT = "file"
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

class MyScanner(Scanner_OSX):

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

	def makeblacklistnames(self):
		return [
			# Constants with incompatible definitions
			"kioACAccessOwnerMask",
			"kFSCatInfoReserved",
			"kFSIterateReserved",
			
			"FSRefMakePath", # Do this manually
			
			"FSRead", # Couldn't be bothered
			"FSWrite", # ditto
			"FSReadFork", # ditto
			"FSWriteFork", # ditto
			
			# Old routines:
			"GetWDInfo",
			"OpenWD",
			"CloseWD",
			"FInitQueue",
			"rstflock",
			"setflock",
			"setfinfo",
			"fsrename",
			"fsdelete",
			"create",
			"flushvol",
			"eject",
			"umountvol",
			"setvol",
			"getvol",
			"getfinfo",
			"getvinfo",
			"fsopen",
			"RstFLock",
			"SetFLock",
			"SetFInfo",
			"Rename",
			"OpenRF",
			"FSDelete",
			"Create",
			"GetVol",
			"GetFInfo",
			"GetVInfo",
			"FSOpen",
			"Eject",
			"SetVol",
			"openrf",
			"unmountvol",
			"OpenDF",
			
			]

	def makeblacklisttypes(self):
		return [
			"CInfoPBPtr", # Old stuff
			"CMovePBPtr", # Old stuff
			"ParmBlkPtr", # Old stuff
			"HParmBlkPtr", # Old stuff
			"DTPBPtr", # Old stuff
			"FCBPBPtr", # Old stuff
			"QHdrPtr", # Old stuff
			"CSParamPtr", # Old stuff
			"FSCatalogBulkParam", # old stuff
			"FSForkCBInfoParam", # old stuff
			"FSForkIOParam", # old stuff
			"FSRefParam",  # old stuff
			"FSVolumeInfoParam", # old stuff
			"WDPBPtr", # old stuff
			"XCInfoPBPtr", # old stuff
			"XVolumeParamPtr", # old stuff

			
			"CatPositionRec", # State variable, not too difficult
			"FSCatalogInfo", # Lots of fields, difficult struct
			"FSCatalogInfo_ptr", # Lots of fields, difficult struct
			"FSIterator", # Should become an object
			"FSForkInfo", # Lots of fields, difficult struct
			"FSSearchParams", # Also catsearch stuff
			"FSVolumeInfo", # big struct
			"FSVolumeInfo_ptr", # big struct
			
			"IOCompletionProcPtr", # proc pointer
			"IOCompletionUPP", # Proc pointer
			
			
			]

	def makerepairinstructions(self):
		return [
			# Various ways to give pathnames
			([('UInt8_ptr', 'path', 'InMode')],
			 [('stringptr', 'path', 'InMode')]
			),
			 
			([('char_ptr', '*', 'InMode')],
			 [('stringptr', '*', 'InMode')]
			),
			 
			# Unicode filenames passed as length, buffer
			([('UniCharCount', '*', 'InMode'),
			  ('UniChar_ptr', '*', 'InMode')],
			 [('UnicodeReverseInBuffer', '*', 'InMode')]
			),
		]
		
   
	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
		self.defsfile.write("true = True\n")
		self.defsfile.write("false = False\n")
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner
from bgenlocations import MWERKSDIR, TOOLBOXDIR

#WASTEDIR=":::::Waste 1.3 Distribution:WASTE C/C++ Headers:"
WASTEDIR=MWERKSDIR + 'MacOS Support:(Third Party Support):Waste 2.0 Distribution:C_C++ Headers:'

OBJECT = "TEHandle"
SHORT = "waste"
OBJECT = "WEReference"
OBJECT2 = "WEObjectReference"

def main():
	input = WASTEDIR + "WASTE.h"
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + "WASTEconst.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
##	scanner.gentypetest(SHORT+"typetest.py")
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

#class MyScanner(Scanner_PreUH3):
class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[-1]
			# This is non-functional today
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
			else:
				t, n, m = arglist[0]
				if t == OBJECT2 and m == "InMode":
					classname = "Method2"
					listname = "methods2"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("kPascalStackBased = None # workaround for header parsing\n")
	def makeblacklistnames(self):
		return [
			"WEDispose",
			"WESetInfo", # Argument type unknown...
			"WEGetInfo",
			"WEVersion", # Unfortunately...
			"WEPut", # XXXX TBD: needs array of flavortypes.
			"WEGetOneAttribute", # XXXX TBD: output buffer
			]

	def makeblacklisttypes(self):
		return [
			"DragReference",	# For now...
			"UniversalProcPtr",
			"WEFontIDToNameUPP",
			"WEFontNameToIDUPP",
			"WEClickLoopUPP",
			"WEScrollUPP",
			"WETSMPreUpdateUPP",
			"WETSMPostUpdateUPP",
			"WEPreTrackDragUPP",
			"WETranslateDragUPP",
			"WEHiliteDropAreaUPP",
			"WEDrawTextUPP",
			"WEDrawTSMHiliteUPP",
			"WEPixelToCharUPP",
			"WECharToPixelUPP",
			"WELineBreakUPP",
			"WEWordBreakUPP",
			"WECharByteUPP",
			"WECharTypeUPP",
			"WEEraseUPP",
			"WEFluxUPP",
			"WENewObjectUPP",
			"WEDisposeObjectUPP",
			"WEDrawObjectUPP",
			"WEClickObjectUPP",
			"WEStreamObjectUPP",
			"WEHoverObjectUPP",
			"WERuler",		# XXXX To be done
			"WERuler_ptr",	# ditto
			"WEParaInfo",	# XXXX To be done
			"WEPrintSession",	# XXXX To be done
			"WEPrintOptions_ptr", # XXXX To be done
			]

	def makerepairinstructions(self):
		return [
			([("void_ptr", "*", "InMode"), ("SInt32", "*", "InMode")],
			 [("InBuffer", "*", "*")]),

			# WEContinuousStyle
			([("WEStyleMode", "ioMode", "OutMode"), ("TextStyle", "outTextStyle", "OutMode")],
			 [("WEStyleMode", "*", "InOutMode"), ("TextStyle", "*", "*")]),
			 
			# WECopyRange
			([('Handle', 'outText', 'InMode'), ('StScrpHandle', 'outStyles', 'InMode'),
    			('WESoupHandle', 'outSoup', 'InMode')],
    		 [('OptHandle', '*', '*'), ('OptStScrpHandle', '*', '*'),
    			('OptSoupHandle', '*', '*')]),
			 
			# WEInsert
			([('StScrpHandle', 'inStyles', 'InMode'), ('WESoupHandle', 'inSoup', 'InMode')],
    		 [('OptStScrpHandle', '*', '*'), ('OptSoupHandle', '*', '*')]),
    		 
    		# WEGetObjectOwner
    		("WEGetObjectOwner",
    		 [('WEReference', '*', 'ReturnMode')],
    		 [('ExistingWEReference', '*', 'ReturnMode')]),
    		 
    		# WEFindParagraph
    		([("char_ptr", "inKey", "InMode")],
    		 [("stringptr", "*", "*")]),
			
			# WESetOneAttribute
			([("void_ptr", "*", "InMode"), ("ByteCount", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			]
			
if __name__ == "__main__":
	main()

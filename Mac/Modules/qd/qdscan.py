# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack(':Tools:bgen:bgen')

from scantools import Scanner
from bgenlocations import TOOLBOXDIR

def main():
	input = "QuickDraw.h"
	output = "qdgen.py"
	defsoutput = TOOLBOXDIR + "QuickDraw.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	
	# Grmpf. Universal Headers have Text-stuff in a different include file...
	input = "QuickDrawText.h"
	output = "@qdgentext.py"
	defsoutput = "@QuickDrawText.py"
	have_extra = 0
	try:
		scanner = MyScanner(input, output, defsoutput)
		scanner.scan()
		scanner.close()
		have_extra = 1
	except IOError:
		pass
	if have_extra:
		print "=== Copying QuickDrawText stuff into main files... ==="
		ifp = open("@qdgentext.py")
		ofp = open("qdgen.py", "a")
		ofp.write(ifp.read())
		ifp.close()
		ofp.close()
		ifp = open("@QuickDrawText.py")
		ofp = open(TOOLBOXDIR + "QuickDraw.py", "a")
		ofp.write(ifp.read())
		ifp.close()
		ofp.close()
		
	print "=== Done scanning and generating, now importing the generated code... ==="
	import qdsupport
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
##			elif t == "PolyHandle" and m == "InMode":
##				classname = "Method"
##				listname = "p_methods"
##			elif t == "RgnHandle" and m == "InMode":
##				classname = "Method"
##				listname = "r_methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			'InitGraf',
			'StuffHex',
			'StdLine',
			'StdComment',
			'StdGetPic',
			'OpenPort',
			'InitPort',
			'ClosePort',
			'OpenCPort',
			'InitCPort',
			'CloseCPort',
			'BitMapToRegionGlue',
			]

	def makeblacklisttypes(self):
		return [
##			'CCrsrHandle',
			'CIconHandle', # Obsolete
			'CQDProcs',
			'CSpecArray',
##			'CTabHandle',
			'ColorComplementProcPtr',
			'ColorComplementUPP',
			'ColorSearchProcPtr',
			'ColorSearchUPP',
			'ConstPatternParam',
			'DeviceLoopDrawingProcPtr',
			'DeviceLoopFlags',
##			'FontInfo',
##			'GDHandle',
			'GrafVerb',
			'OpenCPicParams_ptr',
			'Ptr',
			'QDProcs',
			'ReqListRec',
			'void_ptr',
			]

	def makerepairinstructions(self):
		return [
			([('void_ptr', 'textBuf', 'InMode'),
			  ('short', 'firstByte', 'InMode'),
			  ('short', 'byteCount', 'InMode')],
			 [('TextThingie', '*', '*'), ('*', '*', '*'), ('*', '*', '*')]),
			
			# GetPen and SetPt use a point-pointer as output-only:
			('GetPen', [('Point', '*', 'OutMode')], [('*', '*', 'OutMode')]),
			('SetPt', [('Point', '*', 'OutMode')], [('*', '*', 'OutMode')]),
			
			# All others use it as input/output:
			([('Point', '*', 'OutMode')],
			 [('*', '*', 'InOutMode')]),
			 
			 # InsetRect, OffsetRect
			 ([('Rect', 'r', 'OutMode'),
			 	('short', 'dh', 'InMode'),
			 	('short', 'dv', 'InMode')],
			  [('Rect', 'r', 'InOutMode'),
			 	('short', 'dh', 'InMode'),
			 	('short', 'dv', 'InMode')]),

			 # MapRect
			 ([('Rect', 'r', 'OutMode'),
			 	('Rect_ptr', 'srcRect', 'InMode'),
			 	('Rect_ptr', 'dstRect', 'InMode')],
			  [('Rect', 'r', 'InOutMode'),
			 	('Rect_ptr', 'srcRect', 'InMode'),
			 	('Rect_ptr', 'dstRect', 'InMode')]),
			 	
			 # CopyBits and friends
			 ([('RgnHandle', 'maskRgn', 'InMode')],
			  [('OptRgnHandle', 'maskRgn', 'InMode')]),
			
			]

if __name__ == "__main__":
	main()

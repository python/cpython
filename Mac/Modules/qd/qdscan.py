# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack('D:python:Tools:bgen:bgen')

from scantools import Scanner

def main():
	input = "QuickDraw.h"
	output = "qdgen.py"
	defsoutput = "QuickDraw.py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	import qdsupport
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in ("WindowPtr", "WindowPeek") and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			'InitGraf',
			'StuffHex',
			'StdLine',
			'StdComment',
			'StdGetPic',
			'StdLine',
			]

	def makeblacklisttypes(self):
		return [
			'BitMap_ptr',
			'CCrsrHandle',
			'CGrafPtr',
			'CIconHandle',
			'CQDProcs',
			'CSpecArray',
			'CTabHandle',
			'ColorComplementProcPtr',
			'ColorSearchProcPtr',
			'ConstPatternParam',
			'Cursor_ptr',
			'DeviceLoopDrawingProcPtr',
			'DeviceLoopFlags',
			'FontInfo',
			'GDHandle',
			'GrafVerb',
			'OpenCPicParams_ptr',
			'PenState',
			'PenState_ptr',
			'Ptr',
			'QDProcs',
			'RGBColor',
			'RGBColor_ptr',
			'ReqListRec',
			'void_ptr',
			]

	def makerepairinstructions(self):
		return [
			([('void_ptr', 'textBuf', 'InMode'),
			  ('short', 'firstByte', 'InMode'),
			  ('short', 'byteCount', 'InMode')],
			 [('TextThingie', '*', '*'), ('*', '*', '*'), ('*', '*', '*')]),
			
			([('Point', '*', 'OutMode')],
			 [('*', '*', 'InOutMode')]),
			
			]

if __name__ == "__main__":
	main()

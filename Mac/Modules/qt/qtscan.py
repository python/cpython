# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack(':tools:bgen:bgen')
from scantools import Scanner

LONG = "QuickTime"
SHORT = "qt"
OBJECT = "Movie"

def main():
	input = "Movies.h"
	output = SHORT + "gen.py"
	defsoutput = LONG + ".py"
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
		if arglist:
			t, n, m = arglist[0]
			if t == OBJECT and m == "InMode":
				classname = "Method"
				listname = "methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"DisposeMovie",		# Done on python-object disposal
			"GetMovieCreationTime",	# type "unsigned long" in C, inparseable
			"GetMovieModificationTime",	# Ditto
			]

	def makeblacklisttypes(self):
		return [
			"MoviesErrorUPP",
			"Track",	# XXXX To be done in the future
			"Media",
			"UserData",
			"TimeBase",
			"QTCallBack",
			"Component",
			"TimeRecord",
			"TimeRecord_ptr",
			"TrackEditState",
			"MovieEditState",
			"MoviePreviewCallOutUPP",
			"CGrafPtr",
			"GDHandle",
			"MovieDrawingCompleteUPP",
			"PixMapHandle",
			"MatrixRecord",
			"MatrixRecord_ptr",
			"QTCallBackUPP",
			"TextMediaUPP",
			"MovieProgressUPP",
			"MovieRgnCoverUPP",
			"MCActionFilterUPP",
			"MCActionFilterWithRefConUPP",
			"SampleDescription",
			"SoundDescription",
			"TextDescription",
			"MusicDescription",
			]

	def makerepairinstructions(self):
		return [
			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import addpack
addpack.addpack(':tools:bgen:bgen')
from scantools import Scanner

LONG = "QuickTime"
SHORT = "qt"
OBJECTS = ("Movie", "Track", "Media", "UserData", "TimeBase")

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
			if t in OBJECTS and m == "InMode":
				classname = "Method"
				listname = t + "_methods"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"DisposeMovie",		# Done on python-object disposal
			"DisposeMovieTrack",	# ditto
			"DisposeTrackMedia",	# ditto
			"DisposeUserData",		# ditto
			"DisposeTimeBase",		# ditto
			"GetMovieCreationTime",	# type "unsigned long" in C, inparseable
			"GetMovieModificationTime",	# Ditto
			"GetTrackCreationTime",		# ditto
			"GetTrackModificationTime",	# Ditto
			"GetMediaCreationTime",		# ditto
			"GetMediaModificationTime",	# Ditto
			# The following 4 use 'void *' in an uncontrolled way
			# TBD when I've read the manual...
			"GetUserDataItem",
			"SetUserDataItem",
			"SetTextSampleData",
			"MCDoAction",
			# bgen gets the argument in/out wrong..
			"AddTextSample",
			"AddTESample",
			"AddHiliteSample",
			"HiliteTextSample",
			]

	def makeblacklisttypes(self):
		return [
			# I don't think we want to do these
			"QTSyncTaskPtr",
			# We dont do callbacks yet, so no need for these
			"QTCallBack",
			# Skipped for now, due to laziness
			"TimeRecord",
			"TimeRecord_ptr",
			"TrackEditState",
			"MovieEditState",
			"MatrixRecord",
			"MatrixRecord_ptr",
			"SampleReferencePtr",
#			"SampleDescription",
#			"SoundDescription",
#			"TextDescription",
#			"MusicDescription",
			# I dont know yet how to do these.
			"CGrafPtr",
			"GDHandle",
			# Routine pointers, not yet.
			"MoviesErrorUPP",
			"MoviePreviewCallOutUPP",
			"MovieDrawingCompleteUPP",
			"QTCallBackUPP",
			"TextMediaUPP",
			"MovieProgressUPP",
			"MovieRgnCoverUPP",
			"MCActionFilterUPP",
			"MCActionFilterWithRefConUPP",
			"GetMovieUPP",
			"ModalFilterUPP",
			]

	def makerepairinstructions(self):
		return [
			([('FSSpec', '*', 'OutMode')], [('FSSpec_ptr', '*', 'InMode')]),
			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner
from bgenlocations import TOOLBOXDIR

LONG = "QuickTime"
SHORT = "qt"
OBJECTS = ("Movie", "Track", "Media", "UserData", "TimeBase", "MovieController")

def main():
	input = "Movies.h"
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
		if arglist:
			t, n, m = arglist[0]
			if t in OBJECTS and m == "InMode":
				classname = "Method"
				listname = t + "_methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

	def makeblacklistnames(self):
		return [
			"DisposeMovie",		# Done on python-object disposal
			"DisposeMovieTrack",	# ditto
			"DisposeTrackMedia",	# ditto
			"DisposeUserData",		# ditto
#			"DisposeTimeBase",		# ditto
			"DisposeMovieController", # ditto

			# The following 4 use 'void *' in an uncontrolled way
			# TBD when I've read the manual...
			"GetUserDataItem",
			"SetUserDataItem",
			"SetTextSampleData",
			"BeginFullScreen",
			# bgen gets the argument in/out wrong..
			"AddTextSample",
			"AddTESample",
			"AddHiliteSample",
			"HiliteTextSample",
			
			"MakeTrackTimeTable", # Uses long * return?
			"MakeMediaTimeTable", # ditto
##			"VideoMediaGetStallCount", # Undefined in CW Pro 3 library
			]

	def makegreylist(self):
		return [
			('#if !TARGET_API_MAC_CARBON', [
				'SpriteMediaGetIndImageProperty',	# XXXX Why isn't this in carbon?
				'CheckQuickTimeRegistration',
				'SetMovieAnchorDataRef',
				'GetMovieAnchorDataRef',
				'GetMovieLoadState',
				'OpenADataHandler',
				'MovieMediaGetCurrentMovieProperty',
				'MovieMediaGetCurrentTrackProperty',
				'MovieMediaGetChildMovieDataReference',
				'MovieMediaSetChildMovieDataReference',
				'MovieMediaLoadChildMovieFromDataReference',
				'Media3DGetViewObject',
			])]

	def makeblacklisttypes(self):
		return [
			# I don't think we want to do these
			"QTSyncTaskPtr",
			# We dont do callbacks yet, so no need for these
			"QTCallBack",
			# Skipped for now, due to laziness
			"TrackEditState",
			"MovieEditState",
			"MatrixRecord",
			"MatrixRecord_ptr",
			"SampleReferencePtr",
			"QTTweener",

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
			"TrackTransferUPP",
			"MoviePrePrerollCompleteUPP",
			"MovieExecuteWiredActionsUPP",
			"QTBandwidthNotificationUPP",
			"DoMCActionUPP",
			
			"SampleReference64Ptr",	# Don't know what this does, yet
			"QTRuntimeSpriteDescPtr",
			"QTBandwidthReference",
			"QTScheduledBandwidthReference",
			"QTAtomContainer",
			"SpriteWorld",
			"Sprite",
			]

	def makerepairinstructions(self):
		return [
			([('FSSpec', '*', 'OutMode')], [('FSSpec_ptr', '*', 'InMode')]),
			
			# Movie controller creation
			([('ComponentInstance', 'NewMovieController', 'ReturnMode')],
			 [('MovieController', '*', 'ReturnMode')]),
			 
			# NewMovieFromFile
			([('short', 'resId', 'OutMode'), ('StringPtr', 'resName', 'InMode')],
			 [('short', 'resId', 'InOutMode'), ('dummyStringPtr', 'resName', 'InMode')]),
			 
			# MCDoAction and more
			([('void', '*', 'OutMode')], [('mcactionparams', '*', 'InMode')]),
			
			# SetTimeBaseZero. Does not handle NULLs, unfortunately
			([('TimeRecord', 'zero', 'OutMode')], [('TimeRecord', 'zero', 'InMode')]),
			
			# ConvertTime and ConvertTimeScale
			([('TimeRecord', 'inout', 'OutMode')], [('TimeRecord', 'inout', 'InOutMode')]),
			
			# AddTime and SubtractTime
			([('TimeRecord', 'dst', 'OutMode')], [('TimeRecord', 'dst', 'InOutMode')]),
			]
			
if __name__ == "__main__":
	main()

# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "QuickTime"
SHORT = "qt"
HEADERFILES= ("Movies.h", "ImageCompression.h", "QuickTimeComponents.h")
OBJECTS = ("Movie", "Track", "Media", "UserData", "TimeBase", "MovieController", 
	"IdleManager", "SGOutput")

def main():
	input = HEADERFILES
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
		self.defsfile.write("xmlIdentifierUnrecognized = -1\n")

	def makeblacklistnames(self):
		return [
			"xmlIdentifierUnrecognized", # const with incompatible definition
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
			# OS8 only:
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

            # these are ImageCompression blacklists
			"GraphicsExportGetInputPtr",
			
			# QuickTimeComponents
			# These two need some help: the first returns a point to a databuffer that
			# the second disposes. Generate manually?
			"VDCompressDone",
			"VDReleaseCompressBuffer",
			"QTVideoOutputGetGWorldParameters", # How useful is this?
			]

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
			"QTErrorReplacementPtr",
			"QTRestrictionSet",
			"QTUUID",
			"QTUUID_ptr",

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
			"QTNextTaskNeededSoonerCallbackUPP",
			
			"SampleReference64Ptr",	# Don't know what this does, yet
			"QTRuntimeSpriteDescPtr",
			"QTBandwidthReference",
			"QTScheduledBandwidthReference",
			"QTAtomContainer",
			"SpriteWorld",
			"Sprite",

            # these are ImageCompression blacklists
            "ICMDataUPP",
            "ICMFlushUPP",
            "ICMCompletionUPP",
            "ICMProgressUPP",
            "StdPixUPP",
            "QDPixUPP",
            "ICMAlignmentUPP",
            "ICMCursorShieldedUPP",
            "ICMMemoryDisposedUPP",
            "ICMConvertDataFormatUPP",
            "ModalFilterYDUPP",
			"FileFilterUPP",

            "CodecNameSpecListPtr",
            "CodecInfo",
             "ImageSequence",
            "MatrixRecordPtr",
            "ICMDataProcRecordPtr",
            "OpenCPicParams",
            "ICMProgressProcRecordPtr",
            "ICMAlignmentProcRecordPtr",
            "ICMPixelFormatInfoPtr",
            "ImageSequenceDataSource",
            "ConstStrFileNameParam",
            "ImageTranscodeSequence",
            "ImageFieldSequence",
            "Fract",
            "PixMapPtr",
            "GWorldFlags",
            "void_ptr",   # XXX Being lazy, this one is doable.
            
            # These are from QuickTimeComponents
            "CDataHandlerUPP",
            "CharDataHandlerUPP",
            "CommentHandlerUPP",
            "DataHCompletionUPP",
            "'MovieExportGetDataUPP",
            "MovieExportGetPropertyUPP",
            "PreprocessInstructionHandlerUPP",
            "SGModalFilterUPP",
            "StartDocumentHandlerUPP",
            "StartElementHandlerUPP",
            "VdigIntUPP",
            "SGDataUPP",
            "EndDocumentHandlerUPP",
            "EndElementHandlerUPP",
            "VideoBottles", # Record full of UPPs
            
            "SCParams",
            "ICMCompletionProcRecordPtr",
            "DataHVolumeList",
            "DigitizerInfo",
            "SGCompressInfo",
            "SeqGrabExtendedFrameInfoPtr",
            "SeqGrabFrameInfoPtr",
            "TCTextOptionsPtr",
            "SGCompressInfo_ptr",
            "SGDeviceList",
            "TextDisplayData",
            "TimeCodeDef",
            "TimeCodeRecord",
            "TweenRecord",
            "VDGamRecPtr",
            "ToneDescription", 	# XXXX Just lazy: this one is easy.
            "XMLDoc",
            "UInt64", 	# XXXX lazy
            "UInt64_ptr", # XXXX lazy
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
			([('TimeRecord', 'theTime', 'OutMode')], [('TimeRecord', 'theTime', 'InOutMode')]),
			
			# AddTime and SubtractTime
			([('TimeRecord', 'dst', 'OutMode')], [('TimeRecord', 'dst', 'InOutMode')]),
			
			# Funny definitions
			([('char_ptr', '*', 'InMode')], [('stringptr', '*', 'InMode')]),
			([('FSSpecPtr', '*', 'InMode')], [('FSSpec_ptr', '*', 'InMode')]),
			([('unsigned_char', 'swfVersion', 'OutMode')], [('UInt8', 'swfVersion', 'OutMode')]),
			]
			
if __name__ == "__main__":
	main()

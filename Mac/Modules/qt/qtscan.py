# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "QuickTime"
SHORT = "qt"
HEADERFILES= (
#       "Components.h"  -- In Carbon.Cm
        "Movies.h",
        "ImageCompression.h",
        "QuickTimeComponents.h",
#       "ImageCodec.h"  -- seems not too useful, and difficult.
#       "IsochronousDataHandlers.h" -- Is this useful?
        "MediaHandlers.h",
#       "QTML.h", -- Windows only, needs separate module
#       "QuickTimeStreaming.h", -- Difficult
#       "QTStreamingComponents.h", -- Needs QTStreaming
        "QuickTimeMusic.h",
#       "QuickTimeVR.h", -- Not done yet
#       "Sound.h", -- In Carbon.Snd
        )
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
    exec(open(defsoutput).read(), {}, {})
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
        self.defsfile.write("kControllerMinimum = -0xf777\n")
        self.defsfile.write("notImplementedMusicOSErr      = -2071\n")
        self.defsfile.write("cantSendToSynthesizerOSErr    = -2072\n")
        self.defsfile.write("cantReceiveFromSynthesizerOSErr = -2073\n")
        self.defsfile.write("illegalVoiceAllocationOSErr   = -2074\n")
        self.defsfile.write("illegalPartOSErr              = -2075\n")
        self.defsfile.write("illegalChannelOSErr           = -2076\n")
        self.defsfile.write("illegalKnobOSErr              = -2077\n")
        self.defsfile.write("illegalKnobValueOSErr         = -2078\n")
        self.defsfile.write("illegalInstrumentOSErr        = -2079\n")
        self.defsfile.write("illegalControllerOSErr        = -2080\n")
        self.defsfile.write("midiManagerAbsentOSErr        = -2081\n")
        self.defsfile.write("synthesizerNotRespondingOSErr = -2082\n")
        self.defsfile.write("synthesizerOSErr              = -2083\n")
        self.defsfile.write("illegalNoteChannelOSErr       = -2084\n")
        self.defsfile.write("noteChannelNotAllocatedOSErr  = -2085\n")
        self.defsfile.write("tunePlayerFullOSErr           = -2086\n")
        self.defsfile.write("tuneParseOSErr                = -2087\n")

    def makeblacklistnames(self):
        return [
                "xmlIdentifierUnrecognized", # const with incompatible definition
                "DisposeMovie",         # Done on python-object disposal
                "DisposeMovieTrack",    # ditto
                "DisposeTrackMedia",    # ditto
                "DisposeUserData",              # ditto
#                       "DisposeTimeBase",              # ditto
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
##                      "VideoMediaGetStallCount", # Undefined in CW Pro 3 library
                # OS8 only:
                'SpriteMediaGetIndImageProperty',       # XXXX Why isn't this in carbon?
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

                # MediaHandlers
                "MediaMakeMediaTimeTable", # just lazy
                "MediaGetSampleDataPointer", # funny output pointer

                # QuickTimeMusic
                "kControllerMinimum",
                # These are artefacts of a macro definition
                "ulen",
                "_ext",
                "x",
                "w1",
                "w2",
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

                "SampleReference64Ptr", # Don't know what this does, yet
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
    "ToneDescription",  # XXXX Just lazy: this one is easy.
    "XMLDoc",
    "UInt64",   # XXXX lazy
    "UInt64_ptr", # XXXX lazy

    # From MediaHandlers
    "ActionsUPP",
    "PrePrerollCompleteUPP",
    "CodecComponentHandle", # Difficult: handle containing list of components.
    "GetMovieCompleteParams", # Immense struct
    "LevelMeterInfoPtr", # Lazy. Also: can be an output parameter!!
    "MediaEQSpectrumBandsRecordPtr", # ditto

    # From QuickTimeMusic
    "MusicMIDISendUPP",
    "MusicOfflineDataUPP",
    "TuneCallBackUPP",
    "TunePlayCallBackUPP",
    "GCPart", # Struct with lots of fields
    "GCPart_ptr",
    "GenericKnobDescription", # Struct with lots of fields
    "KnobDescription",  # Struct with lots of fields
    "InstrumentAboutInfo", # Struct, not too difficult
    "NoteChannel", # XXXX Lazy. Could be opaque, I think
    "NoteRequest", # XXXX Lazy. Not-too-difficult struct
    "SynthesizerConnections", # Struct with lots of fields
    "SynthesizerDescription", # Struct with lots of fields
    "TuneStatus", # Struct with lots of fields

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

                # It seems MusicMIDIPacket if never flagged with const but always used
                # for sending only. If that ever changes this needs to be fixed.
                ([('MusicMIDIPacket', '*', 'OutMode')], [('MusicMIDIPacket_ptr', '*', 'InMode')]),

                # QTMusic const-less input parameters
                ([('unsigned_long', 'header', 'OutMode')], [('UnsignedLongPtr', 'header', 'InMode')]),
                ]

if __name__ == "__main__":
    main()

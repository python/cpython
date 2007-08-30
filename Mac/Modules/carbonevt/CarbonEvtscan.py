# IBCarbonscan.py

import sys
import os
import string
import MacOS
import sys

from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner, Scanner_OSX

def main():
    print("---Scanning CarbonEvents.h---")
    input = ["CarbonEvents.h"]
    output = "CarbonEventsgen.py"
    defsoutput = TOOLBOXDIR + "CarbonEvents.py"
    scanner = CarbonEvents_Scanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("--done scanning, importing--")
    import CarbonEvtsupport
    print("done")

RefObjectTypes = ["EventRef",
                                "EventQueueRef",
                                "EventLoopRef",
                                "EventLoopTimerRef",
                                "EventHandlerRef",
                                "EventHandlerCallRef",
                                "EventTargetRef",
                                "EventHotKeyRef",
                                ]

class CarbonEvents_Scanner(Scanner_OSX):
    def destination(self, type, name, arglist):
        classname = "CarbonEventsFunction"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in RefObjectTypes and m == "InMode":
                if t == "EventHandlerRef":
                    classname = "EventHandlerRefMethod"
                else:
                    classname = "CarbonEventsMethod"
                listname = t + "methods"
            #else:
            #       print "not method"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("false = 0\n")
        self.defsfile.write("true = 1\n")
        self.defsfile.write("keyAEEventClass = FOUR_CHAR_CODE('evcl')\n")
        self.defsfile.write("keyAEEventID = FOUR_CHAR_CODE('evti')\n")

    def makeblacklistnames(self):
        return [
                "sHandler",
                "MacCreateEvent",
#                       "TrackMouseLocationWithOptions",
#                       "TrackMouseLocation",
#                       "TrackMouseRegion",
                "RegisterToolboxObjectClass",
                "UnregisterToolboxObjectClass",
                "ProcessHICommand",
                "GetCFRunLoopFromEventLoop",

                "InvokeEventHandlerUPP",
                "InvokeEventComparatorUPP",
                "InvokeEventLoopTimerUPP",
                "NewEventComparatorUPP",
                "NewEventLoopTimerUPP",
                "NewEventHandlerUPP",
                "DisposeEventComparatorUPP",
                "DisposeEventLoopTimerUPP",
                "DisposeEventHandlerUPP",

                # Wrote by hand
                "InstallEventHandler",
                "RemoveEventHandler",

                # Write by hand?
                "GetEventParameter",
                "FlushSpecificEventsFromQueue",
                "FindSpecificEventInQueue",
                "InstallEventLoopTimer",

                # Don't do these because they require a CFRelease
                "CreateTypeStringWithOSType",
                "CopyEvent",
                ]

#       def makeblacklisttypes(self):
#               return ["EventComparatorUPP",
#                               "EventLoopTimerUPP",
#                               #"EventHandlerUPP",
#                               "EventComparatorProcPtr",
#                               "EventLoopTimerProcPtr",
#                               "EventHandlerProcPtr",
#                               ]

    def makerepairinstructions(self):
        return [
                ([("UInt32", 'inSize', "InMode"), ("void_ptr", 'inDataPtr', "InMode")],
                 [("MyInBuffer", 'inDataPtr', "InMode")]),
                ([("Boolean", 'ioWasInRgn', "OutMode")],
                 [("Boolean", 'ioWasInRgn', "InOutMode")]),
        ]

if __name__ == "__main__":
    main()

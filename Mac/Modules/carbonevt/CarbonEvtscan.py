# IBCarbonscan.py

import sys
import os
import string
import MacOS

BGENDIR= '/Users/dsp/Documents/python/dist/src/Tools/bgen/bgen'
sys.path.append(BGENDIR)

from bgenlocations import TOOLBOXDIR

from scantools import Scanner, Scanner_OSX

def main():
	print "---Scanning CarbonEvents.h---"
	input = ["CarbonEvents.h"]
	output = "CarbonEventsgen.py"
	defsoutput = TOOLBOXDIR + "CarbonEvents.py"
	scanner = CarbonEvents_Scanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "--done scanning, importing--"
	import CarbonEventssupport
	print "done"

RefObjectTypes = ["EventRef", 
				"EventQueueRef", 
				"EventLoopRef",
				"EventLoopTimerRef",
				"EventHandlerRef",
				"EventHandlerCallRef",
				"EventTargetRef",
				"EventHotKeyRef",
				]

class CarbonEvents_Scanner(Scanner):
	def destination(self, type, name, arglist):
		classname = "CarbonEventsFunction"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			print "*********", t,
			if t in RefObjectTypes and m == "InMode":
				print "method"
				classname = "CarbonEventsMethod"
				listname = t + "methods"
			else:
				print "not method"
		return classname, listname

	def makeblacklistnames(self):
		return [
			"MacCreateEvent",
			"TrackMouseLocationWithOptions",
			"TrackMouseLocation",
			"TrackMouseRegion",
			"RegisterToolboxObjectClass",
			"UnregisterToolboxObjectClass",
			"ProcessHICommand",
			"GetCFRunLoopFromEventLoop",
			
			"InvokeEventHandlerUPP",
			"InvokeEventComparatorUPP",
			"InvokeEventLoopTimerUPP",

			# Wrote by hand
			"InstallEventHandler",
			"RunApplicationEventLoop",
						
			# Write by hand?
			"GetEventParameter",
			"FlushSpecificEventsFromQueue",
			"FindSpecificEventInQueue",
			"InstallEventLoopTimer",

			# Don't do these because they require a CFRelease
			"CreateTypeStringWithOSType",
			"CopyEvent",
			]

#	def makeblacklisttypes(self):
#		return ["EventComparatorUPP",
#				"EventLoopTimerUPP",
#				#"EventHandlerUPP",
#				"EventComparatorProcPtr",
#				"EventLoopTimerProcPtr",
#				"EventHandlerProcPtr",
#				]
if __name__ == "__main__":
	main()

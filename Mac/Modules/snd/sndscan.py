# Scan Sound.h header file, generate sndgen.py and Sound.py files.
# Then import sndsupport (which execs sndgen.py) to generate Sndmodule.c.
# (Should learn how to tell the compiler to compile it as well.)

import addpack
addpack.addpack(':Tools:bgen:bgen')
from bgenlocations import TOOLBOXDIR

from scantools import Scanner

def main():
	input = "Sound.h"
	output = "sndgen.py"
	defsoutput = TOOLBOXDIR + "Sound.py"
	scanner = SoundScanner(input, output, defsoutput)
	scanner.scan()
	scanner.close()
	print "=== Done scanning and generating, now doing 'import sndsupport' ==="
	import sndsupport
	print "=== Done.  It's up to you to compile Sndmodule.c ==="

class SoundScanner(Scanner):

	def destination(self, type, name, arglist):
		classname = "SndFunction"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t == "SndChannelPtr" and m == "InMode":
				classname = "SndMethod"
				listname = "sndmethods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

	def makeblacklistnames(self):
		return [
			'SndDisposeChannel',		# automatic on deallocation
			'SndAddModifier',		# for internal use only
			'SndPlayDoubleBuffer',		# very low level routine
			# Obsolete:
			'StartSound',
			'StopSound',
			'SoundDone',
			# These are soundMgr 3.0 routines that I can't seem to find...
			'GetSoundPreference',
			'SetSoundPreference',
			'GetCompressionInfo',
			'GetCompressionName',
			# Calls with void_ptr arguments (to be done).
			'SndGetInfo',
			'SndSetInfo',
			'SPBGetDeviceInfo',
			'SPBSetDeviceInfo',
			# And old calls that are no longer supported
			'SetSoundVol',
			'GetSoundVol',

			]

	def makeblacklisttypes(self):
		return [
			"GetSoundVol",
			"SetSoundVol",
			"UnsignedFixed",
			# Don't have the time to dig into this...
			"Component",
			"ComponentInstance",
			"SoundComponentDataPtr",
			"SoundComponentData",
			"SoundComponentData_ptr",
			"SoundConverter",
			"ModalFilterUPP",
			"SPBPtr",
			]

	def makerepairinstructions(self):
		return [
			([("Str255", "*", "InMode")],
			 [("*", "*", "OutMode")]),
			
			([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
			 [("InBuffer", "*", "*")]),
			
			([("void", "*", "OutMode"), ("long", "*", "InMode"),
			                            ("long", "*", "OutMode")],
			 [("VarVarOutBuffer", "*", "InOutMode")]),
			
			([("SCStatusPtr", "*", "InMode")],
			 [("SCStatus", "*", "OutMode")]),
			
			([("SMStatusPtr", "*", "InMode")],
			 [("SMStatus", "*", "OutMode")]),
			 
			([("CompressionInfoPtr", "*", "InMode")],
			 [("CompressionInfo", "*", "OutMode")]),
			
			# For SndPlay's SndListHandle argument
			([("Handle", "sndHdl", "InMode")],
			 [("SndListHandle", "*", "*")]),
			 
			# For SndStartFilePlay
			([("long", "bufferSize", "InMode"), ("void", "theBuffer", "OutMode")],
			 [("*", "*", "*"), ("FakeType('0')", "*", "InMode")]),
			
			# For Comp3to1 etc.
			([("void_ptr", "inBuffer", "InMode"),
			  ("void", "outBuffer", "OutMode"),
			  ("unsigned_long", "cnt", "InMode")],
			 [("InOutBuffer", "buffer", "InOutMode")]),
			
			# Ditto
##			([("void_ptr", "inState", "InMode"), ("void", "outState", "OutMode")],
##			 [("InOutBuf128", "state", "InOutMode")]),
			([("StateBlockPtr", "inState", "InMode"), ("StateBlockPtr", "outState", "InMode")],
			 [("StateBlock", "state", "InOutMode")]),

			]

if __name__ == "__main__":
	main()

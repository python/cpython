# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
BGENDIR=os.path.join(sys.prefix, ':Tools:bgen:bgen')
sys.path.append(BGENDIR)
from scantools import Scanner_OSX
from bgenlocations import TOOLBOXDIR

LONG = "CoreFoundation"
SHORT = "cf"
OBJECTS = ("CFTypeRef", 
		"CFArrayRef", "CFMutableArrayRef",
		"CFDataRef", "CFMutableDataRef",
		"CFDictionaryRef", "CFMutableDictionaryRef",
		"CFStringRef", "CFMutableStringRef",
		"CFURLRef",
		)
# ADD object typenames here

def main():
	input = [
		"CFBase.h",
		"CFArray.h",
##		"CFBag.h",
##		"CFBundle.h",
##		"CFCharacterSet.h",
		"CFData.h",
##		"CFDate.h",
		"CFDictionary.h",
##		"CFNumber.h",
##		"CFPlugIn.h",
##		"CFPreferences.h",
##		"CFPropertyList.h",
##		"CFSet.h",
		"CFString.h",
##		"CFStringEncodingExt.h",
##		"CFTimeZone.h",
		"CFURL.h",
		]
	output = SHORT + "gen.py"
	defsoutput = TOOLBOXDIR + LONG + ".py"
	scanner = MyScanner(input, output, defsoutput)
	scanner.scan()
	scanner.gentypetest(SHORT+"typetest.py")
	scanner.close()
	print "=== Done scanning and generating, now importing the generated code... ==="
	exec "import " + SHORT + "support"
	print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner_OSX):

	def destination(self, type, name, arglist):
		classname = "Function"
		listname = "functions"
		if arglist:
			t, n, m = arglist[0]
			if t in OBJECTS and m == "InMode":
				classname = "Method"
				listname = t + "_methods"
			# Special case for the silly first AllocatorRef argument
			if t == 'CFAllocatorRef' and m == 'InMode' and len(arglist) > 1:
				t, n, m = arglist[1]
				if t in OBJECTS and m == "InMode":
					classname = "MethodSkipArg1"
					listname = t + "_methods"
		return classname, listname

	def writeinitialdefs(self):
		self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

	def makeblacklistnames(self):
		return [
			# Memory allocator functions
			"CFAllocatorGetDefault",
			"CFAllocatorSetDefault",
			"CFAllocatorAllocate",
			"CFAllocatorReallocate",
			"CFAllocatorDeallocate",
			"CFGetAllocator",
			# Array functions we skip for now.
			"CFArrayGetValueAtIndex",
			# Data pointer functions. Skip for now.
			"CFDataGetBytePtr",
			"CFDataGetMutableBytePtr",
			"CFDataGetBytes",   # XXXX Should support this one
			# String functions
			"CFStringGetPascalString", # Use the C-string methods.
			"CFStringGetPascalStringPtr", # TBD automatically
			"CFStringGetCStringPtr", 
			"CFStringGetCharactersPtr",
			"CFStringGetCString", 
			"CFStringGetCharacters",
			"CFURLCreateStringWithFileSystemPath", # Gone in later releases
			]

	def makegreylist(self):
		return []

	def makeblacklisttypes(self):
		return [
			"CFComparatorFunction", # Callback function pointer
			"CFAllocatorContext", # Not interested in providing our own allocator
			"void_ptr_ptr",  # Tricky. This is the initializer for arrays...
			"void_ptr", # Ditto for various array lookup methods
			"CFArrayApplierFunction", # Callback function pointer
			"CFDictionaryApplierFunction", # Callback function pointer
			"UniChar_ptr", # XXXX To be done
			"const_UniChar_ptr", # XXXX To be done
			"UniChar", # XXXX To be done
			"va_list", # For printf-to-a-cfstring. Use Python.
			"const_CFStringEncoding_ptr", # To be done, I guess
			]

	def makerepairinstructions(self):
		return [
			# Buffers in CF seem to be passed as UInt8 * normally.
			([("UInt8_ptr", "*", "InMode"), ("CFIndex", "*", "InMode")],
			 [("UcharInBuffer", "*", "*")]),
			 
			# Some functions return a const char *. Don't worry, we won't modify it.
			([("const_char_ptr", "*", "ReturnMode")],
			 [("return_stringptr", "*", "*")]),
			 
			# base URLs are optional (pass None for NULL)
			([("CFURLRef", "baseURL", "InMode")],
			 [("OptionalCFURLRef", "*", "*")]),
			 
			]
			
if __name__ == "__main__":
	main()

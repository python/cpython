"""\
Augment the "bgen" package with definitions that are useful on the Apple Macintosh.

Intended usage is "from macsupport import *" -- this implies all bgen's goodies.
"""


# Import everything from bgen (for ourselves as well as for re-export)
from bgen import *


# Simple types
Boolean = Type("Boolean", "b")
SignedByte = Type("SignedByte", "b")
ScriptCode = Type("ScriptCode", "h")
Size = Type("Size", "l")
Style = Type("Style", "b")
StyleParameter = Type("StyleParameter", "h")
CharParameter = Type("CharParameter", "h")
TextEncoding = Type("TextEncoding", "l")

UInt8 = Type("UInt8", "b")
SInt8 = Type("SInt8", "b")
UInt16 = Type("UInt16", "H")
SInt16 = Type("SInt16", "h")
UInt32 = Type("UInt32", "l")
SInt32 = Type("SInt32", "l")

# Pascal strings
ConstStr255Param = OpaqueArrayType("Str255", "PyMac_BuildStr255", "PyMac_GetStr255")
Str255 = OpaqueArrayType("Str255", "PyMac_BuildStr255", "PyMac_GetStr255")

# File System Specifications
FSSpec_ptr = OpaqueType("FSSpec", "PyMac_BuildFSSpec", "PyMac_GetFSSpec")
FSSpec = OpaqueByValueType("FSSpec", "PyMac_BuildFSSpec", "PyMac_GetFSSpec")

# OSType and ResType: 4-byte character strings
def OSTypeType(typename):
	return OpaqueByValueType(typename, "PyMac_BuildOSType", "PyMac_GetOSType")
OSType = OSTypeType("OSType")
ResType = OSTypeType("ResType")

# Version numbers
NumVersion = OpaqueByValueType("NumVersion", "PyMac_BuildNumVersion", "BUG")

# Handles (always resources in our case)
Handle = OpaqueByValueType("Handle", "ResObj")
MenuHandle = OpaqueByValueType("MenuHandle", "MenuObj")
MenuRef = MenuHandle
ControlHandle = OpaqueByValueType("ControlHandle", "CtlObj")
ControlRef = ControlHandle

# Windows and Dialogs
WindowPtr = OpaqueByValueType("WindowPtr", "WinObj")
WindowRef = WindowPtr
DialogPtr = OpaqueByValueType("DialogPtr", "DlgObj")
DialogRef = DialogPtr
ExistingWindowPtr = OpaqueByValueType("WindowPtr", "WinObj_WhichWindow", "BUG")
ExistingDialogPtr = OpaqueByValueType("DialogPtr", "WinObj_WhichWindow", "BUG")

# NULL pointer passed in as optional storage -- not present in Python version
NullStorage = FakeType("(void *)0")

# More standard datatypes
Fixed = OpaqueByValueType("Fixed", "PyMac_BuildFixed", "PyMac_GetFixed")

# Quickdraw data types
Rect = Rect_ptr = OpaqueType("Rect", "PyMac_BuildRect", "PyMac_GetRect")
Point = OpaqueByValueType("Point", "PyMac_BuildPoint", "PyMac_GetPoint")
Point_ptr = OpaqueType("Point", "PyMac_BuildPoint", "PyMac_GetPoint")

# Event records
EventRecord = OpaqueType("EventRecord", "PyMac_BuildEventRecord", "PyMac_GetEventRecord")
EventRecord_ptr = EventRecord

# OSErr is special because it is turned into an exception
# (Could do this with less code using a variant of mkvalue("O&")?)
class OSErrType(Type):
	def errorCheck(self, name):
		Output("if (%s != noErr) return PyMac_Error(%s);", name, name)
		self.used = 1
OSErr = OSErrType("OSErr", 'h')
OSStatus = OSErrType("OSStatus", 'l')


# Various buffer types

InBuffer = VarInputBufferType('char', 'long', 'l')		# (buf, len)
OptionalInBuffer = OptionalVarInputBufferType('char', 'long', 'l')		# (buf, len)

InOutBuffer = HeapInputOutputBufferType('char', 'long', 'l')	# (inbuf, outbuf, len)
VarInOutBuffer = VarHeapInputOutputBufferType('char', 'long', 'l') # (inbuf, outbuf, &len)

OutBuffer = HeapOutputBufferType('char', 'long', 'l')		# (buf, len)
VarOutBuffer = VarHeapOutputBufferType('char', 'long', 'l')	# (buf, &len)
VarVarOutBuffer = VarVarHeapOutputBufferType('char', 'long', 'l') # (buf, len, &len)


# Predefine various pieces of program text to be passed to Module() later:

# Stuff added immediately after the system include files
includestuff = """
#include "macglue.h"
#include "pymactoolbox.h"
"""

# Stuff added just before the module's init function
finalstuff = """
"""

# Stuff added inside the module's init function
initstuff = """
"""


# Generator classes with a twist -- if the function returns OSErr,
# its mode is manipulated so that it turns into an exception or disappears
# (and its name is changed to _err, for documentation purposes).
# This requires that the OSErr type (defined above) has a non-trivial
# errorCheck method.
class OSErrMixIn:
	"Mix-in class to treat OSErr/OSStatus return values special"
	def makereturnvar(self):
		if self.returntype.__class__ == OSErrType:
			return Variable(self.returntype, "_err", ErrorMode)
		else:
			return Variable(self.returntype, "_rv", OutMode)

class OSErrFunctionGenerator(OSErrMixIn, FunctionGenerator): pass
class OSErrMethodGenerator(OSErrMixIn, MethodGenerator): pass


class MacModule(Module):
	"Subclass which gets the exception initializer from macglue.c"
	def exceptionInitializer(self):
		return "PyMac_GetOSErrException()"

_SetOutputFileName = SetOutputFileName # Save original
def SetOutputFileName(file = None):
	"Set the output file name and set its creator&type to CWIE&TEXT"
	_SetOutputFileName(file)
	if file:
		import MacOS
		MacOS.SetCreatorAndType(file, 'CWIE', 'TEXT')

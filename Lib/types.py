# Define names for all type symbols known in the standard interpreter.
# Types that are part of optional modules (e.g. array) are not listed.

import sys

NoneType = type(None)
TypeType = type(NoneType)

IntType = type(0)
LongType = type(0L)
FloatType = type(0.0)

StringType = type('')

TupleType = type(())
ListType = type([])
DictionaryType = type({})

def func(): pass
FunctionType = type(func)

class C:
	def meth(self): pass
ClassType = type(C)
UnboundMethodType = type(C.meth)	# Same as MethodType
x = C()
InstanceType = type(x)
MethodType = type(x.meth)

BuiltinFunctionType = type(len)		# Also used for built-in methods

ModuleType = type(sys)

FileType = type(sys.stdin)
XRangeType = type(xrange(0))

try:
	raise TypeError
except TypeError:
	TracebackType = type(sys.exc_traceback)
	FrameType = type(sys.exc_traceback.tb_frame)

del sys, func, C, x			# These are not for export

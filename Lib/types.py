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
DictType = DictionaryType = type({})

def _f(): pass
FunctionType = type(_f)
LambdaType = type(lambda: None)		# Same as FunctionType
CodeType = type(_f.func_code)

class _C:
	def _m(self): pass
ClassType = type(_C)
UnboundMethodType = type(_C._m)		# Same as MethodType
_x = _C()
InstanceType = type(_x)
MethodType = type(_x._m)

BuiltinFunctionType = type(len)
BuiltinMethodType = type([].append)	# Same as BuiltinFunctionType

ModuleType = type(sys)

FileType = type(sys.stdin)		# XXX what if it was assigned to?
XRangeType = type(xrange(0))

try:
	raise TypeError
except TypeError:
	TracebackType = type(sys.exc_traceback)
	FrameType = type(sys.exc_traceback.tb_frame)

del sys, _f, _C, _x			# Not for export

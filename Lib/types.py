"""Define names for all type symbols known in the standard interpreter.

Types that are part of optional modules (e.g. array) are not listed.
"""

import sys

NoneType = type(None)
TypeType = type(NoneType)

IntType = type(0)
LongType = type(0L)
FloatType = type(0.0)
try:
    ComplexType = type(complex(0,1))
except NameError:
    pass

StringType = type('')
UnicodeType = type(u'')
BufferType = type(buffer(''))

TupleType = type(())
ListType = type([])
DictType = DictionaryType = type({})

def _f(): pass
FunctionType = type(_f)
LambdaType = type(lambda: None)         # Same as FunctionType
try:
    CodeType = type(_f.func_code)
except:
    pass

class _C:
    def _m(self): pass
ClassType = type(_C)
UnboundMethodType = type(_C._m)         # Same as MethodType
_x = _C()
InstanceType = type(_x)
MethodType = type(_x._m)

BuiltinFunctionType = type(len)
BuiltinMethodType = type([].append)     # Same as BuiltinFunctionType

ModuleType = type(sys)

try:
    FileType = type(sys.__stdin__)
except:
    pass
XRangeType = type(xrange(0))

try:
    raise TypeError
except TypeError:
    try:
        tb = sys.exc_info()[2]
        TracebackType = type(tb)
        FrameType = type(tb.tb_frame)
    except:
        pass
    tb = None; del tb

SliceType = type(slice(0))
EllipsisType = type(Ellipsis)

del sys, _f, _C, _x                     # Not for export

"""Define names for all type symbols known in the standard interpreter.

Types that are part of optional modules (e.g. array) are not listed.
"""
import sys

# Iterators in Python aren't a matter of type but of protocol.  A large
# and changing number of builtin types implement *some* flavor of
# iterator.  Don't check the type!  Use hasattr to check for both
# "__iter__" and "__next__" attributes instead.

NoneType = type(None)
TypeType = type
ObjectType = object

IntType = int
LongType = int
FloatType = float
BooleanType = bool
try:
    ComplexType = complex
except NameError:
    pass

BufferType = buffer

TupleType = tuple
ListType = list
DictType = DictionaryType = dict

def _f(): pass
FunctionType = type(_f)
LambdaType = type(lambda: None)         # Same as FunctionType
try:
    CodeType = type(_f.__code__)
except RuntimeError:
    # Execution in restricted environment
    pass

def _g():
    yield 1
GeneratorType = type(_g())

class _C:
    def _m(self): pass
ClassType = type(_C)
UnboundMethodType = type(_C._m)         # Same as MethodType
MethodType = type(_C()._m)

BuiltinFunctionType = type(len)
BuiltinMethodType = type([].append)     # Same as BuiltinFunctionType

ModuleType = type(sys)

try:
    raise TypeError
except TypeError:
    try:
        tb = sys.exc_info()[2]
        TracebackType = type(tb)
        FrameType = type(tb.tb_frame)
    except AttributeError:
        # In the restricted environment, exc_info returns (None, None,
        # None) Then, tb.tb_frame gives an attribute error
        pass
    tb = None; del tb

SliceType = slice
EllipsisType = type(Ellipsis)

DictProxyType = type(TypeType.__dict__)
NotImplementedType = type(NotImplemented)

# Extension types defined in a C helper module.  XXX There may be no
# equivalent in implementations other than CPython, so it seems better to
# leave them undefined then to set them to e.g. None.
try:
    import _types
except ImportError:
    pass
else:
    GetSetDescriptorType = type(_types.Helper.getter)
    MemberDescriptorType = type(_types.Helper.member)
    del _types

del sys, _f, _g, _C,                              # Not for export

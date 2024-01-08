# Some classes and types are not export to _ctypes module directly.

from _ctypes import Structure, Union, _Pointer, Array, _SimpleCData, CFuncPtr


_CData = Structure.__base__
assert _CData.__name__ == "_CData"

# metaclasses
PyCStructType = type(Structure)
UnionType = type(Union)
PyCPointerType = type(_Pointer)
PyCArrayType = type(Array)
PyCSimpleType = type(_SimpleCData)
PyCFuncPtrType = type(CFuncPtr)

# type flags
PY_TPFLAGS_DISALLOW_INSTANTIATION = 1 << 7
PY_TPFLAGS_IMMUTABLETYPE = 1 << 8

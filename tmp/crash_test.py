import types, ctypes
class NoVectorCall:
    def __class_getitem__(cls, item):
        return types.GenericAlias(cls, item)
    def __new__(cls, x):
        return x * 2

print(hasattr(NoVectorCall, "__vectorcalloffset__"))
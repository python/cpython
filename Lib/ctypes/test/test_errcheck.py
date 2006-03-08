import sys
from ctypes import *

##class HMODULE(Structure):
##    _fields_ = [("value", c_void_p)]

##    def __repr__(self):
##        return "<HMODULE %s>" % self.value

##windll.kernel32.GetModuleHandleA.restype = HMODULE

##print windll.kernel32.GetModuleHandleA("python23.dll")
##print hex(sys.dllhandle)

##def nonzero(handle):
##    return (GetLastError(), handle)

##windll.kernel32.GetModuleHandleA.errcheck = nonzero
##print windll.kernel32.GetModuleHandleA("spam")

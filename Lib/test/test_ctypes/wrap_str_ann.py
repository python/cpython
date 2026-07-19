from __future__ import annotations
import ctypes.util

def test_ann() -> ctypes.c_char_p:
    ...

# Check that "from __future__ import annotations" works as expected
if not isinstance(test_ann.__annotations__['return'], str):
    raise Exception("annotations must be strings")

@ctypes.util.wrap_dll_function(ctypes.pythonapi)
def Py_GetVersion() -> ctypes.c_char_p:
    ...

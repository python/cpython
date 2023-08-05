/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(create_pointer_type__doc__,
"POINTER($module, type, /)\n"
"--\n"
"\n"
"Creates and returns a new ctypes pointer type.\n"
"\n"
"Pointer types are cached and reused internally, so calling this function\n"
"repeatedly is cheap. \'type\' must be a ctypes type.");

#define CREATE_POINTER_TYPE_METHODDEF    \
    {"POINTER", (PyCFunction)create_pointer_type, METH_O, create_pointer_type__doc__},

PyDoc_STRVAR(create_pointer_inst__doc__,
"pointer($module, obj, /)\n"
"--\n"
"\n"
"Creates a new pointer instance, pointing to \'obj\'.\n"
"\n"
"The returned object is of the type POINTER(type(obj)). Note that if you just\n"
"want to pass a pointer to an object to a foreign function call,\n"
"you should use byref(obj) which is much faster.");

#define CREATE_POINTER_INST_METHODDEF    \
    {"pointer", (PyCFunction)create_pointer_inst, METH_O, create_pointer_inst__doc__},
/*[clinic end generated code: output=923c1c75ce2705e3 input=a9049054013a1b77]*/

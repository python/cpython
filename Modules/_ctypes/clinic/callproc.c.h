/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(create_pointer_type__doc__,
"POINTER($module, type, /)\n"
"--\n"
"\n"
"Create and return a new ctypes pointer type.\n"
"\n"
"  type\n"
"    A ctypes type.\n"
"\n"
"Pointer types are cached and reused internally,\n"
"so calling this function repeatedly is cheap.");

#define CREATE_POINTER_TYPE_METHODDEF    \
    {"POINTER", (PyCFunction)create_pointer_type, METH_O, create_pointer_type__doc__},

PyDoc_STRVAR(create_pointer_inst__doc__,
"pointer($module, obj, /)\n"
"--\n"
"\n"
"Create a new pointer instance, pointing to \'obj\'.\n"
"\n"
"The returned object is of the type POINTER(type(obj)). Note that if you\n"
"just want to pass a pointer to an object to a foreign function call, you\n"
"should use byref(obj) which is much faster.");

#define CREATE_POINTER_INST_METHODDEF    \
    {"pointer", (PyCFunction)create_pointer_inst, METH_O, create_pointer_inst__doc__},
/*[clinic end generated code: output=51b311ea369e5adf input=a9049054013a1b77]*/

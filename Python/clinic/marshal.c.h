/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(marshal_dump__doc__,
"dump($module, value, file, version=version, /)\n"
"--\n"
"\n"
"Write the value on the open file.\n"
"\n"
"  value\n"
"    Must be a supported type.\n"
"  file\n"
"    Must be a writeable binary file.\n"
"  version\n"
"    Indicates the data format that dump should use.\n"
"\n"
"If the value has (or contains an object that has) an unsupported type, a\n"
"ValueError exception is raised - but garbage data will also be written\n"
"to the file. The object will not be properly read back by load().");

#define MARSHAL_DUMP_METHODDEF    \
    {"dump", _PyCFunction_CAST(marshal_dump), METH_FASTCALL, marshal_dump__doc__},

static PyObject *
marshal_dump_impl(PyObject *module, PyObject *value, PyObject *file,
                  int version);

static PyObject *
marshal_dump(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *value;
    PyObject *file;
    int version = Py_MARSHAL_VERSION;

    if (!_PyArg_CheckPositional("dump", nargs, 2, 3)) {
        goto exit;
    }
    value = args[0];
    file = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    version = PyLong_AsInt(args[2]);
    if (version == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = marshal_dump_impl(module, value, file, version);

exit:
    return return_value;
}

PyDoc_STRVAR(marshal_load__doc__,
"load($module, file, /)\n"
"--\n"
"\n"
"Read one value from the open file and return it.\n"
"\n"
"  file\n"
"    Must be readable binary file.\n"
"\n"
"If no valid value is read (e.g. because the data has a different Python\n"
"version\'s incompatible marshal format), raise EOFError, ValueError or\n"
"TypeError.\n"
"\n"
"Note: If an object containing an unsupported type was marshalled with\n"
"dump(), load() will substitute None for the unmarshallable type.");

#define MARSHAL_LOAD_METHODDEF    \
    {"load", (PyCFunction)marshal_load, METH_O, marshal_load__doc__},

PyDoc_STRVAR(marshal_dumps__doc__,
"dumps($module, value, version=version, /)\n"
"--\n"
"\n"
"Return the bytes object that would be written to a file by dump(value, file).\n"
"\n"
"  value\n"
"    Must be a supported type.\n"
"  version\n"
"    Indicates the data format that dumps should use.\n"
"\n"
"Raise a ValueError exception if value has (or contains an object that has) an\n"
"unsupported type.");

#define MARSHAL_DUMPS_METHODDEF    \
    {"dumps", _PyCFunction_CAST(marshal_dumps), METH_FASTCALL, marshal_dumps__doc__},

static PyObject *
marshal_dumps_impl(PyObject *module, PyObject *value, int version);

static PyObject *
marshal_dumps(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *value;
    int version = Py_MARSHAL_VERSION;

    if (!_PyArg_CheckPositional("dumps", nargs, 1, 2)) {
        goto exit;
    }
    value = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    version = PyLong_AsInt(args[1]);
    if (version == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = marshal_dumps_impl(module, value, version);

exit:
    return return_value;
}

PyDoc_STRVAR(marshal_loads__doc__,
"loads($module, bytes, /)\n"
"--\n"
"\n"
"Convert the bytes-like object to a value.\n"
"\n"
"If no valid value is found, raise EOFError, ValueError or TypeError.  Extra\n"
"bytes in the input are ignored.");

#define MARSHAL_LOADS_METHODDEF    \
    {"loads", (PyCFunction)marshal_loads, METH_O, marshal_loads__doc__},

static PyObject *
marshal_loads_impl(PyObject *module, Py_buffer *bytes);

static PyObject *
marshal_loads(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer bytes = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &bytes, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = marshal_loads_impl(module, &bytes);

exit:
    /* Cleanup for bytes */
    if (bytes.obj) {
       PyBuffer_Release(&bytes);
    }

    return return_value;
}
/*[clinic end generated code: output=92d2d47aac9128ee input=a9049054013a1b77]*/

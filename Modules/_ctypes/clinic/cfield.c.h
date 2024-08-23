/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

static PyObject *
PyCField_new_impl(PyTypeObject *type, PyObject *name, PyObject *proto,
                  Py_ssize_t size, Py_ssize_t offset, Py_ssize_t index,
                  PyObject *bit_size_obj, int swapped_bytes, int _ms,
                  PyObject *pack_obj, PyObject *state_to_check,
                  Py_ssize_t padding);

static PyObject *
PyCField_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 11
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(name), &_Py_ID(type), &_Py_ID(size), &_Py_ID(offset), &_Py_ID(index), &_Py_ID(bit_size), &_Py_ID(swapped_bytes), &_Py_ID(_ms), &_Py_ID(pack), &_Py_ID(state_to_check), &_Py_ID(padding), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "type", "size", "offset", "index", "bit_size", "swapped_bytes", "_ms", "pack", "state_to_check", "padding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "CField",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[11];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 5;
    PyObject *name;
    PyObject *proto;
    Py_ssize_t size;
    Py_ssize_t offset;
    Py_ssize_t index;
    PyObject *bit_size_obj = Py_None;
    int swapped_bytes = 0;
    int _ms = 0;
    PyObject *pack_obj = Py_None;
    PyObject *state_to_check = NULL;
    Py_ssize_t padding = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 5, 11, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("CField", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    proto = fastargs[1];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[3]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[4]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[5]) {
        bit_size_obj = fastargs[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[6]) {
        swapped_bytes = PyObject_IsTrue(fastargs[6]);
        if (swapped_bytes < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[7]) {
        _ms = PyObject_IsTrue(fastargs[7]);
        if (_ms < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[8]) {
        pack_obj = fastargs[8];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[9]) {
        state_to_check = fastargs[9];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[10]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        padding = ival;
    }
skip_optional_pos:
    return_value = PyCField_new_impl(type, name, proto, size, offset, index, bit_size_obj, swapped_bytes, _ms, pack_obj, state_to_check, padding);

exit:
    return return_value;
}
/*[clinic end generated code: output=07d746db3e0847a2 input=a9049054013a1b77]*/

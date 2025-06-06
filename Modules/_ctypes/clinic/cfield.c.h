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
                  Py_ssize_t byte_size, Py_ssize_t byte_offset,
                  Py_ssize_t index, int _internal_use,
                  PyObject *bit_size_obj, PyObject *bit_offset_obj);

static PyObject *
PyCField_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(type), &_Py_ID(byte_size), &_Py_ID(byte_offset), &_Py_ID(index), &_Py_ID(_internal_use), &_Py_ID(bit_size), &_Py_ID(bit_offset), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "type", "byte_size", "byte_offset", "index", "_internal_use", "bit_size", "bit_offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "CField",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 6;
    PyObject *name;
    PyObject *proto;
    Py_ssize_t byte_size;
    Py_ssize_t byte_offset;
    Py_ssize_t index;
    int _internal_use;
    PyObject *bit_size_obj = Py_None;
    PyObject *bit_offset_obj = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 6, /*varpos*/ 0, argsbuf);
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
        byte_size = ival;
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
        byte_offset = ival;
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
    _internal_use = PyObject_IsTrue(fastargs[5]);
    if (_internal_use < 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[6]) {
        bit_size_obj = fastargs[6];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    bit_offset_obj = fastargs[7];
skip_optional_kwonly:
    return_value = PyCField_new_impl(type, name, proto, byte_size, byte_offset, index, _internal_use, bit_size_obj, bit_offset_obj);

exit:
    return return_value;
}
/*[clinic end generated code: output=7eb1621e22ea2e05 input=a9049054013a1b77]*/

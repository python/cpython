/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_testcapi_dict_check__doc__,
"dict_check($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_CHECK_METHODDEF    \
    {"dict_check", (PyCFunction)_testcapi_dict_check, METH_O, _testcapi_dict_check__doc__},

PyDoc_STRVAR(_testcapi_dict_checkexact__doc__,
"dict_checkexact($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_CHECKEXACT_METHODDEF    \
    {"dict_checkexact", (PyCFunction)_testcapi_dict_checkexact, METH_O, _testcapi_dict_checkexact__doc__},

PyDoc_STRVAR(_testcapi_dict_new__doc__,
"dict_new($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_NEW_METHODDEF    \
    {"dict_new", (PyCFunction)_testcapi_dict_new, METH_NOARGS, _testcapi_dict_new__doc__},

static PyObject *
_testcapi_dict_new_impl(PyObject *module);

static PyObject *
_testcapi_dict_new(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_dict_new_impl(module);
}

PyDoc_STRVAR(_testcapi_dictproxy_new__doc__,
"dictproxy_new($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICTPROXY_NEW_METHODDEF    \
    {"dictproxy_new", (PyCFunction)_testcapi_dictproxy_new, METH_O, _testcapi_dictproxy_new__doc__},

PyDoc_STRVAR(_testcapi_dict_clear__doc__,
"dict_clear($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_CLEAR_METHODDEF    \
    {"dict_clear", (PyCFunction)_testcapi_dict_clear, METH_O, _testcapi_dict_clear__doc__},

PyDoc_STRVAR(_testcapi_dict_copy__doc__,
"dict_copy($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_COPY_METHODDEF    \
    {"dict_copy", (PyCFunction)_testcapi_dict_copy, METH_O, _testcapi_dict_copy__doc__},

PyDoc_STRVAR(_testcapi_dict_contains__doc__,
"dict_contains($module, obj, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_CONTAINS_METHODDEF    \
    {"dict_contains", _PyCFunction_CAST(_testcapi_dict_contains), METH_FASTCALL, _testcapi_dict_contains__doc__},

static PyObject *
_testcapi_dict_contains_impl(PyObject *module, PyObject *obj, PyObject *key);

static PyObject *
_testcapi_dict_contains(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *key;

    if (!_PyArg_CheckPositional("dict_contains", nargs, 2, 2)) {
        goto exit;
    }
    obj = args[0];
    key = args[1];
    return_value = _testcapi_dict_contains_impl(module, obj, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_size__doc__,
"dict_size($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_SIZE_METHODDEF    \
    {"dict_size", (PyCFunction)_testcapi_dict_size, METH_O, _testcapi_dict_size__doc__},

PyDoc_STRVAR(_testcapi_dict_getitem__doc__,
"dict_getitem($module, mapping, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_GETITEM_METHODDEF    \
    {"dict_getitem", _PyCFunction_CAST(_testcapi_dict_getitem), METH_FASTCALL, _testcapi_dict_getitem__doc__},

static PyObject *
_testcapi_dict_getitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key);

static PyObject *
_testcapi_dict_getitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *key;

    if (!_PyArg_CheckPositional("dict_getitem", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    key = args[1];
    return_value = _testcapi_dict_getitem_impl(module, mapping, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_getitemstring__doc__,
"dict_getitemstring($module, mapping, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_GETITEMSTRING_METHODDEF    \
    {"dict_getitemstring", _PyCFunction_CAST(_testcapi_dict_getitemstring), METH_FASTCALL, _testcapi_dict_getitemstring__doc__},

static PyObject *
_testcapi_dict_getitemstring_impl(PyObject *module, PyObject *mapping,
                                  const char *key, Py_ssize_t key_length);

static PyObject *
_testcapi_dict_getitemstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    const char *key;
    Py_ssize_t key_length;

    if (!_PyArg_ParseStack(args, nargs, "Oy#:dict_getitemstring",
        &mapping, &key, &key_length)) {
        goto exit;
    }
    return_value = _testcapi_dict_getitemstring_impl(module, mapping, key, key_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_getitemwitherror__doc__,
"dict_getitemwitherror($module, mapping, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_GETITEMWITHERROR_METHODDEF    \
    {"dict_getitemwitherror", _PyCFunction_CAST(_testcapi_dict_getitemwitherror), METH_FASTCALL, _testcapi_dict_getitemwitherror__doc__},

static PyObject *
_testcapi_dict_getitemwitherror_impl(PyObject *module, PyObject *mapping,
                                     PyObject *key);

static PyObject *
_testcapi_dict_getitemwitherror(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *key;

    if (!_PyArg_CheckPositional("dict_getitemwitherror", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    key = args[1];
    return_value = _testcapi_dict_getitemwitherror_impl(module, mapping, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_getitemref__doc__,
"dict_getitemref($module, obj, attr_name, value, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_GETITEMREF_METHODDEF    \
    {"dict_getitemref", _PyCFunction_CAST(_testcapi_dict_getitemref), METH_FASTCALL, _testcapi_dict_getitemref__doc__},

static PyObject *
_testcapi_dict_getitemref_impl(PyObject *module, PyObject *obj,
                               PyObject *attr_name, PyObject *value);

static PyObject *
_testcapi_dict_getitemref(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *attr_name;
    PyObject *value;

    if (!_PyArg_CheckPositional("dict_getitemref", nargs, 3, 3)) {
        goto exit;
    }
    obj = args[0];
    attr_name = args[1];
    value = args[2];
    return_value = _testcapi_dict_getitemref_impl(module, obj, attr_name, value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_getitemstringref__doc__,
"dict_getitemstringref($module, obj, value, attr_name, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_GETITEMSTRINGREF_METHODDEF    \
    {"dict_getitemstringref", _PyCFunction_CAST(_testcapi_dict_getitemstringref), METH_FASTCALL, _testcapi_dict_getitemstringref__doc__},

static PyObject *
_testcapi_dict_getitemstringref_impl(PyObject *module, PyObject *obj,
                                     PyObject *value, const char *attr_name,
                                     Py_ssize_t attr_name_length);

static PyObject *
_testcapi_dict_getitemstringref(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *value;
    const char *attr_name;
    Py_ssize_t attr_name_length;

    if (!_PyArg_ParseStack(args, nargs, "OOy#:dict_getitemstringref",
        &obj, &value, &attr_name, &attr_name_length)) {
        goto exit;
    }
    return_value = _testcapi_dict_getitemstringref_impl(module, obj, value, attr_name, attr_name_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_setitem__doc__,
"dict_setitem($module, mapping, key, value, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_SETITEM_METHODDEF    \
    {"dict_setitem", _PyCFunction_CAST(_testcapi_dict_setitem), METH_FASTCALL, _testcapi_dict_setitem__doc__},

static PyObject *
_testcapi_dict_setitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key, PyObject *value);

static PyObject *
_testcapi_dict_setitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *key;
    PyObject *value;

    if (!_PyArg_CheckPositional("dict_setitem", nargs, 3, 3)) {
        goto exit;
    }
    mapping = args[0];
    key = args[1];
    value = args[2];
    return_value = _testcapi_dict_setitem_impl(module, mapping, key, value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_setitemstring__doc__,
"dict_setitemstring($module, mapping, value, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_SETITEMSTRING_METHODDEF    \
    {"dict_setitemstring", _PyCFunction_CAST(_testcapi_dict_setitemstring), METH_FASTCALL, _testcapi_dict_setitemstring__doc__},

static PyObject *
_testcapi_dict_setitemstring_impl(PyObject *module, PyObject *mapping,
                                  PyObject *value, const char *key,
                                  Py_ssize_t key_length);

static PyObject *
_testcapi_dict_setitemstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *value;
    const char *key;
    Py_ssize_t key_length;

    if (!_PyArg_ParseStack(args, nargs, "OOy#:dict_setitemstring",
        &mapping, &value, &key, &key_length)) {
        goto exit;
    }
    return_value = _testcapi_dict_setitemstring_impl(module, mapping, value, key, key_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_setdefault__doc__,
"dict_setdefault($module, mapping, key, defaultobj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_SETDEFAULT_METHODDEF    \
    {"dict_setdefault", _PyCFunction_CAST(_testcapi_dict_setdefault), METH_FASTCALL, _testcapi_dict_setdefault__doc__},

static PyObject *
_testcapi_dict_setdefault_impl(PyObject *module, PyObject *mapping,
                               PyObject *key, PyObject *defaultobj);

static PyObject *
_testcapi_dict_setdefault(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *key;
    PyObject *defaultobj;

    if (!_PyArg_CheckPositional("dict_setdefault", nargs, 3, 3)) {
        goto exit;
    }
    mapping = args[0];
    key = args[1];
    defaultobj = args[2];
    return_value = _testcapi_dict_setdefault_impl(module, mapping, key, defaultobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_delitem__doc__,
"dict_delitem($module, mapping, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_DELITEM_METHODDEF    \
    {"dict_delitem", _PyCFunction_CAST(_testcapi_dict_delitem), METH_FASTCALL, _testcapi_dict_delitem__doc__},

static PyObject *
_testcapi_dict_delitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key);

static PyObject *
_testcapi_dict_delitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *key;

    if (!_PyArg_CheckPositional("dict_delitem", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    key = args[1];
    return_value = _testcapi_dict_delitem_impl(module, mapping, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_delitemstring__doc__,
"dict_delitemstring($module, mapping, key, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_DELITEMSTRING_METHODDEF    \
    {"dict_delitemstring", _PyCFunction_CAST(_testcapi_dict_delitemstring), METH_FASTCALL, _testcapi_dict_delitemstring__doc__},

static PyObject *
_testcapi_dict_delitemstring_impl(PyObject *module, PyObject *mapping,
                                  const char *key, Py_ssize_t key_length);

static PyObject *
_testcapi_dict_delitemstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    const char *key;
    Py_ssize_t key_length;

    if (!_PyArg_ParseStack(args, nargs, "Oy#:dict_delitemstring",
        &mapping, &key, &key_length)) {
        goto exit;
    }
    return_value = _testcapi_dict_delitemstring_impl(module, mapping, key, key_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_keys__doc__,
"dict_keys($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_KEYS_METHODDEF    \
    {"dict_keys", (PyCFunction)_testcapi_dict_keys, METH_O, _testcapi_dict_keys__doc__},

PyDoc_STRVAR(_testcapi_dict_values__doc__,
"dict_values($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_VALUES_METHODDEF    \
    {"dict_values", (PyCFunction)_testcapi_dict_values, METH_O, _testcapi_dict_values__doc__},

PyDoc_STRVAR(_testcapi_dict_items__doc__,
"dict_items($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_ITEMS_METHODDEF    \
    {"dict_items", (PyCFunction)_testcapi_dict_items, METH_O, _testcapi_dict_items__doc__},

PyDoc_STRVAR(_testcapi_dict_next__doc__,
"dict_next($module, mapping, pos, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_NEXT_METHODDEF    \
    {"dict_next", _PyCFunction_CAST(_testcapi_dict_next), METH_FASTCALL, _testcapi_dict_next__doc__},

static PyObject *
_testcapi_dict_next_impl(PyObject *module, PyObject *mapping, Py_ssize_t pos);

static PyObject *
_testcapi_dict_next(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    Py_ssize_t pos;

    if (!_PyArg_CheckPositional("dict_next", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        pos = ival;
    }
    return_value = _testcapi_dict_next_impl(module, mapping, pos);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_merge__doc__,
"dict_merge($module, mapping, mapping2, override, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_MERGE_METHODDEF    \
    {"dict_merge", _PyCFunction_CAST(_testcapi_dict_merge), METH_FASTCALL, _testcapi_dict_merge__doc__},

static PyObject *
_testcapi_dict_merge_impl(PyObject *module, PyObject *mapping,
                          PyObject *mapping2, int override);

static PyObject *
_testcapi_dict_merge(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *mapping2;
    int override;

    if (!_PyArg_CheckPositional("dict_merge", nargs, 3, 3)) {
        goto exit;
    }
    mapping = args[0];
    mapping2 = args[1];
    override = _PyLong_AsInt(args[2]);
    if (override == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_dict_merge_impl(module, mapping, mapping2, override);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_update__doc__,
"dict_update($module, mapping, mapping2, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_UPDATE_METHODDEF    \
    {"dict_update", _PyCFunction_CAST(_testcapi_dict_update), METH_FASTCALL, _testcapi_dict_update__doc__},

static PyObject *
_testcapi_dict_update_impl(PyObject *module, PyObject *mapping,
                           PyObject *mapping2);

static PyObject *
_testcapi_dict_update(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *mapping2;

    if (!_PyArg_CheckPositional("dict_update", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    mapping2 = args[1];
    return_value = _testcapi_dict_update_impl(module, mapping, mapping2);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_dict_mergefromseq2__doc__,
"dict_mergefromseq2($module, mapping, seq, override, /)\n"
"--\n"
"\n");

#define _TESTCAPI_DICT_MERGEFROMSEQ2_METHODDEF    \
    {"dict_mergefromseq2", _PyCFunction_CAST(_testcapi_dict_mergefromseq2), METH_FASTCALL, _testcapi_dict_mergefromseq2__doc__},

static PyObject *
_testcapi_dict_mergefromseq2_impl(PyObject *module, PyObject *mapping,
                                  PyObject *seq, int override);

static PyObject *
_testcapi_dict_mergefromseq2(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *seq;
    int override;

    if (!_PyArg_CheckPositional("dict_mergefromseq2", nargs, 3, 3)) {
        goto exit;
    }
    mapping = args[0];
    seq = args[1];
    override = _PyLong_AsInt(args[2]);
    if (override == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_dict_mergefromseq2_impl(module, mapping, seq, override);

exit:
    return return_value;
}
/*[clinic end generated code: output=7dd6014310c98f76 input=a9049054013a1b77]*/

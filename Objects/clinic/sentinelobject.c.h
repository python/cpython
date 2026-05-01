/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static PyObject *
sentinel_new_impl(PyTypeObject *type, PyObject *name);

static PyObject *
sentinel_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PySentinel_Type;
    PyObject *name;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("sentinel", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("sentinel", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    if (!PyUnicode_Check(PyTuple_GET_ITEM(args, 0))) {
        _PyArg_BadArgument("sentinel", "argument 1", "str", PyTuple_GET_ITEM(args, 0));
        goto exit;
    }
    name = PyTuple_GET_ITEM(args, 0);
    return_value = sentinel_new_impl(type, name);

exit:
    return return_value;
}
/*[clinic end generated code: output=7f28fc0bf0259cba input=a9049054013a1b77]*/

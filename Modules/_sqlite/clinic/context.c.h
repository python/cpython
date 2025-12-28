/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static PyObject *
callback_context_new_impl(PyTypeObject *type, PyObject *callable);

static PyObject *
callback_context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->CallbackContextType;
    PyObject *callable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("_CallbackContext", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_CallbackContext", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    callable = PyTuple_GET_ITEM(args, 0);
    return_value = callback_context_new_impl(type, callable);

exit:
    return return_value;
}
/*[clinic end generated code: output=370246c27daeaaa1 input=a9049054013a1b77]*/

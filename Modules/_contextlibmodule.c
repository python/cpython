
/* Contextlib module */

#include "Python.h"

#include "clinic/_contextlibmodule.c.h"

/*[clinic input]
module _contextlib
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=60c4760e25c64b5e]*/


/*[clinic input]
_contextlib._setup_genmgr_ctx
    exception: object

Force the provided exception to be set as the current exception

Subsequent calls to sys.exception()/sys.exc_info() will return
the provided exception until another exception is caught in the
current thread or the execution stack returns to a frame where
another exception is being handled.
[clinic start generated code]*/

static PyObject *
_contextlib__setup_genmgr_ctx_impl(PyObject *module, PyObject *exception)
/*[clinic end generated code: output=d8996a9f8f52204b input=d61410ba5659bf6f]*/
{
    if (!Py_IsNone(exception) && !PyExceptionInstance_Check(exception)){
        PyErr_SetString(
            PyExc_TypeError,
            "must be an exception/None"
        );
        return NULL;
    }
    PyErr_SetHandledException(exception);
    Py_RETURN_NONE;
}


/* module level code ********************************************************/

PyDoc_STRVAR(_contextlib_doc,
"Helpers for contextlib.");

static PyMethodDef contextlib_methods[] = {
    _CONTEXTLIB__SETUP_GENMGR_CTX_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyModuleDef_Slot contextlib_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _contextlibmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_contextlib",
    .m_doc = _contextlib_doc,
    .m_size = 0,
    .m_methods = contextlib_methods,
    .m_slots = contextlib_slots,
};

PyMODINIT_FUNC
PyInit__contextlib(void)
{
    return PyModuleDef_Init(&_contextlibmodule);
}
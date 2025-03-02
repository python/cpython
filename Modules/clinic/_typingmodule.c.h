/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_typing__idfunc__doc__,
"_idfunc($module, x, /)\n"
"--\n"
"\n");

#define _TYPING__IDFUNC_METHODDEF    \
    {"_idfunc", (PyCFunction)_typing__idfunc, METH_O, _typing__idfunc__doc__},

PyDoc_STRVAR(_typing__restore_anonymous_typeparam__doc__,
"_restore_anonymous_typeparam($module, owner, index, /)\n"
"--\n"
"\n"
"Restore previously pickled anonymous type param from object.__type_params__.");

#define _TYPING__RESTORE_ANONYMOUS_TYPEPARAM_METHODDEF    \
    {"_restore_anonymous_typeparam", _PyCFunction_CAST(_typing__restore_anonymous_typeparam), METH_FASTCALL, _typing__restore_anonymous_typeparam__doc__},

static PyObject *
_typing__restore_anonymous_typeparam_impl(PyObject *module, PyObject *owner,
                                          PyObject *index);

static PyObject *
_typing__restore_anonymous_typeparam(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *owner;
    PyObject *index;

    if (!_PyArg_CheckPositional("_restore_anonymous_typeparam", nargs, 2, 2)) {
        goto exit;
    }
    owner = args[0];
    index = args[1];
    return_value = _typing__restore_anonymous_typeparam_impl(module, owner, index);

exit:
    return return_value;
}
/*[clinic end generated code: output=ad8652d1dc62a084 input=a9049054013a1b77]*/

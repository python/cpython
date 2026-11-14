/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_NoPositional()

PyDoc_STRVAR(namespace___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy with the specified attributes replaced.");

#define NAMESPACE___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(namespace___replace__), METH_VARARGS|METH_KEYWORDS, namespace___replace____doc__},

static PyObject *
namespace___replace___impl(_PyNamespaceObject *self, PyObject *changes);

static PyObject *
namespace___replace__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *changes = NULL;

    if (!_PyArg_NoPositional("__replace__", args)) {
        goto exit;
    }
    changes = Py_XNewRef(kwargs);
    return_value = namespace___replace___impl((_PyNamespaceObject *)self, changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}
/*[clinic end generated code: output=1b1c015f14360d64 input=a9049054013a1b77]*/

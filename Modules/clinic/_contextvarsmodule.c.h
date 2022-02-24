/*[clinic input]
preserve
[clinic start generated code]*/

#ifdef Py_BUILD_CORE
#include "pycore_gc.h"            // PyGC_Head
#include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_contextvars_copy_context__doc__,
"copy_context($module, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_COPY_CONTEXT_METHODDEF    \
    {"copy_context", (PyCFunction)_contextvars_copy_context, METH_NOARGS, _contextvars_copy_context__doc__},

static PyObject *
_contextvars_copy_context_impl(PyObject *module);

static PyObject *
_contextvars_copy_context(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _contextvars_copy_context_impl(module);
}
/*[clinic end generated code: output=26e07024451baf52 input=a9049054013a1b77]*/

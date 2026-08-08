/*[clinic input]
preserve
[clinic start generated code]*/

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

PyDoc_STRVAR(_contextvars__new_thread_context__doc__,
"_new_thread_context($module, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS__NEW_THREAD_CONTEXT_METHODDEF    \
    {"_new_thread_context", (PyCFunction)_contextvars__new_thread_context, METH_NOARGS, _contextvars__new_thread_context__doc__},

static PyObject *
_contextvars__new_thread_context_impl(PyObject *module);

static PyObject *
_contextvars__new_thread_context(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _contextvars__new_thread_context_impl(module);
}
/*[clinic end generated code: output=c2d927c634f05b08 input=a9049054013a1b77]*/

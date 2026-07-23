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

PyDoc_STRVAR(_contextvars__thread_start_context__doc__,
"_thread_start_context($module, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS__THREAD_START_CONTEXT_METHODDEF    \
    {"_thread_start_context", (PyCFunction)_contextvars__thread_start_context, METH_NOARGS, _contextvars__thread_start_context__doc__},

static PyObject *
_contextvars__thread_start_context_impl(PyObject *module);

static PyObject *
_contextvars__thread_start_context(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _contextvars__thread_start_context_impl(module);
}
/*[clinic end generated code: output=2c6c9d89faff4312 input=a9049054013a1b77]*/

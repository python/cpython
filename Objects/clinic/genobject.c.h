/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(gen_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define GEN_CLOSE_METHODDEF    \
    {"close", (PyCFunction)gen_close, METH_NOARGS, gen_close__doc__},

static PyObject *
gen_close_impl(PyGenObject *self);

static PyObject *
gen_close(PyGenObject *self, PyObject *Py_UNUSED(ignored))
{
    return gen_close_impl(self);
}
/*[clinic end generated code: output=e7c9681104424906 input=a9049054013a1b77]*/

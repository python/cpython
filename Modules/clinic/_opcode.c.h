/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect($module, opcode, oparg=None, /)\n"
"--\n"
"\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", (PyCFunction)_opcode_stack_effect, METH_VARARGS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyModuleDef *module, int opcode, PyObject *oparg);

static PyObject *
_opcode_stack_effect(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int opcode;
    PyObject *oparg = Py_None;
    int _return_value;

    if (!PyArg_ParseTuple(args, "i|O:stack_effect",
        &opcode, &oparg))
        goto exit;
    _return_value = _opcode_stack_effect_impl(module, opcode, oparg);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=8ee7cb735705e8b3 input=a9049054013a1b77]*/

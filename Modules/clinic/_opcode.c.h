/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect($module, opcode, oparg=None, /, *, jump=None)\n"
"--\n"
"\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", (PyCFunction)_opcode_stack_effect, METH_FASTCALL|METH_KEYWORDS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyObject *oparg,
                          PyObject *jump);

static PyObject *
_opcode_stack_effect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "jump", NULL};
    static _PyArg_Parser _parser = {"i|O$O:stack_effect", _keywords, 0};
    int opcode;
    PyObject *oparg = Py_None;
    PyObject *jump = Py_None;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &opcode, &oparg, &jump)) {
        goto exit;
    }
    _return_value = _opcode_stack_effect_impl(module, opcode, oparg, jump);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=bbf6c4cfc91edc29 input=a9049054013a1b77]*/

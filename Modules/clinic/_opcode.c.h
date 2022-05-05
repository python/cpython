/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect($module, opcode, oparg=None, /, *, jump=None)\n"
"--\n"
"\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", _PyCFunction_CAST(_opcode_stack_effect), METH_FASTCALL|METH_KEYWORDS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyObject *oparg,
                          PyObject *jump);

static PyObject *
_opcode_stack_effect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "jump", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "stack_effect", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int opcode;
    PyObject *oparg = Py_None;
    PyObject *jump = Py_None;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    oparg = args[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    jump = args[2];
skip_optional_kwonly:
    _return_value = _opcode_stack_effect_impl(module, opcode, oparg, jump);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_enable_specialization_stats__doc__,
"enable_specialization_stats($module, /)\n"
"--\n"
"\n"
"Enable the specialization stats");

#define _OPCODE_ENABLE_SPECIALIZATION_STATS_METHODDEF    \
    {"enable_specialization_stats", (PyCFunction)_opcode_enable_specialization_stats, METH_NOARGS, _opcode_enable_specialization_stats__doc__},

static PyObject *
_opcode_enable_specialization_stats_impl(PyObject *module);

static PyObject *
_opcode_enable_specialization_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_enable_specialization_stats_impl(module);
}

PyDoc_STRVAR(_opcode_disable_specialization_stats__doc__,
"disable_specialization_stats($module, /)\n"
"--\n"
"\n"
"Disable the specialization stats");

#define _OPCODE_DISABLE_SPECIALIZATION_STATS_METHODDEF    \
    {"disable_specialization_stats", (PyCFunction)_opcode_disable_specialization_stats, METH_NOARGS, _opcode_disable_specialization_stats__doc__},

static PyObject *
_opcode_disable_specialization_stats_impl(PyObject *module);

static PyObject *
_opcode_disable_specialization_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_disable_specialization_stats_impl(module);
}

PyDoc_STRVAR(_opcode_init_specialization_stats__doc__,
"init_specialization_stats($module, /)\n"
"--\n"
"\n"
"Initialize the specialization stats");

#define _OPCODE_INIT_SPECIALIZATION_STATS_METHODDEF    \
    {"init_specialization_stats", (PyCFunction)_opcode_init_specialization_stats, METH_NOARGS, _opcode_init_specialization_stats__doc__},

static PyObject *
_opcode_init_specialization_stats_impl(PyObject *module);

static PyObject *
_opcode_init_specialization_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_init_specialization_stats_impl(module);
}

PyDoc_STRVAR(_opcode_get_specialization_stats__doc__,
"get_specialization_stats($module, /)\n"
"--\n"
"\n"
"Return the specialization stats");

#define _OPCODE_GET_SPECIALIZATION_STATS_METHODDEF    \
    {"get_specialization_stats", (PyCFunction)_opcode_get_specialization_stats, METH_NOARGS, _opcode_get_specialization_stats__doc__},

static PyObject *
_opcode_get_specialization_stats_impl(PyObject *module);

static PyObject *
_opcode_get_specialization_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_get_specialization_stats_impl(module);
}
/*[clinic end generated code: output=5db5d2cfd7ed8eec input=a9049054013a1b77]*/

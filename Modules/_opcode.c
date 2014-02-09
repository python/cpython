#include "Python.h"
#include "opcode.h"

/*[clinic input]
module _opcode
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=117442e66eb376e6]*/

/*[clinic input]

_opcode.stack_effect -> int

  opcode: int
  oparg: object = None
  /

Compute the stack effect of the opcode.
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

    if (!PyArg_ParseTuple(args,
        "i|O:stack_effect",
        &opcode, &oparg))
        goto exit;
    _return_value = _opcode_stack_effect_impl(module, opcode, oparg);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
_opcode_stack_effect_impl(PyModuleDef *module, int opcode, PyObject *oparg)
/*[clinic end generated code: output=9e1133f8d587bc67 input=2d0a9ee53c0418f5]*/
{
    int effect;
    int oparg_int = 0;
    if (HAS_ARG(opcode)) {
        if (oparg == Py_None) {
            PyErr_SetString(PyExc_ValueError,
                    "stack_effect: opcode requires oparg but oparg was not specified");
            return -1;
        }
        oparg_int = (int)PyLong_AsLong(oparg);
        if ((oparg_int == -1) && PyErr_Occurred())
            return -1;
    }
    else if (oparg != Py_None) {
        PyErr_SetString(PyExc_ValueError,
                "stack_effect: opcode does not permit oparg but oparg was specified");
        return -1;
    }
    effect = PyCompile_OpcodeStackEffect(opcode, oparg_int);
    if (effect == PY_INVALID_STACK_EFFECT) {
            PyErr_SetString(PyExc_ValueError,
                    "invalid opcode or oparg");
            return -1;
    }
    return effect;
}




static PyMethodDef
opcode_functions[] =  {
    _OPCODE_STACK_EFFECT_METHODDEF
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef opcodemodule = {
    PyModuleDef_HEAD_INIT,
    "_opcode",
    "Opcode support module.",
    -1,
    opcode_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__opcode(void)
{
    return PyModule_Create(&opcodemodule);
}

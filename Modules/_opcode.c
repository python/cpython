#include "Python.h"
#include "opcode.h"

/*[clinic input]
module _opcode
[clinic start generated code]*/
/*[clinic end generated code: checksum=da39a3ee5e6b4b0d3255bfef95601890afd80709]*/

/*[clinic input]

_opcode.stack_effect -> int

  opcode: int

  [
  oparg: int
  ]
  /

Compute the stack effect of the opcode.
[clinic start generated code]*/

PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect(opcode, [oparg])\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", (PyCFunction)_opcode_stack_effect, METH_VARARGS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyModuleDef *module, int opcode, int group_right_1, int oparg);

static PyObject *
_opcode_stack_effect(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int opcode;
    int group_right_1 = 0;
    int oparg = 0;
    int _return_value;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "i:stack_effect", &opcode))
                goto exit;
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:stack_effect", &opcode, &oparg))
                goto exit;
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_opcode.stack_effect requires 1 to 2 arguments");
            goto exit;
    }
    _return_value = _opcode_stack_effect_impl(module, opcode, group_right_1, oparg);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
_opcode_stack_effect_impl(PyModuleDef *module, int opcode, int group_right_1, int oparg)
/*[clinic end generated code: checksum=58fb4f1b174fc92f783dc945ca712fb752a6c283]*/
{
    int effect;
    if (HAS_ARG(opcode)) {
        if (!group_right_1) {
            PyErr_SetString(PyExc_ValueError,
                    "stack_effect: opcode requires oparg but oparg was not specified");
            return -1;
        }
    }
    else if (group_right_1) {
        PyErr_SetString(PyExc_ValueError,
                "stack_effect: opcode does not permit oparg but oparg was specified");
        return -1;
    }
    effect = PyCompile_OpcodeStackEffect(opcode, oparg);
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

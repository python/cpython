#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "compile.h"
#include "opcode.h"
#include "internal/pycore_code.h"
#include "internal/pycore_compile.h"
#include "internal/pycore_intrinsics.h"

/*[clinic input]
module _opcode
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=117442e66eb376e6]*/

#include "clinic/_opcode.c.h"

/*[clinic input]

_opcode.stack_effect -> int

  opcode: int
  oparg: object = None
  /
  *
  jump: object = None

Compute the stack effect of the opcode.
[clinic start generated code]*/

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyObject *oparg,
                          PyObject *jump)
/*[clinic end generated code: output=64a18f2ead954dbb input=461c9d4a44851898]*/
{
    int oparg_int = 0;
    int jump_int;

    if (oparg != Py_None) {
        oparg_int = (int)PyLong_AsLong(oparg);
        if ((oparg_int == -1) && PyErr_Occurred()) {
            return -1;
        }
    }

    if (jump == Py_None) {
        jump_int = -1;
    }
    else if (jump == Py_True) {
        jump_int = 1;
    }
    else if (jump == Py_False) {
        jump_int = 0;
    }
    else {
        PyErr_SetString(PyExc_ValueError,
                "stack_effect: jump must be False, True or None");
        return -1;
    }
    int effect = PyCompile_OpcodeStackEffectWithJump(opcode, oparg_int, jump_int);
    if (effect == PY_INVALID_STACK_EFFECT) {
        PyErr_SetString(PyExc_ValueError, "invalid opcode or oparg");
        return -1;
    }
    return effect;
}

/*[clinic input]

_opcode.is_valid -> bool

  opcode: int

Return True if opcode is valid, False otherwise.
[clinic start generated code]*/

static int
_opcode_is_valid_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=b0d918ea1d073f65 input=fe23e0aa194ddae0]*/
{
    return _PyCompile_OpcodeIsValid(opcode);
}

/*[clinic input]

_opcode.has_arg -> bool

  opcode: int

Return True if the opcode uses its oparg, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_arg_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=7a062d3b2dcc0815 input=93d878ba6361db5f]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasArg(opcode);
}

/*[clinic input]

_opcode.has_const -> bool

  opcode: int

Return True if the opcode accesses a constant, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_const_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=c646d5027c634120 input=a6999e4cf13f9410]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasConst(opcode);
}

/*[clinic input]

_opcode.has_name -> bool

  opcode: int

Return True if the opcode accesses an attribute by name, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_name_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=b49a83555c2fa517 input=448aa5e4bcc947ba]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasName(opcode);
}

/*[clinic input]

_opcode.has_jump -> bool

  opcode: int

Return True if the opcode has a jump target, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_jump_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=e9c583c669f1c46a input=35f711274357a0c3]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasJump(opcode);

}

/*[clinic input]

_opcode.has_free -> bool

  opcode: int

Return True if the opcode accesses a free variable, False otherwise.

Note that 'free' in this context refers to names in the current scope
that are referenced by inner scopes or names in outer scopes that are
referenced from this scope. It does not include references to global
or builtin scopes.
[clinic start generated code]*/

static int
_opcode_has_free_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=d81ae4d79af0ee26 input=117dcd5c19c1139b]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasFree(opcode);

}

/*[clinic input]

_opcode.has_local -> bool

  opcode: int

Return True if the opcode accesses a local variable, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_local_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=da5a8616b7a5097b input=9a798ee24aaef49d]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasLocal(opcode);
}

/*[clinic input]

_opcode.has_exc -> bool

  opcode: int

Return True if the opcode sets an exception handler, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_exc_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=41b68dff0ec82a52 input=db0e4bdb9bf13fa5]*/
{
    return _PyCompile_OpcodeIsValid(opcode) &&
           _PyCompile_OpcodeHasExc(opcode);
}

/*[clinic input]

_opcode.get_specialization_stats

Return the specialization stats
[clinic start generated code]*/

static PyObject *
_opcode_get_specialization_stats_impl(PyObject *module)
/*[clinic end generated code: output=fcbc32fdfbec5c17 input=e1f60db68d8ce5f6]*/
{
#ifdef Py_STATS
    return _Py_GetSpecializationStats();
#else
    Py_RETURN_NONE;
#endif
}

/*[clinic input]

_opcode.get_nb_ops

Return array of symbols of binary ops.

Indexed by the BINARY_OP oparg value.
[clinic start generated code]*/

static PyObject *
_opcode_get_nb_ops_impl(PyObject *module)
/*[clinic end generated code: output=d997d306cc15426f input=9462fc544c823176]*/
{
    PyObject *list = PyList_New(NB_OPARG_LAST + 1);
    if (list == NULL) {
        return NULL;
    }
#define ADD_NB_OP(NUM, STR) \
    do { \
        PyObject *pair = Py_BuildValue("ss", #NUM, STR); \
        if (pair == NULL) { \
            Py_DECREF(list); \
            return NULL; \
        } \
        PyList_SET_ITEM(list, (NUM), pair); \
    } while(0);

    ADD_NB_OP(NB_ADD, "+");
    ADD_NB_OP(NB_AND, "&");
    ADD_NB_OP(NB_FLOOR_DIVIDE, "//");
    ADD_NB_OP(NB_LSHIFT, "<<");
    ADD_NB_OP(NB_MATRIX_MULTIPLY, "@");
    ADD_NB_OP(NB_MULTIPLY, "*");
    ADD_NB_OP(NB_REMAINDER, "%");
    ADD_NB_OP(NB_OR, "|");
    ADD_NB_OP(NB_POWER, "**");
    ADD_NB_OP(NB_RSHIFT, ">>");
    ADD_NB_OP(NB_SUBTRACT, "-");
    ADD_NB_OP(NB_TRUE_DIVIDE, "/");
    ADD_NB_OP(NB_XOR, "^");
    ADD_NB_OP(NB_INPLACE_ADD, "+=");
    ADD_NB_OP(NB_INPLACE_AND, "&=");
    ADD_NB_OP(NB_INPLACE_FLOOR_DIVIDE, "//=");
    ADD_NB_OP(NB_INPLACE_LSHIFT, "<<=");
    ADD_NB_OP(NB_INPLACE_MATRIX_MULTIPLY, "@=");
    ADD_NB_OP(NB_INPLACE_MULTIPLY, "*=");
    ADD_NB_OP(NB_INPLACE_REMAINDER, "%=");
    ADD_NB_OP(NB_INPLACE_OR, "|=");
    ADD_NB_OP(NB_INPLACE_POWER, "**=");
    ADD_NB_OP(NB_INPLACE_RSHIFT, ">>=");
    ADD_NB_OP(NB_INPLACE_SUBTRACT, "-=");
    ADD_NB_OP(NB_INPLACE_TRUE_DIVIDE, "/=");
    ADD_NB_OP(NB_INPLACE_XOR, "^=");

#undef ADD_NB_OP

    for(int i = 0; i <= NB_OPARG_LAST; i++) {
        if (PyList_GET_ITEM(list, i) == NULL) {
            Py_DECREF(list);
            PyErr_Format(PyExc_ValueError,
                         "Missing initialization for NB_OP %d",
                         i);
            return NULL;
        }
    }
    return list;
}

/*[clinic input]

_opcode.get_intrinsic1_descs

Return a list of names of the unary intrinsics.
[clinic start generated code]*/

static PyObject *
_opcode_get_intrinsic1_descs_impl(PyObject *module)
/*[clinic end generated code: output=bd1ddb6b4447d18b input=13b51c712618459b]*/
{
    PyObject *list = PyList_New(MAX_INTRINSIC_1 + 1);
    if (list == NULL) {
        return NULL;
    }
    for (int i=0; i <= MAX_INTRINSIC_1; i++) {
        PyObject *name = _PyCompile_GetUnaryIntrinsicName(i);
        if (name == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, name);
    }
    return list;
}


/*[clinic input]

_opcode.get_intrinsic2_descs

Return a list of names of the binary intrinsics.
[clinic start generated code]*/

static PyObject *
_opcode_get_intrinsic2_descs_impl(PyObject *module)
/*[clinic end generated code: output=40e62bc27584c8a0 input=e83068f249f5471b]*/
{
    PyObject *list = PyList_New(MAX_INTRINSIC_2 + 1);
    if (list == NULL) {
        return NULL;
    }
    for (int i=0; i <= MAX_INTRINSIC_2; i++) {
        PyObject *name = _PyCompile_GetBinaryIntrinsicName(i);
        if (name == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, name);
    }
    return list;
}


static PyMethodDef
opcode_functions[] =  {
    _OPCODE_STACK_EFFECT_METHODDEF
    _OPCODE_IS_VALID_METHODDEF
    _OPCODE_HAS_ARG_METHODDEF
    _OPCODE_HAS_CONST_METHODDEF
    _OPCODE_HAS_NAME_METHODDEF
    _OPCODE_HAS_JUMP_METHODDEF
    _OPCODE_HAS_FREE_METHODDEF
    _OPCODE_HAS_LOCAL_METHODDEF
    _OPCODE_HAS_EXC_METHODDEF
    _OPCODE_GET_SPECIALIZATION_STATS_METHODDEF
    _OPCODE_GET_NB_OPS_METHODDEF
    _OPCODE_GET_INTRINSIC1_DESCS_METHODDEF
    _OPCODE_GET_INTRINSIC2_DESCS_METHODDEF
    {NULL, NULL, 0, NULL}
};

int
_opcode_exec(PyObject *m) {
    if (PyModule_AddIntMacro(m, ENABLE_SPECIALIZATION) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, _opcode_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef opcodemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_opcode",
    .m_doc = "Opcode support module.",
    .m_size = 0,
    .m_methods = opcode_functions,
    .m_slots = module_slots,
};

PyMODINIT_FUNC
PyInit__opcode(void)
{
    return PyModuleDef_Init(&opcodemodule);
}

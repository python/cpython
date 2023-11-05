/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_operator_truth__doc__,
"truth($module, a, /)\n"
"--\n"
"\n"
"Return True if a is true, False otherwise.");

#define _OPERATOR_TRUTH_METHODDEF    \
    {"truth", (PyCFunction)_operator_truth, METH_O, _operator_truth__doc__},

static int
_operator_truth_impl(PyObject *module, PyObject *a);

static PyObject *
_operator_truth(PyObject *module, PyObject *a)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _operator_truth_impl(module, a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_add__doc__,
"add($module, a, b, /)\n"
"--\n"
"\n"
"Same as a + b.");

#define _OPERATOR_ADD_METHODDEF    \
    {"add", _PyCFunction_CAST(_operator_add), METH_FASTCALL, _operator_add__doc__},

static PyObject *
_operator_add_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_add(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("add", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_add_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_sub__doc__,
"sub($module, a, b, /)\n"
"--\n"
"\n"
"Same as a - b.");

#define _OPERATOR_SUB_METHODDEF    \
    {"sub", _PyCFunction_CAST(_operator_sub), METH_FASTCALL, _operator_sub__doc__},

static PyObject *
_operator_sub_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_sub(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("sub", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_sub_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_mul__doc__,
"mul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a * b.");

#define _OPERATOR_MUL_METHODDEF    \
    {"mul", _PyCFunction_CAST(_operator_mul), METH_FASTCALL, _operator_mul__doc__},

static PyObject *
_operator_mul_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_mul(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("mul", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_mul_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_matmul__doc__,
"matmul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a @ b.");

#define _OPERATOR_MATMUL_METHODDEF    \
    {"matmul", _PyCFunction_CAST(_operator_matmul), METH_FASTCALL, _operator_matmul__doc__},

static PyObject *
_operator_matmul_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_matmul(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("matmul", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_matmul_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_floordiv__doc__,
"floordiv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a // b.");

#define _OPERATOR_FLOORDIV_METHODDEF    \
    {"floordiv", _PyCFunction_CAST(_operator_floordiv), METH_FASTCALL, _operator_floordiv__doc__},

static PyObject *
_operator_floordiv_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_floordiv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("floordiv", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_floordiv_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_truediv__doc__,
"truediv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a / b.");

#define _OPERATOR_TRUEDIV_METHODDEF    \
    {"truediv", _PyCFunction_CAST(_operator_truediv), METH_FASTCALL, _operator_truediv__doc__},

static PyObject *
_operator_truediv_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_truediv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("truediv", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_truediv_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_mod__doc__,
"mod($module, a, b, /)\n"
"--\n"
"\n"
"Same as a % b.");

#define _OPERATOR_MOD_METHODDEF    \
    {"mod", _PyCFunction_CAST(_operator_mod), METH_FASTCALL, _operator_mod__doc__},

static PyObject *
_operator_mod_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_mod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("mod", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_mod_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_neg__doc__,
"neg($module, a, /)\n"
"--\n"
"\n"
"Same as -a.");

#define _OPERATOR_NEG_METHODDEF    \
    {"neg", (PyCFunction)_operator_neg, METH_O, _operator_neg__doc__},

PyDoc_STRVAR(_operator_pos__doc__,
"pos($module, a, /)\n"
"--\n"
"\n"
"Same as +a.");

#define _OPERATOR_POS_METHODDEF    \
    {"pos", (PyCFunction)_operator_pos, METH_O, _operator_pos__doc__},

PyDoc_STRVAR(_operator_abs__doc__,
"abs($module, a, /)\n"
"--\n"
"\n"
"Same as abs(a).");

#define _OPERATOR_ABS_METHODDEF    \
    {"abs", (PyCFunction)_operator_abs, METH_O, _operator_abs__doc__},

PyDoc_STRVAR(_operator_inv__doc__,
"inv($module, a, /)\n"
"--\n"
"\n"
"Same as ~a.");

#define _OPERATOR_INV_METHODDEF    \
    {"inv", (PyCFunction)_operator_inv, METH_O, _operator_inv__doc__},

PyDoc_STRVAR(_operator_invert__doc__,
"invert($module, a, /)\n"
"--\n"
"\n"
"Same as ~a.");

#define _OPERATOR_INVERT_METHODDEF    \
    {"invert", (PyCFunction)_operator_invert, METH_O, _operator_invert__doc__},

PyDoc_STRVAR(_operator_lshift__doc__,
"lshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a << b.");

#define _OPERATOR_LSHIFT_METHODDEF    \
    {"lshift", _PyCFunction_CAST(_operator_lshift), METH_FASTCALL, _operator_lshift__doc__},

static PyObject *
_operator_lshift_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_lshift(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("lshift", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_lshift_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_rshift__doc__,
"rshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >> b.");

#define _OPERATOR_RSHIFT_METHODDEF    \
    {"rshift", _PyCFunction_CAST(_operator_rshift), METH_FASTCALL, _operator_rshift__doc__},

static PyObject *
_operator_rshift_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_rshift(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("rshift", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_rshift_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_not___doc__,
"not_($module, a, /)\n"
"--\n"
"\n"
"Same as not a.");

#define _OPERATOR_NOT__METHODDEF    \
    {"not_", (PyCFunction)_operator_not_, METH_O, _operator_not___doc__},

static int
_operator_not__impl(PyObject *module, PyObject *a);

static PyObject *
_operator_not_(PyObject *module, PyObject *a)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _operator_not__impl(module, a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_and___doc__,
"and_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a & b.");

#define _OPERATOR_AND__METHODDEF    \
    {"and_", _PyCFunction_CAST(_operator_and_), METH_FASTCALL, _operator_and___doc__},

static PyObject *
_operator_and__impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_and_(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("and_", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_and__impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_xor__doc__,
"xor($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ^ b.");

#define _OPERATOR_XOR_METHODDEF    \
    {"xor", _PyCFunction_CAST(_operator_xor), METH_FASTCALL, _operator_xor__doc__},

static PyObject *
_operator_xor_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_xor(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("xor", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_xor_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_or___doc__,
"or_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a | b.");

#define _OPERATOR_OR__METHODDEF    \
    {"or_", _PyCFunction_CAST(_operator_or_), METH_FASTCALL, _operator_or___doc__},

static PyObject *
_operator_or__impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_or_(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("or_", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_or__impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iadd__doc__,
"iadd($module, a, b, /)\n"
"--\n"
"\n"
"Same as a += b.");

#define _OPERATOR_IADD_METHODDEF    \
    {"iadd", _PyCFunction_CAST(_operator_iadd), METH_FASTCALL, _operator_iadd__doc__},

static PyObject *
_operator_iadd_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_iadd(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("iadd", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_iadd_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_isub__doc__,
"isub($module, a, b, /)\n"
"--\n"
"\n"
"Same as a -= b.");

#define _OPERATOR_ISUB_METHODDEF    \
    {"isub", _PyCFunction_CAST(_operator_isub), METH_FASTCALL, _operator_isub__doc__},

static PyObject *
_operator_isub_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_isub(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("isub", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_isub_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imul__doc__,
"imul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a *= b.");

#define _OPERATOR_IMUL_METHODDEF    \
    {"imul", _PyCFunction_CAST(_operator_imul), METH_FASTCALL, _operator_imul__doc__},

static PyObject *
_operator_imul_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_imul(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("imul", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_imul_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imatmul__doc__,
"imatmul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a @= b.");

#define _OPERATOR_IMATMUL_METHODDEF    \
    {"imatmul", _PyCFunction_CAST(_operator_imatmul), METH_FASTCALL, _operator_imatmul__doc__},

static PyObject *
_operator_imatmul_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_imatmul(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("imatmul", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_imatmul_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ifloordiv__doc__,
"ifloordiv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a //= b.");

#define _OPERATOR_IFLOORDIV_METHODDEF    \
    {"ifloordiv", _PyCFunction_CAST(_operator_ifloordiv), METH_FASTCALL, _operator_ifloordiv__doc__},

static PyObject *
_operator_ifloordiv_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ifloordiv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ifloordiv", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ifloordiv_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_itruediv__doc__,
"itruediv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a /= b.");

#define _OPERATOR_ITRUEDIV_METHODDEF    \
    {"itruediv", _PyCFunction_CAST(_operator_itruediv), METH_FASTCALL, _operator_itruediv__doc__},

static PyObject *
_operator_itruediv_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_itruediv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("itruediv", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_itruediv_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imod__doc__,
"imod($module, a, b, /)\n"
"--\n"
"\n"
"Same as a %= b.");

#define _OPERATOR_IMOD_METHODDEF    \
    {"imod", _PyCFunction_CAST(_operator_imod), METH_FASTCALL, _operator_imod__doc__},

static PyObject *
_operator_imod_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_imod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("imod", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_imod_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ilshift__doc__,
"ilshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a <<= b.");

#define _OPERATOR_ILSHIFT_METHODDEF    \
    {"ilshift", _PyCFunction_CAST(_operator_ilshift), METH_FASTCALL, _operator_ilshift__doc__},

static PyObject *
_operator_ilshift_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ilshift(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ilshift", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ilshift_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_irshift__doc__,
"irshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >>= b.");

#define _OPERATOR_IRSHIFT_METHODDEF    \
    {"irshift", _PyCFunction_CAST(_operator_irshift), METH_FASTCALL, _operator_irshift__doc__},

static PyObject *
_operator_irshift_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_irshift(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("irshift", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_irshift_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iand__doc__,
"iand($module, a, b, /)\n"
"--\n"
"\n"
"Same as a &= b.");

#define _OPERATOR_IAND_METHODDEF    \
    {"iand", _PyCFunction_CAST(_operator_iand), METH_FASTCALL, _operator_iand__doc__},

static PyObject *
_operator_iand_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_iand(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("iand", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_iand_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ixor__doc__,
"ixor($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ^= b.");

#define _OPERATOR_IXOR_METHODDEF    \
    {"ixor", _PyCFunction_CAST(_operator_ixor), METH_FASTCALL, _operator_ixor__doc__},

static PyObject *
_operator_ixor_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ixor(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ixor", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ixor_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ior__doc__,
"ior($module, a, b, /)\n"
"--\n"
"\n"
"Same as a |= b.");

#define _OPERATOR_IOR_METHODDEF    \
    {"ior", _PyCFunction_CAST(_operator_ior), METH_FASTCALL, _operator_ior__doc__},

static PyObject *
_operator_ior_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ior(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ior", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ior_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_concat__doc__,
"concat($module, a, b, /)\n"
"--\n"
"\n"
"Same as a + b, for a and b sequences.");

#define _OPERATOR_CONCAT_METHODDEF    \
    {"concat", _PyCFunction_CAST(_operator_concat), METH_FASTCALL, _operator_concat__doc__},

static PyObject *
_operator_concat_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_concat(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("concat", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_concat_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iconcat__doc__,
"iconcat($module, a, b, /)\n"
"--\n"
"\n"
"Same as a += b, for a and b sequences.");

#define _OPERATOR_ICONCAT_METHODDEF    \
    {"iconcat", _PyCFunction_CAST(_operator_iconcat), METH_FASTCALL, _operator_iconcat__doc__},

static PyObject *
_operator_iconcat_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_iconcat(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("iconcat", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_iconcat_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_contains__doc__,
"contains($module, a, b, /)\n"
"--\n"
"\n"
"Same as b in a (note reversed operands).");

#define _OPERATOR_CONTAINS_METHODDEF    \
    {"contains", _PyCFunction_CAST(_operator_contains), METH_FASTCALL, _operator_contains__doc__},

static int
_operator_contains_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_contains(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    int _return_value;

    if (!_PyArg_CheckPositional("contains", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    _return_value = _operator_contains_impl(module, a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_indexOf__doc__,
"indexOf($module, a, b, /)\n"
"--\n"
"\n"
"Return the first index of b in a.");

#define _OPERATOR_INDEXOF_METHODDEF    \
    {"indexOf", _PyCFunction_CAST(_operator_indexOf), METH_FASTCALL, _operator_indexOf__doc__},

static Py_ssize_t
_operator_indexOf_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_indexOf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("indexOf", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    _return_value = _operator_indexOf_impl(module, a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_countOf__doc__,
"countOf($module, a, b, /)\n"
"--\n"
"\n"
"Return the number of items in a which are, or which equal, b.");

#define _OPERATOR_COUNTOF_METHODDEF    \
    {"countOf", _PyCFunction_CAST(_operator_countOf), METH_FASTCALL, _operator_countOf__doc__},

static Py_ssize_t
_operator_countOf_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_countOf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("countOf", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    _return_value = _operator_countOf_impl(module, a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_getitem__doc__,
"getitem($module, a, b, /)\n"
"--\n"
"\n"
"Same as a[b].");

#define _OPERATOR_GETITEM_METHODDEF    \
    {"getitem", _PyCFunction_CAST(_operator_getitem), METH_FASTCALL, _operator_getitem__doc__},

static PyObject *
_operator_getitem_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_getitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("getitem", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_getitem_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_setitem__doc__,
"setitem($module, a, b, c, /)\n"
"--\n"
"\n"
"Same as a[b] = c.");

#define _OPERATOR_SETITEM_METHODDEF    \
    {"setitem", _PyCFunction_CAST(_operator_setitem), METH_FASTCALL, _operator_setitem__doc__},

static PyObject *
_operator_setitem_impl(PyObject *module, PyObject *a, PyObject *b,
                       PyObject *c);

static PyObject *
_operator_setitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    PyObject *c;

    if (!_PyArg_CheckPositional("setitem", nargs, 3, 3)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    return_value = _operator_setitem_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_delitem__doc__,
"delitem($module, a, b, /)\n"
"--\n"
"\n"
"Same as del a[b].");

#define _OPERATOR_DELITEM_METHODDEF    \
    {"delitem", _PyCFunction_CAST(_operator_delitem), METH_FASTCALL, _operator_delitem__doc__},

static PyObject *
_operator_delitem_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_delitem(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("delitem", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_delitem_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_eq__doc__,
"eq($module, a, b, /)\n"
"--\n"
"\n"
"Same as a == b.");

#define _OPERATOR_EQ_METHODDEF    \
    {"eq", _PyCFunction_CAST(_operator_eq), METH_FASTCALL, _operator_eq__doc__},

static PyObject *
_operator_eq_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_eq(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("eq", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_eq_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ne__doc__,
"ne($module, a, b, /)\n"
"--\n"
"\n"
"Same as a != b.");

#define _OPERATOR_NE_METHODDEF    \
    {"ne", _PyCFunction_CAST(_operator_ne), METH_FASTCALL, _operator_ne__doc__},

static PyObject *
_operator_ne_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ne(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ne", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ne_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_lt__doc__,
"lt($module, a, b, /)\n"
"--\n"
"\n"
"Same as a < b.");

#define _OPERATOR_LT_METHODDEF    \
    {"lt", _PyCFunction_CAST(_operator_lt), METH_FASTCALL, _operator_lt__doc__},

static PyObject *
_operator_lt_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_lt(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("lt", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_lt_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_le__doc__,
"le($module, a, b, /)\n"
"--\n"
"\n"
"Same as a <= b.");

#define _OPERATOR_LE_METHODDEF    \
    {"le", _PyCFunction_CAST(_operator_le), METH_FASTCALL, _operator_le__doc__},

static PyObject *
_operator_le_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_le(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("le", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_le_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_gt__doc__,
"gt($module, a, b, /)\n"
"--\n"
"\n"
"Same as a > b.");

#define _OPERATOR_GT_METHODDEF    \
    {"gt", _PyCFunction_CAST(_operator_gt), METH_FASTCALL, _operator_gt__doc__},

static PyObject *
_operator_gt_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_gt(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("gt", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_gt_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ge__doc__,
"ge($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >= b.");

#define _OPERATOR_GE_METHODDEF    \
    {"ge", _PyCFunction_CAST(_operator_ge), METH_FASTCALL, _operator_ge__doc__},

static PyObject *
_operator_ge_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ge(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ge", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ge_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_pow__doc__,
"pow($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ** b.");

#define _OPERATOR_POW_METHODDEF    \
    {"pow", _PyCFunction_CAST(_operator_pow), METH_FASTCALL, _operator_pow__doc__},

static PyObject *
_operator_pow_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_pow(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("pow", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_pow_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ipow__doc__,
"ipow($module, a, b, /)\n"
"--\n"
"\n"
"Same as a **= b.");

#define _OPERATOR_IPOW_METHODDEF    \
    {"ipow", _PyCFunction_CAST(_operator_ipow), METH_FASTCALL, _operator_ipow__doc__},

static PyObject *
_operator_ipow_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_ipow(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("ipow", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_ipow_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_index__doc__,
"index($module, a, /)\n"
"--\n"
"\n"
"Same as a.__index__()");

#define _OPERATOR_INDEX_METHODDEF    \
    {"index", (PyCFunction)_operator_index, METH_O, _operator_index__doc__},

PyDoc_STRVAR(_operator_is___doc__,
"is_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a is b.");

#define _OPERATOR_IS__METHODDEF    \
    {"is_", _PyCFunction_CAST(_operator_is_), METH_FASTCALL, _operator_is___doc__},

static PyObject *
_operator_is__impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_is_(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("is_", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_is__impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_is_not__doc__,
"is_not($module, a, b, /)\n"
"--\n"
"\n"
"Same as a is not b.");

#define _OPERATOR_IS_NOT_METHODDEF    \
    {"is_not", _PyCFunction_CAST(_operator_is_not), METH_FASTCALL, _operator_is_not__doc__},

static PyObject *
_operator_is_not_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator_is_not(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("is_not", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator_is_not_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_length_hint__doc__,
"length_hint($module, obj, default=0, /)\n"
"--\n"
"\n"
"Return an estimate of the number of items in obj.\n"
"\n"
"This is useful for presizing containers when building from an iterable.\n"
"\n"
"If the object supports len(), the result will be exact.\n"
"Otherwise, it may over- or under-estimate by an arbitrary amount.\n"
"The result will be an integer >= 0.");

#define _OPERATOR_LENGTH_HINT_METHODDEF    \
    {"length_hint", _PyCFunction_CAST(_operator_length_hint), METH_FASTCALL, _operator_length_hint__doc__},

static Py_ssize_t
_operator_length_hint_impl(PyObject *module, PyObject *obj,
                           Py_ssize_t default_value);

static PyObject *
_operator_length_hint(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    Py_ssize_t default_value = 0;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("length_hint", nargs, 1, 2)) {
        goto exit;
    }
    obj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        default_value = ival;
    }
skip_optional:
    _return_value = _operator_length_hint_impl(module, obj, default_value);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator__compare_digest__doc__,
"_compare_digest($module, a, b, /)\n"
"--\n"
"\n"
"Return \'a == b\'.\n"
"\n"
"This function uses an approach designed to prevent\n"
"timing analysis, making it appropriate for cryptography.\n"
"\n"
"a and b must both be of the same type: either str (ASCII only),\n"
"or any bytes-like object.\n"
"\n"
"Note: If a and b are of different lengths, or if an error occurs,\n"
"a timing attack could theoretically reveal information about the\n"
"types and lengths of a and b--but not their values.");

#define _OPERATOR__COMPARE_DIGEST_METHODDEF    \
    {"_compare_digest", _PyCFunction_CAST(_operator__compare_digest), METH_FASTCALL, _operator__compare_digest__doc__},

static PyObject *
_operator__compare_digest_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_operator__compare_digest(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("_compare_digest", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _operator__compare_digest_impl(module, a, b);

exit:
    return return_value;
}
/*[clinic end generated code: output=ddbba2cd943571eb input=a9049054013a1b77]*/

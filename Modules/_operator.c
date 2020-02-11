
#include "Python.h"

#include "clinic/_operator.c.h"

/*[clinic input]
module _operator
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=672ecf48487521e7]*/

PyDoc_STRVAR(operator_doc,
"Operator interface.\n\
\n\
This module exports a set of functions implemented in C corresponding\n\
to the intrinsic operators of Python.  For example, operator.add(x, y)\n\
is equivalent to the expression x+y.  The function names are those\n\
used for special methods; variants without leading and trailing\n\
'__' are also provided for convenience.");


/*[clinic input]
_operator.truth -> bool

    a: object
    /

Return True if a is true, False otherwise.
[clinic start generated code]*/

static int
_operator_truth_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=eaf87767234fa5d7 input=bc74a4cd90235875]*/
{
    return PyObject_IsTrue(a);
}

/*[clinic input]
_operator.add

    a: object
    b: object
    /

Same as a + b.
[clinic start generated code]*/

static PyObject *
_operator_add_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8292984204f45164 input=5efe3bff856ac215]*/
{
    return PyNumber_Add(a, b);
}

/*[clinic input]
_operator.sub = _operator.add

Same as a - b.
[clinic start generated code]*/

static PyObject *
_operator_sub_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4adfc3b888c1ee2e input=6494c6b100b8e795]*/
{
    return PyNumber_Subtract(a, b);
}

/*[clinic input]
_operator.mul = _operator.add

Same as a * b.
[clinic start generated code]*/

static PyObject *
_operator_mul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=d24d66f55a01944c input=2368615b4358b70d]*/
{
    return PyNumber_Multiply(a, b);
}

/*[clinic input]
_operator.matmul = _operator.add

Same as a @ b.
[clinic start generated code]*/

static PyObject *
_operator_matmul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=a20d917eb35d0101 input=9ab304e37fb42dd4]*/
{
    return PyNumber_MatrixMultiply(a, b);
}

/*[clinic input]
_operator.floordiv = _operator.add

Same as a // b.
[clinic start generated code]*/

static PyObject *
_operator_floordiv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=df26b71a60589f99 input=bb2e88ba446c612c]*/
{
    return PyNumber_FloorDivide(a, b);
}

/*[clinic input]
_operator.truediv = _operator.add

Same as a / b.
[clinic start generated code]*/

static PyObject *
_operator_truediv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=0e6a959944d77719 input=ecbb947673f4eb1f]*/
{
    return PyNumber_TrueDivide(a, b);
}

/*[clinic input]
_operator.mod = _operator.add

Same as a % b.
[clinic start generated code]*/

static PyObject *
_operator_mod_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=9519822f0bbec166 input=102e19b422342ac1]*/
{
    return PyNumber_Remainder(a, b);
}

/*[clinic input]
_operator.neg

    a: object
    /

Same as -a.
[clinic start generated code]*/

static PyObject *
_operator_neg(PyObject *module, PyObject *a)
/*[clinic end generated code: output=36e08ecfc6a1c08c input=84f09bdcf27c96ec]*/
{
    return PyNumber_Negative(a);
}

/*[clinic input]
_operator.pos = _operator.neg

Same as +a.
[clinic start generated code]*/

static PyObject *
_operator_pos(PyObject *module, PyObject *a)
/*[clinic end generated code: output=dad7a126221dd091 input=b6445b63fddb8772]*/
{
    return PyNumber_Positive(a);
}

/*[clinic input]
_operator.abs = _operator.neg

Same as abs(a).
[clinic start generated code]*/

static PyObject *
_operator_abs(PyObject *module, PyObject *a)
/*[clinic end generated code: output=1389a93ba053ea3e input=341d07ba86f58039]*/
{
    return PyNumber_Absolute(a);
}

/*[clinic input]
_operator.inv = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyObject *
_operator_inv(PyObject *module, PyObject *a)
/*[clinic end generated code: output=a56875ba075ee06d input=b01a4677739f6eb2]*/
{
    return PyNumber_Invert(a);
}

/*[clinic input]
_operator.invert = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyObject *
_operator_invert(PyObject *module, PyObject *a)
/*[clinic end generated code: output=406b5aa030545fcc input=7f2d607176672e55]*/
{
    return PyNumber_Invert(a);
}

/*[clinic input]
_operator.lshift = _operator.add

Same as a << b.
[clinic start generated code]*/

static PyObject *
_operator_lshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=37f7e52c41435bd8 input=746e8a160cbbc9eb]*/
{
    return PyNumber_Lshift(a, b);
}

/*[clinic input]
_operator.rshift = _operator.add

Same as a >> b.
[clinic start generated code]*/

static PyObject *
_operator_rshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4593c7ef30ec2ee3 input=d2c85bb5a64504c2]*/
{
    return PyNumber_Rshift(a, b);
}

/*[clinic input]
_operator.not_ = _operator.truth

Same as not a.
[clinic start generated code]*/

static int
_operator_not__impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=743f9c24a09759ef input=854156d50804d9b8]*/
{
    return PyObject_Not(a);
}

/*[clinic input]
_operator.and_ = _operator.add

Same as a & b.
[clinic start generated code]*/

static PyObject *
_operator_and__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=93c4fe88f7b76d9e input=4f3057c90ec4c99f]*/
{
    return PyNumber_And(a, b);
}

/*[clinic input]
_operator.xor = _operator.add

Same as a ^ b.
[clinic start generated code]*/

static PyObject *
_operator_xor_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=b24cd8b79fde0004 input=3c5cfa7253d808dd]*/
{
    return PyNumber_Xor(a, b);
}

/*[clinic input]
_operator.or_ = _operator.add

Same as a | b.
[clinic start generated code]*/

static PyObject *
_operator_or__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=58024867b8d90461 input=b40c6c44f7c79c09]*/
{
    return PyNumber_Or(a, b);
}

/*[clinic input]
_operator.iadd = _operator.add

Same as a += b.
[clinic start generated code]*/

static PyObject *
_operator_iadd_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=07dc627832526eb5 input=d22a91c07ac69227]*/
{
    return PyNumber_InPlaceAdd(a, b);
}

/*[clinic input]
_operator.isub = _operator.add

Same as a -= b.
[clinic start generated code]*/

static PyObject *
_operator_isub_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4513467d23b5e0b1 input=4591b00d0a0ccafd]*/
{
    return PyNumber_InPlaceSubtract(a, b);
}

/*[clinic input]
_operator.imul = _operator.add

Same as a *= b.
[clinic start generated code]*/

static PyObject *
_operator_imul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=5e87dacd19a71eab input=0e01fb8631e1b76f]*/
{
    return PyNumber_InPlaceMultiply(a, b);
}

/*[clinic input]
_operator.imatmul = _operator.add

Same as a @= b.
[clinic start generated code]*/

static PyObject *
_operator_imatmul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=d603cbdf716ce519 input=bb614026372cd542]*/
{
    return PyNumber_InPlaceMatrixMultiply(a, b);
}

/*[clinic input]
_operator.ifloordiv = _operator.add

Same as a //= b.
[clinic start generated code]*/

static PyObject *
_operator_ifloordiv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=535336048c681794 input=9df3b5021cff4ca1]*/
{
    return PyNumber_InPlaceFloorDivide(a, b);
}

/*[clinic input]
_operator.itruediv = _operator.add

Same as a /= b.
[clinic start generated code]*/

static PyObject *
_operator_itruediv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=28017fbd3563952f input=9a1ee01608f5f590]*/
{
    return PyNumber_InPlaceTrueDivide(a, b);
}

/*[clinic input]
_operator.imod = _operator.add

Same as a %= b.
[clinic start generated code]*/

static PyObject *
_operator_imod_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=f7c540ae0fc70904 input=d0c384a3ce38e1dd]*/
{
    return PyNumber_InPlaceRemainder(a, b);
}

/*[clinic input]
_operator.ilshift = _operator.add

Same as a <<= b.
[clinic start generated code]*/

static PyObject *
_operator_ilshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=e73a8fee1ac18749 input=e21b6b310f54572e]*/
{
    return PyNumber_InPlaceLshift(a, b);
}

/*[clinic input]
_operator.irshift = _operator.add

Same as a >>= b.
[clinic start generated code]*/

static PyObject *
_operator_irshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=97f2af6b5ff2ed81 input=6778dbd0f6e1ec16]*/
{
    return PyNumber_InPlaceRshift(a, b);
}

/*[clinic input]
_operator.iand = _operator.add

Same as a &= b.
[clinic start generated code]*/

static PyObject *
_operator_iand_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4599e9d40cbf7d00 input=71dfd8e70c156a7b]*/
{
    return PyNumber_InPlaceAnd(a, b);
}

/*[clinic input]
_operator.ixor = _operator.add

Same as a ^= b.
[clinic start generated code]*/

static PyObject *
_operator_ixor_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=5ff881766872be03 input=695c32bec0604d86]*/
{
    return PyNumber_InPlaceXor(a, b);
}

/*[clinic input]
_operator.ior = _operator.add

Same as a |= b.
[clinic start generated code]*/

static PyObject *
_operator_ior_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=48aac319445bf759 input=8f01d03eda9920cf]*/
{
    return PyNumber_InPlaceOr(a, b);
}

/*[clinic input]
_operator.concat = _operator.add

Same as a + b, for a and b sequences.
[clinic start generated code]*/

static PyObject *
_operator_concat_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=80028390942c5f11 input=8544ccd5341a3658]*/
{
    return PySequence_Concat(a, b);
}

/*[clinic input]
_operator.iconcat = _operator.add

Same as a += b, for a and b sequences.
[clinic start generated code]*/

static PyObject *
_operator_iconcat_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=3ea0a162ebb2e26d input=8f5fe5722fcd837e]*/
{
    return PySequence_InPlaceConcat(a, b);
}

/*[clinic input]
_operator.contains -> bool

    a: object
    b: object
    /

Same as b in a (note reversed operands).
[clinic start generated code]*/

static int
_operator_contains_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=413b4dbe82b6ffc1 input=9122a69b505fde13]*/
{
    return PySequence_Contains(a, b);
}

/*[clinic input]
_operator.indexOf -> Py_ssize_t

    a: object
    b: object
    /

Return the first index of b in a.
[clinic start generated code]*/

static Py_ssize_t
_operator_indexOf_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=c6226d8e0fb60fa6 input=8be2e43b6a6fffe3]*/
{
    return PySequence_Index(a, b);
}

/*[clinic input]
_operator.countOf = _operator.indexOf

Return the number of times b occurs in a.
[clinic start generated code]*/

static Py_ssize_t
_operator_countOf_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=9e1623197daf3382 input=0c3a2656add252db]*/
{
    return PySequence_Count(a, b);
}

/*[clinic input]
_operator.getitem

    a: object
    b: object
    /

Same as a[b].
[clinic start generated code]*/

static PyObject *
_operator_getitem_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=6c8d8101a676e594 input=6682797320e48845]*/
{
    return PyObject_GetItem(a, b);
}

/*[clinic input]
_operator.setitem

    a: object
    b: object
    c: object
    /

Same as a[b] = c.
[clinic start generated code]*/

static PyObject *
_operator_setitem_impl(PyObject *module, PyObject *a, PyObject *b,
                       PyObject *c)
/*[clinic end generated code: output=1324f9061ae99e25 input=ceaf453c4d3a58df]*/
{
    if (-1 == PyObject_SetItem(a, b, c))
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_operator.delitem = _operator.getitem

Same as del a[b].
[clinic start generated code]*/

static PyObject *
_operator_delitem_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=db18f61506295799 input=991bec56a0d3ec7f]*/
{
    if (-1 == PyObject_DelItem(a, b))
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_operator.eq

    a: object
    b: object
    /

Same as a == b.
[clinic start generated code]*/

static PyObject *
_operator_eq_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8d7d46ed4135677c input=586fca687a95a83f]*/
{
    return PyObject_RichCompare(a, b, Py_EQ);
}

/*[clinic input]
_operator.ne = _operator.eq

Same as a != b.
[clinic start generated code]*/

static PyObject *
_operator_ne_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=c99bd0c3a4c01297 input=5d88f23d35e9abac]*/
{
    return PyObject_RichCompare(a, b, Py_NE);
}

/*[clinic input]
_operator.lt = _operator.eq

Same as a < b.
[clinic start generated code]*/

static PyObject *
_operator_lt_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=082d7c45c440e535 input=34a59ad6d39d3a2b]*/
{
    return PyObject_RichCompare(a, b, Py_LT);
}

/*[clinic input]
_operator.le = _operator.eq

Same as a <= b.
[clinic start generated code]*/

static PyObject *
_operator_le_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=00970a2923d0ae17 input=b812a7860a0bef44]*/
{
    return PyObject_RichCompare(a, b, Py_LE);
}

/*[clinic input]
_operator.gt = _operator.eq

Same as a > b.
[clinic start generated code]*/

static PyObject *
_operator_gt_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8d373349ecf25641 input=9bdb45b995ada35b]*/
{
    return PyObject_RichCompare(a, b, Py_GT);
}

/*[clinic input]
_operator.ge = _operator.eq

Same as a >= b.
[clinic start generated code]*/

static PyObject *
_operator_ge_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=7ce3882256d4b137 input=cf1dc4a5ca9c35f5]*/
{
    return PyObject_RichCompare(a, b, Py_GE);
}

/*[clinic input]
_operator.pow = _operator.add

Same as a ** b.
[clinic start generated code]*/

static PyObject *
_operator_pow_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=09e668ad50036120 input=690b40f097ab1637]*/
{
    return PyNumber_Power(a, b, Py_None);
}

/*[clinic input]
_operator.ipow = _operator.add

Same as a **= b.
[clinic start generated code]*/

static PyObject *
_operator_ipow_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=7189ff4d4367c808 input=f00623899d07499a]*/
{
    return PyNumber_InPlacePower(a, b, Py_None);
}

/*[clinic input]
_operator.index

    a: object
    /

Same as a.__index__()
[clinic start generated code]*/

static PyObject *
_operator_index(PyObject *module, PyObject *a)
/*[clinic end generated code: output=d972b0764ac305fc input=6f54d50ea64a579c]*/
{
    return PyNumber_Index(a);
}

/*[clinic input]
_operator.is_ = _operator.add

Same as a is b.
[clinic start generated code]*/

static PyObject *
_operator_is__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=bcd47a402e482e1d input=5fa9b97df03c427f]*/
{
    PyObject *result;
    result = (a == b) ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

/*[clinic input]
_operator.is_not = _operator.add

Same as a is not b.
[clinic start generated code]*/

static PyObject *
_operator_is_not_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=491a1f2f81f6c7f9 input=5a93f7e1a93535f1]*/
{
    PyObject *result;
    result = (a != b) ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

/* compare_digest **********************************************************/

/*
 * timing safe compare
 *
 * Returns 1 of the strings are equal.
 * In case of len(a) != len(b) the function tries to keep the timing
 * dependent on the length of b. CPU cache locally may still alter timing
 * a bit.
 */
static int
_tscmp(const unsigned char *a, const unsigned char *b,
        Py_ssize_t len_a, Py_ssize_t len_b)
{
    /* The volatile type declarations make sure that the compiler has no
     * chance to optimize and fold the code in any way that may change
     * the timing.
     */
    volatile Py_ssize_t length;
    volatile const unsigned char *left;
    volatile const unsigned char *right;
    Py_ssize_t i;
    unsigned char result;

    /* loop count depends on length of b */
    length = len_b;
    left = NULL;
    right = b;

    /* don't use else here to keep the amount of CPU instructions constant,
     * volatile forces re-evaluation
     *  */
    if (len_a == length) {
        left = *((volatile const unsigned char**)&a);
        result = 0;
    }
    if (len_a != length) {
        left = b;
        result = 1;
    }

    for (i=0; i < length; i++) {
        result |= *left++ ^ *right++;
    }

    return (result == 0);
}

/*[clinic input]
_operator.length_hint -> Py_ssize_t

    obj: object
    default: Py_ssize_t = 0
    /

Return an estimate of the number of items in obj.

This is useful for presizing containers when building from an iterable.

If the object supports len(), the result will be exact.
Otherwise, it may over- or under-estimate by an arbitrary amount.
The result will be an integer >= 0.
[clinic start generated code]*/

static Py_ssize_t
_operator_length_hint_impl(PyObject *module, PyObject *obj,
                           Py_ssize_t default_value)
/*[clinic end generated code: output=01d469edc1d612ad input=65ed29f04401e96a]*/
{
    return PyObject_LengthHint(obj, default_value);
}

/*[clinic input]
_operator._compare_digest = _operator.eq

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.
[clinic start generated code]*/

static PyObject *
_operator__compare_digest_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=11d452bdd3a23cbc input=9ac7e2c4e30bc356]*/
{
    int rc;

    /* ASCII unicode string */
    if(PyUnicode_Check(a) && PyUnicode_Check(b)) {
        if (PyUnicode_READY(a) == -1 || PyUnicode_READY(b) == -1) {
            return NULL;
        }
        if (!PyUnicode_IS_ASCII(a) || !PyUnicode_IS_ASCII(b)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return NULL;
        }

        rc = _tscmp(PyUnicode_DATA(a),
                    PyUnicode_DATA(b),
                    PyUnicode_GET_LENGTH(a),
                    PyUnicode_GET_LENGTH(b));
    }
    /* fallback to buffer interface for bytes, bytesarray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (PyObject_CheckBuffer(a) == 0 && PyObject_CheckBuffer(b) == 0) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            return NULL;
        }

        if (PyObject_GetBuffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return NULL;
        }

        if (PyObject_GetBuffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyBool_FromLong(rc);
}

/* operator methods **********************************************************/

static struct PyMethodDef operator_methods[] = {

    _OPERATOR_TRUTH_METHODDEF
    _OPERATOR_CONTAINS_METHODDEF
    _OPERATOR_INDEXOF_METHODDEF
    _OPERATOR_COUNTOF_METHODDEF
    _OPERATOR_IS__METHODDEF
    _OPERATOR_IS_NOT_METHODDEF
    _OPERATOR_INDEX_METHODDEF
    _OPERATOR_ADD_METHODDEF
    _OPERATOR_SUB_METHODDEF
    _OPERATOR_MUL_METHODDEF
    _OPERATOR_MATMUL_METHODDEF
    _OPERATOR_FLOORDIV_METHODDEF
    _OPERATOR_TRUEDIV_METHODDEF
    _OPERATOR_MOD_METHODDEF
    _OPERATOR_NEG_METHODDEF
    _OPERATOR_POS_METHODDEF
    _OPERATOR_ABS_METHODDEF
    _OPERATOR_INV_METHODDEF
    _OPERATOR_INVERT_METHODDEF
    _OPERATOR_LSHIFT_METHODDEF
    _OPERATOR_RSHIFT_METHODDEF
    _OPERATOR_NOT__METHODDEF
    _OPERATOR_AND__METHODDEF
    _OPERATOR_XOR_METHODDEF
    _OPERATOR_OR__METHODDEF
    _OPERATOR_IADD_METHODDEF
    _OPERATOR_ISUB_METHODDEF
    _OPERATOR_IMUL_METHODDEF
    _OPERATOR_IMATMUL_METHODDEF
    _OPERATOR_IFLOORDIV_METHODDEF
    _OPERATOR_ITRUEDIV_METHODDEF
    _OPERATOR_IMOD_METHODDEF
    _OPERATOR_ILSHIFT_METHODDEF
    _OPERATOR_IRSHIFT_METHODDEF
    _OPERATOR_IAND_METHODDEF
    _OPERATOR_IXOR_METHODDEF
    _OPERATOR_IOR_METHODDEF
    _OPERATOR_CONCAT_METHODDEF
    _OPERATOR_ICONCAT_METHODDEF
    _OPERATOR_GETITEM_METHODDEF
    _OPERATOR_SETITEM_METHODDEF
    _OPERATOR_DELITEM_METHODDEF
    _OPERATOR_POW_METHODDEF
    _OPERATOR_IPOW_METHODDEF
    _OPERATOR_EQ_METHODDEF
    _OPERATOR_NE_METHODDEF
    _OPERATOR_LT_METHODDEF
    _OPERATOR_LE_METHODDEF
    _OPERATOR_GT_METHODDEF
    _OPERATOR_GE_METHODDEF
    _OPERATOR__COMPARE_DIGEST_METHODDEF
    _OPERATOR_LENGTH_HINT_METHODDEF
    {NULL,              NULL}           /* sentinel */

};

/* itemgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nitems;
    PyObject *item;
    Py_ssize_t index; // -1 unless *item* is a single non-negative integer index
} itemgetterobject;

static PyTypeObject itemgetter_type;

/* AC 3.5: treats first argument as an iterable, otherwise uses *args */
static PyObject *
itemgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    itemgetterobject *ig;
    PyObject *item;
    Py_ssize_t nitems;
    Py_ssize_t index;

    if (!_PyArg_NoKeywords("itemgetter", kwds))
        return NULL;

    nitems = PyTuple_GET_SIZE(args);
    if (nitems <= 1) {
        if (!PyArg_UnpackTuple(args, "itemgetter", 1, 1, &item))
            return NULL;
    } else
        item = args;

    /* create itemgetterobject structure */
    ig = PyObject_GC_New(itemgetterobject, &itemgetter_type);
    if (ig == NULL)
        return NULL;

    Py_INCREF(item);
    ig->item = item;
    ig->nitems = nitems;
    ig->index = -1;
    if (PyLong_CheckExact(item)) {
        index = PyLong_AsSsize_t(item);
        if (index < 0) {
            /* If we get here, then either the index conversion failed
             * due to being out of range, or the index was a negative
             * integer.  Either way, we clear any possible exception
             * and fall back to the slow path, where ig->index is -1.
             */
            PyErr_Clear();
        }
        else {
            ig->index = index;
        }
    }

    PyObject_GC_Track(ig);
    return (PyObject *)ig;
}

static void
itemgetter_dealloc(itemgetterobject *ig)
{
    PyObject_GC_UnTrack(ig);
    Py_XDECREF(ig->item);
    PyObject_GC_Del(ig);
}

static int
itemgetter_traverse(itemgetterobject *ig, visitproc visit, void *arg)
{
    Py_VISIT(ig->item);
    return 0;
}

static PyObject *
itemgetter_call(itemgetterobject *ig, PyObject *args, PyObject *kw)
{
    PyObject *obj, *result;
    Py_ssize_t i, nitems=ig->nitems;

    assert(PyTuple_CheckExact(args));
    if (!_PyArg_NoKeywords("itemgetter", kw))
        return NULL;
    if (!_PyArg_CheckPositional("itemgetter", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;

    obj = PyTuple_GET_ITEM(args, 0);
    if (nitems == 1) {
        if (ig->index >= 0
            && PyTuple_CheckExact(obj)
            && ig->index < PyTuple_GET_SIZE(obj))
        {
            result = PyTuple_GET_ITEM(obj, ig->index);
            Py_INCREF(result);
            return result;
        }
        return PyObject_GetItem(obj, ig->item);
    }

    assert(PyTuple_Check(ig->item));
    assert(PyTuple_GET_SIZE(ig->item) == nitems);

    result = PyTuple_New(nitems);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nitems ; i++) {
        PyObject *item, *val;
        item = PyTuple_GET_ITEM(ig->item, i);
        val = PyObject_GetItem(obj, item);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, i, val);
    }
    return result;
}

static PyObject *
itemgetter_repr(itemgetterobject *ig)
{
    PyObject *repr;
    const char *reprfmt;

    int status = Py_ReprEnter((PyObject *)ig);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ig)->tp_name);
    }

    reprfmt = ig->nitems == 1 ? "%s(%R)" : "%s%R";
    repr = PyUnicode_FromFormat(reprfmt, Py_TYPE(ig)->tp_name, ig->item);
    Py_ReprLeave((PyObject *)ig);
    return repr;
}

static PyObject *
itemgetter_reduce(itemgetterobject *ig, PyObject *Py_UNUSED(ignored))
{
    if (ig->nitems == 1)
        return Py_BuildValue("O(O)", Py_TYPE(ig), ig->item);
    return PyTuple_Pack(2, Py_TYPE(ig), ig->item);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling");

static PyMethodDef itemgetter_methods[] = {
    {"__reduce__", (PyCFunction)itemgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

PyDoc_STRVAR(itemgetter_doc,
"itemgetter(item, ...) --> itemgetter object\n\
\n\
Return a callable object that fetches the given item(s) from its operand.\n\
After f = itemgetter(2), the call f(r) returns r[2].\n\
After g = itemgetter(2, 5, 3), the call g(r) returns (r[2], r[5], r[3])");

static PyTypeObject itemgetter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.itemgetter",              /* tp_name */
    sizeof(itemgetterobject),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)itemgetter_dealloc,     /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)itemgetter_repr,          /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)itemgetter_call,       /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,            /* tp_flags */
    itemgetter_doc,                     /* tp_doc */
    (traverseproc)itemgetter_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    itemgetter_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    itemgetter_new,                     /* tp_new */
    0,                                  /* tp_free */
};


/* attrgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nattrs;
    PyObject *attr;
} attrgetterobject;

static PyTypeObject attrgetter_type;

/* AC 3.5: treats first argument as an iterable, otherwise uses *args */
static PyObject *
attrgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    attrgetterobject *ag;
    PyObject *attr;
    Py_ssize_t nattrs, idx, char_idx;

    if (!_PyArg_NoKeywords("attrgetter", kwds))
        return NULL;

    nattrs = PyTuple_GET_SIZE(args);
    if (nattrs <= 1) {
        if (!PyArg_UnpackTuple(args, "attrgetter", 1, 1, &attr))
            return NULL;
    }

    attr = PyTuple_New(nattrs);
    if (attr == NULL)
        return NULL;

    /* prepare attr while checking args */
    for (idx = 0; idx < nattrs; ++idx) {
        PyObject *item = PyTuple_GET_ITEM(args, idx);
        Py_ssize_t item_len;
        void *data;
        unsigned int kind;
        int dot_count;

        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError,
                            "attribute name must be a string");
            Py_DECREF(attr);
            return NULL;
        }
        if (PyUnicode_READY(item)) {
            Py_DECREF(attr);
            return NULL;
        }
        item_len = PyUnicode_GET_LENGTH(item);
        kind = PyUnicode_KIND(item);
        data = PyUnicode_DATA(item);

        /* check whethere the string is dotted */
        dot_count = 0;
        for (char_idx = 0; char_idx < item_len; ++char_idx) {
            if (PyUnicode_READ(kind, data, char_idx) == '.')
                ++dot_count;
        }

        if (dot_count == 0) {
            Py_INCREF(item);
            PyUnicode_InternInPlace(&item);
            PyTuple_SET_ITEM(attr, idx, item);
        } else { /* make it a tuple of non-dotted attrnames */
            PyObject *attr_chain = PyTuple_New(dot_count + 1);
            PyObject *attr_chain_item;
            Py_ssize_t unibuff_from = 0;
            Py_ssize_t unibuff_till = 0;
            Py_ssize_t attr_chain_idx = 0;

            if (attr_chain == NULL) {
                Py_DECREF(attr);
                return NULL;
            }

            for (; dot_count > 0; --dot_count) {
                while (PyUnicode_READ(kind, data, unibuff_till) != '.') {
                    ++unibuff_till;
                }
                attr_chain_item = PyUnicode_Substring(item,
                                      unibuff_from,
                                      unibuff_till);
                if (attr_chain_item == NULL) {
                    Py_DECREF(attr_chain);
                    Py_DECREF(attr);
                    return NULL;
                }
                PyUnicode_InternInPlace(&attr_chain_item);
                PyTuple_SET_ITEM(attr_chain, attr_chain_idx, attr_chain_item);
                ++attr_chain_idx;
                unibuff_till = unibuff_from = unibuff_till + 1;
            }

            /* now add the last dotless name */
            attr_chain_item = PyUnicode_Substring(item,
                                                  unibuff_from, item_len);
            if (attr_chain_item == NULL) {
                Py_DECREF(attr_chain);
                Py_DECREF(attr);
                return NULL;
            }
            PyUnicode_InternInPlace(&attr_chain_item);
            PyTuple_SET_ITEM(attr_chain, attr_chain_idx, attr_chain_item);

            PyTuple_SET_ITEM(attr, idx, attr_chain);
        }
    }

    /* create attrgetterobject structure */
    ag = PyObject_GC_New(attrgetterobject, &attrgetter_type);
    if (ag == NULL) {
        Py_DECREF(attr);
        return NULL;
    }

    ag->attr = attr;
    ag->nattrs = nattrs;

    PyObject_GC_Track(ag);
    return (PyObject *)ag;
}

static void
attrgetter_dealloc(attrgetterobject *ag)
{
    PyObject_GC_UnTrack(ag);
    Py_XDECREF(ag->attr);
    PyObject_GC_Del(ag);
}

static int
attrgetter_traverse(attrgetterobject *ag, visitproc visit, void *arg)
{
    Py_VISIT(ag->attr);
    return 0;
}

static PyObject *
dotted_getattr(PyObject *obj, PyObject *attr)
{
    PyObject *newobj;

    /* attr is either a tuple or instance of str.
       Ensured by the setup code of attrgetter_new */
    if (PyTuple_CheckExact(attr)) { /* chained getattr */
        Py_ssize_t name_idx = 0, name_count;
        PyObject *attr_name;

        name_count = PyTuple_GET_SIZE(attr);
        Py_INCREF(obj);
        for (name_idx = 0; name_idx < name_count; ++name_idx) {
            attr_name = PyTuple_GET_ITEM(attr, name_idx);
            newobj = PyObject_GetAttr(obj, attr_name);
            Py_DECREF(obj);
            if (newobj == NULL) {
                return NULL;
            }
            /* here */
            obj = newobj;
        }
    } else { /* single getattr */
        newobj = PyObject_GetAttr(obj, attr);
        if (newobj == NULL)
            return NULL;
        obj = newobj;
    }

    return obj;
}

static PyObject *
attrgetter_call(attrgetterobject *ag, PyObject *args, PyObject *kw)
{
    PyObject *obj, *result;
    Py_ssize_t i, nattrs=ag->nattrs;

    if (!_PyArg_NoKeywords("attrgetter", kw))
        return NULL;
    if (!_PyArg_CheckPositional("attrgetter", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;
    obj = PyTuple_GET_ITEM(args, 0);
    if (ag->nattrs == 1) /* ag->attr is always a tuple */
        return dotted_getattr(obj, PyTuple_GET_ITEM(ag->attr, 0));

    assert(PyTuple_Check(ag->attr));
    assert(PyTuple_GET_SIZE(ag->attr) == nattrs);

    result = PyTuple_New(nattrs);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nattrs ; i++) {
        PyObject *attr, *val;
        attr = PyTuple_GET_ITEM(ag->attr, i);
        val = dotted_getattr(obj, attr);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, i, val);
    }
    return result;
}

static PyObject *
dotjoinattr(PyObject *attr, PyObject **attrsep)
{
    if (PyTuple_CheckExact(attr)) {
        if (*attrsep == NULL) {
            *attrsep = PyUnicode_FromString(".");
            if (*attrsep == NULL)
                return NULL;
        }
        return PyUnicode_Join(*attrsep, attr);
    } else {
        Py_INCREF(attr);
        return attr;
    }
}

static PyObject *
attrgetter_args(attrgetterobject *ag)
{
    Py_ssize_t i;
    PyObject *attrsep = NULL;
    PyObject *attrstrings = PyTuple_New(ag->nattrs);
    if (attrstrings == NULL)
        return NULL;

    for (i = 0; i < ag->nattrs; ++i) {
        PyObject *attr = PyTuple_GET_ITEM(ag->attr, i);
        PyObject *attrstr = dotjoinattr(attr, &attrsep);
        if (attrstr == NULL) {
            Py_XDECREF(attrsep);
            Py_DECREF(attrstrings);
            return NULL;
        }
        PyTuple_SET_ITEM(attrstrings, i, attrstr);
    }
    Py_XDECREF(attrsep);
    return attrstrings;
}

static PyObject *
attrgetter_repr(attrgetterobject *ag)
{
    PyObject *repr = NULL;
    int status = Py_ReprEnter((PyObject *)ag);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ag)->tp_name);
    }

    if (ag->nattrs == 1) {
        PyObject *attrsep = NULL;
        PyObject *attr = dotjoinattr(PyTuple_GET_ITEM(ag->attr, 0), &attrsep);
        if (attr != NULL) {
            repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(ag)->tp_name, attr);
            Py_DECREF(attr);
        }
        Py_XDECREF(attrsep);
    }
    else {
        PyObject *attrstrings = attrgetter_args(ag);
        if (attrstrings != NULL) {
            repr = PyUnicode_FromFormat("%s%R",
                                        Py_TYPE(ag)->tp_name, attrstrings);
            Py_DECREF(attrstrings);
        }
    }
    Py_ReprLeave((PyObject *)ag);
    return repr;
}

static PyObject *
attrgetter_reduce(attrgetterobject *ag, PyObject *Py_UNUSED(ignored))
{
    PyObject *attrstrings = attrgetter_args(ag);
    if (attrstrings == NULL)
        return NULL;

    return Py_BuildValue("ON", Py_TYPE(ag), attrstrings);
}

static PyMethodDef attrgetter_methods[] = {
    {"__reduce__", (PyCFunction)attrgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

PyDoc_STRVAR(attrgetter_doc,
"attrgetter(attr, ...) --> attrgetter object\n\
\n\
Return a callable object that fetches the given attribute(s) from its operand.\n\
After f = attrgetter('name'), the call f(r) returns r.name.\n\
After g = attrgetter('name', 'date'), the call g(r) returns (r.name, r.date).\n\
After h = attrgetter('name.first', 'name.last'), the call h(r) returns\n\
(r.name.first, r.name.last).");

static PyTypeObject attrgetter_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.attrgetter",              /* tp_name */
    sizeof(attrgetterobject),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)attrgetter_dealloc,     /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)attrgetter_repr,          /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)attrgetter_call,       /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,            /* tp_flags */
    attrgetter_doc,                     /* tp_doc */
    (traverseproc)attrgetter_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    attrgetter_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    attrgetter_new,                     /* tp_new */
    0,                                  /* tp_free */
};


/* methodcaller object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *args;
    PyObject *kwds;
} methodcallerobject;

static PyTypeObject methodcaller_type;

/* AC 3.5: variable number of arguments, not currently support by AC */
static PyObject *
methodcaller_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    methodcallerobject *mc;
    PyObject *name;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "methodcaller needs at least "
                        "one argument, the method name");
        return NULL;
    }

    name = PyTuple_GET_ITEM(args, 0);
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "method name must be a string");
        return NULL;
    }

    /* create methodcallerobject structure */
    mc = PyObject_GC_New(methodcallerobject, &methodcaller_type);
    if (mc == NULL)
        return NULL;

    name = PyTuple_GET_ITEM(args, 0);
    Py_INCREF(name);
    PyUnicode_InternInPlace(&name);
    mc->name = name;

    Py_XINCREF(kwds);
    mc->kwds = kwds;

    mc->args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (mc->args == NULL) {
        Py_DECREF(mc);
        return NULL;
    }

    PyObject_GC_Track(mc);
    return (PyObject *)mc;
}

static void
methodcaller_dealloc(methodcallerobject *mc)
{
    PyObject_GC_UnTrack(mc);
    Py_XDECREF(mc->name);
    Py_XDECREF(mc->args);
    Py_XDECREF(mc->kwds);
    PyObject_GC_Del(mc);
}

static int
methodcaller_traverse(methodcallerobject *mc, visitproc visit, void *arg)
{
    Py_VISIT(mc->args);
    Py_VISIT(mc->kwds);
    return 0;
}

static PyObject *
methodcaller_call(methodcallerobject *mc, PyObject *args, PyObject *kw)
{
    PyObject *method, *obj, *result;

    if (!_PyArg_NoKeywords("methodcaller", kw))
        return NULL;
    if (!_PyArg_CheckPositional("methodcaller", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;
    obj = PyTuple_GET_ITEM(args, 0);
    method = PyObject_GetAttr(obj, mc->name);
    if (method == NULL)
        return NULL;
    result = PyObject_Call(method, mc->args, mc->kwds);
    Py_DECREF(method);
    return result;
}

static PyObject *
methodcaller_repr(methodcallerobject *mc)
{
    PyObject *argreprs, *repr = NULL, *sep, *joinedargreprs;
    Py_ssize_t numtotalargs, numposargs, numkwdargs, i;
    int status = Py_ReprEnter((PyObject *)mc);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(mc)->tp_name);
    }

    numkwdargs = mc->kwds != NULL ? PyDict_GET_SIZE(mc->kwds) : 0;
    numposargs = PyTuple_GET_SIZE(mc->args);
    numtotalargs = numposargs + numkwdargs;

    if (numtotalargs == 0) {
        repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(mc)->tp_name, mc->name);
        Py_ReprLeave((PyObject *)mc);
        return repr;
    }

    argreprs = PyTuple_New(numtotalargs);
    if (argreprs == NULL) {
        Py_ReprLeave((PyObject *)mc);
        return NULL;
    }

    for (i = 0; i < numposargs; ++i) {
        PyObject *onerepr = PyObject_Repr(PyTuple_GET_ITEM(mc->args, i));
        if (onerepr == NULL)
            goto done;
        PyTuple_SET_ITEM(argreprs, i, onerepr);
    }

    if (numkwdargs != 0) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(mc->kwds, &pos, &key, &value)) {
            PyObject *onerepr = PyUnicode_FromFormat("%U=%R", key, value);
            if (onerepr == NULL)
                goto done;
            if (i >= numtotalargs) {
                i = -1;
                Py_DECREF(onerepr);
                break;
            }
            PyTuple_SET_ITEM(argreprs, i, onerepr);
            ++i;
        }
        if (i != numtotalargs) {
            PyErr_SetString(PyExc_RuntimeError,
                            "keywords dict changed size during iteration");
            goto done;
        }
    }

    sep = PyUnicode_FromString(", ");
    if (sep == NULL)
        goto done;

    joinedargreprs = PyUnicode_Join(sep, argreprs);
    Py_DECREF(sep);
    if (joinedargreprs == NULL)
        goto done;

    repr = PyUnicode_FromFormat("%s(%R, %U)", Py_TYPE(mc)->tp_name,
                                mc->name, joinedargreprs);
    Py_DECREF(joinedargreprs);

done:
    Py_DECREF(argreprs);
    Py_ReprLeave((PyObject *)mc);
    return repr;
}

static PyObject *
methodcaller_reduce(methodcallerobject *mc, PyObject *Py_UNUSED(ignored))
{
    PyObject *newargs;
    if (!mc->kwds || PyDict_GET_SIZE(mc->kwds) == 0) {
        Py_ssize_t i;
        Py_ssize_t callargcount = PyTuple_GET_SIZE(mc->args);
        newargs = PyTuple_New(1 + callargcount);
        if (newargs == NULL)
            return NULL;
        Py_INCREF(mc->name);
        PyTuple_SET_ITEM(newargs, 0, mc->name);
        for (i = 0; i < callargcount; ++i) {
            PyObject *arg = PyTuple_GET_ITEM(mc->args, i);
            Py_INCREF(arg);
            PyTuple_SET_ITEM(newargs, i + 1, arg);
        }
        return Py_BuildValue("ON", Py_TYPE(mc), newargs);
    }
    else {
        PyObject *functools;
        PyObject *partial;
        PyObject *constructor;
        PyObject *newargs[2];

        _Py_IDENTIFIER(partial);
        functools = PyImport_ImportModule("functools");
        if (!functools)
            return NULL;
        partial = _PyObject_GetAttrId(functools, &PyId_partial);
        Py_DECREF(functools);
        if (!partial)
            return NULL;

        newargs[0] = (PyObject *)Py_TYPE(mc);
        newargs[1] = mc->name;
        constructor = PyObject_VectorcallDict(partial, newargs, 2, mc->kwds);

        Py_DECREF(partial);
        return Py_BuildValue("NO", constructor, mc->args);
    }
}

static PyMethodDef methodcaller_methods[] = {
    {"__reduce__", (PyCFunction)methodcaller_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};
PyDoc_STRVAR(methodcaller_doc,
"methodcaller(name, ...) --> methodcaller object\n\
\n\
Return a callable object that calls the given method on its operand.\n\
After f = methodcaller('name'), the call f(r) returns r.name().\n\
After g = methodcaller('name', 'date', foo=1), the call g(r) returns\n\
r.name('date', foo=1).");

static PyTypeObject methodcaller_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "operator.methodcaller",            /* tp_name */
    sizeof(methodcallerobject),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)methodcaller_dealloc, /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)methodcaller_repr,        /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)methodcaller_call,     /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    methodcaller_doc,                           /* tp_doc */
    (traverseproc)methodcaller_traverse,        /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    methodcaller_methods,               /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    methodcaller_new,                   /* tp_new */
    0,                                  /* tp_free */
};


/* Initialization function for the module (*must* be called PyInit__operator) */


static struct PyModuleDef operatormodule = {
    PyModuleDef_HEAD_INIT,
    "_operator",
    operator_doc,
    -1,
    operator_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__operator(void)
{
    PyObject *m;

    /* Create the module and add the functions */
    m = PyModule_Create(&operatormodule);
    if (m == NULL)
        return NULL;

    if (PyType_Ready(&itemgetter_type) < 0)
        return NULL;
    Py_INCREF(&itemgetter_type);
    PyModule_AddObject(m, "itemgetter", (PyObject *)&itemgetter_type);

    if (PyType_Ready(&attrgetter_type) < 0)
        return NULL;
    Py_INCREF(&attrgetter_type);
    PyModule_AddObject(m, "attrgetter", (PyObject *)&attrgetter_type);

    if (PyType_Ready(&methodcaller_type) < 0)
        return NULL;
    Py_INCREF(&methodcaller_type);
    PyModule_AddObject(m, "methodcaller", (PyObject *)&methodcaller_type);
    return m;
}

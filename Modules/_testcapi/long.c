#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "parts.h"
#include "util.h"
#include "clinic/long.c.h"

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/


/*[clinic input]
_testcapi.call_long_compact_api
    arg: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_call_long_compact_api(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=7e3894f611b1b2b7 input=87b87396967af14c]*/

{
    assert(PyLong_Check(arg));
    int is_compact = PyUnstable_Long_IsCompact((PyLongObject*)arg);
    Py_ssize_t value = -1;
    if (is_compact) {
        value = PyUnstable_Long_CompactValue((PyLongObject*)arg);
    }
    return Py_BuildValue("in", is_compact, value);
}


static PyObject *
pylong_fromunicodeobject(PyObject *module, PyObject *args)
{
    PyObject *unicode;
    int base;
    if (!PyArg_ParseTuple(args, "Oi", &unicode, &base)) {
        return NULL;
    }

    NULLABLE(unicode);
    return PyLong_FromUnicodeObject(unicode, base);
}


static PyObject *
pylong_asnativebytes(PyObject *module, PyObject *args)
{
    PyObject *v;
    Py_buffer buffer;
    Py_ssize_t n, flags;
    if (!PyArg_ParseTuple(args, "Ow*nn", &v, &buffer, &n, &flags)) {
        return NULL;
    }
    if (buffer.readonly) {
        PyErr_SetString(PyExc_TypeError, "buffer must be writable");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    if (buffer.len < n) {
        PyErr_SetString(PyExc_ValueError, "buffer must be at least 'n' bytes");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    Py_ssize_t res = PyLong_AsNativeBytes(v, buffer.buf, n, (int)flags);
    PyBuffer_Release(&buffer);
    return res >= 0 ? PyLong_FromSsize_t(res) : NULL;
}


static PyObject *
pylong_fromnativebytes(PyObject *module, PyObject *args)
{
    Py_buffer buffer;
    Py_ssize_t n, flags, signed_;
    if (!PyArg_ParseTuple(args, "y*nnn", &buffer, &n, &flags, &signed_)) {
        return NULL;
    }
    if (buffer.len < n) {
        PyErr_SetString(PyExc_ValueError, "buffer must be at least 'n' bytes");
        PyBuffer_Release(&buffer);
        return NULL;
    }
    PyObject *res = signed_
        ? PyLong_FromNativeBytes(buffer.buf, n, (int)flags)
        : PyLong_FromUnsignedNativeBytes(buffer.buf, n, (int)flags);
    PyBuffer_Release(&buffer);
    return res;
}


static PyObject *
pylong_getsign(PyObject *module, PyObject *arg)
{
    int sign;
    NULLABLE(arg);
    if (PyLong_GetSign(arg, &sign) == -1) {
        return NULL;
    }
    return PyLong_FromLong(sign);
}


static PyObject *
pylong_ispositive(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    RETURN_INT(PyLong_IsPositive(arg));
}


static PyObject *
pylong_isnegative(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    RETURN_INT(PyLong_IsNegative(arg));
}


static PyObject *
pylong_iszero(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    RETURN_INT(PyLong_IsZero(arg));
}


static PyObject *
pylong_aspid(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    pid_t value = PyLong_AsPid(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromPid(value);
}


static PyObject *
layout_to_dict(const PyLongLayout *layout)
{
    return Py_BuildValue("{sisisisi}",
        "bits_per_digit", (int)layout->bits_per_digit,
        "digit_size", (int)layout->digit_size,
        "digits_order", (int)layout->digits_order,
        "digit_endianness", (int)layout->digit_endianness);
}


static PyObject *
pylong_export(PyObject *module, PyObject *obj)
{
    PyLongExport export_long;
    if (PyLong_Export(obj, &export_long) < 0) {
        return NULL;
    }

    if (export_long.digits == NULL) {
        assert(export_long.negative == 0);
        assert(export_long.ndigits == 0);
        assert(export_long.digits == NULL);
        PyObject *res = PyLong_FromInt64(export_long.value);
        PyLong_FreeExport(&export_long);
        return res;
    }

    assert(PyLong_GetNativeLayout()->digit_size == sizeof(digit));
    const digit *export_long_digits = export_long.digits;

    PyObject *digits = PyList_New(0);
    if (digits == NULL) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < export_long.ndigits; i++) {
        PyObject *item = PyLong_FromUnsignedLong(export_long_digits[i]);
        if (item == NULL) {
            goto error;
        }

        if (PyList_Append(digits, item) < 0) {
            Py_DECREF(item);
            goto error;
        }
        Py_DECREF(item);
    }

    assert(export_long.value == 0);
    PyObject *res = Py_BuildValue("(iN)", export_long.negative, digits);

    PyLong_FreeExport(&export_long);
    assert(export_long._reserved == 0);

    return res;

error:
    Py_XDECREF(digits);
    PyLong_FreeExport(&export_long);
    return NULL;
}


static PyObject *
pylongwriter_create(PyObject *module, PyObject *args)
{
    int negative;
    PyObject *list;
    // TODO(vstinner): write test for negative ndigits and digits==NULL
    if (!PyArg_ParseTuple(args, "iO!", &negative, &PyList_Type, &list)) {
        return NULL;
    }
    Py_ssize_t ndigits = PyList_GET_SIZE(list);

    digit *digits = PyMem_Malloc((size_t)ndigits * sizeof(digit));
    if (digits == NULL) {
        return PyErr_NoMemory();
    }

    for (Py_ssize_t i = 0; i < ndigits; i++) {
        PyObject *item = PyList_GET_ITEM(list, i);

        long num = PyLong_AsLong(item);
        if (num == -1 && PyErr_Occurred()) {
            goto error;
        }

        if (num < 0 || num >= PyLong_BASE) {
            PyErr_SetString(PyExc_ValueError, "digit doesn't fit into digit");
            goto error;
        }
        digits[i] = (digit)num;
    }

    void *writer_digits;
    PyLongWriter *writer = PyLongWriter_Create(negative, ndigits,
                                               &writer_digits);
    if (writer == NULL) {
        goto error;
    }
    assert(PyLong_GetNativeLayout()->digit_size == sizeof(digit));
    memcpy(writer_digits, digits, (size_t)ndigits * sizeof(digit));
    PyObject *res = PyLongWriter_Finish(writer);
    PyMem_Free(digits);

    return res;

error:
    PyMem_Free(digits);
    return NULL;
}


static PyObject *
get_pylong_layout(PyObject *module, PyObject *Py_UNUSED(args))
{
    const PyLongLayout *layout = PyLong_GetNativeLayout();
    return layout_to_dict(layout);
}


static PyMethodDef test_methods[] = {
    _TESTCAPI_CALL_LONG_COMPACT_API_METHODDEF
    {"pylong_fromunicodeobject",    pylong_fromunicodeobject,   METH_VARARGS},
    {"pylong_asnativebytes",        pylong_asnativebytes,       METH_VARARGS},
    {"pylong_fromnativebytes",      pylong_fromnativebytes,     METH_VARARGS},
    {"pylong_getsign",              pylong_getsign,             METH_O},
    {"pylong_aspid",                pylong_aspid,               METH_O},
    {"pylong_export",               pylong_export,              METH_O},
    {"pylongwriter_create",         pylongwriter_create,        METH_VARARGS},
    {"get_pylong_layout",           get_pylong_layout,          METH_NOARGS},
    {"pylong_ispositive",           pylong_ispositive,          METH_O},
    {"pylong_isnegative",           pylong_isnegative,          METH_O},
    {"pylong_iszero",               pylong_iszero,              METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Long(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}

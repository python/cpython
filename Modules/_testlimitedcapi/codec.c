#include "pyconfig.h"   // Py_GIL_DISABLED
#ifdef Py_GIL_DISABLED
#  define Py_TARGET_ABI3T 0x030f0000
#else
   // Need limited C API version 3.13 for PyMem_RawFree()
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"

static PyObject *
codec_namereplace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_NameReplaceErrors(exc);
}


// Test Py_DecodeLocale()
static PyObject *
decode_locale(PyObject *Py_UNUSED(module), PyObject *arg)
{
    const char *str;
    Py_ssize_t unused_len;
    // Accept embedded null bytes
    if (PyArg_Parse(arg, "y#", &str, &unused_len) < 0) {
        return NULL;
    }

    const size_t size_canary = (size_t)-123;
    size_t size = size_canary;
    wchar_t *wstr = Py_DecodeLocale(str, &size);

    if (wstr == NULL) {
        if (size == (size_t)-1) {
            PyErr_NoMemory();
        }
        else if (size == (size_t)-2) {
            PyErr_SetString(PyExc_RuntimeError, "decode error");
        }
        else {
            PyErr_Format(PyExc_SystemError,
                         "unknown Py_DecodeLocale() return value: %zd",
                         (Py_ssize_t)size);
        }
        return NULL;
    }
    assert(wstr != NULL);
    assert(size != size_canary);

    PyObject *result = PyUnicode_FromWideChar(wstr, size);
    PyMem_RawFree(wstr);
    return result;
}


// Test Py_EncodeLocale()
static PyObject *
encode_locale(PyObject *Py_UNUSED(module), PyObject *arg)
{
    PyObject *unicode;
    if (PyArg_Parse(arg, "U", &unicode) < 0) {
        return NULL;
    }

    // Accept embedded null characters
    Py_ssize_t unused_wlen;
    wchar_t *wstr = PyUnicode_AsWideCharString(unicode, &unused_wlen);
    if (wstr == NULL) {
        return NULL;
    }

    const size_t error_pos_canary = (size_t)-123;
    size_t error_pos = error_pos_canary;
    char *str = Py_EncodeLocale(wstr, &error_pos);
    PyMem_Free(wstr);

    if (str == NULL) {
        if (error_pos == (size_t)-1) {
            return PyErr_NoMemory();
        }
        else {
            assert(error_pos != error_pos_canary);
            return PyErr_Format(PyExc_RuntimeError,
                                "encode error: pos=%zd", error_pos);
        }
    }
    assert(error_pos == error_pos_canary);

    PyObject *result = PyBytes_FromString(str);
    PyMem_Free(str);
    return result;
}


static PyMethodDef test_methods[] = {
    {"codec_namereplace_errors", codec_namereplace_errors, METH_O},
    {"decode_locale", decode_locale, METH_O},
    {"encode_locale", encode_locale, METH_O},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Codec(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}

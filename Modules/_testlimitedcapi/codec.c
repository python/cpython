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
    if (PyArg_Parse(arg, "y", &str) < 0) {
        return NULL;
    }

    size_t wstr_len = (size_t)-123;
    wchar_t *wstr = Py_DecodeLocale(str, &wstr_len);

    if (str == NULL) {
        if (wstr_len == (size_t)-1) {
            PyErr_NoMemory();
        }
        else if (wstr_len == (size_t)-2) {
            PyErr_SetString(PyExc_ValueError, "decode error");
        }
        else {
            PyErr_Format(PyExc_SystemError,
                         "unknown Py_DecodeLocale() return value: %zd",
                         (Py_ssize_t)wstr_len);
        }
        return NULL;
    }

    PyObject *result = PyUnicode_FromWideChar(wstr, wstr_len);
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

    wchar_t *wstr = PyUnicode_AsWideCharString(unicode, NULL);
    if (wstr == NULL) {
        return NULL;
    }

    const size_t error_pos_canary = (size_t)-123;
    size_t error_pos = error_pos_canary;
    char *str = Py_EncodeLocale(wstr, &error_pos);
    PyMem_Free(wstr);

    if (str == NULL) {
        assert(error_pos != error_pos_canary);
        return PyErr_Format(PyExc_ValueError,
                            "Py_EncodeLocale failed: error_pos=%zd",
                            error_pos);
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

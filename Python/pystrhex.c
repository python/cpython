/* bytes to hex implementation */

#include "Python.h"

#include "pystrhex.h"

static PyObject *_Py_strhex_impl(const char* argbuf, const Py_ssize_t arglen,
                                 const PyObject* sep, int bytes_per_sep_group,
                                 const int return_bytes)
{
    assert(arglen >= 0);

    Py_UCS1 sep_char = 0;
    if (sep) {
        Py_ssize_t seplen = PyObject_Length((PyObject*)sep);
        if (seplen < 0) {
            return NULL;
        }
        if (seplen != 1) {
            PyErr_SetString(PyExc_ValueError, "sep must be length 1.");
            return NULL;
        }
        if (PyUnicode_Check(sep)) {
            if (PyUnicode_READY(sep))
                return NULL;
            if (PyUnicode_KIND(sep) != PyUnicode_1BYTE_KIND) {
                PyErr_SetString(PyExc_ValueError, "sep must be ASCII.");
                return NULL;
            }
            sep_char = PyUnicode_READ_CHAR(sep, 0);
        }
        else if (PyBytes_Check(sep)) {
            sep_char = PyBytes_AS_STRING(sep)[0];
        }
        else {
            PyErr_SetString(PyExc_TypeError, "sep must be str or bytes.");
            return NULL;
        }
        if (sep_char > 127 && !return_bytes) {
            PyErr_SetString(PyExc_ValueError, "sep must be ASCII.");
            return NULL;
        }
    }
    else {
        bytes_per_sep_group = 0;
    }

    unsigned int abs_bytes_per_sep = abs(bytes_per_sep_group);
    Py_ssize_t resultlen = 0;
    if (bytes_per_sep_group && arglen > 0) {
        /* How many sep characters we'll be inserting. */
        resultlen = (arglen - 1) / abs_bytes_per_sep;
    }
    /* Bounds checking for our Py_ssize_t indices. */
    if (arglen >= PY_SSIZE_T_MAX / 2 - resultlen) {
        return PyErr_NoMemory();
    }
    resultlen += arglen * 2;

    if ((size_t)abs_bytes_per_sep >= (size_t)arglen) {
        bytes_per_sep_group = 0;
        abs_bytes_per_sep = 0;
    }

    PyObject *retval;
    Py_UCS1 *retbuf;
    if (return_bytes) {
        /* If _PyBytes_FromSize() were public we could avoid malloc+copy. */
        retval = PyBytes_FromStringAndSize(NULL, resultlen);
        if (!retval) {
            return NULL;
        }
        retbuf = (Py_UCS1 *)PyBytes_AS_STRING(retval);
    }
    else {
        retval = PyUnicode_New(resultlen, 127);
        if (!retval) {
            return NULL;
        }
        retbuf = PyUnicode_1BYTE_DATA(retval);
    }

    /* Hexlify */
    Py_ssize_t i, j;
    unsigned char c;

    if (bytes_per_sep_group == 0) {
        for (i = j = 0; i < arglen; ++i) {
            assert((j + 1) < resultlen);
            c = argbuf[i];
            retbuf[j++] = Py_hexdigits[c >> 4];
            retbuf[j++] = Py_hexdigits[c & 0x0f];
        }
        assert(j == resultlen);
    }
    else {
        /* The number of complete chunk+sep periods */
        Py_ssize_t chunks = (arglen - 1) / abs_bytes_per_sep;
        Py_ssize_t chunk;
        unsigned int k;

        if (bytes_per_sep_group < 0) {
            i = j = 0;
            for (chunk = 0; chunk < chunks; chunk++) {
                for (k = 0; k < abs_bytes_per_sep; k++) {
                    c = argbuf[i++];
                    retbuf[j++] = Py_hexdigits[c >> 4];
                    retbuf[j++] = Py_hexdigits[c & 0x0f];
                }
                retbuf[j++] = sep_char;
            }
            while (i < arglen) {
                c = argbuf[i++];
                retbuf[j++] = Py_hexdigits[c >> 4];
                retbuf[j++] = Py_hexdigits[c & 0x0f];
            }
            assert(j == resultlen);
        }
        else {
            i = arglen - 1;
            j = resultlen - 1;
            for (chunk = 0; chunk < chunks; chunk++) {
                for (k = 0; k < abs_bytes_per_sep; k++) {
                    c = argbuf[i--];
                    retbuf[j--] = Py_hexdigits[c & 0x0f];
                    retbuf[j--] = Py_hexdigits[c >> 4];
                }
                retbuf[j--] = sep_char;
            }
            while (i >= 0) {
                c = argbuf[i--];
                retbuf[j--] = Py_hexdigits[c & 0x0f];
                retbuf[j--] = Py_hexdigits[c >> 4];
            }
            assert(j == -1);
        }
    }

#ifdef Py_DEBUG
    if (!return_bytes) {
        assert(_PyUnicode_CheckConsistency(retval, 1));
    }
#endif

    return retval;
}

PyObject * _Py_strhex(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, NULL, 0, 0);
}

/* Same as above but returns a bytes() instead of str() to avoid the
 * need to decode the str() when bytes are needed. */
PyObject * _Py_strhex_bytes(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, NULL, 0, 1);
}

/* These variants include support for a separator between every N bytes: */

PyObject * _Py_strhex_with_sep(const char* argbuf, const Py_ssize_t arglen, const PyObject* sep, const int bytes_per_group)
{
    return _Py_strhex_impl(argbuf, arglen, sep, bytes_per_group, 0);
}

/* Same as above but returns a bytes() instead of str() to avoid the
 * need to decode the str() when bytes are needed. */
PyObject * _Py_strhex_bytes_with_sep(const char* argbuf, const Py_ssize_t arglen, const PyObject* sep, const int bytes_per_group)
{
    return _Py_strhex_impl(argbuf, arglen, sep, bytes_per_group, 1);
}

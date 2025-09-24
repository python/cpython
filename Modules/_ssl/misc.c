#include "Python.h"
#include "../_ssl.h"

#include "openssl/bio.h"

/* BIO_s_mem() to PyBytes
 */
static PyObject *
_PySSL_BytesFromBIO(_sslmodulestate *state, BIO *bio)
{
    long size;
    char *data = NULL;
    size = BIO_get_mem_data(bio, &data);
    if (data == NULL || size < 0) {
        PyErr_SetString(PyExc_ValueError, "Not a memory BIO");
        return NULL;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(size);
    if (writer == NULL) {
        return NULL;
    }
    char *str = PyBytesWriter_GetData(writer);
    memcpy(str, data, size);
    return PyBytesWriter_Finish(writer);
}

/* BIO_s_mem() to PyUnicode
 */
static PyObject *
_PySSL_UnicodeFromBIO(_sslmodulestate *state, BIO *bio, const char *error)
{
    long size;
    char *data = NULL;
    size = BIO_get_mem_data(bio, &data);
    if (data == NULL || size < 0) {
        PyErr_SetString(PyExc_ValueError, "Not a memory BIO");
        return NULL;
    }
    return PyUnicode_DecodeUTF8(data, size, error);
}

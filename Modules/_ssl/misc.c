#include "Python.h"
#include "../_ssl.h"

#include "openssl/bio.h"

/* Create BIO from object(converter="PyUnicode_FSConverter") */
static BIO *
_PySSL_filebio(_sslmodulestate *state, PyObject *path)
{
    BIO *bio = NULL;
    int result;

    if ((bio = BIO_new(BIO_s_file())) == NULL) {
        _setSSLError(state, "Can't allocate BIO", 0, __FILE__, __LINE__);
        return NULL;
    }
    PySSL_BEGIN_ALLOW_THREADS
    result = BIO_read_filename(bio, PyBytes_AsString(path));
    PySSL_END_ALLOW_THREADS

    if (result <= 0) {
        _setSSLError(state, "Cannot read file", 0, __FILE__, __LINE__);
        BIO_free(bio);
        return NULL;
    }
    return bio;
}

/* Create BIO from Py_buffer
 *
 * NOTE: The BIO uses the buffer directly. You must BIO_free(bio) first,
 *       then PyBuffer_Release(&b)!
 */
static BIO *
_PySSL_bufferbio(_sslmodulestate *state, Py_buffer *b)
{
    BIO *bio = NULL;

    if (b->len > INT_MAX) {
        PyErr_Format(PyExc_OverflowError,
                     "buffer longer than %d bytes", INT_MAX);
        return NULL;
    }

    bio = BIO_new_mem_buf(b->buf, (int)b->len);
    if (bio == NULL) {
        _setSSLError(state, "Cannot allocate BIO", 0, __FILE__, __LINE__);
        return NULL;
    }
    return bio;
}

/* BIO_s_mem() to PyBytes
 */
static PyObject *
_PySSL_BytesFromBIO(_sslmodulestate *state, BIO *bio)
{
    long size;
    char *data = NULL;
    size = BIO_get_mem_data(bio, &data);
    /* Only memory BIO set pointer for BIO_CTRL_INFO */
    if (data == NULL || size < 0) {
        PyErr_SetString(PyExc_ValueError, "Not a memory BIO");
        return NULL;
    }
    return PyBytes_FromStringAndSize(data, size);
}

/* BIO_s_mem() to PyUnicode
 */
static PyObject *
_PySSL_UnicodeFromBIO(_sslmodulestate *state, BIO *bio, const char *error)
{
    long size;
    char *data = NULL;
    size = BIO_get_mem_data(bio, &data);
    /* Only memory BIO set pointer for BIO_CTRL_INFO */
    if (data == NULL || size < 0) {
        PyErr_SetString(PyExc_ValueError, "Not a memory BIO");
        return NULL;
    }
    return PyUnicode_DecodeUTF8(data, size, error);
}

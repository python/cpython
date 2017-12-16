/*[clinic input]
preserve
[clinic start generated code]*/

#if (OPENSSL_VERSION_NUMBER > 0x10100000L && !defined(OPENSSL_NO_SCRYPT) && !defined(LIBRESSL_VERSION_NUMBER))

PyDoc_STRVAR(_hashlib_scrypt__doc__,
"scrypt($module, /, password, *, salt=None, n=None, r=None, p=None,\n"
"       maxmem=0, dklen=64)\n"
"--\n"
"\n"
"scrypt password-based key derivation function.");

#define _HASHLIB_SCRYPT_METHODDEF    \
    {"scrypt", (PyCFunction)_hashlib_scrypt, METH_FASTCALL|METH_KEYWORDS, _hashlib_scrypt__doc__},

static PyObject *
_hashlib_scrypt_impl(PyObject *module, Py_buffer *password, Py_buffer *salt,
                     PyObject *n_obj, PyObject *r_obj, PyObject *p_obj,
                     long maxmem, long dklen);

static PyObject *
_hashlib_scrypt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"password", "salt", "n", "r", "p", "maxmem", "dklen", NULL};
    static _PyArg_Parser _parser = {"y*|$y*O!O!O!ll:scrypt", _keywords, 0};
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    PyObject *n_obj = Py_None;
    PyObject *r_obj = Py_None;
    PyObject *p_obj = Py_None;
    long maxmem = 0;
    long dklen = 64;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &password, &salt, &PyLong_Type, &n_obj, &PyLong_Type, &r_obj, &PyLong_Type, &p_obj, &maxmem, &dklen)) {
        goto exit;
    }
    return_value = _hashlib_scrypt_impl(module, &password, &salt, n_obj, r_obj, p_obj, maxmem, dklen);

exit:
    /* Cleanup for password */
    if (password.obj) {
       PyBuffer_Release(&password);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }

    return return_value;
}

#endif /* (OPENSSL_VERSION_NUMBER > 0x10100000L && !defined(OPENSSL_NO_SCRYPT) && !defined(LIBRESSL_VERSION_NUMBER)) */

#ifndef _HASHLIB_SCRYPT_METHODDEF
    #define _HASHLIB_SCRYPT_METHODDEF
#endif /* !defined(_HASHLIB_SCRYPT_METHODDEF) */
/*[clinic end generated code: output=1ea7d0397f38e2c2 input=a9049054013a1b77]*/

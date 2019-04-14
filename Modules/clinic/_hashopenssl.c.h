/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(EVP_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define EVP_COPY_METHODDEF    \
    {"copy", (PyCFunction)EVP_copy, METH_NOARGS, EVP_copy__doc__},

static PyObject *
EVP_copy_impl(EVPobject *self);

static PyObject *
EVP_copy(EVPobject *self, PyObject *Py_UNUSED(ignored))
{
    return EVP_copy_impl(self);
}

PyDoc_STRVAR(EVP_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define EVP_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)EVP_digest, METH_NOARGS, EVP_digest__doc__},

static PyObject *
EVP_digest_impl(EVPobject *self);

static PyObject *
EVP_digest(EVPobject *self, PyObject *Py_UNUSED(ignored))
{
    return EVP_digest_impl(self);
}

PyDoc_STRVAR(EVP_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define EVP_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)EVP_hexdigest, METH_NOARGS, EVP_hexdigest__doc__},

static PyObject *
EVP_hexdigest_impl(EVPobject *self);

static PyObject *
EVP_hexdigest(EVPobject *self, PyObject *Py_UNUSED(ignored))
{
    return EVP_hexdigest_impl(self);
}

PyDoc_STRVAR(EVP_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define EVP_UPDATE_METHODDEF    \
    {"update", (PyCFunction)EVP_update, METH_O, EVP_update__doc__},

PyDoc_STRVAR(EVP_new__doc__,
"new($module, /, name, string=b\'\')\n"
"--\n"
"\n"
"Return a new hash object using the named algorithm.\n"
"\n"
"An optional string argument may be provided and will be\n"
"automatically hashed.\n"
"\n"
"The MD5 and SHA1 algorithms are always supported.");

#define EVP_NEW_METHODDEF    \
    {"new", (PyCFunction)(void(*)(void))EVP_new, METH_FASTCALL|METH_KEYWORDS, EVP_new__doc__},

static PyObject *
EVP_new_impl(PyObject *module, PyObject *name_obj, PyObject *data_obj);

static PyObject *
EVP_new(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", "string", NULL};
    static _PyArg_Parser _parser = {"O|O:new", _keywords, 0};
    PyObject *name_obj;
    PyObject *data_obj = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &name_obj, &data_obj)) {
        goto exit;
    }
    return_value = EVP_new_impl(module, name_obj, data_obj);

exit:
    return return_value;
}

#if ((OPENSSL_VERSION_NUMBER >= 0x10000000 && !defined(OPENSSL_NO_HMAC) && !defined(OPENSSL_NO_SHA)))

PyDoc_STRVAR(pbkdf2_hmac__doc__,
"pbkdf2_hmac($module, /, hash_name, password, salt, iterations,\n"
"            dklen=None)\n"
"--\n"
"\n"
"Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as pseudorandom function.");

#define PBKDF2_HMAC_METHODDEF    \
    {"pbkdf2_hmac", (PyCFunction)(void(*)(void))pbkdf2_hmac, METH_FASTCALL|METH_KEYWORDS, pbkdf2_hmac__doc__},

static PyObject *
pbkdf2_hmac_impl(PyObject *module, const char *hash_name,
                 Py_buffer *password, Py_buffer *salt, long iterations,
                 PyObject *dklen_obj);

static PyObject *
pbkdf2_hmac(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"hash_name", "password", "salt", "iterations", "dklen", NULL};
    static _PyArg_Parser _parser = {"sy*y*l|O:pbkdf2_hmac", _keywords, 0};
    const char *hash_name;
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    long iterations;
    PyObject *dklen_obj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &hash_name, &password, &salt, &iterations, &dklen_obj)) {
        goto exit;
    }
    return_value = pbkdf2_hmac_impl(module, hash_name, &password, &salt, iterations, dklen_obj);

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

#endif /* ((OPENSSL_VERSION_NUMBER >= 0x10000000 && !defined(OPENSSL_NO_HMAC) && !defined(OPENSSL_NO_SHA))) */

#if (OPENSSL_VERSION_NUMBER > 0x10100000L && !defined(OPENSSL_NO_SCRYPT) && !defined(LIBRESSL_VERSION_NUMBER))

PyDoc_STRVAR(_hashlib_scrypt__doc__,
"scrypt($module, /, password, *, salt=None, n=None, r=None, p=None,\n"
"       maxmem=0, dklen=64)\n"
"--\n"
"\n"
"scrypt password-based key derivation function.");

#define _HASHLIB_SCRYPT_METHODDEF    \
    {"scrypt", (PyCFunction)(void(*)(void))_hashlib_scrypt, METH_FASTCALL|METH_KEYWORDS, _hashlib_scrypt__doc__},

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

PyDoc_STRVAR(_hashlib_hmac_digest__doc__,
"hmac_digest($module, /, key, msg, digest)\n"
"--\n"
"\n"
"Single-shot HMAC.");

#define _HASHLIB_HMAC_DIGEST_METHODDEF    \
    {"hmac_digest", (PyCFunction)(void(*)(void))_hashlib_hmac_digest, METH_FASTCALL|METH_KEYWORDS, _hashlib_hmac_digest__doc__},

static PyObject *
_hashlib_hmac_digest_impl(PyObject *module, Py_buffer *key, Py_buffer *msg,
                          const char *digest);

static PyObject *
_hashlib_hmac_digest(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "msg", "digest", NULL};
    static _PyArg_Parser _parser = {"y*y*s:hmac_digest", _keywords, 0};
    Py_buffer key = {NULL, NULL};
    Py_buffer msg = {NULL, NULL};
    const char *digest;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &key, &msg, &digest)) {
        goto exit;
    }
    return_value = _hashlib_hmac_digest_impl(module, &key, &msg, digest);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for msg */
    if (msg.obj) {
       PyBuffer_Release(&msg);
    }

    return return_value;
}

#ifndef PBKDF2_HMAC_METHODDEF
    #define PBKDF2_HMAC_METHODDEF
#endif /* !defined(PBKDF2_HMAC_METHODDEF) */

#ifndef _HASHLIB_SCRYPT_METHODDEF
    #define _HASHLIB_SCRYPT_METHODDEF
#endif /* !defined(_HASHLIB_SCRYPT_METHODDEF) */
/*[clinic end generated code: output=fe5931d2b301ca12 input=a9049054013a1b77]*/

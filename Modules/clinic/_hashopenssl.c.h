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

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(EVPXOF_digest__doc__,
"digest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define EVPXOF_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)(void(*)(void))EVPXOF_digest, METH_FASTCALL|METH_KEYWORDS, EVPXOF_digest__doc__},

static PyObject *
EVPXOF_digest_impl(EVPobject *self, Py_ssize_t length);

static PyObject *
EVPXOF_digest(EVPobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "digest", 0};
    PyObject *argsbuf[1];
    Py_ssize_t length;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = EVPXOF_digest_impl(self, length);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(EVPXOF_hexdigest__doc__,
"hexdigest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define EVPXOF_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)(void(*)(void))EVPXOF_hexdigest, METH_FASTCALL|METH_KEYWORDS, EVPXOF_hexdigest__doc__},

static PyObject *
EVPXOF_hexdigest_impl(EVPobject *self, Py_ssize_t length);

static PyObject *
EVPXOF_hexdigest(EVPobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hexdigest", 0};
    PyObject *argsbuf[1];
    Py_ssize_t length;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = EVPXOF_hexdigest_impl(self, length);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

PyDoc_STRVAR(EVP_new__doc__,
"new($module, /, name, string=b\'\', *, usedforsecurity=True)\n"
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
EVP_new_impl(PyObject *module, PyObject *name_obj, PyObject *data_obj,
             int usedforsecurity);

static PyObject *
EVP_new(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", "string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "new", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name_obj;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name_obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        data_obj = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[2]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = EVP_new_impl(module, name_obj, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_md5__doc__,
"openssl_md5($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a md5 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_MD5_METHODDEF    \
    {"openssl_md5", (PyCFunction)(void(*)(void))_hashlib_openssl_md5, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_md5__doc__},

static PyObject *
_hashlib_openssl_md5_impl(PyObject *module, PyObject *data_obj,
                          int usedforsecurity);

static PyObject *
_hashlib_openssl_md5(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_md5", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_md5_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha1__doc__,
"openssl_sha1($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha1 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA1_METHODDEF    \
    {"openssl_sha1", (PyCFunction)(void(*)(void))_hashlib_openssl_sha1, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha1__doc__},

static PyObject *
_hashlib_openssl_sha1_impl(PyObject *module, PyObject *data_obj,
                           int usedforsecurity);

static PyObject *
_hashlib_openssl_sha1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha1", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha1_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha224__doc__,
"openssl_sha224($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha224 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA224_METHODDEF    \
    {"openssl_sha224", (PyCFunction)(void(*)(void))_hashlib_openssl_sha224, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha224__doc__},

static PyObject *
_hashlib_openssl_sha224_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity);

static PyObject *
_hashlib_openssl_sha224(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha224", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha224_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha256__doc__,
"openssl_sha256($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha256 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA256_METHODDEF    \
    {"openssl_sha256", (PyCFunction)(void(*)(void))_hashlib_openssl_sha256, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha256__doc__},

static PyObject *
_hashlib_openssl_sha256_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity);

static PyObject *
_hashlib_openssl_sha256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha256", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha256_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha384__doc__,
"openssl_sha384($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha384 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA384_METHODDEF    \
    {"openssl_sha384", (PyCFunction)(void(*)(void))_hashlib_openssl_sha384, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha384__doc__},

static PyObject *
_hashlib_openssl_sha384_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity);

static PyObject *
_hashlib_openssl_sha384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha384", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha384_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha512__doc__,
"openssl_sha512($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha512 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA512_METHODDEF    \
    {"openssl_sha512", (PyCFunction)(void(*)(void))_hashlib_openssl_sha512, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha512__doc__},

static PyObject *
_hashlib_openssl_sha512_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity);

static PyObject *
_hashlib_openssl_sha512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha512", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha512_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_224__doc__,
"openssl_sha3_224($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha3-224 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_224_METHODDEF    \
    {"openssl_sha3_224", (PyCFunction)(void(*)(void))_hashlib_openssl_sha3_224, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha3_224__doc__},

static PyObject *
_hashlib_openssl_sha3_224_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity);

static PyObject *
_hashlib_openssl_sha3_224(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha3_224", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha3_224_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_256__doc__,
"openssl_sha3_256($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha3-256 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_256_METHODDEF    \
    {"openssl_sha3_256", (PyCFunction)(void(*)(void))_hashlib_openssl_sha3_256, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha3_256__doc__},

static PyObject *
_hashlib_openssl_sha3_256_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity);

static PyObject *
_hashlib_openssl_sha3_256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha3_256", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha3_256_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_384__doc__,
"openssl_sha3_384($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha3-384 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_384_METHODDEF    \
    {"openssl_sha3_384", (PyCFunction)(void(*)(void))_hashlib_openssl_sha3_384, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha3_384__doc__},

static PyObject *
_hashlib_openssl_sha3_384_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity);

static PyObject *
_hashlib_openssl_sha3_384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha3_384", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha3_384_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_512__doc__,
"openssl_sha3_512($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a sha3-512 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_512_METHODDEF    \
    {"openssl_sha3_512", (PyCFunction)(void(*)(void))_hashlib_openssl_sha3_512, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_sha3_512__doc__},

static PyObject *
_hashlib_openssl_sha3_512_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity);

static PyObject *
_hashlib_openssl_sha3_512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_sha3_512", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_sha3_512_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_openssl_shake_128__doc__,
"openssl_shake_128($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a shake-128 variable hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHAKE_128_METHODDEF    \
    {"openssl_shake_128", (PyCFunction)(void(*)(void))_hashlib_openssl_shake_128, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_shake_128__doc__},

static PyObject *
_hashlib_openssl_shake_128_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity);

static PyObject *
_hashlib_openssl_shake_128(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_shake_128", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_shake_128_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_openssl_shake_256__doc__,
"openssl_shake_256($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Returns a shake-256 variable hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHAKE_256_METHODDEF    \
    {"openssl_shake_256", (PyCFunction)(void(*)(void))_hashlib_openssl_shake_256, METH_FASTCALL|METH_KEYWORDS, _hashlib_openssl_shake_256__doc__},

static PyObject *
_hashlib_openssl_shake_256_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity);

static PyObject *
_hashlib_openssl_shake_256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "openssl_shake_256", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _hashlib_openssl_shake_256_impl(module, data_obj, usedforsecurity);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

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
    static _PyArg_Parser _parser = {NULL, _keywords, "pbkdf2_hmac", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 4;
    const char *hash_name;
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    long iterations;
    PyObject *dklen_obj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 5, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("pbkdf2_hmac", "argument 'hash_name'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t hash_name_length;
    hash_name = PyUnicode_AsUTF8AndSize(args[0], &hash_name_length);
    if (hash_name == NULL) {
        goto exit;
    }
    if (strlen(hash_name) != (size_t)hash_name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &password, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&password, 'C')) {
        _PyArg_BadArgument("pbkdf2_hmac", "argument 'password'", "contiguous buffer", args[1]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[2], &salt, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&salt, 'C')) {
        _PyArg_BadArgument("pbkdf2_hmac", "argument 'salt'", "contiguous buffer", args[2]);
        goto exit;
    }
    if (PyFloat_Check(args[3])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    iterations = PyLong_AsLong(args[3]);
    if (iterations == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    dklen_obj = args[4];
skip_optional_pos:
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
    static _PyArg_Parser _parser = {NULL, _keywords, "scrypt", 0};
    PyObject *argsbuf[7];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    PyObject *n_obj = Py_None;
    PyObject *r_obj = Py_None;
    PyObject *p_obj = Py_None;
    long maxmem = 0;
    long dklen = 64;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &password, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&password, 'C')) {
        _PyArg_BadArgument("scrypt", "argument 'password'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        if (PyObject_GetBuffer(args[1], &salt, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&salt, 'C')) {
            _PyArg_BadArgument("scrypt", "argument 'salt'", "contiguous buffer", args[1]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (!PyLong_Check(args[2])) {
            _PyArg_BadArgument("scrypt", "argument 'n'", "int", args[2]);
            goto exit;
        }
        n_obj = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!PyLong_Check(args[3])) {
            _PyArg_BadArgument("scrypt", "argument 'r'", "int", args[3]);
            goto exit;
        }
        r_obj = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        if (!PyLong_Check(args[4])) {
            _PyArg_BadArgument("scrypt", "argument 'p'", "int", args[4]);
            goto exit;
        }
        p_obj = args[4];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        if (PyFloat_Check(args[5])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        maxmem = PyLong_AsLong(args[5]);
        if (maxmem == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (PyFloat_Check(args[6])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    dklen = PyLong_AsLong(args[6]);
    if (dklen == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
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

PyDoc_STRVAR(_hashlib_hmac_singleshot__doc__,
"hmac_digest($module, /, key, msg, digest)\n"
"--\n"
"\n"
"Single-shot HMAC.");

#define _HASHLIB_HMAC_SINGLESHOT_METHODDEF    \
    {"hmac_digest", (PyCFunction)(void(*)(void))_hashlib_hmac_singleshot, METH_FASTCALL|METH_KEYWORDS, _hashlib_hmac_singleshot__doc__},

static PyObject *
_hashlib_hmac_singleshot_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg, const char *digest);

static PyObject *
_hashlib_hmac_singleshot(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "msg", "digest", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hmac_digest", 0};
    PyObject *argsbuf[3];
    Py_buffer key = {NULL, NULL};
    Py_buffer msg = {NULL, NULL};
    const char *digest;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&key, 'C')) {
        _PyArg_BadArgument("hmac_digest", "argument 'key'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &msg, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&msg, 'C')) {
        _PyArg_BadArgument("hmac_digest", "argument 'msg'", "contiguous buffer", args[1]);
        goto exit;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("hmac_digest", "argument 'digest'", "str", args[2]);
        goto exit;
    }
    Py_ssize_t digest_length;
    digest = PyUnicode_AsUTF8AndSize(args[2], &digest_length);
    if (digest == NULL) {
        goto exit;
    }
    if (strlen(digest) != (size_t)digest_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _hashlib_hmac_singleshot_impl(module, &key, &msg, digest);

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

PyDoc_STRVAR(_hashlib_hmac_new__doc__,
"hmac_new($module, /, key, msg=b\'\', digestmod=None)\n"
"--\n"
"\n"
"Return a new hmac object.");

#define _HASHLIB_HMAC_NEW_METHODDEF    \
    {"hmac_new", (PyCFunction)(void(*)(void))_hashlib_hmac_new, METH_FASTCALL|METH_KEYWORDS, _hashlib_hmac_new__doc__},

static PyObject *
_hashlib_hmac_new_impl(PyObject *module, Py_buffer *key, PyObject *msg_obj,
                       const char *digestmod);

static PyObject *
_hashlib_hmac_new(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "msg", "digestmod", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hmac_new", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer key = {NULL, NULL};
    PyObject *msg_obj = NULL;
    const char *digestmod = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&key, 'C')) {
        _PyArg_BadArgument("hmac_new", "argument 'key'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        msg_obj = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("hmac_new", "argument 'digestmod'", "str", args[2]);
        goto exit;
    }
    Py_ssize_t digestmod_length;
    digestmod = PyUnicode_AsUTF8AndSize(args[2], &digestmod_length);
    if (digestmod == NULL) {
        goto exit;
    }
    if (strlen(digestmod) != (size_t)digestmod_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = _hashlib_hmac_new_impl(module, &key, msg_obj, digestmod);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }

    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy (\"clone\") of the HMAC object.");

#define _HASHLIB_HMAC_COPY_METHODDEF    \
    {"copy", (PyCFunction)_hashlib_HMAC_copy, METH_NOARGS, _hashlib_HMAC_copy__doc__},

static PyObject *
_hashlib_HMAC_copy_impl(HMACobject *self);

static PyObject *
_hashlib_HMAC_copy(HMACobject *self, PyObject *Py_UNUSED(ignored))
{
    return _hashlib_HMAC_copy_impl(self);
}

PyDoc_STRVAR(_hashlib_HMAC_update__doc__,
"update($self, /, msg)\n"
"--\n"
"\n"
"Update the HMAC object with msg.");

#define _HASHLIB_HMAC_UPDATE_METHODDEF    \
    {"update", (PyCFunction)(void(*)(void))_hashlib_HMAC_update, METH_FASTCALL|METH_KEYWORDS, _hashlib_HMAC_update__doc__},

static PyObject *
_hashlib_HMAC_update_impl(HMACobject *self, PyObject *msg);

static PyObject *
_hashlib_HMAC_update(HMACobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"msg", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "update", 0};
    PyObject *argsbuf[1];
    PyObject *msg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    msg = args[0];
    return_value = _hashlib_HMAC_update_impl(self, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest of the bytes passed to the update() method so far.");

#define _HASHLIB_HMAC_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_hashlib_HMAC_digest, METH_NOARGS, _hashlib_HMAC_digest__doc__},

static PyObject *
_hashlib_HMAC_digest_impl(HMACobject *self);

static PyObject *
_hashlib_HMAC_digest(HMACobject *self, PyObject *Py_UNUSED(ignored))
{
    return _hashlib_HMAC_digest_impl(self);
}

PyDoc_STRVAR(_hashlib_HMAC_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return hexadecimal digest of the bytes passed to the update() method so far.\n"
"\n"
"This may be used to exchange the value safely in email or other non-binary\n"
"environments.");

#define _HASHLIB_HMAC_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_hashlib_HMAC_hexdigest, METH_NOARGS, _hashlib_HMAC_hexdigest__doc__},

static PyObject *
_hashlib_HMAC_hexdigest_impl(HMACobject *self);

static PyObject *
_hashlib_HMAC_hexdigest(HMACobject *self, PyObject *Py_UNUSED(ignored))
{
    return _hashlib_HMAC_hexdigest_impl(self);
}

#if !defined(LIBRESSL_VERSION_NUMBER)

PyDoc_STRVAR(_hashlib_get_fips_mode__doc__,
"get_fips_mode($module, /)\n"
"--\n"
"\n"
"Determine the OpenSSL FIPS mode of operation.\n"
"\n"
"For OpenSSL 3.0.0 and newer it returns the state of the default provider\n"
"in the default OSSL context. It\'s not quite the same as FIPS_mode() but good\n"
"enough for unittests.\n"
"\n"
"Effectively any non-zero return value indicates FIPS mode;\n"
"values other than 1 may have additional significance.");

#define _HASHLIB_GET_FIPS_MODE_METHODDEF    \
    {"get_fips_mode", (PyCFunction)_hashlib_get_fips_mode, METH_NOARGS, _hashlib_get_fips_mode__doc__},

static int
_hashlib_get_fips_mode_impl(PyObject *module);

static PyObject *
_hashlib_get_fips_mode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _hashlib_get_fips_mode_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* !defined(LIBRESSL_VERSION_NUMBER) */

PyDoc_STRVAR(_hashlib_compare_digest__doc__,
"compare_digest($module, a, b, /)\n"
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

#define _HASHLIB_COMPARE_DIGEST_METHODDEF    \
    {"compare_digest", (PyCFunction)(void(*)(void))_hashlib_compare_digest, METH_FASTCALL, _hashlib_compare_digest__doc__},

static PyObject *
_hashlib_compare_digest_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
_hashlib_compare_digest(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("compare_digest", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _hashlib_compare_digest_impl(module, a, b);

exit:
    return return_value;
}

#ifndef EVPXOF_DIGEST_METHODDEF
    #define EVPXOF_DIGEST_METHODDEF
#endif /* !defined(EVPXOF_DIGEST_METHODDEF) */

#ifndef EVPXOF_HEXDIGEST_METHODDEF
    #define EVPXOF_HEXDIGEST_METHODDEF
#endif /* !defined(EVPXOF_HEXDIGEST_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_224_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_224_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_224_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_256_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_256_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_256_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_384_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_384_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_384_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_512_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_512_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_512_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
    #define _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHAKE_128_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
    #define _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHAKE_256_METHODDEF) */

#ifndef _HASHLIB_SCRYPT_METHODDEF
    #define _HASHLIB_SCRYPT_METHODDEF
#endif /* !defined(_HASHLIB_SCRYPT_METHODDEF) */

#ifndef _HASHLIB_GET_FIPS_MODE_METHODDEF
    #define _HASHLIB_GET_FIPS_MODE_METHODDEF
#endif /* !defined(_HASHLIB_GET_FIPS_MODE_METHODDEF) */
/*[clinic end generated code: output=b6b280e46bf0b139 input=a9049054013a1b77]*/

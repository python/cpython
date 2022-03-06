/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(SHA256Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA256TYPE_COPY_METHODDEF    \
    {"copy", (PyCFunction)SHA256Type_copy, METH_NOARGS, SHA256Type_copy__doc__},

static PyObject *
SHA256Type_copy_impl(SHAobject *self);

static PyObject *
SHA256Type_copy(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA256Type_copy_impl(self);
}

PyDoc_STRVAR(SHA256Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA256TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)SHA256Type_digest, METH_NOARGS, SHA256Type_digest__doc__},

static PyObject *
SHA256Type_digest_impl(SHAobject *self);

static PyObject *
SHA256Type_digest(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA256Type_digest_impl(self);
}

PyDoc_STRVAR(SHA256Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA256TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)SHA256Type_hexdigest, METH_NOARGS, SHA256Type_hexdigest__doc__},

static PyObject *
SHA256Type_hexdigest_impl(SHAobject *self);

static PyObject *
SHA256Type_hexdigest(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA256Type_hexdigest_impl(self);
}

PyDoc_STRVAR(SHA256Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA256TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)SHA256Type_update, METH_O, SHA256Type_update__doc__},

PyDoc_STRVAR(_sha256_sha256__doc__,
"sha256($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-256 hash object; optionally initialized with a string.");

#define _SHA256_SHA256_METHODDEF    \
    {"sha256", (PyCFunction)_sha256_sha256, METH_FASTCALL|METH_KEYWORDS, _sha256_sha256__doc__},

static PyObject *
_sha256_sha256_impl(PyObject *module, PyObject *string);

static PyObject *
_sha256_sha256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", NULL};
    static _PyArg_Parser _parser = {"|O:sha256", _keywords, 0};
    PyObject *string = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &string)) {
        goto exit;
    }
    return_value = _sha256_sha256_impl(module, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha256_sha224__doc__,
"sha224($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-224 hash object; optionally initialized with a string.");

#define _SHA256_SHA224_METHODDEF    \
    {"sha224", (PyCFunction)_sha256_sha224, METH_FASTCALL|METH_KEYWORDS, _sha256_sha224__doc__},

static PyObject *
_sha256_sha224_impl(PyObject *module, PyObject *string);

static PyObject *
_sha256_sha224(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", NULL};
    static _PyArg_Parser _parser = {"|O:sha224", _keywords, 0};
    PyObject *string = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &string)) {
        goto exit;
    }
    return_value = _sha256_sha224_impl(module, string);

exit:
    return return_value;
}
/*[clinic end generated code: output=0086286cffcbc31c input=a9049054013a1b77]*/

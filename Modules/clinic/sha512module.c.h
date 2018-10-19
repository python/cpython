/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(SHA512Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA512TYPE_COPY_METHODDEF    \
    {"copy", (PyCFunction)SHA512Type_copy, METH_NOARGS, SHA512Type_copy__doc__},

static PyObject *
SHA512Type_copy_impl(SHAobject *self);

static PyObject *
SHA512Type_copy(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_copy_impl(self);
}

PyDoc_STRVAR(SHA512Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA512TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)SHA512Type_digest, METH_NOARGS, SHA512Type_digest__doc__},

static PyObject *
SHA512Type_digest_impl(SHAobject *self);

static PyObject *
SHA512Type_digest(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_digest_impl(self);
}

PyDoc_STRVAR(SHA512Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA512TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)SHA512Type_hexdigest, METH_NOARGS, SHA512Type_hexdigest__doc__},

static PyObject *
SHA512Type_hexdigest_impl(SHAobject *self);

static PyObject *
SHA512Type_hexdigest(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_hexdigest_impl(self);
}

PyDoc_STRVAR(SHA512Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA512TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)SHA512Type_update, METH_O, SHA512Type_update__doc__},

PyDoc_STRVAR(_sha512_sha512__doc__,
"sha512($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA512_SHA512_METHODDEF    \
    {"sha512", (PyCFunction)_sha512_sha512, METH_FASTCALL|METH_KEYWORDS, _sha512_sha512__doc__},

static PyObject *
_sha512_sha512_impl(PyObject *module, PyObject *string);

static PyObject *
_sha512_sha512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", NULL};
    static _PyArg_Parser _parser = {"|O:sha512", _keywords, 0};
    PyObject *string = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &string)) {
        goto exit;
    }
    return_value = _sha512_sha512_impl(module, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha512_sha384__doc__,
"sha384($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA512_SHA384_METHODDEF    \
    {"sha384", (PyCFunction)_sha512_sha384, METH_FASTCALL|METH_KEYWORDS, _sha512_sha384__doc__},

static PyObject *
_sha512_sha384_impl(PyObject *module, PyObject *string);

static PyObject *
_sha512_sha384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", NULL};
    static _PyArg_Parser _parser = {"|O:sha384", _keywords, 0};
    PyObject *string = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &string)) {
        goto exit;
    }
    return_value = _sha512_sha384_impl(module, string);

exit:
    return return_value;
}
/*[clinic end generated code: output=fcc3306fb6672222 input=a9049054013a1b77]*/

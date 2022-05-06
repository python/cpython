/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(SHA512Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA512TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(SHA512Type_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, SHA512Type_copy__doc__},

static PyObject *
SHA512Type_copy_impl(SHAobject *self, PyTypeObject *cls);

static PyObject *
SHA512Type_copy(SHAobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return SHA512Type_copy_impl(self, cls);
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
"sha512($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA512_SHA512_METHODDEF    \
    {"sha512", _PyCFunction_CAST(_sha512_sha512), METH_FASTCALL|METH_KEYWORDS, _sha512_sha512__doc__},

static PyObject *
_sha512_sha512_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha512_sha512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sha512", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
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
    return_value = _sha512_sha512_impl(module, string, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha512_sha384__doc__,
"sha384($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA512_SHA384_METHODDEF    \
    {"sha384", _PyCFunction_CAST(_sha512_sha384), METH_FASTCALL|METH_KEYWORDS, _sha512_sha384__doc__},

static PyObject *
_sha512_sha384_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha512_sha384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "sha384", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
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
    return_value = _sha512_sha384_impl(module, string, usedforsecurity);

exit:
    return return_value;
}
/*[clinic end generated code: output=60a0a1a28c07f391 input=a9049054013a1b77]*/

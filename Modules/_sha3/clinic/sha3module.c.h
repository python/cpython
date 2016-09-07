/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(py_sha3_new__doc__,
"sha3_224(string=None)\n"
"--\n"
"\n"
"Return a new SHA3 hash object with a hashbit length of 28 bytes.");

static PyObject *
py_sha3_new_impl(PyTypeObject *type, PyObject *data);

static PyObject *
py_sha3_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", NULL};
    PyObject *data = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:sha3_224", _keywords,
        &data))
        goto exit;
    return_value = py_sha3_new_impl(type, data);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_sha3_224_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA3_SHA3_224_COPY_METHODDEF    \
    {"copy", (PyCFunction)_sha3_sha3_224_copy, METH_NOARGS, _sha3_sha3_224_copy__doc__},

static PyObject *
_sha3_sha3_224_copy_impl(SHA3object *self);

static PyObject *
_sha3_sha3_224_copy(SHA3object *self, PyObject *Py_UNUSED(ignored))
{
    return _sha3_sha3_224_copy_impl(self);
}

PyDoc_STRVAR(_sha3_sha3_224_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of binary data.");

#define _SHA3_SHA3_224_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha3_sha3_224_digest, METH_NOARGS, _sha3_sha3_224_digest__doc__},

static PyObject *
_sha3_sha3_224_digest_impl(SHA3object *self);

static PyObject *
_sha3_sha3_224_digest(SHA3object *self, PyObject *Py_UNUSED(ignored))
{
    return _sha3_sha3_224_digest_impl(self);
}

PyDoc_STRVAR(_sha3_sha3_224_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHA3_224_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha3_sha3_224_hexdigest, METH_NOARGS, _sha3_sha3_224_hexdigest__doc__},

static PyObject *
_sha3_sha3_224_hexdigest_impl(SHA3object *self);

static PyObject *
_sha3_sha3_224_hexdigest(SHA3object *self, PyObject *Py_UNUSED(ignored))
{
    return _sha3_sha3_224_hexdigest_impl(self);
}

PyDoc_STRVAR(_sha3_sha3_224_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _SHA3_SHA3_224_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha3_sha3_224_update, METH_O, _sha3_sha3_224_update__doc__},

PyDoc_STRVAR(_sha3_shake_128_digest__doc__,
"digest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of binary data.");

#define _SHA3_SHAKE_128_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha3_shake_128_digest, METH_VARARGS|METH_KEYWORDS, _sha3_shake_128_digest__doc__},

static PyObject *
_sha3_shake_128_digest_impl(SHA3object *self, unsigned long length);

static PyObject *
_sha3_shake_128_digest(SHA3object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"length", NULL};
    unsigned long length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k:digest", _keywords,
        &length))
        goto exit;
    return_value = _sha3_shake_128_digest_impl(self, length);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_shake_128_hexdigest__doc__,
"hexdigest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHAKE_128_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha3_shake_128_hexdigest, METH_VARARGS|METH_KEYWORDS, _sha3_shake_128_hexdigest__doc__},

static PyObject *
_sha3_shake_128_hexdigest_impl(SHA3object *self, unsigned long length);

static PyObject *
_sha3_shake_128_hexdigest(SHA3object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"length", NULL};
    unsigned long length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k:hexdigest", _keywords,
        &length))
        goto exit;
    return_value = _sha3_shake_128_hexdigest_impl(self, length);

exit:
    return return_value;
}
/*[clinic end generated code: output=2eb6db41778eeb50 input=a9049054013a1b77]*/

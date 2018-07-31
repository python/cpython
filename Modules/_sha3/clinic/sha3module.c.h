/*[clinic input]
preserve
[clinic start generated code]*/

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
"Return the digest value as a bytes object.");

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
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _SHA3_SHA3_224_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha3_sha3_224_update, METH_O, _sha3_sha3_224_update__doc__},

PyDoc_STRVAR(_sha3_shake_128_digest__doc__,
"digest($self, length, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA3_SHAKE_128_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha3_shake_128_digest, METH_O, _sha3_shake_128_digest__doc__},

PyDoc_STRVAR(_sha3_shake_128_hexdigest__doc__,
"hexdigest($self, length, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHAKE_128_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha3_shake_128_hexdigest, METH_O, _sha3_shake_128_hexdigest__doc__},
/*[clinic end generated code: output=826b6b5a7c3406eb input=a9049054013a1b77]*/

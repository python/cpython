/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(PY_LONG_LONG)

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

#endif /* defined(PY_LONG_LONG) */

#if defined(PY_LONG_LONG)

PyDoc_STRVAR(SHA512Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of binary data.");

#define SHA512TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)SHA512Type_digest, METH_NOARGS, SHA512Type_digest__doc__},

static PyObject *
SHA512Type_digest_impl(SHAobject *self);

static PyObject *
SHA512Type_digest(SHAobject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_digest_impl(self);
}

#endif /* defined(PY_LONG_LONG) */

#if defined(PY_LONG_LONG)

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

#endif /* defined(PY_LONG_LONG) */

#if defined(PY_LONG_LONG)

PyDoc_STRVAR(SHA512Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA512TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)SHA512Type_update, METH_O, SHA512Type_update__doc__},

#endif /* defined(PY_LONG_LONG) */

#if defined(PY_LONG_LONG)

PyDoc_STRVAR(_sha512_sha512__doc__,
"sha512($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA512_SHA512_METHODDEF    \
    {"sha512", (PyCFunction)_sha512_sha512, METH_VARARGS|METH_KEYWORDS, _sha512_sha512__doc__},

static PyObject *
_sha512_sha512_impl(PyModuleDef *module, PyObject *string);

static PyObject *
_sha512_sha512(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", NULL};
    PyObject *string = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:sha512", _keywords,
        &string))
        goto exit;
    return_value = _sha512_sha512_impl(module, string);

exit:
    return return_value;
}

#endif /* defined(PY_LONG_LONG) */

#if defined(PY_LONG_LONG)

PyDoc_STRVAR(_sha512_sha384__doc__,
"sha384($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA512_SHA384_METHODDEF    \
    {"sha384", (PyCFunction)_sha512_sha384, METH_VARARGS|METH_KEYWORDS, _sha512_sha384__doc__},

static PyObject *
_sha512_sha384_impl(PyModuleDef *module, PyObject *string);

static PyObject *
_sha512_sha384(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", NULL};
    PyObject *string = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:sha384", _keywords,
        &string))
        goto exit;
    return_value = _sha512_sha384_impl(module, string);

exit:
    return return_value;
}

#endif /* defined(PY_LONG_LONG) */

#ifndef SHA512TYPE_COPY_METHODDEF
    #define SHA512TYPE_COPY_METHODDEF
#endif /* !defined(SHA512TYPE_COPY_METHODDEF) */

#ifndef SHA512TYPE_DIGEST_METHODDEF
    #define SHA512TYPE_DIGEST_METHODDEF
#endif /* !defined(SHA512TYPE_DIGEST_METHODDEF) */

#ifndef SHA512TYPE_HEXDIGEST_METHODDEF
    #define SHA512TYPE_HEXDIGEST_METHODDEF
#endif /* !defined(SHA512TYPE_HEXDIGEST_METHODDEF) */

#ifndef SHA512TYPE_UPDATE_METHODDEF
    #define SHA512TYPE_UPDATE_METHODDEF
#endif /* !defined(SHA512TYPE_UPDATE_METHODDEF) */

#ifndef _SHA512_SHA512_METHODDEF
    #define _SHA512_SHA512_METHODDEF
#endif /* !defined(_SHA512_SHA512_METHODDEF) */

#ifndef _SHA512_SHA384_METHODDEF
    #define _SHA512_SHA384_METHODDEF
#endif /* !defined(_SHA512_SHA384_METHODDEF) */
/*[clinic end generated code: output=1c7d385731fee7c0 input=a9049054013a1b77]*/

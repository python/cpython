/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(MD5Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define MD5TYPE_COPY_METHODDEF    \
    {"copy", (PyCFunction)MD5Type_copy, METH_NOARGS, MD5Type_copy__doc__},

static PyObject *
MD5Type_copy_impl(MD5object *self);

static PyObject *
MD5Type_copy(MD5object *self, PyObject *Py_UNUSED(ignored))
{
    return MD5Type_copy_impl(self);
}

PyDoc_STRVAR(MD5Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define MD5TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)MD5Type_digest, METH_NOARGS, MD5Type_digest__doc__},

static PyObject *
MD5Type_digest_impl(MD5object *self);

static PyObject *
MD5Type_digest(MD5object *self, PyObject *Py_UNUSED(ignored))
{
    return MD5Type_digest_impl(self);
}

PyDoc_STRVAR(MD5Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define MD5TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)MD5Type_hexdigest, METH_NOARGS, MD5Type_hexdigest__doc__},

static PyObject *
MD5Type_hexdigest_impl(MD5object *self);

static PyObject *
MD5Type_hexdigest(MD5object *self, PyObject *Py_UNUSED(ignored))
{
    return MD5Type_hexdigest_impl(self);
}

PyDoc_STRVAR(MD5Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define MD5TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)MD5Type_update, METH_O, MD5Type_update__doc__},

PyDoc_STRVAR(_md5_md5__doc__,
"md5($module, /, string=b\'\')\n"
"--\n"
"\n"
"Return a new MD5 hash object; optionally initialized with a string.");

#define _MD5_MD5_METHODDEF    \
    {"md5", (PyCFunction)(void(*)(void))_md5_md5, METH_FASTCALL|METH_KEYWORDS, _md5_md5__doc__},

static PyObject *
_md5_md5_impl(PyObject *module, PyObject *string);

static PyObject *
_md5_md5(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "md5", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    string = args[0];
skip_optional_pos:
    return_value = _md5_md5_impl(module, string);

exit:
    return return_value;
}
/*[clinic end generated code: output=53133f08cf9095fc input=a9049054013a1b77]*/

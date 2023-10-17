/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(MD5Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define MD5TYPE_COPY_METHODDEF    \
    {"copy", (PyCFunction)(void(*)(void))MD5Type_copy, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, MD5Type_copy__doc__},

static PyObject *
MD5Type_copy_impl(MD5object *self, PyTypeObject *cls);

static PyObject *
MD5Type_copy(MD5object *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return MD5Type_copy_impl(self, cls);
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
"md5($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new MD5 hash object; optionally initialized with a string.");

#define _MD5_MD5_METHODDEF    \
    {"md5", (PyCFunction)(void(*)(void))_md5_md5, METH_VARARGS|METH_KEYWORDS, _md5_md5__doc__},

static PyObject *
_md5_md5_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_md5_md5(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "usedforsecurity", NULL};
    PyObject *string = NULL;
    int usedforsecurity = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O$p:md5", _keywords,
        &string, &usedforsecurity))
        goto exit;
    return_value = _md5_md5_impl(module, string, usedforsecurity);

exit:
    return return_value;
}
/*[clinic end generated code: output=81702ec915f36236 input=a9049054013a1b77]*/

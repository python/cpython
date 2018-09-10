
/* MD5 module */

/* This module provides an interface to the RSA Data Security,
   Inc. MD5 Message-Digest Algorithm, described in RFC 1321.
   It requires the files md5c.c and md5.h (which are slightly changed
   from the versions in the RFC to avoid the "global.h" file.) */


/* MD5 objects */

#include "Python.h"
#include "structmember.h"
#include "md5.h"

typedef struct {
    PyObject_HEAD
    md5_state_t         md5;            /* the context holder */
} md5object;

static PyTypeObject MD5type;

#define is_md5object(v)         ((v)->ob_type == &MD5type)

static md5object *
newmd5object(void)
{
    md5object *md5p;

    md5p = PyObject_New(md5object, &MD5type);
    if (md5p == NULL)
        return NULL;

    md5_init(&md5p->md5);       /* actual initialisation */
    return md5p;
}


/* MD5 methods */

static void
md5_dealloc(md5object *md5p)
{
    PyObject_Del(md5p);
}


/* MD5 methods-as-attributes */

static PyObject *
md5_update(md5object *self, PyObject *args)
{
    Py_buffer view;
    Py_ssize_t n;
    unsigned char *buf;

    if (!PyArg_ParseTuple(args, "s*:update", &view))
        return NULL;

    n = view.len;
    buf = (unsigned char *) view.buf;
    while (n > 0) {
        Py_ssize_t nbytes;
        if (n > INT_MAX)
            nbytes = INT_MAX;
        else
            nbytes = n;
        md5_append(&self->md5, buf,
                   Py_SAFE_DOWNCAST(nbytes, Py_ssize_t, unsigned int));
        buf += nbytes;
        n -= nbytes;
    }

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc,
"update (arg)\n\
\n\
Update the md5 object with the string arg. Repeated calls are\n\
equivalent to a single call with the concatenation of all the\n\
arguments.");


static PyObject *
md5_digest(md5object *self)
{
    md5_state_t mdContext;
    unsigned char aDigest[16];

    /* make a temporary copy, and perform the final */
    mdContext = self->md5;
    md5_finish(&mdContext, aDigest);

    return PyString_FromStringAndSize((char *)aDigest, 16);
}

PyDoc_STRVAR(digest_doc,
"digest() -> string\n\
\n\
Return the digest of the strings passed to the update() method so\n\
far. This is a 16-byte string which may contain non-ASCII characters,\n\
including null bytes.");


static PyObject *
md5_hexdigest(md5object *self)
{
    md5_state_t mdContext;
    unsigned char digest[16];
    unsigned char hexdigest[32];
    int i, j;

    /* make a temporary copy, and perform the final */
    mdContext = self->md5;
    md5_finish(&mdContext, digest);

    /* Make hex version of the digest */
    for(i=j=0; i<16; i++) {
        char c;
        c = (digest[i] >> 4) & 0xf;
        c = (c>9) ? c+'a'-10 : c + '0';
        hexdigest[j++] = c;
        c = (digest[i] & 0xf);
        c = (c>9) ? c+'a'-10 : c + '0';
        hexdigest[j++] = c;
    }
    return PyString_FromStringAndSize((char*)hexdigest, 32);
}


PyDoc_STRVAR(hexdigest_doc,
"hexdigest() -> string\n\
\n\
Like digest(), but returns the digest as a string of hexadecimal digits.");


static PyObject *
md5_copy(md5object *self)
{
    md5object *md5p;

    if ((md5p = newmd5object()) == NULL)
        return NULL;

    md5p->md5 = self->md5;

    return (PyObject *)md5p;
}

PyDoc_STRVAR(copy_doc,
"copy() -> md5 object\n\
\n\
Return a copy (``clone'') of the md5 object.");


static PyMethodDef md5_methods[] = {
    {"update",    (PyCFunction)md5_update,    METH_VARARGS, update_doc},
    {"digest",    (PyCFunction)md5_digest,    METH_NOARGS,  digest_doc},
    {"hexdigest", (PyCFunction)md5_hexdigest, METH_NOARGS,  hexdigest_doc},
    {"copy",      (PyCFunction)md5_copy,      METH_NOARGS,  copy_doc},
    {NULL, NULL}                             /* sentinel */
};

static PyObject *
md5_get_block_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(64);
}

static PyObject *
md5_get_digest_size(PyObject *self, void *closure)
{
    return PyInt_FromLong(16);
}

static PyObject *
md5_get_name(PyObject *self, void *closure)
{
    return PyString_FromStringAndSize("MD5", 3);
}

static PyGetSetDef md5_getseters[] = {
    {"digest_size",
     (getter)md5_get_digest_size, NULL,
     NULL,
     NULL},
    {"block_size",
     (getter)md5_get_block_size, NULL,
     NULL,
     NULL},
    {"name",
     (getter)md5_get_name, NULL,
     NULL,
     NULL},
    /* the old md5 and sha modules support 'digest_size' as in PEP 247.
     * the old sha module also supported 'digestsize'.  ugh. */
    {"digestsize",
     (getter)md5_get_digest_size, NULL,
     NULL,
     NULL},
    {NULL}  /* Sentinel */
};


PyDoc_STRVAR(module_doc,
"This module implements the interface to RSA's MD5 message digest\n\
algorithm (see also Internet RFC 1321). Its use is quite\n\
straightforward: use the new() to create an md5 object. You can now\n\
feed this object with arbitrary strings using the update() method, and\n\
at any point you can ask it for the digest (a strong kind of 128-bit\n\
checksum, a.k.a. ``fingerprint'') of the concatenation of the strings\n\
fed to it so far using the digest() method.\n\
\n\
Functions:\n\
\n\
new([arg]) -- return a new md5 object, initialized with arg if provided\n\
md5([arg]) -- DEPRECATED, same as new, but for compatibility\n\
\n\
Special Objects:\n\
\n\
MD5Type -- type object for md5 objects");

PyDoc_STRVAR(md5type_doc,
"An md5 represents the object used to calculate the MD5 checksum of a\n\
string of information.\n\
\n\
Methods:\n\
\n\
update() -- updates the current digest with an additional string\n\
digest() -- return the current digest value\n\
hexdigest() -- return the current digest as a string of hexadecimal digits\n\
copy() -- return a copy of the current md5 object");

static PyTypeObject MD5type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_md5.md5",                   /*tp_name*/
    sizeof(md5object),            /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)md5_dealloc,  /*tp_dealloc*/
    0,                            /*tp_print*/
    0,                        /*tp_getattr*/
    0,                            /*tp_setattr*/
    0,                            /*tp_compare*/
    0,                            /*tp_repr*/
    0,                            /*tp_as_number*/
    0,                        /*tp_as_sequence*/
    0,                            /*tp_as_mapping*/
    0,                            /*tp_hash*/
    0,                            /*tp_call*/
    0,                            /*tp_str*/
    0,                            /*tp_getattro*/
    0,                            /*tp_setattro*/
    0,                            /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,           /*tp_flags*/
    md5type_doc,                  /*tp_doc*/
    0,                        /*tp_traverse*/
    0,                            /*tp_clear*/
    0,                            /*tp_richcompare*/
    0,                            /*tp_weaklistoffset*/
    0,                            /*tp_iter*/
    0,                            /*tp_iternext*/
    md5_methods,                  /*tp_methods*/
    0,                            /*tp_members*/
    md5_getseters,            /*tp_getset*/
};


/* MD5 functions */

static PyObject *
MD5_new(PyObject *self, PyObject *args)
{
    md5object *md5p;
    Py_buffer view = { 0 };
    Py_ssize_t n;
    unsigned char *buf;

    if (!PyArg_ParseTuple(args, "|s*:new", &view))
        return NULL;

    if ((md5p = newmd5object()) == NULL) {
        PyBuffer_Release(&view);
        return NULL;
    }

    n = view.len;
    buf = (unsigned char *) view.buf;
    while (n > 0) {
        Py_ssize_t nbytes;
        if (n > INT_MAX)
            nbytes = INT_MAX;
        else
            nbytes = n;
        md5_append(&md5p->md5, buf,
                   Py_SAFE_DOWNCAST(nbytes, Py_ssize_t, unsigned int));
        buf += nbytes;
        n -= nbytes;
    }
    PyBuffer_Release(&view);

    return (PyObject *)md5p;
}

PyDoc_STRVAR(new_doc,
"new([arg]) -> md5 object\n\
\n\
Return a new md5 object. If arg is present, the method call update(arg)\n\
is made.");


/* List of functions exported by this module */

static PyMethodDef md5_functions[] = {
    {"new",             (PyCFunction)MD5_new, METH_VARARGS, new_doc},
    {NULL,              NULL}   /* Sentinel */
};


/* Initialize this module. */

PyMODINIT_FUNC
init_md5(void)
{
    PyObject *m, *d;

    Py_TYPE(&MD5type) = &PyType_Type;
    if (PyType_Ready(&MD5type) < 0)
        return;
    m = Py_InitModule3("_md5", md5_functions, module_doc);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "MD5Type", (PyObject *)&MD5type);
    PyModule_AddIntConstant(m, "digest_size", 16);
    /* No need to check the error here, the caller will do that */
}

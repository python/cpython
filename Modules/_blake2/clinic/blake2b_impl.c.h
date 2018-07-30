/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(py_blake2b_new__doc__,
"blake2b(string=None, *, digest_size=_blake2b.blake2b.MAX_DIGEST_SIZE,\n"
"        key=None, salt=None, person=None, fanout=1, depth=1,\n"
"        leaf_size=0, node_offset=0, node_depth=0, inner_size=0,\n"
"        last_node=False)\n"
"--\n"
"\n"
"Return a new BLAKE2b hash object.");

static PyObject *
py_blake2b_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node);

static PyObject *
py_blake2b_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"string", "digest_size", "key", "salt", "person", "fanout", "depth", "leaf_size", "node_offset", "node_depth", "inner_size", "last_node", NULL};
    static _PyArg_Parser _parser = {"|O$iy*y*y*iiO&O&iip:blake2b", _keywords, 0};
    PyObject *data = NULL;
    int digest_size = BLAKE2B_OUTBYTES;
    Py_buffer key = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    Py_buffer person = {NULL, NULL};
    int fanout = 1;
    int depth = 1;
    unsigned long leaf_size = 0;
    unsigned long long node_offset = 0;
    int node_depth = 0;
    int inner_size = 0;
    int last_node = 0;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &data, &digest_size, &key, &salt, &person, &fanout, &depth, _PyLong_UnsignedLong_Converter, &leaf_size, _PyLong_UnsignedLongLong_Converter, &node_offset, &node_depth, &inner_size, &last_node)) {
        goto exit;
    }
    return_value = py_blake2b_new_impl(type, data, digest_size, &key, &salt, &person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }
    /* Cleanup for person */
    if (person.obj) {
       PyBuffer_Release(&person);
    }

    return return_value;
}

PyDoc_STRVAR(_blake2b_blake2b_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _BLAKE2B_BLAKE2B_COPY_METHODDEF    \
    {"copy", (PyCFunction)_blake2b_blake2b_copy, METH_NOARGS, _blake2b_blake2b_copy__doc__},

static PyObject *
_blake2b_blake2b_copy_impl(BLAKE2bObject *self);

static PyObject *
_blake2b_blake2b_copy(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2b_blake2b_copy_impl(self);
}

PyDoc_STRVAR(_blake2b_blake2b_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _BLAKE2B_BLAKE2B_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_blake2b_blake2b_update, METH_O, _blake2b_blake2b_update__doc__},

PyDoc_STRVAR(_blake2b_blake2b_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of binary data.");

#define _BLAKE2B_BLAKE2B_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_blake2b_blake2b_digest, METH_NOARGS, _blake2b_blake2b_digest__doc__},

static PyObject *
_blake2b_blake2b_digest_impl(BLAKE2bObject *self);

static PyObject *
_blake2b_blake2b_digest(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2b_blake2b_digest_impl(self);
}

PyDoc_STRVAR(_blake2b_blake2b_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _BLAKE2B_BLAKE2B_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_blake2b_blake2b_hexdigest, METH_NOARGS, _blake2b_blake2b_hexdigest__doc__},

static PyObject *
_blake2b_blake2b_hexdigest_impl(BLAKE2bObject *self);

static PyObject *
_blake2b_blake2b_hexdigest(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2b_blake2b_hexdigest_impl(self);
}
/*[clinic end generated code: output=afc5c45dff0a24f9 input=a9049054013a1b77]*/

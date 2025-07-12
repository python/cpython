/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedLong_Converter()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(py_blake2b_new__doc__,
"blake2b(data=b\'\', *, digest_size=_blake2.blake2b.MAX_DIGEST_SIZE,\n"
"        key=b\'\', salt=b\'\', person=b\'\', fanout=1, depth=1, leaf_size=0,\n"
"        node_offset=0, node_depth=0, inner_size=0, last_node=False,\n"
"        usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new BLAKE2b hash object.");

static PyObject *
py_blake2b_new_impl(PyTypeObject *type, PyObject *data_obj, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity,
                    PyObject *string);

static PyObject *
py_blake2b_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 14
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(data), &_Py_ID(digest_size), &_Py_ID(key), &_Py_ID(salt), &_Py_ID(person), &_Py_ID(fanout), &_Py_ID(depth), &_Py_ID(leaf_size), &_Py_ID(node_offset), &_Py_ID(node_depth), &_Py_ID(inner_size), &_Py_ID(last_node), &_Py_ID(usedforsecurity), &_Py_ID(string), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "digest_size", "key", "salt", "person", "fanout", "depth", "leaf_size", "node_offset", "node_depth", "inner_size", "last_node", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "blake2b",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[14];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *data_obj = NULL;
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
    int usedforsecurity = 1;
    PyObject *string = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        data_obj = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        digest_size = PyLong_AsInt(fastargs[1]);
        if (digest_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        if (PyObject_GetBuffer(fastargs[2], &key, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        if (PyObject_GetBuffer(fastargs[3], &salt, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[4]) {
        if (PyObject_GetBuffer(fastargs[4], &person, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[5]) {
        fanout = PyLong_AsInt(fastargs[5]);
        if (fanout == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[6]) {
        depth = PyLong_AsInt(fastargs[6]);
        if (depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[7]) {
        if (!_PyLong_UnsignedLong_Converter(fastargs[7], &leaf_size)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[8]) {
        if (!_PyLong_UnsignedLongLong_Converter(fastargs[8], &node_offset)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[9]) {
        node_depth = PyLong_AsInt(fastargs[9]);
        if (node_depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[10]) {
        inner_size = PyLong_AsInt(fastargs[10]);
        if (inner_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[11]) {
        last_node = PyObject_IsTrue(fastargs[11]);
        if (last_node < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[12]) {
        usedforsecurity = PyObject_IsTrue(fastargs[12]);
        if (usedforsecurity < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    string = fastargs[13];
skip_optional_kwonly:
    return_value = py_blake2b_new_impl(type, data_obj, digest_size, &key, &salt, &person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node, usedforsecurity, string);

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

PyDoc_STRVAR(_blake2_blake2b_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _BLAKE2_BLAKE2B_COPY_METHODDEF    \
    {"copy", (PyCFunction)_blake2_blake2b_copy, METH_NOARGS, _blake2_blake2b_copy__doc__},

static PyObject *
_blake2_blake2b_copy_impl(BLAKE2bObject *self);

static PyObject *
_blake2_blake2b_copy(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2_blake2b_copy_impl(self);
}

PyDoc_STRVAR(_blake2_blake2b_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _BLAKE2_BLAKE2B_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_blake2_blake2b_update, METH_O, _blake2_blake2b_update__doc__},

PyDoc_STRVAR(_blake2_blake2b_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _BLAKE2_BLAKE2B_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_blake2_blake2b_digest, METH_NOARGS, _blake2_blake2b_digest__doc__},

static PyObject *
_blake2_blake2b_digest_impl(BLAKE2bObject *self);

static PyObject *
_blake2_blake2b_digest(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2_blake2b_digest_impl(self);
}

PyDoc_STRVAR(_blake2_blake2b_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _BLAKE2_BLAKE2B_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_blake2_blake2b_hexdigest, METH_NOARGS, _blake2_blake2b_hexdigest__doc__},

static PyObject *
_blake2_blake2b_hexdigest_impl(BLAKE2bObject *self);

static PyObject *
_blake2_blake2b_hexdigest(BLAKE2bObject *self, PyObject *Py_UNUSED(ignored))
{
    return _blake2_blake2b_hexdigest_impl(self);
}
/*[clinic end generated code: output=7abd17b22b1095c4 input=a9049054013a1b77]*/

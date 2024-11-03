/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_hmac_compute_md5__doc__,
"compute_md5($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_MD5_METHODDEF    \
    {"compute_md5", _PyCFunction_CAST(_hmac_compute_md5), METH_FASTCALL, _hmac_compute_md5__doc__},

static PyObject *
_hmac_compute_md5_impl(PyObject *module, Py_buffer *key, Py_buffer *data);

static PyObject *
_hmac_compute_md5(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_md5", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_md5_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha1__doc__,
"compute_sha1($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA1_METHODDEF    \
    {"compute_sha1", _PyCFunction_CAST(_hmac_compute_sha1), METH_FASTCALL, _hmac_compute_sha1__doc__},

static PyObject *
_hmac_compute_sha1_impl(PyObject *module, Py_buffer *key, Py_buffer *data);

static PyObject *
_hmac_compute_sha1(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha1", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha1_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_224__doc__,
"compute_sha2_224($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_224_METHODDEF    \
    {"compute_sha2_224", _PyCFunction_CAST(_hmac_compute_sha2_224), METH_FASTCALL, _hmac_compute_sha2_224__doc__},

static PyObject *
_hmac_compute_sha2_224_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha2_224(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha2_224", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha2_224_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_256__doc__,
"compute_sha2_256($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_256_METHODDEF    \
    {"compute_sha2_256", _PyCFunction_CAST(_hmac_compute_sha2_256), METH_FASTCALL, _hmac_compute_sha2_256__doc__},

static PyObject *
_hmac_compute_sha2_256_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha2_256(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha2_256", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha2_256_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_384__doc__,
"compute_sha2_384($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_384_METHODDEF    \
    {"compute_sha2_384", _PyCFunction_CAST(_hmac_compute_sha2_384), METH_FASTCALL, _hmac_compute_sha2_384__doc__},

static PyObject *
_hmac_compute_sha2_384_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha2_384(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha2_384", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha2_384_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_512__doc__,
"compute_sha2_512($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_512_METHODDEF    \
    {"compute_sha2_512", _PyCFunction_CAST(_hmac_compute_sha2_512), METH_FASTCALL, _hmac_compute_sha2_512__doc__},

static PyObject *
_hmac_compute_sha2_512_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha2_512(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha2_512", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha2_512_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_224__doc__,
"compute_sha3_224($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_224_METHODDEF    \
    {"compute_sha3_224", _PyCFunction_CAST(_hmac_compute_sha3_224), METH_FASTCALL, _hmac_compute_sha3_224__doc__},

static PyObject *
_hmac_compute_sha3_224_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha3_224(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha3_224", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha3_224_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_256__doc__,
"compute_sha3_256($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_256_METHODDEF    \
    {"compute_sha3_256", _PyCFunction_CAST(_hmac_compute_sha3_256), METH_FASTCALL, _hmac_compute_sha3_256__doc__},

static PyObject *
_hmac_compute_sha3_256_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha3_256(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha3_256", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha3_256_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_384__doc__,
"compute_sha3_384($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_384_METHODDEF    \
    {"compute_sha3_384", _PyCFunction_CAST(_hmac_compute_sha3_384), METH_FASTCALL, _hmac_compute_sha3_384__doc__},

static PyObject *
_hmac_compute_sha3_384_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha3_384(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha3_384", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha3_384_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_512__doc__,
"compute_sha3_512($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_512_METHODDEF    \
    {"compute_sha3_512", _PyCFunction_CAST(_hmac_compute_sha3_512), METH_FASTCALL, _hmac_compute_sha3_512__doc__},

static PyObject *
_hmac_compute_sha3_512_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data);

static PyObject *
_hmac_compute_sha3_512(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_sha3_512", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_sha3_512_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_blake2s_32__doc__,
"compute_blake2s_32($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_BLAKE2S_32_METHODDEF    \
    {"compute_blake2s_32", _PyCFunction_CAST(_hmac_compute_blake2s_32), METH_FASTCALL, _hmac_compute_blake2s_32__doc__},

static PyObject *
_hmac_compute_blake2s_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *data);

static PyObject *
_hmac_compute_blake2s_32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_blake2s_32", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_blake2s_32_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_hmac_compute_blake2b_32__doc__,
"compute_blake2b_32($module, key, data, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_BLAKE2B_32_METHODDEF    \
    {"compute_blake2b_32", _PyCFunction_CAST(_hmac_compute_blake2b_32), METH_FASTCALL, _hmac_compute_blake2b_32__doc__},

static PyObject *
_hmac_compute_blake2b_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *data);

static PyObject *
_hmac_compute_blake2b_32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer key = {NULL, NULL};
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_CheckPositional("compute_blake2b_32", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _hmac_compute_blake2b_32_impl(module, &key, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}
/*[clinic end generated code: output=c10b8daa18a66857 input=a9049054013a1b77]*/

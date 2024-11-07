#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "pyconfig.h"
#include "Python.h"
#include "hashlib.h"

#include "_hacl/Hacl_HMAC.h"

#include "clinic/hmacmodule.c.h"

#define HACL_HMAC_COMPUTE_HASH(HACL_FUNCTION, DIGEST_SIZE, KEY, SRC)    \
    do {                                                                \
        unsigned char out[DIGEST_SIZE];                                 \
        HACL_FUNCTION(                                                  \
            out,                                                        \
            (uint8_t *)KEY->buf, (uint32_t)KEY->len,                    \
            (uint8_t *)SRC->buf, (uint32_t)SRC->len                     \
        );                                                              \
        return PyBytes_FromString((const char *)out);                   \
    } while (0)

/*[clinic input]
module _hmac
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=799f0f10157d561f]*/

/*[clinic input]
_hmac.compute_md5

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_md5_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=06415d62c949b812 input=ba930327d472e0be]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_md5, 16, key, msg);
}

/*[clinic input]
_hmac.compute_sha1

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha1_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=3daf26128c9e84b5 input=6015854f4040c058]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha1, 20, key, msg);
}

/*[clinic input]
_hmac.compute_sha2_224

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_224_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=f665a01d0ce8873b input=b82974de99696949]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_224, 28, key, msg);
}

/*[clinic input]
_hmac.compute_sha2_256

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_256_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=6eda2182e50c3832 input=ae9639dccbca11bb]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_256, 32, key, msg);
}

/*[clinic input]
_hmac.compute_sha2_384

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_384_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=0fc9803f1d0b731c input=d643b1254bc142e1]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_384, 48, key, msg);
}

/*[clinic input]
_hmac.compute_sha2_512

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_512_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=f4b3f79c749c2100 input=1252a28c102c1d23]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_512, 64, key, msg);
}

/*[clinic input]
_hmac.compute_sha3_224

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_224_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=ba0f59d80a557e20 input=b02a4325fbc691ad]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_224, 28, key, msg);
}

/*[clinic input]
_hmac.compute_sha3_256

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_256_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=cda6fbc13c233f45 input=64a7b8ac5fc62521]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_256, 32, key, msg);
}

/*[clinic input]
_hmac.compute_sha3_384

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_384_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=5a0fc341caa1b4ed input=3e9e2f74c65193bd]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_384, 48, key, msg);
}

/*[clinic input]
_hmac.compute_sha3_512

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_512_impl(PyObject *module, Py_buffer *key, Py_buffer *msg)
/*[clinic end generated code: output=af9773a23df74056 input=da79fd5e1de89478]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_512, 64, key, msg);
}

/*[clinic input]
_hmac.compute_blake2s_32

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2s_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg)
/*[clinic end generated code: output=9951eb111793d727 input=cc384ff59f0bf43b]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_blake2s_32, 32, key, msg);
}

/*[clinic input]
_hmac.compute_blake2b_32

    key: Py_buffer
    msg: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2b_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg)
/*[clinic end generated code: output=341c892645174059 input=52f8b6ccfc97bcba]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_blake2b_32, 64, key, msg);
}

static PyMethodDef hmacmodule_methods[] = {
    _HMAC_COMPUTE_MD5_METHODDEF
    _HMAC_COMPUTE_SHA1_METHODDEF
    _HMAC_COMPUTE_SHA2_224_METHODDEF
    _HMAC_COMPUTE_SHA2_256_METHODDEF
    _HMAC_COMPUTE_SHA2_384_METHODDEF
    _HMAC_COMPUTE_SHA2_512_METHODDEF
    _HMAC_COMPUTE_SHA3_224_METHODDEF
    _HMAC_COMPUTE_SHA3_256_METHODDEF
    _HMAC_COMPUTE_SHA3_384_METHODDEF
    _HMAC_COMPUTE_SHA3_512_METHODDEF
    _HMAC_COMPUTE_BLAKE2S_32_METHODDEF
    _HMAC_COMPUTE_BLAKE2B_32_METHODDEF
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef_Slot hmacmodule_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _hmacmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_hmac",
    .m_size = 0,
    .m_methods = hmacmodule_methods,
    .m_slots = hmacmodule_slots,
};

PyMODINIT_FUNC
PyInit__hmac(void)
{
    return PyModuleDef_Init(&_hmacmodule);
}

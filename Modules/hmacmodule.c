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
@critical_section
_hmac.compute_md5

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_md5_impl(PyObject *module, Py_buffer *key, Py_buffer *data)
/*[clinic end generated code: output=bcf3dfafd7092a5a input=9ceaaa27ec318007]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_md5, 16, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha1

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha1_impl(PyObject *module, Py_buffer *key, Py_buffer *data)
/*[clinic end generated code: output=f26338ed3aa68853 input=2380452bf9d1fe7d]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha1, 20, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha2_224

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_224_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=d9907a240da31c07 input=b874f95fd4b0fb99]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_224, 28, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha2_256

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_256_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=1ba977f01c332460 input=c880969b65dca329]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_256, 32, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha2_384

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_384_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=5ad8e1c6346fcf5b input=e206968b3c4aad3d]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_384, 48, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha2_512

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_512_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=8e73b2c39812934c input=839c27c90c3aed01]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha2_512, 64, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha3_224

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_224_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=5b3ee358e5d96fa8 input=f52550611ea10725]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_224, 28, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha3_256

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_256_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=cf977eed9c59ed3b input=ce59d1ddd77c0624]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_256, 32, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha3_384

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_384_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=3f576e31d4d05f35 input=f4bca88551693caa]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_384, 48, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_sha3_512

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_512_impl(PyObject *module, Py_buffer *key,
                            Py_buffer *data)
/*[clinic end generated code: output=238126dcba98fda2 input=2f98f302c64eca64]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_sha3_512, 64, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_blake2s_32

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2s_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *data)
/*[clinic end generated code: output=72a8231623e4ccf9 input=0be9099b69bcd9e7]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_blake2s_32, 32, key, data);
}

/*[clinic input]
@critical_section
_hmac.compute_blake2b_32

    key: Py_buffer
    data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2b_32_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *data)
/*[clinic end generated code: output=ea083dfa29679029 input=aecba54a3e2dff72]*/
{
    HACL_HMAC_COMPUTE_HASH(Hacl_HMAC_compute_blake2b_32, 32, key, data);
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

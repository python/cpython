#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "hashlib.h"

#include "_hacl/Hacl_HMAC.h"

#include "clinic/hmacmodule.c.h"

typedef void (*HACL_HMAC_digest_func_t)(uint8_t *out,
                                        uint8_t *key, uint32_t keylen,
                                        uint8_t *msg, uint32_t msglen);

// HMAC underlying hash function static information.

/* MD-5 */
#define Py_hmac_md5_block_size          64
#define Py_hmac_md5_digest_size         16
#define Py_hmac_md5_digest_func         Hacl_HMAC_compute_md5

/* SHA-1 family */
#define Py_hmac_sha1_block_size         64
#define Py_hmac_sha1_digest_size        20
#define Py_hmac_sha1_digest_func        Hacl_HMAC_compute_sha1

/* SHA-2 family */
#define Py_hmac_sha2_224_block_size     64
#define Py_hmac_sha2_224_digest_size    28
#define Py_hmac_sha2_224_digest_func    Hacl_HMAC_compute_sha2_224

#define Py_hmac_sha2_256_block_size     64
#define Py_hmac_sha2_256_digest_size    32
#define Py_hmac_sha2_256_digest_func    Hacl_HMAC_compute_sha2_256

#define Py_hmac_sha2_384_block_size     128
#define Py_hmac_sha2_384_digest_size    48
#define Py_hmac_sha2_384_digest_func    Hacl_HMAC_compute_sha2_384

#define Py_hmac_sha2_512_block_size     128
#define Py_hmac_sha2_512_digest_size    64
#define Py_hmac_sha2_512_digest_func    Hacl_HMAC_compute_sha2_512

/* SHA-3 family */
#define Py_hmac_sha3_224_block_size     144
#define Py_hmac_sha3_224_digest_size    28
#define Py_hmac_sha3_224_digest_func    Hacl_HMAC_compute_sha3_224

#define Py_hmac_sha3_256_block_size     136
#define Py_hmac_sha3_256_digest_size    32
#define Py_hmac_sha3_256_digest_func    Hacl_HMAC_compute_sha3_256

#define Py_hmac_sha3_384_block_size     104
#define Py_hmac_sha3_384_digest_size    48
#define Py_hmac_sha3_384_digest_func    Hacl_HMAC_compute_sha3_384

#define Py_hmac_sha3_512_block_size     72
#define Py_hmac_sha3_512_digest_size    64
#define Py_hmac_sha3_512_digest_func    Hacl_HMAC_compute_sha3_512

/* Blake family */
#define Py_hmac_blake2s_block_size      64
#define Py_hmac_blake2s_digest_size     32
#define Py_hmac_blake2s_digest_func     Hacl_HMAC_compute_blake2s_32

#define Py_hmac_blake2b_block_size      128
#define Py_hmac_blake2b_digest_size     64
#define Py_hmac_blake2b_digest_func     Hacl_HMAC_compute_blake2b_32

/* Check that the buffer length fits on a uint32_t. */
static inline int
has_uint32_t_buffer_length(const Py_buffer *buffer)
{
#if PY_SSIZE_T_MAX > UINT32_MAX
    return buffer->len <= (Py_ssize_t)UINT32_MAX;
#else
    return 1;
#endif
}

/* One-shot HMAC-HASH using the given HACL_HMAC_FUNCTION. */
#define Py_HACL_HMAC_ONESHOT(HACL_HMAC_FUNCTION, DIGEST_SIZE, KEY, MSG)     \
    do {                                                                    \
        Py_buffer keyview, msgview;                                         \
        GET_BUFFER_VIEW_OR_ERROUT((KEY), &keyview);                         \
        if (!has_uint32_t_buffer_length(&keyview)) {                        \
            PyErr_SetString(PyExc_ValueError,                               \
                            "key length exceeds UINT32_MAX");               \
            return NULL;                                                    \
        }                                                                   \
        GET_BUFFER_VIEW_OR_ERROUT((MSG), &msgview);                         \
        if (!has_uint32_t_buffer_length(&msgview)) {                        \
            PyErr_SetString(PyExc_ValueError,                               \
                            "message length exceeds UINT32_MAX");           \
            return NULL;                                                    \
        }                                                                   \
        uint8_t out[(DIGEST_SIZE)];                                         \
        HACL_HMAC_FUNCTION(                                                 \
            out,                                                            \
            (uint8_t *)keyview.buf, (uint32_t)keyview.len,                  \
            (uint8_t *)msgview.buf, (uint32_t)msgview.len                   \
        );                                                                  \
        PyBuffer_Release(&msgview);                                         \
        PyBuffer_Release(&keyview);                                         \
        return PyBytes_FromStringAndSize((const char *)out, (DIGEST_SIZE)); \
    } while (0)

/*[clinic input]
module _hmac
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=799f0f10157d561f]*/

/*[clinic input]
_hmac.compute_md5

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_md5_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=7837a4ceccbbf636 input=77a4b774c7d61218]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_md5_digest_func,
                         Py_hmac_md5_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha1

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha1_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=79fd7689c83691d8 input=3b64dccc6bdbe4ba]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha1_digest_func,
                         Py_hmac_sha1_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha2_224

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_224_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=7f21f1613e53979e input=bcaac7a3637484ce]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha2_224_digest_func,
                         Py_hmac_sha2_224_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha2_256

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_256_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=d4a291f7d9a82459 input=6e2d1f6fe9c56d21]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha2_256_digest_func,
                         Py_hmac_sha2_256_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha2_384

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_384_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=f211fa26e3700c27 input=9ce8de89dda79e62]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha2_384_digest_func,
                         Py_hmac_sha2_384_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha2_512

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_512_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=d5c20373762cecca input=b964bb8487d7debd]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha2_512_digest_func,
                         Py_hmac_sha2_512_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha3_224

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_224_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=a242ccac9ad9c22b input=d0ab0c7d189c3d87]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha3_224_digest_func,
                         Py_hmac_sha3_224_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha3_256

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_256_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=b539dbb61af2fe0b input=f05d7b6364b35d02]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha3_256_digest_func,
                         Py_hmac_sha3_256_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha3_384

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_384_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=5eb372fb5c4ffd3a input=d842d393e7aa05ae]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha3_384_digest_func,
                         Py_hmac_sha3_384_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_sha3_512

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha3_512_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=154bcbf8c2eacac1 input=166fe5baaeaabfde]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_sha3_512_digest_func,
                         Py_hmac_sha3_512_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_blake2s_32

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2s_32_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=cfc730791bc62361 input=d22c36e7fe31a985]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_blake2s_digest_func,
                         Py_hmac_blake2s_digest_size,
                         key, msg);
}

/*[clinic input]
_hmac.compute_blake2b_32

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_blake2b_32_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=765c5c4fb9124636 input=4a35ee058d172f4b]*/
{
    Py_HACL_HMAC_ONESHOT(Py_hmac_blake2b_digest_func,
                         Py_hmac_blake2b_digest_size,
                         key, msg);
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

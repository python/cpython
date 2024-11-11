#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_hashtable.h"
#include "pycore_strhex.h"              // _Py_strhex()

#include <openssl/obj_mac.h>            // LN_* macros

#include "_hacl/Hacl_HMAC.h"
#include "hashlib.h"

// --- HMAC underlying hash function static information -----------------------

#define Py_hmac_hash_max_digest_size            64

/* MD-5 */
// HACL_HID = md5
#define Py_hmac_md5_block_size                  64
#define Py_hmac_md5_digest_size                 16

#define Py_hmac_md5_compute_func                Hacl_HMAC_compute_md5

#define Py_OpenSSL_LN_md5                       LN_md5

/* SHA-1 family */
// HACL_HID = sha1
#define Py_hmac_sha1_block_size                 64
#define Py_hmac_sha1_digest_size                20

#define Py_hmac_sha1_compute_func               Hacl_HMAC_compute_sha1

#define Py_OpenSSL_LN_sha1                      LN_sha1

/* SHA-2 family */
// HACL_HID = sha2_224
#define Py_hmac_sha2_224_block_size             64
#define Py_hmac_sha2_224_digest_size            28

#define Py_hmac_sha2_224_compute_func           Hacl_HMAC_compute_sha2_224

#define Py_OpenSSL_LN_sha2_224                  LN_sha224

// HACL_HID = sha2_256
#define Py_hmac_sha2_256_block_size             64
#define Py_hmac_sha2_256_digest_size            32

#define Py_hmac_sha2_256_compute_func           Hacl_HMAC_compute_sha2_256

#define Py_OpenSSL_LN_sha2_256                  LN_sha256

// HACL_HID = sha2_384
#define Py_hmac_sha2_384_block_size             128
#define Py_hmac_sha2_384_digest_size            48

#define Py_hmac_sha2_384_compute_func           Hacl_HMAC_compute_sha2_384

#define Py_OpenSSL_LN_sha2_384                  LN_sha384

// HACL_HID = sha2_512
#define Py_hmac_sha2_512_block_size             128
#define Py_hmac_sha2_512_digest_size            64

#define Py_hmac_sha2_512_compute_func           Hacl_HMAC_compute_sha2_512

#define Py_OpenSSL_LN_sha2_512                  LN_sha512

/* SHA-3 family */
// HACL_HID = sha3_224
#define Py_hmac_sha3_224_block_size             144
#define Py_hmac_sha3_224_digest_size            28

#define Py_hmac_sha3_224_compute_func           Hacl_HMAC_compute_sha3_224

#if defined(LN_sha3_224)
#  define Py_OpenSSL_LN_sha3_224                LN_sha3_224
#else
#  define Py_OpenSSL_LN_sha3_224                "sha3_224"
#endif

// HACL_HID = sha3_256
#define Py_hmac_sha3_256_block_size             136
#define Py_hmac_sha3_256_digest_size            32

#define Py_hmac_sha3_256_compute_func           Hacl_HMAC_compute_sha3_256

#if defined(LN_sha3_256)
#  define Py_OpenSSL_LN_sha3_256                LN_sha3_256
#else
#  define Py_OpenSSL_LN_sha3_256                "sha3_256"
#endif

// HACL_HID = sha3_384
#define Py_hmac_sha3_384_block_size             104
#define Py_hmac_sha3_384_digest_size            48

#define Py_hmac_sha3_384_compute_func           Hacl_HMAC_compute_sha3_384

#if defined(LN_sha3_384)
#  define Py_OpenSSL_LN_sha3_384                LN_sha3_384
#else
#  define Py_OpenSSL_LN_sha3_384                "sha3_384"
#endif

// HACL_HID = sha3_512
#define Py_hmac_sha3_512_block_size             72
#define Py_hmac_sha3_512_digest_size            64

#define Py_hmac_sha3_512_compute_func           Hacl_HMAC_compute_sha3_512

#if defined(LN_sha3_512)
#  define Py_OpenSSL_LN_sha3_512                LN_sha3_512
#else
#  define Py_OpenSSL_LN_sha3_512                "sha3_512"
#endif

/* Blake family */
// HACL_HID = blake2s_32
#define Py_hmac_blake2s_32_block_size           64
#define Py_hmac_blake2s_32_digest_size          32

#define Py_hmac_blake2s_32_compute_func         Hacl_HMAC_compute_blake2s_32

#if defined(LN_blake2s256)
#  define Py_OpenSSL_LN_blake2s_32              LN_blake2s256
#else
#  define Py_OpenSSL_LN_blake2s_32              "blake2s256"
#endif

// HACL_HID = blake2b_32
#define Py_hmac_blake2b_32_block_size           128
#define Py_hmac_blake2b_32_digest_size          64

#define Py_hmac_blake2b_32_compute_func         Hacl_HMAC_compute_blake2b_32

#if defined(LN_blake2b512)
#  define Py_OpenSSL_LN_blake2b_32              LN_blake2b512
#else
#  define Py_OpenSSL_LN_blake2b_32              "blake2b512"
#endif

/* Enumeration indicating the underlying hash function used by HMAC. */
typedef enum HMAC_Hash_Kind {
    Py_hmac_kind_unknown = 0,
    /* MD5 */
    Py_hmac_kind_hmac_md5,
    /* SHA-1 */
    Py_hmac_kind_hmac_sha1,
    /* SHA-2 family */
    Py_hmac_kind_hmac_sha2_224,
    Py_hmac_kind_hmac_sha2_256,
    Py_hmac_kind_hmac_sha2_384,
    Py_hmac_kind_hmac_sha2_512,
    /* SHA-3 family */
    Py_hmac_kind_hmac_sha3_224,
    Py_hmac_kind_hmac_sha3_256,
    Py_hmac_kind_hmac_sha3_384,
    Py_hmac_kind_hmac_sha3_512,
    /* Blake family */
    Py_hmac_kind_hmac_blake2s_32,
    Py_hmac_kind_hmac_blake2b_32,
} HMAC_Hash_Kind;

/* Function pointer type for 1-shot HACL* HMAC functions. */
typedef void
(*HACL_HMAC_compute_func)(uint8_t *out,
                          uint8_t *key, uint32_t keylen,
                          uint8_t *msg, uint32_t msglen);

/* Function pointer type for 1-shot HACL* HMAC CPython AC functions. */
typedef PyObject *
(*PYAC_HMAC_compute_func)(PyObject *module, PyObject *key, PyObject *msg);

/*
 * HACL* HMAC minimal interface.
 */
typedef struct py_hmac_hacl_api {
    HACL_HMAC_compute_func compute;
    PYAC_HMAC_compute_func compute_py;
} py_hmac_hacl_api;

/*
 * HMAC underlying hash function static information.
 *
 * The '_hmac' built-in module is able to recognize the same hash
 * functions as the '_hashlib' built-in module with the exception
 * of truncated SHA-2-512-224/256 which are not yet implemented by
 * the HACL* project.
 */
typedef struct py_hmac_hinfo {
    /*
     * Name of the hash function used by the HACL* HMAC module.
     *
     * This name may differ from the hashlib names and OpenSSL names.
     * For instance, SHA-2/224 is named "sha2_224" instead of "sha224"
     * as it is done by 'hashlib'.
     */
    const char *name;
    /*
     * Optional field to cache storing the 'name' field as a Python string.
     *
     * This field is NULL by default in the items of "py_hmac_hinfo_table"
     * but will be populated when creating the module's state "hinfo_table".
     */
    PyObject *p_name;

    /* hash function information */
    HMAC_Hash_Kind kind;
    uint32_t block_size;
    uint32_t digest_size;

    /* HACL* HMAC API */
    py_hmac_hacl_api api;

    const char *hashlib_name;   /* hashlib preferred name (default: name) */
    const char *openssl_name;   /* hashlib preferred OpenSSL alias (if any) */

    Py_ssize_t refcnt;
} py_hmac_hinfo;

// --- HMAC module state ------------------------------------------------------

typedef struct hmacmodule_state {
    _Py_hashtable_t *hinfo_table;
    /* imported from _hashlib */
    PyObject *hashlib_constructs_mappingproxy;
    PyObject *hashlib_unsupported_digestmod_error;
    /* interned strings */
    PyObject *str_lower;
} hmacmodule_state;

static inline hmacmodule_state *
get_hmacmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (hmacmodule_state *)state;
}

/*[clinic input]
module _hmac
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=799f0f10157d561f]*/

#include "clinic/hmacmodule.c.h"

// --- Helpers ----------------------------------------------------------------

static inline int
find_hash_info_by_utf8name(hmacmodule_state *state,
                           const char *name,
                           const py_hmac_hinfo **info)
{
    if (name == NULL) {
        *info = NULL;
        return -1;
    }
    *info = _Py_hashtable_get(state->hinfo_table, name);
    return *info != NULL;
}

static int
find_hash_info_by_name(hmacmodule_state *state,
                       PyObject *name,
                       const py_hmac_hinfo **info)
{
    int rc = find_hash_info_by_utf8name(state, PyUnicode_AsUTF8(name), info);
    if (rc == 0) {
        // try to find an alternative using the lowercase name
        PyObject *lower = PyObject_CallMethodNoArgs(name, state->str_lower);
        if (lower == NULL) {
            *info = NULL;
            return -1;
        }
        rc = find_hash_info_by_utf8name(state, PyUnicode_AsUTF8(lower), info);
        Py_DECREF(lower);
    }
    return rc;
}

/*
 * Find the corresponding HMAC hash function static information.
 *
 * If an error occurs or if nothing can be found, this
 * returns -1 or 0 respectively, and sets 'info' to NULL.
 * Otherwise, this returns 1 and stores the result in 'info'.
 *
 * Parameters
 *
 *      state           The HMAC module state.
 *      hash_info_ref   An input to hashlib.new().
 *      info            The deduced information, if any.
 */
static int
find_hash_info_impl(hmacmodule_state *state,
                    PyObject *hash_info_ref,
                    const py_hmac_hinfo **info)
{
    if (PyUnicode_Check(hash_info_ref)) {
        return find_hash_info_by_name(state, hash_info_ref, info);
    }
    PyObject *hashlib_name = NULL;
    int rc = PyMapping_GetOptionalItem(state->hashlib_constructs_mappingproxy,
                                       hash_info_ref, &hashlib_name);
    if (rc <= 0) {
        *info = NULL;
        return rc;
    }
    rc = find_hash_info_by_name(state, hashlib_name, info);
    Py_DECREF(hashlib_name);
    return rc;
}

static const py_hmac_hinfo *
find_hash_info(hmacmodule_state *state, PyObject *hash_info_ref)
{
    const py_hmac_hinfo *info = NULL;
    int rc = find_hash_info_impl(state, hash_info_ref, &info);
    if (rc < 0) {
        assert(info == NULL);
        assert(PyErr_Occurred());
        return NULL;
    }
    if (rc == 0) {
        assert(info == NULL);
        assert(!PyErr_Occurred());
        PyErr_Format(state->hashlib_unsupported_digestmod_error,
                     "unsupported hash type: %R", hash_info_ref);
        return NULL;
    }
    return info;
}

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

// --- One-shot HMAC-HASH interface -------------------------------------------

/*[clinic input]
_hmac.compute_digest

    key: object
    msg: object
    digestmod: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_digest_impl(PyObject *module, PyObject *key, PyObject *msg,
                          PyObject *digestmod)
/*[clinic end generated code: output=593ea8a175024c9a input=bd3be7c5b717c950]*/
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    const py_hmac_hinfo *info = find_hash_info(state, digestmod);
    if (info == NULL) {
        return NULL;
    }
    assert(info->api.compute_py != NULL);
    return info->api.compute_py(module, key, msg);
}

/*
 * One-shot HMAC-HASH using the given HACL_HID.
 *
 * The length of the key and message buffers must not exceed UINT32_MAX,
 * lest an OverflowError is raised. The Python implementation takes care
 * of dispatching to the OpenSSL implementation in this case.
 */
#define Py_HMAC_HACL_ONESHOT(HACL_HID, KEY, MSG)                    \
    do {                                                            \
        Py_buffer keyview, msgview;                                 \
        GET_BUFFER_VIEW_OR_ERROUT((KEY), &keyview);                 \
        if (!has_uint32_t_buffer_length(&keyview)) {                \
            PyBuffer_Release(&keyview);                             \
            PyErr_SetString(PyExc_OverflowError,                    \
                            "key length exceeds UINT32_MAX");       \
            return NULL;                                            \
        }                                                           \
        GET_BUFFER_VIEW_OR_ERROUT((MSG), &msgview);                 \
        if (!has_uint32_t_buffer_length(&msgview)) {                \
            PyBuffer_Release(&msgview);                             \
            PyBuffer_Release(&keyview);                             \
            PyErr_SetString(PyExc_OverflowError,                    \
                            "message length exceeds UINT32_MAX");   \
            return NULL;                                            \
        }                                                           \
        uint8_t out[Py_hmac_## HACL_HID ##_digest_size];            \
        Py_hmac_## HACL_HID ##_compute_func(                        \
            out,                                                    \
            (uint8_t *)keyview.buf, (uint32_t)keyview.len,          \
            (uint8_t *)msgview.buf, (uint32_t)msgview.len           \
        );                                                          \
        PyBuffer_Release(&msgview);                                 \
        PyBuffer_Release(&keyview);                                 \
        return PyBytes_FromStringAndSize(                           \
            (const char *)out,                                      \
            Py_hmac_## HACL_HID ##_digest_size                      \
        );                                                          \
    } while (0)

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
    Py_HMAC_HACL_ONESHOT(md5, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha1, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha2_224, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha2_256, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha2_384, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha2_512, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha3_224, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha3_256, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha3_384, key, msg);
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
    Py_HMAC_HACL_ONESHOT(sha3_512, key, msg);
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
    Py_HMAC_HACL_ONESHOT(blake2s_32, key, msg);
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
    Py_HMAC_HACL_ONESHOT(blake2b_32, key, msg);
}

// --- HMAC module methods ----------------------------------------------------

static PyMethodDef hmacmodule_methods[] = {
    /* one-shot dispatcher */
    _HMAC_COMPUTE_DIGEST_METHODDEF
    /* one-shot methods */
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

// --- HMAC static information table ------------------------------------------

/* Static information used to construct the hash table. */
static const py_hmac_hinfo py_hmac_static_hinfo[] = {
#define Py_HMAC_HINFO_HACL_API(HACL_HID)                                \
    {                                                                   \
        .compute = &Py_hmac_## HACL_HID ##_compute_func,                \
        .compute_py = &_hmac_compute_## HACL_HID ##_impl,               \
    }

#define Py_HMAC_HINFO_ENTRY(HACL_HID, HLIB_NAME)            \
    {                                                       \
        .name = Py_STRINGIFY(HACL_HID),                     \
        .p_name = NULL,                                     \
        .kind = Py_hmac_kind_hmac_ ## HACL_HID,             \
        .block_size = Py_hmac_## HACL_HID ##_block_size,    \
        .digest_size = Py_hmac_## HACL_HID ##_digest_size,  \
        .api = Py_HMAC_HINFO_HACL_API(HACL_HID),            \
        .hashlib_name = HLIB_NAME,                          \
        .openssl_name = Py_OpenSSL_LN_ ## HACL_HID,         \
        .refcnt = 0,                                        \
    }
    /* MD5 */
    Py_HMAC_HINFO_ENTRY(md5, "md5"),
    /* SHA-1 */
    Py_HMAC_HINFO_ENTRY(sha1, "sha1"),
    /* SHA-2 family */
    Py_HMAC_HINFO_ENTRY(sha2_224, "sha224"),
    Py_HMAC_HINFO_ENTRY(sha2_256, "sha256"),
    Py_HMAC_HINFO_ENTRY(sha2_384, "sha384"),
    Py_HMAC_HINFO_ENTRY(sha2_512, "sha512"),
    /* SHA-3 family */
    Py_HMAC_HINFO_ENTRY(sha3_224, NULL),
    Py_HMAC_HINFO_ENTRY(sha3_256, NULL),
    Py_HMAC_HINFO_ENTRY(sha3_384, NULL),
    Py_HMAC_HINFO_ENTRY(sha3_512, NULL),
    /* Blake family */
    Py_HMAC_HINFO_ENTRY(blake2s_32, "blake2s256"),
    Py_HMAC_HINFO_ENTRY(blake2b_32, "blake2b512"),
#undef Py_HMAC_HINFO_ENTRY
#undef Py_HMAC_HINFO_HACL_API
    /* sentinel */
    {
        NULL, NULL,
        Py_hmac_kind_unknown, 0, 0,
        {NULL, NULL},
        NULL, NULL,
        0
    },
};

static inline Py_uhash_t
py_hmac_hinfo_ht_hash(const void *name)
{
    return Py_HashBuffer(name, strlen((const char *)name));
}

static inline int
py_hmac_hinfo_ht_comp(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b) == 0;
}

static void
py_hmac_hinfo_ht_free(void *hinfo)
{
    py_hmac_hinfo *entry = (py_hmac_hinfo *)hinfo;
    assert(entry->p_name != NULL);
    if (--(entry->refcnt) == 0) {
        Py_CLEAR(entry->p_name);
        PyMem_Free(hinfo);
    }
}

static inline int
py_hmac_hinfo_ht_add(_Py_hashtable_t *table, const void *key, void *info)
{
    if (key == NULL || _Py_hashtable_get(table, key) != NULL) {
        return 0;
    }
    int ok = _Py_hashtable_set(table, key, info);
    return ok < 0 ? -1 : ok == 0;
}

static _Py_hashtable_t *
py_hmac_hinfo_ht_new(void)
{
    _Py_hashtable_t *table = _Py_hashtable_new_full(
        py_hmac_hinfo_ht_hash,
        py_hmac_hinfo_ht_comp,
        NULL,
        py_hmac_hinfo_ht_free,
        NULL
    );

    if (table == NULL) {
        return NULL;
    }

    for (const py_hmac_hinfo *e = py_hmac_static_hinfo; e->name != NULL; e++) {
        py_hmac_hinfo *value = PyMem_Malloc(sizeof(py_hmac_hinfo));
        if (value == NULL) {
            goto error;
        }

        memcpy(value, e, sizeof(py_hmac_hinfo));
        assert(value->p_name == NULL);
        value->refcnt = 0;

#define Py_HMAC_HINFO_LINK(KEY)                                 \
        do {                                                    \
            int rc = py_hmac_hinfo_ht_add(table, KEY, value);   \
            if (rc < 0) {                                       \
                PyMem_Free(value);                              \
                goto error;                                     \
            }                                                   \
            else if (rc == 1) {                                 \
                value->refcnt++;                                \
            }                                                   \
        } while (0)
        Py_HMAC_HINFO_LINK(e->name);
        Py_HMAC_HINFO_LINK(e->hashlib_name);
        Py_HMAC_HINFO_LINK(e->openssl_name);
#undef Py_HMAC_HINFO_LINK
        assert(value->refcnt > 0);
        value->p_name = PyUnicode_FromString(e->name);
        if (value->p_name == NULL) {
            PyMem_Free(value);
            goto error;
        }
    }

    return table;

error:
    _Py_hashtable_destroy(table);
    return NULL;
}

// --- HMAC module initialization and finalization functions ------------------

static int
hmacmodule_init_hash_info_table(hmacmodule_state *state)
{
    state->hinfo_table = py_hmac_hinfo_ht_new();
    if (state->hinfo_table == NULL) {
        // An exception other than a memory error can be raised
        // by PyUnicode_FromString() or _Py_hashtable_set() when
        // creating the hash table entries.
        if (!PyErr_Occurred()) {
            PyErr_NoMemory();
        }
        return -1;
    }
    return 0;
}

static int
hmacmodule_init_from_hashlib(hmacmodule_state *state)
{
    PyObject *_hashlib = PyImport_ImportModule("_hashlib");
    if (_hashlib == NULL) {
        return -1;
    }
#define IMPORT_FROM_HASHLIB(VAR, NAME)                  \
    do {                                                \
        (VAR) = PyObject_GetAttrString(_hashlib, NAME); \
        if ((VAR) == NULL) {                            \
            Py_DECREF(_hashlib);                        \
            return -1;                                  \
        }                                               \
    } while (0)

    IMPORT_FROM_HASHLIB(state->hashlib_constructs_mappingproxy,
                        "_constructors");
    IMPORT_FROM_HASHLIB(state->hashlib_unsupported_digestmod_error,
                        "UnsupportedDigestmodError");
#undef IMPORT_FROM_HASHLIB
    Py_DECREF(_hashlib);
    return 0;
}

static int
hmacmodule_init_strings(hmacmodule_state *state)
{
    state->str_lower = PyUnicode_FromString("lower");
    if (state->str_lower == NULL) {
        return -1;
    }
    return 0;
}

static int
hmacmodule_exec(PyObject *module)
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    if (hmacmodule_init_hash_info_table(state) < 0) {
        return -1;
    }
    if (hmacmodule_init_from_hashlib(state) < 0) {
        return -1;
    }
    if (hmacmodule_init_strings(state) < 0) {
        return -1;
    }
    return 0;
}

static int
hmacmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    hmacmodule_state *state = get_hmacmodule_state(mod);
    Py_VISIT(state->hashlib_constructs_mappingproxy);
    Py_VISIT(state->hashlib_unsupported_digestmod_error);
    Py_VISIT(state->str_lower);
    return 0;
}

static int
hmacmodule_clear(PyObject *mod)
{
    hmacmodule_state *state = get_hmacmodule_state(mod);
    if (state->hinfo_table != NULL) {
        _Py_hashtable_destroy(state->hinfo_table);
        state->hinfo_table = NULL;
    }
    Py_CLEAR(state->hashlib_constructs_mappingproxy);
    Py_CLEAR(state->hashlib_unsupported_digestmod_error);
    Py_CLEAR(state->str_lower);
    return 0;
}

static inline void
hmacmodule_free(void *mod)
{
    (void)hmacmodule_clear((PyObject *)mod);
}

static struct PyModuleDef_Slot hmacmodule_slots[] = {
    {Py_mod_exec, hmacmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _hmacmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_hmac",
    .m_size = sizeof(hmacmodule_state),
    .m_methods = hmacmodule_methods,
    .m_slots = hmacmodule_slots,
    .m_traverse = hmacmodule_traverse,
    .m_clear = hmacmodule_clear,
    .m_free = hmacmodule_free,
};

PyMODINIT_FUNC
PyInit__hmac(void)
{
    return PyModuleDef_Init(&_hmacmodule);
}

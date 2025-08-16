/* Module that wraps all OpenSSL hash algorithms */

/*
 * Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
 * Licensed to PSF under a Contributor Agreement.
 *
 * Derived from a skeleton of shamodule.c containing work performed by:
 *
 * Andrew Kuchling (amk@amk.ca)
 * Greg Stein (gstein@lyra.org)
 *
 */

/* Don't warn about deprecated functions, */
#ifndef OPENSSL_API_COMPAT
  // 0x10101000L == 1.1.1, 30000 == 3.0.0
  #define OPENSSL_API_COMPAT 0x10101000L
#endif
#define OPENSSL_NO_DEPRECATED 1

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_hashtable.h"
#include "pycore_strhex.h"               // _Py_strhex()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_PTR_RELAXED
#include "hashlib.h"

/* EVP is the preferred interface to hashing in OpenSSL */
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>              // FIPS_mode()
/* We use the object interface to discover what hashes OpenSSL supports. */
#include <openssl/objects.h>
#include <openssl/err.h>

#include <stdbool.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#  define Py_HAS_OPENSSL3_SUPPORT
#endif

#ifndef OPENSSL_THREADS
#  error "OPENSSL_THREADS is not defined, Python requires thread-safe OpenSSL"
#endif

#define MUNCH_SIZE INT_MAX

#if defined(NID_sha3_224) && defined(NID_sha3_256) && defined(NID_sha3_384) && defined(NID_sha3_512)
#define PY_OPENSSL_HAS_SHA3 1
#endif
#if defined(NID_shake128) || defined(NID_shake256)
#define PY_OPENSSL_HAS_SHAKE 1
#endif
#if defined(NID_blake2s256) || defined(NID_blake2b512)
#define PY_OPENSSL_HAS_BLAKE2 1
#endif

#ifdef Py_HAS_OPENSSL3_SUPPORT
#define PY_EVP_MD EVP_MD
#define PY_EVP_MD_fetch(algorithm, properties) EVP_MD_fetch(NULL, algorithm, properties)
#define PY_EVP_MD_up_ref(md) EVP_MD_up_ref(md)
#define PY_EVP_MD_free(md) EVP_MD_free(md)

#define PY_EVP_MD_CTX_md(CTX)   EVP_MD_CTX_get0_md(CTX)
#else
#define PY_EVP_MD const EVP_MD
#define PY_EVP_MD_fetch(algorithm, properties) EVP_get_digestbyname(algorithm)
#define PY_EVP_MD_up_ref(md) do {} while(0)
#define PY_EVP_MD_free(md) do {} while(0)

#define PY_EVP_MD_CTX_md(CTX)   EVP_MD_CTX_md(CTX)
#endif

/*
 * Return 1 if *md* is an extendable-output Function (XOF) and 0 otherwise.
 * SHAKE128 and SHAKE256 are XOF functions but not BLAKE2B algorithms.
 *
 * This is a backport of the EVP_MD_xof() helper added in OpenSSL 3.4.
 */
static inline int
PY_EVP_MD_xof(PY_EVP_MD *md)
{
    return md != NULL && ((EVP_MD_flags(md) & EVP_MD_FLAG_XOF) != 0);
}

/* hash alias map and fast lookup
 *
 * Map between Python's preferred names and OpenSSL internal names. Maintain
 * cache of fetched EVP MD objects. The EVP_get_digestbyname() and
 * EVP_MD_fetch() API calls have a performance impact.
 *
 * The py_hashentry_t items are stored in a _Py_hashtable_t with py_name and
 * py_alias as keys.
 */

typedef enum Py_hash_type {
    Py_ht_evp,              // usedforsecurity=True / default
    Py_ht_evp_nosecurity,   // usedforsecurity=False
    Py_ht_mac,              // HMAC
    Py_ht_pbkdf2,           // PKBDF2
} Py_hash_type;

typedef struct {
    const char *py_name;
    const char *py_alias;
    const char *ossl_name;
    int ossl_nid;
    int refcnt;
    PY_EVP_MD *evp;
    PY_EVP_MD *evp_nosecurity;
} py_hashentry_t;

// Fundamental to TLS, assumed always present in any libcrypto:
#define Py_hash_md5 "md5"
#define Py_hash_sha1 "sha1"
#define Py_hash_sha224 "sha224"
#define Py_hash_sha256 "sha256"
#define Py_hash_sha384 "sha384"
#define Py_hash_sha512 "sha512"

// Not all OpenSSL-like libcrypto libraries provide these:
#if defined(NID_sha512_224)
# define Py_hash_sha512_224 "sha512_224"
#endif
#if defined(NID_sha512_256)
# define Py_hash_sha512_256 "sha512_256"
#endif
#if defined(NID_sha3_224)
# define Py_hash_sha3_224 "sha3_224"
#endif
#if defined(NID_sha3_256)
# define Py_hash_sha3_256 "sha3_256"
#endif
#if defined(NID_sha3_384)
# define Py_hash_sha3_384 "sha3_384"
#endif
#if defined(NID_sha3_512)
# define Py_hash_sha3_512 "sha3_512"
#endif
#if defined(NID_shake128)
# define Py_hash_shake_128 "shake_128"
#endif
#if defined(NID_shake256)
# define Py_hash_shake_256 "shake_256"
#endif
#if defined(NID_blake2s256)
# define Py_hash_blake2s "blake2s"
#endif
#if defined(NID_blake2b512)
# define Py_hash_blake2b "blake2b"
#endif

#define PY_HASH_ENTRY(py_name, py_alias, ossl_name, ossl_nid) \
    {py_name, py_alias, ossl_name, ossl_nid, 0, NULL, NULL}

static const py_hashentry_t py_hashes[] = {
    /* md5 */
    PY_HASH_ENTRY(Py_hash_md5, "MD5", SN_md5, NID_md5),
    /* sha1 */
    PY_HASH_ENTRY(Py_hash_sha1, "SHA1", SN_sha1, NID_sha1),
    /* sha2 family */
    PY_HASH_ENTRY(Py_hash_sha224, "SHA224", SN_sha224, NID_sha224),
    PY_HASH_ENTRY(Py_hash_sha256, "SHA256", SN_sha256, NID_sha256),
    PY_HASH_ENTRY(Py_hash_sha384, "SHA384", SN_sha384, NID_sha384),
    PY_HASH_ENTRY(Py_hash_sha512, "SHA512", SN_sha512, NID_sha512),
    /* truncated sha2 */
#ifdef Py_hash_sha512_224
    PY_HASH_ENTRY(Py_hash_sha512_224, "SHA512_224", SN_sha512_224, NID_sha512_224),
#endif
#ifdef Py_hash_sha512_256
    PY_HASH_ENTRY(Py_hash_sha512_256, "SHA512_256", SN_sha512_256, NID_sha512_256),
#endif
    /* sha3 */
#ifdef Py_hash_sha3_224
    PY_HASH_ENTRY(Py_hash_sha3_224, NULL, SN_sha3_224, NID_sha3_224),
#endif
#ifdef Py_hash_sha3_256
    PY_HASH_ENTRY(Py_hash_sha3_256, NULL, SN_sha3_256, NID_sha3_256),
#endif
#ifdef Py_hash_sha3_384
    PY_HASH_ENTRY(Py_hash_sha3_384, NULL, SN_sha3_384, NID_sha3_384),
#endif
#ifdef Py_hash_sha3_512
    PY_HASH_ENTRY(Py_hash_sha3_512, NULL, SN_sha3_512, NID_sha3_512),
#endif
    /* sha3 shake */
#ifdef Py_hash_shake_128
    PY_HASH_ENTRY(Py_hash_shake_128, NULL, SN_shake128, NID_shake128),
#endif
#ifdef Py_hash_shake_256
    PY_HASH_ENTRY(Py_hash_shake_256, NULL, SN_shake256, NID_shake256),
#endif
    /* blake2 digest */
#ifdef Py_hash_blake2s
    PY_HASH_ENTRY(Py_hash_blake2s, "blake2s256", SN_blake2s256, NID_blake2s256),
#endif
#ifdef Py_hash_blake2b
    PY_HASH_ENTRY(Py_hash_blake2b, "blake2b512", SN_blake2b512, NID_blake2b512),
#endif
    PY_HASH_ENTRY(NULL, NULL, NULL, 0),
};

static Py_uhash_t
py_hashentry_t_hash_name(const void *key) {
    return Py_HashBuffer(key, strlen((const char *)key));
}

static int
py_hashentry_t_compare_name(const void *key1, const void *key2) {
    return strcmp((const char *)key1, (const char *)key2) == 0;
}

static void
py_hashentry_t_destroy_value(void *entry) {
    py_hashentry_t *h = (py_hashentry_t *)entry;
    if (--(h->refcnt) == 0) {
        if (h->evp != NULL) {
            PY_EVP_MD_free(h->evp);
            h->evp = NULL;
        }
        if (h->evp_nosecurity != NULL) {
            PY_EVP_MD_free(h->evp_nosecurity);
            h->evp_nosecurity = NULL;
        }
        PyMem_Free(entry);
    }
}

static _Py_hashtable_t *
py_hashentry_table_new(void) {
    _Py_hashtable_t *ht = _Py_hashtable_new_full(
        py_hashentry_t_hash_name,
        py_hashentry_t_compare_name,
        NULL,
        py_hashentry_t_destroy_value,
        NULL
    );
    if (ht == NULL) {
        return NULL;
    }

    for (const py_hashentry_t *h = py_hashes; h->py_name != NULL; h++) {
        py_hashentry_t *entry = (py_hashentry_t *)PyMem_Malloc(sizeof(py_hashentry_t));
        if (entry == NULL) {
            goto error;
        }
        memcpy(entry, h, sizeof(py_hashentry_t));

        if (_Py_hashtable_set(ht, (const void*)entry->py_name, (void*)entry) < 0) {
            PyMem_Free(entry);
            goto error;
        }
        entry->refcnt = 1;

        if (h->py_alias != NULL) {
            if (_Py_hashtable_set(ht, (const void*)entry->py_alias, (void*)entry) < 0) {
                PyMem_Free(entry);
                goto error;
            }
            entry->refcnt++;
        }
    }

    return ht;
  error:
    _Py_hashtable_destroy(ht);
    return NULL;
}

// --- Module state -----------------------------------------------------------

static PyModuleDef _hashlibmodule;

typedef struct {
    PyTypeObject *HASH_type;    // based on EVP_MD
    PyTypeObject *HMAC_type;
#ifdef PY_OPENSSL_HAS_SHAKE
    PyTypeObject *HASHXOF_type; // based on EVP_MD
#endif
    PyObject *constructs;
    PyObject *unsupported_digestmod_error;
    _Py_hashtable_t *hashtable;
} _hashlibstate;

static inline _hashlibstate*
get_hashlib_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_hashlibstate *)state;
}

// --- Module objects ---------------------------------------------------------

typedef struct {
    HASHLIB_OBJECT_HEAD
    EVP_MD_CTX *ctx;    /* OpenSSL message digest context */
} HASHobject;

#define HASHobject_CAST(op) ((HASHobject *)(op))

typedef struct {
    HASHLIB_OBJECT_HEAD
    HMAC_CTX *ctx;            /* OpenSSL hmac context */
} HMACobject;

#define HMACobject_CAST(op) ((HMACobject *)(op))

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
module _hashlib
class _hashlib.HASH "HASHobject *" "&PyType_Type"
class _hashlib.HASHXOF "HASHobject *" "&PyType_Type"
class _hashlib.HMAC "HMACobject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6b5c9ce5c28bdc58]*/

#include "clinic/_hashopenssl.c.h"

/* LCOV_EXCL_START */

/* Thin wrapper around ERR_reason_error_string() returning non-NULL text. */
static const char *
py_wrapper_ERR_reason_error_string(unsigned long errcode)
{
    const char *reason = ERR_reason_error_string(errcode);
    return reason ? reason : "no reason";
}

#ifdef Py_HAS_OPENSSL3_SUPPORT
/*
 * Set an exception with additional information.
 *
 * This is only useful in OpenSSL 3.0 and later as the default reason
 * usually lacks information and function locations are no longer encoded
 * in the error code.
 */
static void
set_exception_with_ssl_errinfo(PyObject *exc_type, PyObject *exc_text,
                               const char *lib, const char *reason)
{
    assert(exc_type != NULL);
    assert(exc_text != NULL);
    if (lib && reason) {
        PyErr_Format(exc_type, "[%s] %U (reason: %s)", lib, exc_text, reason);
    }
    else if (lib) {
        PyErr_Format(exc_type, "[%s] %U", lib, exc_text);
    }
    else if (reason) {
        PyErr_Format(exc_type, "%U (reason: %s)", exc_text, reason);
    }
    else {
        PyErr_SetObject(exc_type, exc_text);
    }
}
#endif

/* Set an exception of given type using the given OpenSSL error code. */
static void
set_ssl_exception_from_errcode(PyObject *exc_type, unsigned long errcode)
{
    assert(exc_type != NULL);
    assert(errcode != 0);

    /* ERR_ERROR_STRING(3) ensures that the messages below are ASCII */
    const char *lib = ERR_lib_error_string(errcode);
#ifdef Py_HAS_OPENSSL3_SUPPORT
    // Since OpenSSL 3.0, ERR_func_error_string() always returns NULL.
    const char *func = NULL;
#else
    const char *func = ERR_func_error_string(errcode);
#endif
    const char *reason = py_wrapper_ERR_reason_error_string(errcode);

    if (lib && func) {
        PyErr_Format(exc_type, "[%s: %s] %s", lib, func, reason);
    }
    else if (lib) {
        PyErr_Format(exc_type, "[%s] %s", lib, reason);
    }
    else {
        PyErr_SetString(exc_type, reason);
    }
}

/*
 * Get an appropriate exception type for the given OpenSSL error code.
 *
 * The exception type depends on the error code reason.
 */
static PyObject *
get_smart_ssl_exception_type(unsigned long errcode, PyObject *default_exc_type)
{
    switch (ERR_GET_REASON(errcode)) {
        case ERR_R_MALLOC_FAILURE:
            return PyExc_MemoryError;
        default:
            return default_exc_type;
    }
}

/*
 * Set an exception of given type.
 *
 * By default, the exception's message is constructed by using the last SSL
 * error that occurred. If no error occurred, the 'fallback_message' is used
 * to create an exception message.
 */
static void
raise_ssl_error(PyObject *exc_type, const char *fallback_message)
{
    assert(fallback_message != NULL);
    unsigned long errcode = ERR_peek_last_error();
    if (errcode) {
        ERR_clear_error();
        set_ssl_exception_from_errcode(exc_type, errcode);
    }
    else {
        PyErr_SetString(exc_type, fallback_message);
    }
}

/* Same as raise_ssl_error() but with a C-style formatted message. */
static void
raise_ssl_error_f(PyObject *exc_type, const char *fallback_format, ...)
{
    assert(fallback_format != NULL);
    unsigned long errcode = ERR_peek_last_error();
    if (errcode) {
        ERR_clear_error();
        set_ssl_exception_from_errcode(exc_type, errcode);
    }
    else {
        va_list vargs;
        va_start(vargs, fallback_format);
        PyErr_FormatV(exc_type, fallback_format, vargs);
        va_end(vargs);
    }
}

/* Same as raise_ssl_error_f() with smart exception types. */
static void
raise_smart_ssl_error_f(PyObject *exc_type, const char *fallback_format, ...)
{
    unsigned long errcode = ERR_peek_last_error();
    if (errcode) {
        ERR_clear_error();
        exc_type = get_smart_ssl_exception_type(errcode, exc_type);
        set_ssl_exception_from_errcode(exc_type, errcode);
    }
    else {
        va_list vargs;
        va_start(vargs, fallback_format);
        PyErr_FormatV(exc_type, fallback_format, vargs);
        va_end(vargs);
    }
}

/*
 * Raise a ValueError with a default message after an error occurred.
 * It can also be used without previous calls to SSL built-in functions.
 */
static inline void
notify_ssl_error_occurred(const char *message)
{
    raise_ssl_error(PyExc_ValueError, message);
}

/* Same as notify_ssl_error_occurred() for failed OpenSSL functions. */
static inline void
notify_ssl_error_occurred_in(const char *funcname)
{
    raise_ssl_error_f(PyExc_ValueError,
                      "error in OpenSSL function %s()", funcname);
}

/* Same as notify_ssl_error_occurred_in() with smart exception types. */
static inline void
notify_smart_ssl_error_occurred_in(const char *funcname)
{
    raise_smart_ssl_error_f(PyExc_ValueError,
                            "error in OpenSSL function %s()", funcname);
}

#ifdef Py_HAS_OPENSSL3_SUPPORT
static void
raise_unsupported_algorithm_impl(PyObject *exc_type,
                                 const char *fallback_format,
                                 const void *format_arg)
{
    // Since OpenSSL 3.0, if the algorithm is not supported or fetching fails,
    // the reason lacks the algorithm name.
    int errcode = ERR_peek_last_error();
    switch (ERR_GET_REASON(errcode)) {
        case ERR_R_UNSUPPORTED: {
            PyObject *text = PyUnicode_FromFormat(fallback_format, format_arg);
            if (text != NULL) {
                const char *lib = ERR_lib_error_string(errcode);
                set_exception_with_ssl_errinfo(exc_type, text, lib, NULL);
                Py_DECREF(text);
            }
            break;
        }
        case ERR_R_FETCH_FAILED: {
            PyObject *text = PyUnicode_FromFormat(fallback_format, format_arg);
            if (text != NULL) {
                const char *lib = ERR_lib_error_string(errcode);
                const char *reason = ERR_reason_error_string(errcode);
                set_exception_with_ssl_errinfo(exc_type, text, lib, reason);
                Py_DECREF(text);
            }
            break;
        }
        default:
            raise_ssl_error_f(exc_type, fallback_format, format_arg);
            break;
    }
    assert(PyErr_Occurred());
}
#else
/* Before OpenSSL 3.0, error messages included enough information. */
#define raise_unsupported_algorithm_impl    raise_ssl_error_f
#endif

static inline void
raise_unsupported_algorithm_error(_hashlibstate *state, PyObject *digestmod)
{
    raise_unsupported_algorithm_impl(
        state->unsupported_digestmod_error,
        HASHLIB_UNSUPPORTED_ALGORITHM,
        digestmod
    );
}

static inline void
raise_unsupported_str_algorithm_error(_hashlibstate *state, const char *name)
{
    raise_unsupported_algorithm_impl(
        state->unsupported_digestmod_error,
        HASHLIB_UNSUPPORTED_STR_ALGORITHM,
        name
    );
}

#undef raise_unsupported_algorithm_impl
/* LCOV_EXCL_STOP */

/*
 * OpenSSL provides a way to go from NIDs to digest names for hash functions
 * but lacks this granularity for MAC objects where it is not possible to get
 * the underlying digest name (only the block size and digest size are allowed
 * to be recovered).
 *
 * In addition, OpenSSL aliases pollute the list of known digest names
 * as OpenSSL appears to have its own definition of alias. In particular,
 * the resulting list still contains duplicate and alternate names for several
 * algorithms.
 *
 * Therefore, digest names, whether they are used by hash functions or HMAC,
 * are handled through EVP_MD objects or directly by using some NID.
 */

/* Get a cached entry by OpenSSL NID. */
static const py_hashentry_t *
get_hashentry_by_nid(int nid)
{
    for (const py_hashentry_t *h = py_hashes; h->py_name != NULL; h++) {
        if (h->ossl_nid == nid) {
            return h;
        }
    }
    return NULL;
}

/*
 * Convert the NID to a string via OBJ_nid2*() functions.
 *
 * If 'nid' cannot be resolved, set an exception and return NULL.
 */
static const char *
get_asn1_utf8name_by_nid(int nid)
{
    const char *name = OBJ_nid2ln(nid);
    if (name == NULL) {
        // In OpenSSL 3.0 and later, OBJ_nid*() are thread-safe and may raise.
        assert(ERR_peek_last_error() != 0);
        if (ERR_GET_REASON(ERR_peek_last_error()) != OBJ_R_UNKNOWN_NID) {
            goto error;
        }
        // fallback to short name and unconditionally propagate errors
        name = OBJ_nid2sn(nid);
        if (name == NULL) {
            goto error;
        }
    }
    return name;

error:
    raise_ssl_error_f(PyExc_ValueError, "cannot resolve NID %d", nid);
    return NULL;
}

/*
 * Convert the NID to an OpenSSL digest name.
 *
 * On error, set an exception and return NULL.
 */
static const char *
get_hashlib_utf8name_by_nid(int nid)
{
    const py_hashentry_t *e = get_hashentry_by_nid(nid);
    return e ? e->py_name : get_asn1_utf8name_by_nid(nid);
}

/* Same as get_hashlib_utf8name_by_nid() but using an EVP_MD object. */
static const char *
get_hashlib_utf8name_by_evp_md(const EVP_MD *md)
{
    assert(md != NULL);
    return get_hashlib_utf8name_by_nid(EVP_MD_nid(md));
}

/*
 * Return 1 if the property query clause [1] must be "-fips" and 0 otherwise.
 *
 * [1] https://docs.openssl.org/master/man7/property
 */
static inline int
disable_fips_property(Py_hash_type py_ht)
{
    switch (py_ht) {
        case Py_ht_evp:
        case Py_ht_mac:
        case Py_ht_pbkdf2:
            return 0;
        case Py_ht_evp_nosecurity:
            return 1;
        default:
            Py_FatalError("unsupported hash type");
    }
}

/*
 * Get a new reference to an EVP_MD object described by name and purpose.
 *
 * If 'name' is an OpenSSL indexed name, the return value is cached.
 */
static PY_EVP_MD *
get_openssl_evp_md_by_utf8name(_hashlibstate *state, const char *name,
                               Py_hash_type py_ht)
{
    PY_EVP_MD *digest = NULL, *other_digest = NULL;
    py_hashentry_t *entry = _Py_hashtable_get(state->hashtable, name);

    if (entry != NULL) {
        if (!disable_fips_property(py_ht)) {
            digest = FT_ATOMIC_LOAD_PTR_RELAXED(entry->evp);
            if (digest == NULL) {
                digest = PY_EVP_MD_fetch(entry->ossl_name, NULL);
#ifdef Py_GIL_DISABLED
                // exchange just in case another thread did same thing at same time
                other_digest = _Py_atomic_exchange_ptr(&entry->evp, (void *)digest);
#else
                entry->evp = digest;
#endif
            }
        }
        else {
            digest = FT_ATOMIC_LOAD_PTR_RELAXED(entry->evp_nosecurity);
            if (digest == NULL) {
                digest = PY_EVP_MD_fetch(entry->ossl_name, "-fips");
#ifdef Py_GIL_DISABLED
                // exchange just in case another thread did same thing at same time
                other_digest = _Py_atomic_exchange_ptr(&entry->evp_nosecurity, (void *)digest);
#else
                entry->evp_nosecurity = digest;
#endif
            }
        }
        // if another thread same thing at same time make sure we got same ptr
        assert(other_digest == NULL || other_digest == digest);
        if (digest != NULL && other_digest == NULL) {
            PY_EVP_MD_up_ref(digest);
        }
    }
    else {
        // Fall back for looking up an unindexed OpenSSL specific name.
        const char *props = disable_fips_property(py_ht) ? "-fips" : NULL;
        (void)props;  // will only be used in OpenSSL 3.0 and later
        digest = PY_EVP_MD_fetch(name, props);
    }
    if (digest == NULL) {
        raise_unsupported_str_algorithm_error(state, name);
        return NULL;
    }
    return digest;
}

/*
 * Get a new reference to an EVP_MD described by 'digestmod' and purpose.
 *
 * On error, set an exception and return NULL.
 *
 * Parameters
 *
 *      digestmod   A digest name or a _hashopenssl builtin function
 *      py_ht       The message digest purpose.
 */
static PY_EVP_MD *
get_openssl_evp_md(_hashlibstate *state, PyObject *digestmod, Py_hash_type py_ht)
{
    const char *name;
    if (PyUnicode_Check(digestmod)) {
        name = PyUnicode_AsUTF8(digestmod);
    }
    else {
        PyObject *dict = state->constructs;
        assert(dict != NULL);
        PyObject *borrowed_ref = PyDict_GetItemWithError(dict, digestmod);
        name = borrowed_ref == NULL ? NULL : PyUnicode_AsUTF8(borrowed_ref);
    }
    if (name == NULL) {
        if (!PyErr_Occurred()) {
            raise_unsupported_algorithm_error(state, digestmod);
        }
        return NULL;
    }
    return get_openssl_evp_md_by_utf8name(state, name, py_ht);
}

// --- OpenSSL HASH wrappers --------------------------------------------------

/* Thin wrapper around EVP_MD_CTX_new() which sets an exception on failure. */
static EVP_MD_CTX *
py_wrapper_EVP_MD_CTX_new(void)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    return ctx;
}

// --- HASH interface ---------------------------------------------------------

static HASHobject *
new_hash_object(PyTypeObject *type)
{
    HASHobject *retval = PyObject_New(HASHobject, type);
    if (retval == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(retval);

    retval->ctx = py_wrapper_EVP_MD_CTX_new();
    if (retval->ctx == NULL) {
        Py_DECREF(retval);
        return NULL;
    }

    return retval;
}

static int
_hashlib_HASH_hash(HASHobject *self, const void *vp, Py_ssize_t len)
{
    unsigned int process;
    const unsigned char *cp = (const unsigned char *)vp;
    while (0 < len) {
        if (len > (Py_ssize_t)MUNCH_SIZE)
            process = MUNCH_SIZE;
        else
            process = Py_SAFE_DOWNCAST(len, Py_ssize_t, unsigned int);
        if (!EVP_DigestUpdate(self->ctx, (const void*)cp, process)) {
            notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_DigestUpdate));
            return -1;
        }
        len -= process;
        cp += process;
    }
    return 0;
}

/* Internal methods for a hash object */

static void
_hashlib_HASH_dealloc(PyObject *op)
{
    HASHobject *self = HASHobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    EVP_MD_CTX_free(self->ctx);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static int
_hashlib_HASH_copy_locked(HASHobject *self, EVP_MD_CTX *new_ctx_p)
{
    int result;
    HASHLIB_ACQUIRE_LOCK(self);
    result = EVP_MD_CTX_copy(new_ctx_p, self->ctx);
    HASHLIB_RELEASE_LOCK(self);
    if (result == 0) {
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(EVP_MD_CTX_copy));
        return -1;
    }
    return 0;
}

/* External methods for a hash object */

/*[clinic input]
_hashlib.HASH.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_hashlib_HASH_copy_impl(HASHobject *self)
/*[clinic end generated code: output=2545541af18d53d7 input=814b19202cd08a26]*/
{
    HASHobject *newobj;

    if ((newobj = new_hash_object(Py_TYPE(self))) == NULL)
        return NULL;

    if (_hashlib_HASH_copy_locked(self, newobj->ctx) < 0) {
        Py_DECREF(newobj);
        return NULL;
    }
    return (PyObject *)newobj;
}

static Py_ssize_t
_hashlib_HASH_digest_compute(HASHobject *self, unsigned char *digest)
{
    EVP_MD_CTX *ctx = py_wrapper_EVP_MD_CTX_new();
    if (ctx == NULL) {
        return -1;
    }
    if (_hashlib_HASH_copy_locked(self, ctx) < 0) {
        goto error;
    }
    Py_ssize_t digest_size = EVP_MD_CTX_size(ctx);
    if (!EVP_DigestFinal(ctx, digest, NULL)) {
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_DigestFinal));
        goto error;
    }
    EVP_MD_CTX_free(ctx);
    return digest_size;

error:
    EVP_MD_CTX_free(ctx);
    return -1;
}

/*[clinic input]
_hashlib.HASH.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_hashlib_HASH_digest_impl(HASHobject *self)
/*[clinic end generated code: output=3fc6f9671d712850 input=d8d528d6e50af0de]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    Py_ssize_t n = _hashlib_HASH_digest_compute(self, digest);
    return n < 0 ? NULL : PyBytes_FromStringAndSize((const char *)digest, n);
}

/*[clinic input]
_hashlib.HASH.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_hashlib_HASH_hexdigest_impl(HASHobject *self)
/*[clinic end generated code: output=1b8e60d9711e7f4d input=ae7553f78f8372d8]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    Py_ssize_t n = _hashlib_HASH_digest_compute(self, digest);
    return n < 0 ? NULL : _Py_strhex((const char *)digest, n);
}

/*[clinic input]
_hashlib.HASH.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
_hashlib_HASH_update_impl(HASHobject *self, PyObject *obj)
/*[clinic end generated code: output=62ad989754946b86 input=aa1ce20e3f92ceb6]*/
{
    int result;
    Py_buffer view;
    GET_BUFFER_VIEW_OR_ERROUT(obj, &view);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, view.len,
        result = _hashlib_HASH_hash(self, view.buf, view.len)
    );
    PyBuffer_Release(&view);
    return result < 0 ? NULL : Py_None;
}

static PyMethodDef HASH_methods[] = {
    _HASHLIB_HASH_COPY_METHODDEF
    _HASHLIB_HASH_DIGEST_METHODDEF
    _HASHLIB_HASH_HEXDIGEST_METHODDEF
    _HASHLIB_HASH_UPDATE_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static PyObject *
_hashlib_HASH_get_blocksize(PyObject *op, void *Py_UNUSED(closure))
{
    HASHobject *self = HASHobject_CAST(op);
    long block_size = EVP_MD_CTX_block_size(self->ctx);
    return PyLong_FromLong(block_size);
}

static PyObject *
_hashlib_HASH_get_digestsize(PyObject *op, void *Py_UNUSED(closure))
{
    HASHobject *self = HASHobject_CAST(op);
    long size = EVP_MD_CTX_size(self->ctx);
    return PyLong_FromLong(size);
}

static PyObject *
_hashlib_HASH_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    HASHobject *self = HASHobject_CAST(op);
    const EVP_MD *md = PY_EVP_MD_CTX_md(self->ctx);
    if (md == NULL) {
        notify_ssl_error_occurred("missing EVP_MD for HASH context");
        return NULL;
    }
    const char *name = get_hashlib_utf8name_by_evp_md(md);
    assert(name != NULL || PyErr_Occurred());
    return name == NULL ? NULL : PyUnicode_FromString(name);
}

static PyGetSetDef HASH_getsets[] = {
    {"digest_size", _hashlib_HASH_get_digestsize, NULL, NULL, NULL},
    {"block_size", _hashlib_HASH_get_blocksize, NULL, NULL, NULL},
    {"name", _hashlib_HASH_get_name, NULL, NULL, PyDoc_STR("algorithm name.")},
    {NULL}  /* Sentinel */
};


static PyObject *
_hashlib_HASH_repr(PyObject *self)
{
    PyObject *name = _hashlib_HASH_get_name(self, NULL);
    if (name == NULL) {
        return NULL;
    }
    PyObject *repr = PyUnicode_FromFormat("<%U %T object @ %p>",
                                          name, self, self);
    Py_DECREF(name);
    return repr;
}

PyDoc_STRVAR(HASHobject_type_doc,
"HASH(name, string=b\'\')\n"
"--\n"
"\n"
"A hash is an object used to calculate a checksum of a string of information.\n"
"\n"
"Methods:\n"
"\n"
"update() -- updates the current digest with an additional string\n"
"digest() -- return the current digest value\n"
"hexdigest() -- return the current digest as a string of hexadecimal digits\n"
"copy() -- return a copy of the current hash object\n"
"\n"
"Attributes:\n"
"\n"
"name -- the hash algorithm being used by this object\n"
"digest_size -- number of bytes in this hashes output");

static PyType_Slot HASHobject_type_slots[] = {
    {Py_tp_dealloc, _hashlib_HASH_dealloc},
    {Py_tp_repr, _hashlib_HASH_repr},
    {Py_tp_doc, (char *)HASHobject_type_doc},
    {Py_tp_methods, HASH_methods},
    {Py_tp_getset, HASH_getsets},
    {0, 0},
};

static PyType_Spec HASHobject_type_spec = {
    .name = "_hashlib.HASH",
    .basicsize = sizeof(HASHobject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
    ),
    .slots = HASHobject_type_slots
};

#ifdef PY_OPENSSL_HAS_SHAKE

/*[clinic input]
_hashlib.HASHXOF.digest

  length: Py_ssize_t

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_hashlib_HASHXOF_digest_impl(HASHobject *self, Py_ssize_t length)
/*[clinic end generated code: output=dcb09335dd2fe908 input=3eb034ce03c55b21]*/
{
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;

    if (length < 0) {
        PyErr_SetString(PyExc_ValueError, "negative digest length");
        return NULL;
    }

    if (length == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }

    retval = PyBytes_FromStringAndSize(NULL, length);
    if (retval == NULL) {
        return NULL;
    }

    temp_ctx = py_wrapper_EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        Py_DECREF(retval);
        return NULL;
    }

    if (_hashlib_HASH_copy_locked(self, temp_ctx) < 0) {
        goto error;
    }
    if (!EVP_DigestFinalXOF(temp_ctx,
                            (unsigned char*)PyBytes_AS_STRING(retval),
                            length))
    {
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_DigestFinalXOF));
        goto error;
    }

    EVP_MD_CTX_free(temp_ctx);
    return retval;

error:
    Py_DECREF(retval);
    EVP_MD_CTX_free(temp_ctx);
    return NULL;
}

/*[clinic input]
_hashlib.HASHXOF.hexdigest

    length: Py_ssize_t

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_hashlib_HASHXOF_hexdigest_impl(HASHobject *self, Py_ssize_t length)
/*[clinic end generated code: output=519431cafa014f39 input=0e58f7238adb7ab8]*/
{
    unsigned char *digest;
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;

    if (length < 0) {
        PyErr_SetString(PyExc_ValueError, "negative digest length");
        return NULL;
    }

    if (length == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }

    digest = (unsigned char*)PyMem_Malloc(length);
    if (digest == NULL) {
        (void)PyErr_NoMemory();
        return NULL;
    }

    temp_ctx = py_wrapper_EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyMem_Free(digest);
        return NULL;
    }

    /* Get the raw (binary) digest value */
    if (_hashlib_HASH_copy_locked(self, temp_ctx) < 0) {
        goto error;
    }
    if (!EVP_DigestFinalXOF(temp_ctx, digest, length)) {
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_DigestFinalXOF));
        goto error;
    }

    EVP_MD_CTX_free(temp_ctx);

    retval = _Py_strhex((const char *)digest, length);
    PyMem_Free(digest);
    return retval;

error:
    PyMem_Free(digest);
    EVP_MD_CTX_free(temp_ctx);
    return NULL;
}

static PyMethodDef HASHXOFobject_methods[] = {
    _HASHLIB_HASHXOF_DIGEST_METHODDEF
    _HASHLIB_HASHXOF_HEXDIGEST_METHODDEF
    {NULL, NULL}  /* sentinel */
};


static PyObject *
_hashlib_HASHXOF_digest_size(PyObject *Py_UNUSED(self),
                             void *Py_UNUSED(closure))
{
    return PyLong_FromLong(0);
}

static PyGetSetDef HASHXOFobject_getsets[] = {
    {"digest_size", _hashlib_HASHXOF_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(HASHXOFobject_type_doc,
"HASHXOF(name, string=b\'\')\n"
"--\n"
"\n"
"A hash is an object used to calculate a checksum of a string of information.\n"
"\n"
"Methods:\n"
"\n"
"update() -- updates the current digest with an additional string\n"
"digest(length) -- return the current digest value\n"
"hexdigest(length) -- return the current digest as a string of hexadecimal digits\n"
"copy() -- return a copy of the current hash object\n"
"\n"
"Attributes:\n"
"\n"
"name -- the hash algorithm being used by this object\n"
"digest_size -- number of bytes in this hashes output");

static PyType_Slot HASHXOFobject_type_slots[] = {
    {Py_tp_doc, (char *)HASHXOFobject_type_doc},
    {Py_tp_methods, HASHXOFobject_methods},
    {Py_tp_getset, HASHXOFobject_getsets},
    {0, 0},
};

static PyType_Spec HASHXOFobject_type_spec = {
    .name = "_hashlib.HASHXOF",
    .basicsize = sizeof(HASHobject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
    ),
    .slots = HASHXOFobject_type_slots
};


#endif

static PyObject *
_hashlib_HASH(_hashlibstate *state, const char *digestname, PyObject *data_obj,
              int usedforsecurity)
{
    Py_buffer view = { 0 };
    PY_EVP_MD *digest = NULL;
    PyTypeObject *type;
    HASHobject *self = NULL;

    if (data_obj != NULL) {
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);
    }

    Py_hash_type purpose = usedforsecurity ? Py_ht_evp : Py_ht_evp_nosecurity;
    digest = get_openssl_evp_md_by_utf8name(state, digestname, purpose);
    if (digest == NULL) {
        goto exit;
    }

    type = PY_EVP_MD_xof(digest) ? state->HASHXOF_type : state->HASH_type;
    self = new_hash_object(type);
    if (self == NULL) {
        goto exit;
    }

#if defined(EVP_MD_CTX_FLAG_NON_FIPS_ALLOW) && OPENSSL_VERSION_NUMBER < 0x30000000L
    // In OpenSSL 1.1.1 the non FIPS allowed flag is context specific while
    // in 3.0.0 it is a different EVP_MD provider.
    if (!usedforsecurity) {
        EVP_MD_CTX_set_flags(self->ctx, EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
    }
#endif

    int result = EVP_DigestInit_ex(self->ctx, digest, NULL);
    if (!result) {
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_DigestInit_ex));
        Py_CLEAR(self);
        goto exit;
    }

    if (view.buf && view.len) {
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            view.len,
            result = _hashlib_HASH_hash(self, view.buf, view.len)
        );
        if (result == -1) {
            assert(PyErr_Occurred());
            Py_CLEAR(self);
            goto exit;
        }
    }

exit:
    if (data_obj != NULL) {
        PyBuffer_Release(&view);
    }
    if (digest != NULL) {
        PY_EVP_MD_free(digest);
    }

    return (PyObject *)self;
}

// In Python 3.19, we can remove the "STRING" argument and would also be able
// to remove the macro (or keep it as an alias for better naming) since calls
// to _hashlib_HASH_new_impl() would fit on 80 characters.
#define CALL_HASHLIB_NEW(MODULE, NAME, DATA, STRING, USEDFORSECURITY)   \
    return _hashlib_HASH_new_impl(MODULE, NAME, DATA, USEDFORSECURITY, STRING)

/* The module-level function: new() */

/*[clinic input]
_hashlib.new as _hashlib_HASH_new

    name: str
    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Return a new hash object using the named algorithm.

An optional string argument may be provided and will be
automatically hashed.

The MD5 and SHA1 algorithms are always supported.
[clinic start generated code]*/

static PyObject *
_hashlib_HASH_new_impl(PyObject *module, const char *name, PyObject *data,
                       int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=b905aaf9840c1bbd input=c34af6c6e696d44e]*/
{
    PyObject *data_obj;
    if (_Py_hashlib_data_argument(&data_obj, data, string) < 0) {
        return NULL;
    }
    _hashlibstate *state = get_hashlib_state(module);
    return _hashlib_HASH(state, name, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_md5

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a md5 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_md5_impl(PyObject *module, PyObject *data,
                          int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=ca8cf184d90f7432 input=e7c0adbd6a867db1]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_md5, data, string, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha1

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha1 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha1_impl(PyObject *module, PyObject *data,
                           int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=1736fb7b310d64be input=f7e5bb1711e952d8]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha1, data, string, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha224

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha224_impl(PyObject *module, PyObject *data,
                             int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=0d6ff57be5e5c140 input=3820fff7ed3a53b8]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha224, data, string, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha256

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha256_impl(PyObject *module, PyObject *data,
                             int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=412ea7111555b6e7 input=9a2f115cf1f7e0eb]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha256, data, string, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha384

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha384_impl(PyObject *module, PyObject *data,
                             int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=2e0dc395b59ed726 input=1ea48f6f01e77cfb]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha384, data, string, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha512

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha512_impl(PyObject *module, PyObject *data,
                             int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=4bdd760388dbfc0f input=3cf56903e07d1f5c]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha512, data, string, usedforsecurity);
}


#ifdef PY_OPENSSL_HAS_SHA3

/*[clinic input]
_hashlib.openssl_sha3_224

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha3-224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_224_impl(PyObject *module, PyObject *data,
                               int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=6d8dc2a924f3ba35 input=7f14f16a9f6a3158]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha3_224, data, string, usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_256

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha3-256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_256_impl(PyObject *module, PyObject *data,
                               int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=9e520f537b3a4622 input=7987150939d5e352]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha3_256, data, string, usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_384

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha3-384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_384_impl(PyObject *module, PyObject *data,
                               int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=d239ba0463fd6138 input=fc943401f67e3b81]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha3_384, data, string, usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_512

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a sha3-512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_512_impl(PyObject *module, PyObject *data,
                               int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=17662f21038c2278 input=6601ddd2c6c1516d]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_sha3_512, data, string, usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHA3 */

#ifdef PY_OPENSSL_HAS_SHAKE
/*[clinic input]
@permit_long_summary
_hashlib.openssl_shake_128

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a shake-128 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_128_impl(PyObject *module, PyObject *data,
                                int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=4e6afed8d18980ad input=0d2803af1158b23c]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_shake_128, data, string, usedforsecurity);
}

/*[clinic input]
@permit_long_summary
_hashlib.openssl_shake_256

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Returns a shake-256 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_256_impl(PyObject *module, PyObject *data,
                                int usedforsecurity, PyObject *string)
/*[clinic end generated code: output=62481bce4a77d16c input=f27b98d9c749f55d]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_shake_256, data, string, usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHAKE */

#undef CALL_HASHLIB_NEW

/*[clinic input]
@permit_long_summary
_hashlib.pbkdf2_hmac as pbkdf2_hmac

    hash_name: str
    password: Py_buffer
    salt: Py_buffer
    iterations: long
    dklen as dklen_obj: object = None

Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as pseudorandom function.
[clinic start generated code]*/

static PyObject *
pbkdf2_hmac_impl(PyObject *module, const char *hash_name,
                 Py_buffer *password, Py_buffer *salt, long iterations,
                 PyObject *dklen_obj)
/*[clinic end generated code: output=144b76005416599b input=83417fbd9ec2b8a3]*/
{
    _hashlibstate *state = get_hashlib_state(module);
    PyObject *key_obj = NULL;
    char *key;
    long dklen;
    int retval;

    PY_EVP_MD *digest = get_openssl_evp_md_by_utf8name(state, hash_name,
                                                       Py_ht_pbkdf2);
    if (digest == NULL) {
        goto end;
    }

    if (password->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "password is too long.");
        goto end;
    }

    if (salt->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "salt is too long.");
        goto end;
    }

    if (iterations < 1) {
        PyErr_SetString(PyExc_ValueError,
                        "iteration value must be greater than 0.");
        goto end;
    }
    if (iterations > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "iteration value is too great.");
        goto end;
    }

    if (dklen_obj == Py_None) {
        dklen = EVP_MD_size(digest);
    } else {
        dklen = PyLong_AsLong(dklen_obj);
        if ((dklen == -1) && PyErr_Occurred()) {
            goto end;
        }
    }
    if (dklen < 1) {
        PyErr_SetString(PyExc_ValueError,
                        "key length must be greater than 0.");
        goto end;
    }
    if (dklen > INT_MAX) {
        /* INT_MAX is always smaller than dkLen max (2^32 - 1) * hLen */
        PyErr_SetString(PyExc_OverflowError,
                        "key length is too great.");
        goto end;
    }

    key_obj = PyBytes_FromStringAndSize(NULL, dklen);
    if (key_obj == NULL) {
        goto end;
    }
    key = PyBytes_AS_STRING(key_obj);

    Py_BEGIN_ALLOW_THREADS
    retval = PKCS5_PBKDF2_HMAC((const char *)password->buf, (int)password->len,
                               (const unsigned char *)salt->buf, (int)salt->len,
                               iterations, digest, dklen,
                               (unsigned char *)key);
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_CLEAR(key_obj);
        notify_ssl_error_occurred_in(Py_STRINGIFY(PKCS5_PBKDF2_HMAC));
        goto end;
    }

end:
    if (digest != NULL) {
        PY_EVP_MD_free(digest);
    }
    return key_obj;
}

// --- PBKDF: scrypt (RFC 7914) -----------------------------------------------

/*
 * By default, OpenSSL 1.1.0 restricts 'maxmem' in EVP_PBE_scrypt()
 * to 32 MiB (1024 * 1024 * 32) but only if 'maxmem = 0' and allows
 * for an arbitrary large limit fitting on an uint64_t otherwise.
 *
 * For legacy reasons, we limited 'maxmem' to be at most INTMAX,
 * but if users need a more relaxed value, we will revisit this
 * limit in the future.
 */
#define HASHLIB_SCRYPT_MAX_MAXMEM   INT_MAX

/*
 * Limit 'dklen' to INT_MAX even if it can be at most (32 * UINT32_MAX).
 *
 * See https://datatracker.ietf.org/doc/html/rfc7914.html for details.
 */
#define HASHLIB_SCRYPT_MAX_DKLEN    INT_MAX

/*[clinic input]
_hashlib.scrypt

    password: Py_buffer
    *
    salt: Py_buffer
    n: unsigned_long
    r: unsigned_long
    p: unsigned_long
    maxmem: long = 0
    dklen: long = 64

scrypt password-based key derivation function.
[clinic start generated code]*/

static PyObject *
_hashlib_scrypt_impl(PyObject *module, Py_buffer *password, Py_buffer *salt,
                     unsigned long n, unsigned long r, unsigned long p,
                     long maxmem, long dklen)
/*[clinic end generated code: output=d424bc3e8c6b9654 input=bdeac9628d07f7a1]*/
{
    PyObject *key = NULL;
    int retval;

    if (password->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "password is too long");
        return NULL;
    }

    if (salt->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "salt is too long");
        return NULL;
    }

    if (n < 2 || n & (n - 1)) {
        PyErr_SetString(PyExc_ValueError, "n must be a power of 2");
        return NULL;
    }

    if (maxmem < 0 || maxmem > HASHLIB_SCRYPT_MAX_MAXMEM) {
        PyErr_Format(PyExc_ValueError,
                     "maxmem must be positive and at most %d",
                     HASHLIB_SCRYPT_MAX_MAXMEM);
        return NULL;
    }

    if (dklen < 1 || dklen > HASHLIB_SCRYPT_MAX_DKLEN) {
        PyErr_Format(PyExc_ValueError,
                     "dklen must be at least 1 and at most %d",
                     HASHLIB_SCRYPT_MAX_DKLEN);
        return NULL;
    }

    /* let OpenSSL validate the rest */
    retval = EVP_PBE_scrypt(NULL, 0, NULL, 0, n, r, p,
                            (uint64_t)maxmem, NULL, 0);
    if (!retval) {
        notify_ssl_error_occurred("invalid parameter combination for "
                                  "n, r, p, and maxmem");
        return NULL;
   }

    key = PyBytes_FromStringAndSize(NULL, dklen);
    if (key == NULL) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    retval = EVP_PBE_scrypt(
        (const char *)password->buf, (size_t)password->len,
        (const unsigned char *)salt->buf, (size_t)salt->len,
        (uint64_t)n, (uint64_t)r, (uint64_t)p, (uint64_t)maxmem,
        (unsigned char *)PyBytes_AS_STRING(key), (size_t)dklen
    );
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_DECREF(key);
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_PBE_scrypt));
        return NULL;
    }
    return key;
}

#undef HASHLIB_SCRYPT_MAX_DKLEN
#undef HASHLIB_SCRYPT_MAX_MAXMEM

/* Fast HMAC for hmac.digest()
 */

/*[clinic input]
_hashlib.hmac_digest as _hashlib_hmac_singleshot

    key: Py_buffer
    msg: Py_buffer
    digest: object

Single-shot HMAC.
[clinic start generated code]*/

static PyObject *
_hashlib_hmac_singleshot_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg, PyObject *digest)
/*[clinic end generated code: output=82f19965d12706ac input=0a0790cc3db45c2e]*/
{
    _hashlibstate *state = get_hashlib_state(module);
    unsigned char md[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len = 0;
    unsigned char *result;
    PY_EVP_MD *evp;
    int is_xof;

    if (key->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "key is too long.");
        return NULL;
    }
    if (msg->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "msg is too long.");
        return NULL;
    }

    evp = get_openssl_evp_md(state, digest, Py_ht_mac);
    if (evp == NULL) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    is_xof = PY_EVP_MD_xof(evp);
    result = HMAC(
        evp,
        (const void *)key->buf, (int)key->len,
        (const unsigned char *)msg->buf, (size_t)msg->len,
        md, &md_len
    );
    Py_END_ALLOW_THREADS
    PY_EVP_MD_free(evp);

    if (result == NULL) {
        if (is_xof) {
            /* use a better default error message if an XOF is used */
            raise_unsupported_algorithm_error(state, digest);
        }
        else {
            notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC));
        }
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char*)md, md_len);
}

/* OpenSSL-based HMAC implementation
 */

/* Thin wrapper around HMAC_CTX_new() which sets an exception on failure. */
static HMAC_CTX *
py_openssl_wrapper_HMAC_CTX_new(void)
{
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    return ctx;
}

static int _hmac_update(HMACobject*, PyObject*);

static const EVP_MD *
_hashlib_hmac_get_md(HMACobject *self)
{
    const EVP_MD *md = HMAC_CTX_get_md(self->ctx);
    if (md == NULL) {
        notify_ssl_error_occurred("missing EVP_MD for HMAC context");
    }
    return md;
}

/*[clinic input]
_hashlib.hmac_new

    key: Py_buffer
    msg as msg_obj: object(c_default="NULL") = b''
    digestmod: object(c_default="NULL") = None

Return a new hmac object.
[clinic start generated code]*/

static PyObject *
_hashlib_hmac_new_impl(PyObject *module, Py_buffer *key, PyObject *msg_obj,
                       PyObject *digestmod)
/*[clinic end generated code: output=c20d9e4d9ed6d219 input=5f4071dcc7f34362]*/
{
    _hashlibstate *state = get_hashlib_state(module);
    PY_EVP_MD *digest;
    HMAC_CTX *ctx = NULL;
    HMACobject *self = NULL;
    int is_xof, r;

    if (key->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "key is too long.");
        return NULL;
    }

    if (digestmod == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Missing required parameter 'digestmod'.");
        return NULL;
    }

    digest = get_openssl_evp_md(state, digestmod, Py_ht_mac);
    if (digest == NULL) {
        return NULL;
    }

    ctx = py_openssl_wrapper_HMAC_CTX_new();
    if (ctx == NULL) {
        PY_EVP_MD_free(digest);
        goto error;
    }

    is_xof = PY_EVP_MD_xof(digest);
    r = HMAC_Init_ex(ctx, key->buf, (int)key->len, digest, NULL /* impl */);
    PY_EVP_MD_free(digest);
    if (r == 0) {
        if (is_xof) {
            /* use a better default error message if an XOF is used */
            raise_unsupported_algorithm_error(state, digestmod);
        }
        else {
            notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC_Init_ex));
        }
        goto error;
    }

    self = PyObject_New(HMACobject, state->HMAC_type);
    if (self == NULL) {
        goto error;
    }

    self->ctx = ctx;
    ctx = NULL;  // 'ctx' is now owned by 'self'
    HASHLIB_INIT_MUTEX(self);

    if ((msg_obj != NULL) && (msg_obj != Py_None)) {
        if (!_hmac_update(self, msg_obj)) {
            goto error;
        }
    }
    return (PyObject *)self;

error:
    if (ctx) HMAC_CTX_free(ctx);
    Py_XDECREF(self);
    return NULL;
}

/* helper functions */
static int
locked_HMAC_CTX_copy(HMAC_CTX *new_ctx_p, HMACobject *self)
{
    int result;
    HASHLIB_ACQUIRE_LOCK(self);
    result = HMAC_CTX_copy(new_ctx_p, self->ctx);
    HASHLIB_RELEASE_LOCK(self);
    if (result == 0) {
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(HMAC_CTX_copy));
        return -1;
    }
    return 0;
}

/* returning 0 means that an error occurred and an exception is set */
static unsigned int
_hashlib_hmac_digest_size(HMACobject *self)
{
    const EVP_MD *md = _hashlib_hmac_get_md(self);
    if (md == NULL) {
        return 0;
    }
    unsigned int digest_size = EVP_MD_size(md);
    assert(digest_size <= EVP_MAX_MD_SIZE);
    if (digest_size == 0) {
        notify_ssl_error_occurred("invalid digest size");
    }
    return digest_size;
}

static int
_hmac_update(HMACobject *self, PyObject *obj)
{
    int r;
    Py_buffer view = {0};

    GET_BUFFER_VIEW_OR_ERROR(obj, &view, return 0);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, view.len,
        r = HMAC_Update(
            self->ctx, (const unsigned char *)view.buf, (size_t)view.len
        )
    );
    PyBuffer_Release(&view);

    if (r == 0) {
        notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC_Update));
        return 0;
    }
    return 1;
}

/*[clinic input]
_hashlib.HMAC.copy

Return a copy ("clone") of the HMAC object.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_copy_impl(HMACobject *self)
/*[clinic end generated code: output=29aa28b452833127 input=e2fa6a05db61a4d6]*/
{
    HMACobject *retval;

    HMAC_CTX *ctx = py_openssl_wrapper_HMAC_CTX_new();
    if (ctx == NULL) {
        return NULL;
    }
    if (locked_HMAC_CTX_copy(ctx, self) < 0) {
        HMAC_CTX_free(ctx);
        return NULL;
    }

    retval = PyObject_New(HMACobject, Py_TYPE(self));
    if (retval == NULL) {
        HMAC_CTX_free(ctx);
        return NULL;
    }
    retval->ctx = ctx;
    HASHLIB_INIT_MUTEX(retval);

    return (PyObject *)retval;
}

static void
_hmac_dealloc(PyObject *op)
{
    HMACobject *self = HMACobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    if (self->ctx != NULL) {
        HMAC_CTX_free(self->ctx);
        self->ctx = NULL;
    }
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyObject *
_hmac_repr(PyObject *op)
{
    const char *digest_name;
    HMACobject *self = HMACobject_CAST(op);
    const EVP_MD *md = _hashlib_hmac_get_md(self);
    digest_name = md == NULL ? NULL : get_hashlib_utf8name_by_evp_md(md);
    if (digest_name == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return PyUnicode_FromFormat("<%s HMAC object @ %p>", digest_name, self);
}

/*[clinic input]
_hashlib.HMAC.update
    msg: object

Update the HMAC object with msg.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_update_impl(HMACobject *self, PyObject *msg)
/*[clinic end generated code: output=f31f0ace8c625b00 input=1829173bb3cfd4e6]*/
{
    if (!_hmac_update(self, msg)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
_hmac_digest(HMACobject *self, unsigned char *buf, unsigned int len)
{
    HMAC_CTX *temp_ctx = py_openssl_wrapper_HMAC_CTX_new();
    if (temp_ctx == NULL) {
        return 0;
    }
    if (locked_HMAC_CTX_copy(temp_ctx, self) < 0) {
        HMAC_CTX_free(temp_ctx);
        return 0;
    }
    int r = HMAC_Final(temp_ctx, buf, &len);
    HMAC_CTX_free(temp_ctx);
    if (r == 0) {
        notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC_Final));
        return 0;
    }
    return 1;
}

/*[clinic input]
_hashlib.HMAC.digest
Return the digest of the bytes passed to the update() method so far.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_digest_impl(HMACobject *self)
/*[clinic end generated code: output=1b1424355af7a41e input=bff07f74da318fb4]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = _hashlib_hmac_digest_size(self);
    if (digest_size == 0) {
        return NULL;
    }
    int r = _hmac_digest(self, digest, digest_size);
    if (r == 0) {
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)digest, digest_size);
}

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
_hashlib.HMAC.hexdigest

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_hexdigest_impl(HMACobject *self)
/*[clinic end generated code: output=80d825be1eaae6a7 input=5e48db83ab1a4d19]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = _hashlib_hmac_digest_size(self);
    if (digest_size == 0) {
        return NULL;
    }
    int r = _hmac_digest(self, digest, digest_size);
    if (r == 0) {
        return NULL;
    }
    return _Py_strhex((const char *)digest, digest_size);
}

static PyObject *
_hashlib_hmac_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
    unsigned int digest_size = _hashlib_hmac_digest_size(self);
    return digest_size == 0 ? NULL : PyLong_FromLong(digest_size);
}

static PyObject *
_hashlib_hmac_get_block_size(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
    const EVP_MD *md = _hashlib_hmac_get_md(self);
    return md == NULL ? NULL : PyLong_FromLong(EVP_MD_block_size(md));
}

static PyObject *
_hashlib_hmac_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
    const EVP_MD *md = _hashlib_hmac_get_md(self);
    if (md == NULL) {
        return NULL;
    }
    const char *digest_name = get_hashlib_utf8name_by_evp_md(md);
    if (digest_name == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return PyUnicode_FromFormat("hmac-%s", digest_name);
}

static PyMethodDef HMAC_methods[] = {
    _HASHLIB_HMAC_UPDATE_METHODDEF
    _HASHLIB_HMAC_DIGEST_METHODDEF
    _HASHLIB_HMAC_HEXDIGEST_METHODDEF
    _HASHLIB_HMAC_COPY_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static PyGetSetDef HMAC_getset[] = {
    {"digest_size", _hashlib_hmac_get_digest_size, NULL, NULL, NULL},
    {"block_size", _hashlib_hmac_get_block_size, NULL, NULL, NULL},
    {"name", _hashlib_hmac_get_name, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};


PyDoc_STRVAR(hmactype_doc,
"The object used to calculate HMAC of a message.\n\
\n\
Methods:\n\
\n\
update() -- updates the current digest with an additional string\n\
digest() -- return the current digest value\n\
hexdigest() -- return the current digest as a string of hexadecimal digits\n\
copy() -- return a copy of the current hash object\n\
\n\
Attributes:\n\
\n\
name -- the name, including the hash algorithm used by this object\n\
digest_size -- number of bytes in digest() output\n");

static PyType_Slot HMACtype_slots[] = {
    {Py_tp_doc, (char *)hmactype_doc},
    {Py_tp_repr, _hmac_repr},
    {Py_tp_dealloc, _hmac_dealloc},
    {Py_tp_methods, HMAC_methods},
    {Py_tp_getset, HMAC_getset},
    {0, NULL}
};

PyType_Spec HMACtype_spec = {
    "_hashlib.HMAC",    /* name */
    sizeof(HMACobject),     /* basicsize */
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = HMACtype_slots,
};


/* State for our callback function so that it can accumulate a result. */
typedef struct _internal_name_mapper_state {
    PyObject *set;
    int error;
} _InternalNameMapperState;


/* A callback function to pass to OpenSSL's OBJ_NAME_do_all(...) */
static void
#ifdef Py_HAS_OPENSSL3_SUPPORT
_openssl_hash_name_mapper(EVP_MD *md, void *arg)
#else
_openssl_hash_name_mapper(const EVP_MD *md, const char *from,
                          const char *to, void *arg)
#endif
{
    _InternalNameMapperState *state = (_InternalNameMapperState *)arg;
    PyObject *py_name;

    assert(state != NULL);
    // ignore all undefined providers
    if ((md == NULL) || (EVP_MD_nid(md) == NID_undef)) {
        return;
    }

    const char *name = get_hashlib_utf8name_by_evp_md(md);
    assert(name != NULL || PyErr_Occurred());
    py_name = name == NULL ? NULL : PyUnicode_FromString(name);
    if (py_name == NULL) {
        state->error = 1;
    } else {
        if (PySet_Add(state->set, py_name) != 0) {
            state->error = 1;
        }
        Py_DECREF(py_name);
    }
}


/* Ask OpenSSL for a list of supported ciphers, filling in a Python set. */
static int
hashlib_md_meth_names(PyObject *module)
{
    _InternalNameMapperState state = {
        .set = PyFrozenSet_New(NULL),
        .error = 0
    };
    if (state.set == NULL) {
        return -1;
    }

#ifdef Py_HAS_OPENSSL3_SUPPORT
    // get algorithms from all activated providers in default context
    EVP_MD_do_all_provided(NULL, &_openssl_hash_name_mapper, &state);
#else
    EVP_MD_do_all(&_openssl_hash_name_mapper, &state);
#endif

    if (state.error) {
        Py_DECREF(state.set);
        return -1;
    }

    return PyModule_Add(module, "openssl_md_meth_names", state.set);
}

/*[clinic input]
_hashlib.get_fips_mode -> int

Determine the OpenSSL FIPS mode of operation.

For OpenSSL 3.0.0 and newer it returns the state of the default provider
in the default OSSL context. It's not quite the same as FIPS_mode() but good
enough for unittests.

Effectively any non-zero return value indicates FIPS mode;
values other than 1 may have additional significance.
[clinic start generated code]*/

static int
_hashlib_get_fips_mode_impl(PyObject *module)
/*[clinic end generated code: output=87eece1bab4d3fa9 input=2db61538c41c6fef]*/

{
#ifdef Py_HAS_OPENSSL3_SUPPORT
    return EVP_default_properties_is_fips_enabled(NULL);
#else
    ERR_clear_error();
    int result = FIPS_mode();
    if (result == 0 && ERR_peek_last_error()) {
        // "If the library was built without support of the FIPS Object Module,
        // then the function will return 0 with an error code of
        // CRYPTO_R_FIPS_MODE_NOT_SUPPORTED (0x0f06d065)."
        // But 0 is also a valid result value.
        notify_ssl_error_occurred_in(Py_STRINGIFY(FIPS_mode));
        return -1;
    }
    return result;
#endif
}


static int
_tscmp(const unsigned char *a, const unsigned char *b,
        Py_ssize_t len_a, Py_ssize_t len_b)
{
    /* loop count depends on length of b. Might leak very little timing
     * information if sizes are different.
     */
    Py_ssize_t length = len_b;
    const void *left = a;
    const void *right = b;
    int result = 0;

    if (len_a != length) {
        left = b;
        result = 1;
    }

    result |= CRYPTO_memcmp(left, right, length);

    return (result == 0);
}

/* NOTE: Keep in sync with _operator.c implementation. */

/*[clinic input]
_hashlib.compare_digest

    a: object
    b: object
    /

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.
[clinic start generated code]*/

static PyObject *
_hashlib_compare_digest_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=6f1c13927480aed9 input=9c40c6e566ca12f5]*/
{
    int rc;

    /* ASCII unicode string */
    if(PyUnicode_Check(a) && PyUnicode_Check(b)) {
        if (!PyUnicode_IS_ASCII(a) || !PyUnicode_IS_ASCII(b)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return NULL;
        }

        rc = _tscmp(PyUnicode_DATA(a),
                    PyUnicode_DATA(b),
                    PyUnicode_GET_LENGTH(a),
                    PyUnicode_GET_LENGTH(b));
    }
    /* fallback to buffer interface for bytes, bytearray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (PyObject_CheckBuffer(a) == 0 && PyObject_CheckBuffer(b) == 0) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            return NULL;
        }

        if (PyObject_GetBuffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return NULL;
        }

        if (PyObject_GetBuffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyBool_FromLong(rc);
}

/* List of functions exported by this module */

static struct PyMethodDef EVP_functions[] = {
    _HASHLIB_HASH_NEW_METHODDEF
    PBKDF2_HMAC_METHODDEF
    _HASHLIB_SCRYPT_METHODDEF
    _HASHLIB_GET_FIPS_MODE_METHODDEF
    _HASHLIB_COMPARE_DIGEST_METHODDEF
    _HASHLIB_HMAC_SINGLESHOT_METHODDEF
    _HASHLIB_HMAC_NEW_METHODDEF
    _HASHLIB_OPENSSL_MD5_METHODDEF
    _HASHLIB_OPENSSL_SHA1_METHODDEF
    _HASHLIB_OPENSSL_SHA224_METHODDEF
    _HASHLIB_OPENSSL_SHA256_METHODDEF
    _HASHLIB_OPENSSL_SHA384_METHODDEF
    _HASHLIB_OPENSSL_SHA512_METHODDEF
    _HASHLIB_OPENSSL_SHA3_224_METHODDEF
    _HASHLIB_OPENSSL_SHA3_256_METHODDEF
    _HASHLIB_OPENSSL_SHA3_384_METHODDEF
    _HASHLIB_OPENSSL_SHA3_512_METHODDEF
    _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
    _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};


/* Initialize this module. */

static int
hashlib_traverse(PyObject *m, visitproc visit, void *arg)
{
    _hashlibstate *state = get_hashlib_state(m);
    Py_VISIT(state->HASH_type);
    Py_VISIT(state->HMAC_type);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_VISIT(state->HASHXOF_type);
#endif
    Py_VISIT(state->constructs);
    Py_VISIT(state->unsupported_digestmod_error);
    return 0;
}

static int
hashlib_clear(PyObject *m)
{
    _hashlibstate *state = get_hashlib_state(m);
    Py_CLEAR(state->HASH_type);
    Py_CLEAR(state->HMAC_type);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_CLEAR(state->HASHXOF_type);
#endif
    Py_CLEAR(state->constructs);
    Py_CLEAR(state->unsupported_digestmod_error);

    if (state->hashtable != NULL) {
        _Py_hashtable_destroy(state->hashtable);
        state->hashtable = NULL;
    }

    return 0;
}

static void
hashlib_free(void *m)
{
    (void)hashlib_clear((PyObject *)m);
}

/* Py_mod_exec functions */
static int
hashlib_init_hashtable(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->hashtable = py_hashentry_table_new();
    if (state->hashtable == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static int
hashlib_init_HASH_type(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->HASH_type = (PyTypeObject *)PyType_FromSpec(&HASHobject_type_spec);
    if (state->HASH_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HASH_type) < 0) {
        return -1;
    }
    return 0;
}

static int
hashlib_init_HASHXOF_type(PyObject *module)
{
#ifdef PY_OPENSSL_HAS_SHAKE
    _hashlibstate *state = get_hashlib_state(module);

    if (state->HASH_type == NULL) {
        return -1;
    }

    state->HASHXOF_type = (PyTypeObject *)PyType_FromSpecWithBases(
        &HASHXOFobject_type_spec, (PyObject *)state->HASH_type
    );
    if (state->HASHXOF_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HASHXOF_type) < 0) {
        return -1;
    }
#endif
    return 0;
}

static int
hashlib_init_hmactype(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->HMAC_type = (PyTypeObject *)PyType_FromSpec(&HMACtype_spec);
    if (state->HMAC_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HMAC_type) < 0) {
        return -1;
    }
    return 0;
}

static int
hashlib_init_constructors(PyObject *module)
{
    /* Create dict from builtin openssl_hash functions to name
     * {_hashlib.openssl_sha256: "sha256", ...}
     */
    PyModuleDef *mdef;
    PyMethodDef *fdef;
    PyObject *func, *name_obj;
    _hashlibstate *state = get_hashlib_state(module);

    mdef = PyModule_GetDef(module);
    if (mdef == NULL) {
        return -1;
    }

    state->constructs = PyDict_New();
    if (state->constructs == NULL) {
        return -1;
    }

    for (fdef = mdef->m_methods; fdef->ml_name != NULL; fdef++) {
        if (strncmp(fdef->ml_name, "openssl_", 8)) {
            continue;
        }
        name_obj = PyUnicode_FromString(fdef->ml_name + 8);
        if (name_obj == NULL) {
            return -1;
        }
        func  = PyObject_GetAttrString(module, fdef->ml_name);
        if (func == NULL) {
            Py_DECREF(name_obj);
            return -1;
        }
        int rc = PyDict_SetItem(state->constructs, func, name_obj);
        Py_DECREF(func);
        Py_DECREF(name_obj);
        if (rc < 0) {
            return -1;
        }
    }

    return PyModule_Add(module, "_constructors",
                        PyDictProxy_New(state->constructs));
}

static int
hashlib_exception(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);
    state->unsupported_digestmod_error = PyErr_NewException(
        "_hashlib.UnsupportedDigestmodError", PyExc_ValueError, NULL);
    if (state->unsupported_digestmod_error == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "UnsupportedDigestmodError",
                              state->unsupported_digestmod_error) < 0) {
        return -1;
    }
    return 0;
}

static int
hashlib_constants(PyObject *module)
{
    if (PyModule_AddIntConstant(module, "_GIL_MINSIZE",
                                HASHLIB_GIL_MINSIZE) < 0)
    {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot hashlib_slots[] = {
    {Py_mod_exec, hashlib_init_hashtable},
    {Py_mod_exec, hashlib_init_HASH_type},
    {Py_mod_exec, hashlib_init_HASHXOF_type},
    {Py_mod_exec, hashlib_init_hmactype},
    {Py_mod_exec, hashlib_md_meth_names},
    {Py_mod_exec, hashlib_init_constructors},
    {Py_mod_exec, hashlib_exception},
    {Py_mod_exec, hashlib_constants},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _hashlibmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_hashlib",
    .m_doc = "OpenSSL interface for hashlib module",
    .m_size = sizeof(_hashlibstate),
    .m_methods = EVP_functions,
    .m_slots = hashlib_slots,
    .m_traverse = hashlib_traverse,
    .m_clear = hashlib_clear,
    .m_free = hashlib_free
};

PyMODINIT_FUNC
PyInit__hashlib(void)
{
    return PyModuleDef_Init(&_hashlibmodule);
}

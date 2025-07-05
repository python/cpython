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
#include "pycore_strhex.h"                  // _Py_strhex()
#include "pycore_pyatomic_ft_wrappers.h"    // FT_ATOMIC_LOAD_PTR_RELAXED
#include "hashlib.h"

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#  define Py_HAS_OPENSSL3_SUPPORT
#endif

#include <openssl/err.h>
/* EVP is the preferred interface to hashing in OpenSSL */
#include <openssl/evp.h>
#include <openssl/crypto.h>                 // FIPS_mode()
/* We use the object interface to discover what hashes OpenSSL supports. */
#include <openssl/objects.h>

#ifdef Py_HAS_OPENSSL3_SUPPORT
#  include <openssl/core_names.h>           // OSSL_MAC_PARAM_DIGEST
#  include <openssl/params.h>               // OSSL_PARAM_*()
#else
#  include <openssl/hmac.h>                 // HMAC()
#endif

#include <stdbool.h>

#ifndef OPENSSL_THREADS
#  error "OPENSSL_THREADS is not defined, Python requires thread-safe OpenSSL"
#endif

#define MUNCH_SIZE INT_MAX

#define PY_OPENSSL_HAS_SCRYPT 1
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

#define Py_HMAC_CTX_TYPE    EVP_MAC_CTX
#define PY_HMAC_CTX_free    EVP_MAC_CTX_free
#define PY_HMAC_update      EVP_MAC_update
#else
#define PY_EVP_MD const EVP_MD
#define PY_EVP_MD_fetch(algorithm, properties) EVP_get_digestbyname(algorithm)
#define PY_EVP_MD_up_ref(md) do {} while(0)
#define PY_EVP_MD_free(md) do {} while(0)

#define Py_HMAC_CTX_TYPE    HMAC_CTX
#define PY_HMAC_CTX_free    HMAC_CTX_free
#define PY_HMAC_update      HMAC_Update
#endif

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
#ifdef Py_HAS_OPENSSL3_SUPPORT
    EVP_MAC *evp_hmac;
#endif
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
#ifdef Py_HAS_OPENSSL3_SUPPORT
    EVP_MAC_CTX *ctx;   /* OpenSSL HMAC EVP-based context */
    int evp_md_nid;     /* needed to find the message digest name */
#else
    HMAC_CTX *ctx;      /* OpenSSL HMAC plain context */
#endif
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

/* Set an exception of given type using the given OpenSSL error code. */
static void
set_ssl_exception_from_errcode(PyObject *exc_type, unsigned long errcode)
{
    assert(exc_type != NULL);
    assert(errcode != 0);

    /* ERR_ERROR_STRING(3) ensures that the messages below are ASCII */
    const char *lib = ERR_lib_error_string(errcode);
    const char *func = ERR_func_error_string(errcode);
    const char *reason = ERR_reason_error_string(errcode);

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

#ifdef Py_HAS_OPENSSL3_SUPPORT
/*
 * Convert the NID to an OpenSSL "canonical" cached, SN_* or LN_* digest name.
 *
 * On error, set an exception and return NULL.
 */
static const char *
get_openssl_utf8name_by_nid(int nid)
{
    const py_hashentry_t *e = get_hashentry_by_nid(nid);
    return e ? e->ossl_name : get_asn1_utf8name_by_nid(nid);
}
#endif

/* Same as get_hashlib_utf8name_by_nid() but using an EVP_MD object. */
static const char *
get_hashlib_utf8name_by_evp_md(const EVP_MD *md)
{
    assert(md != NULL);
    return get_hashlib_utf8name_by_nid(EVP_MD_nid(md));
}

/*
 * Get a new reference to an EVP_MD object described by name and purpose.
 *
 * If 'name' is an OpenSSL indexed name, the return value is cached.
 */
static PY_EVP_MD *
get_openssl_evp_md_by_utf8name(PyObject *module, const char *name,
                               Py_hash_type py_ht)
{
    PY_EVP_MD *digest = NULL, *other_digest = NULL;
    _hashlibstate *state = get_hashlib_state(module);
    py_hashentry_t *entry = (py_hashentry_t *)_Py_hashtable_get(
        state->hashtable, (const void*)name
    );

    if (entry != NULL) {
        switch (py_ht) {
        case Py_ht_evp:
        case Py_ht_mac:
        case Py_ht_pbkdf2:
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
            break;
        case Py_ht_evp_nosecurity:
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
            break;
        default:
            goto invalid_hash_type;
        }
        // if another thread same thing at same time make sure we got same ptr
        assert(other_digest == NULL || other_digest == digest);
        if (digest != NULL && other_digest == NULL) {
            PY_EVP_MD_up_ref(digest);
        }
    }
    else {
        // Fall back for looking up an unindexed OpenSSL specific name.
        switch (py_ht) {
        case Py_ht_evp:
        case Py_ht_mac:
        case Py_ht_pbkdf2:
            digest = PY_EVP_MD_fetch(name, NULL);
            break;
        case Py_ht_evp_nosecurity:
            digest = PY_EVP_MD_fetch(name, "-fips");
            break;
        default:
            goto invalid_hash_type;
        }
    }
    if (digest == NULL) {
        raise_ssl_error_f(state->unsupported_digestmod_error,
                          "unsupported digest name: %s", name);
        return NULL;
    }
    return digest;

invalid_hash_type:
    assert(digest == NULL);
    PyErr_Format(PyExc_SystemError, "unsupported hash type %d", py_ht);
    return NULL;
}

/*
 * Raise an exception indicating that 'digestmod' is not supported.
 */
static void
raise_unsupported_digestmod_error(PyObject *module, PyObject *digestmod)
{
    _hashlibstate *state = get_hashlib_state(module);
    PyErr_Format(state->unsupported_digestmod_error,
                 "Unsupported digestmod %R", digestmod);
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
get_openssl_evp_md(PyObject *module, PyObject *digestmod, Py_hash_type py_ht)
{
    const char *name;
    if (PyUnicode_Check(digestmod)) {
        name = PyUnicode_AsUTF8(digestmod);
    }
    else {
        PyObject *dict = get_hashlib_state(module)->constructs;
        assert(dict != NULL);
        PyObject *borrowed_ref = PyDict_GetItemWithError(dict, digestmod);
        name = borrowed_ref == NULL ? NULL : PyUnicode_AsUTF8(borrowed_ref);
    }
    if (name == NULL) {
        if (!PyErr_Occurred()) {
            raise_unsupported_digestmod_error(module, digestmod);
        }
        return NULL;
    }
    return get_openssl_evp_md_by_utf8name(module, name, py_ht);
}

/*
 * Get the "canonical" name of an EVP_MD described by 'digestmod' and purpose.
 *
 * On error, set an exception and return NULL.
 *
 * This function should not be used to construct the exposed Python name,
 * but rather to invoke OpenSSL EVP_* functions.
 */
#ifdef Py_HAS_OPENSSL3_SUPPORT
static const char *
get_openssl_digest_name(PyObject *module, PyObject *digestmod,
                        Py_hash_type py_ht, int *evp_md_nid)
{
    if (evp_md_nid != NULL) {
        *evp_md_nid = NID_undef;
    }
    PY_EVP_MD *md = get_openssl_evp_md(module, digestmod, py_ht);
    if (md == NULL) {
        return NULL;
    }
    int nid = EVP_MD_nid(md);
    if (evp_md_nid != NULL) {
        *evp_md_nid = nid;
    }
    const char *name = get_openssl_utf8name_by_nid(nid);
    PY_EVP_MD_free(md);
    if (name == NULL) {
        raise_unsupported_digestmod_error(module, digestmod);
    }
    return name;
}
#endif

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
    const EVP_MD *md = EVP_MD_CTX_md(self->ctx);
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
_hashlib_HASH(PyObject *module, const char *digestname, PyObject *data_obj,
              int usedforsecurity)
{
    Py_buffer view = { 0 };
    PY_EVP_MD *digest = NULL;
    PyTypeObject *type;
    HASHobject *self = NULL;

    if (data_obj != NULL) {
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);
    }

    digest = get_openssl_evp_md_by_utf8name(
        module, digestname, usedforsecurity ? Py_ht_evp : Py_ht_evp_nosecurity
    );
    if (digest == NULL) {
        goto exit;
    }

    if ((EVP_MD_flags(digest) & EVP_MD_FLAG_XOF) == EVP_MD_FLAG_XOF) {
        type = get_hashlib_state(module)->HASHXOF_type;
    } else {
        type = get_hashlib_state(module)->HASH_type;
    }

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

#define CALL_HASHLIB_NEW(MODULE, NAME, DATA, STRING, USEDFORSECURITY)   \
    do {                                                                \
        PyObject *data_obj;                                             \
        if (_Py_hashlib_data_argument(&data_obj, DATA, STRING) < 0) {   \
            return NULL;                                                \
        }                                                               \
        return _hashlib_HASH(MODULE, NAME, data_obj, USEDFORSECURITY);  \
    } while (0)

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
    CALL_HASHLIB_NEW(module, name, data, string, usedforsecurity);
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
/*[clinic end generated code: output=4e6afed8d18980ad input=373c3f1c93d87b37]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_shake_128, data, string, usedforsecurity);
}

/*[clinic input]
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
/*[clinic end generated code: output=62481bce4a77d16c input=101c139ea2ddfcbf]*/
{
    CALL_HASHLIB_NEW(module, Py_hash_shake_256, data, string, usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHAKE */

#undef CALL_HASHLIB_NEW

/*[clinic input]
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
/*[clinic end generated code: output=144b76005416599b input=ed3ab0d2d28b5d5c]*/
{
    PyObject *key_obj = NULL;
    char *key;
    long dklen;
    int retval;

    PY_EVP_MD *digest = get_openssl_evp_md_by_utf8name(module, hash_name, Py_ht_pbkdf2);
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

#ifdef PY_OPENSSL_HAS_SCRYPT

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
/*[clinic end generated code: output=d424bc3e8c6b9654 input=0c9a84230238fd79]*/
{
    PyObject *key_obj = NULL;
    char *key;
    int retval;

    if (password->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "password is too long.");
        return NULL;
    }

    if (salt->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "salt is too long.");
        return NULL;
    }

    if (n < 2 || n & (n - 1)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a power of 2.");
        return NULL;
    }

    if (maxmem < 0 || maxmem > INT_MAX) {
        /* OpenSSL 1.1.0 restricts maxmem to 32 MiB. It may change in the
           future. The maxmem constant is private to OpenSSL. */
        PyErr_Format(PyExc_ValueError,
                     "maxmem must be positive and smaller than %d",
                     INT_MAX);
        return NULL;
    }

    if (dklen < 1 || dklen > INT_MAX) {
        PyErr_Format(PyExc_ValueError,
                     "dklen must be greater than 0 and smaller than %d",
                     INT_MAX);
        return NULL;
    }

    /* let OpenSSL validate the rest */
    retval = EVP_PBE_scrypt(NULL, 0, NULL, 0, n, r, p, maxmem, NULL, 0);
    if (!retval) {
        notify_ssl_error_occurred(
            "Invalid parameter combination for n, r, p, maxmem.");
        return NULL;
   }

    key_obj = PyBytes_FromStringAndSize(NULL, dklen);
    if (key_obj == NULL) {
        return NULL;
    }
    key = PyBytes_AS_STRING(key_obj);

    Py_BEGIN_ALLOW_THREADS
    retval = EVP_PBE_scrypt(
        (const char*)password->buf, (size_t)password->len,
        (const unsigned char *)salt->buf, (size_t)salt->len,
        n, r, p, maxmem,
        (unsigned char *)key, (size_t)dklen
    );
    Py_END_ALLOW_THREADS

    if (!retval) {
        Py_CLEAR(key_obj);
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_PBE_scrypt));
        return NULL;
    }
    return key_obj;
}
#endif  /* PY_OPENSSL_HAS_SCRYPT */

// --- OpenSSL HMAC interface -------------------------------------------------

/*
 * Functions prefixed by hashlib_openssl_HMAC_* are wrappers around OpenSSL
 * and implement "atomic" operations (e.g., "free"). These functions are used
 * by those prefixed by _hashlib_HMAC_* that are methods for HMAC objects, or
 * other (local) helper functions prefixed by hashlib_HMAC_*.
 */

#ifdef Py_HAS_OPENSSL3_SUPPORT
/* EVP_MAC_CTX array of parameters specifying the "digest" */
#define HASHLIB_HMAC_OSSL_PARAMS(DIGEST)                        \
    (const OSSL_PARAM []) {                                     \
        OSSL_PARAM_utf8_string(OSSL_MAC_PARAM_DIGEST,           \
                               (char *)DIGEST, strlen(DIGEST)), \
        OSSL_PARAM_END                                          \
    }
#endif

// --- One-shot HMAC interface ------------------------------------------------

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
    const void *result;
    unsigned char md[EVP_MAX_MD_SIZE] = {0};
#ifdef Py_HAS_OPENSSL3_SUPPORT
    size_t md_len = 0;
    const char *digest_name = NULL;
#else
    unsigned int md_len = 0;
    PY_EVP_MD *evp = NULL;
#endif

    if (key->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "key is too long.");
        return NULL;
    }
    if (msg->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "msg is too long.");
        return NULL;
    }

#ifdef Py_HAS_OPENSSL3_SUPPORT
    digest_name = get_openssl_digest_name(module, digest, Py_ht_mac, NULL);
    if (digest_name == NULL) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = EVP_Q_mac(
        NULL, OSSL_MAC_NAME_HMAC, NULL, NULL,
        HASHLIB_HMAC_OSSL_PARAMS(digest_name),
        (const void *)key->buf, (size_t)key->len,
        (const unsigned char *)msg->buf, (size_t)msg->len,
        md, sizeof(md), &md_len
    );
    Py_END_ALLOW_THREADS
    assert(md_len < (size_t)PY_SSIZE_T_MAX);
#else
    evp = get_openssl_evp_md(module, digest, Py_ht_mac);
    if (evp == NULL) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    result = HMAC(
        evp,
        (const void *)key->buf, (int)key->len,
        (const unsigned char *)msg->buf, (size_t)msg->len,
        md, &md_len
    );
    Py_END_ALLOW_THREADS
    PY_EVP_MD_free(evp);
#endif
    if (result == NULL) {
#ifdef Py_HAS_OPENSSL3_SUPPORT
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_Q_mac));
#else
        notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC));
#endif
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char*)md, md_len);
}

// --- HMAC Object ------------------------------------------------------------

#ifndef Py_HAS_OPENSSL3_SUPPORT
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

static const EVP_MD *
hashlib_openssl_HMAC_evp_md_borrowed(HMACobject *self)
{
    assert(self->ctx != NULL);
    const EVP_MD *md = HMAC_CTX_get_md(self->ctx);
    if (md == NULL) {
        notify_ssl_error_occurred("missing EVP_MD for HMAC context");
    }
    return md;
}
#endif

static const char *
hashlib_HMAC_get_hashlib_digest_name(HMACobject *self)
{
#ifdef Py_HAS_OPENSSL3_SUPPORT
    return get_hashlib_utf8name_by_nid(self->evp_md_nid);
#else
    const EVP_MD *md = hashlib_openssl_HMAC_evp_md_borrowed(self);
    return md == NULL ? NULL : get_hashlib_utf8name_by_evp_md(md);
#endif
}

static int
hashlib_openssl_HMAC_update_once(Py_HMAC_CTX_TYPE *ctx, const Py_buffer *v)
{
    if (!PY_HMAC_update(ctx, (const unsigned char *)v->buf, (size_t)v->len)) {
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(PY_HMAC_update));
        return -1;
    }
    return 0;
}

/* Thin wrapper around PY_HMAC_CTX_free that allows to pass a NULL 'ctx'. */
static inline void
hashlib_openssl_HMAC_CTX_free(Py_HMAC_CTX_TYPE *ctx)
{
    /* The NULL check was not present in every OpenSSL versions. */
    if (ctx) {
        PY_HMAC_CTX_free(ctx);
    }
}

static int
hashlib_openssl_HMAC_update_with_lock(HMACobject *self, PyObject *data)
{
    int r;
    Py_buffer view = {0};
    GET_BUFFER_VIEW_OR_ERROR(data, &view, return -1);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, view.len,
        r = hashlib_openssl_HMAC_update_once(self->ctx, &view)
    );
    PyBuffer_Release(&view);
    return r;
}

static Py_HMAC_CTX_TYPE *
hashlib_openssl_HMAC_ctx_copy_with_lock(HMACobject *self)
{
    Py_HMAC_CTX_TYPE *ctx = NULL;
#ifdef Py_HAS_OPENSSL3_SUPPORT
    HASHLIB_ACQUIRE_LOCK(self);
    ctx = EVP_MAC_CTX_dup(self->ctx);
    HASHLIB_RELEASE_LOCK(self);
    if (ctx == NULL) {
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(EVP_MAC_CTX_dup));
        goto error;
    }
#else
    int r;
    ctx = py_openssl_wrapper_HMAC_CTX_new();
    if (ctx == NULL) {
        return NULL;
    }
    HASHLIB_ACQUIRE_LOCK(self);
    r = HMAC_CTX_copy(ctx, self->ctx);
    HASHLIB_RELEASE_LOCK(self);
    if (r == 0) {
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(HMAC_CTX_copy));
        goto error;
    }
#endif
    return ctx;

error:
    hashlib_openssl_HMAC_CTX_free(ctx);
    return NULL;
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
    HMACobject *self = NULL;
    Py_HMAC_CTX_TYPE *ctx = NULL;
#ifdef Py_HAS_OPENSSL3_SUPPORT
    int evp_md_nid = NID_undef;
#endif
    int r;

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

#ifdef Py_HAS_OPENSSL3_SUPPORT
    /*
     * OpenSSL 3.0 does not provide a way to extract the NID from an EVP_MAC
     * object and does not expose the underlying digest name. The reason is
     * that OpenSSL 3.0 treats HMAC objects as being the "same", differing
     * only by their *context* parameters. While it is *required* to set
     * the digest name when constructing EVP_MAC_CTX objects, that name
     * is unfortunately not recoverable through EVP_MAC_CTX_get_params().
     *
     * On the other hand, the (deprecated) interface based on HMAC_CTX is
     * based on EVP_MD, which allows to treat HMAC objects as if they were
     * hash functions when querying the digest name.
     *
     * Since HMAC objects are constructed from DIGESTMOD values and since
     * we have a way to map DIGESTMOD to EVP_MD objects, and then to NIDs,
     * HMAC objects based on EVP_MAC will store the NID of the EVP_MD we
     * used to deduce the digest name to pass to EVP_MAC_CTX_set_params().
     */
    const char *digest = get_openssl_digest_name(
        module, digestmod, Py_ht_mac, &evp_md_nid
    );
    if (digest == NULL) {
        return NULL;
    }
    assert(evp_md_nid != NID_undef);
    /*
     * OpenSSL is responsible for managing the EVP_MAC object's ref. count
     * by calling EVP_MAC_up_ref() and EVP_MAC_free() in EVP_MAC_CTX_new()
     * and EVP_MAC_CTX_free() respectively.
     */
    ctx = EVP_MAC_CTX_new(state->evp_hmac);
    if (ctx == NULL) {
        /* EVP_MAC_CTX_new() may also set an ERR_R_EVP_LIB error */
        notify_smart_ssl_error_occurred_in(Py_STRINGIFY(EVP_MAC_CTX_new));
        return NULL;
    }

    r = EVP_MAC_init(
        ctx,
        (const unsigned char *)key->buf,
        (size_t)key->len,
        HASHLIB_HMAC_OSSL_PARAMS(digest)
    );
#else
    PY_EVP_MD *digest = get_openssl_evp_md(module, digestmod, Py_ht_mac);
    if (digest == NULL) {
        return NULL;
    }
    ctx = py_openssl_wrapper_HMAC_CTX_new();
    if (ctx == NULL) {
        PY_EVP_MD_free(digest);
        goto error;
    }

    r = HMAC_Init_ex(ctx, key->buf, (int)key->len, digest, NULL /* impl */);
    PY_EVP_MD_free(digest);
#endif
    if (r == 0) {
#ifdef Py_HAS_OPENSSL3_SUPPORT
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_MAC_init));
#else
        notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC_Init_ex));
#endif
        goto error;
    }

    self = PyObject_New(HMACobject, state->HMAC_type);
    if (self == NULL) {
        goto error;
    }

    self->ctx = ctx;
    ctx = NULL;  // 'ctx' is now owned by 'self'
#ifdef Py_HAS_OPENSSL3_SUPPORT
    assert(evp_md_nid != NID_undef);
    self->evp_md_nid = evp_md_nid;
#endif
    HASHLIB_INIT_MUTEX(self);

    /* feed initial data */
    if ((msg_obj != NULL) && (msg_obj != Py_None)) {
        if (hashlib_openssl_HMAC_update_with_lock(self, msg_obj) < 0) {
            goto error;
        }
    }
    return (PyObject *)self;

error:
    hashlib_openssl_HMAC_CTX_free(ctx);
    Py_XDECREF(self);
    return NULL;
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
    Py_HMAC_CTX_TYPE *ctx = hashlib_openssl_HMAC_ctx_copy_with_lock(self);
    if (ctx == NULL) {
        return NULL;
    }
    retval = PyObject_New(HMACobject, Py_TYPE(self));
    if (retval == NULL) {
        PY_HMAC_CTX_free(ctx);
        return NULL;
    }
    retval->ctx = ctx;
    HASHLIB_INIT_MUTEX(retval);
    return (PyObject *)retval;
}

static void
_hashlib_HMAC_dealloc(PyObject *op)
{
    HMACobject *self = HMACobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    if (self->ctx != NULL) {
        PY_HMAC_CTX_free(self->ctx);
        self->ctx = NULL;
    }
    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyObject *
_hashlib_HMAC_repr(PyObject *op)
{
    HMACobject *self = HMACobject_CAST(op);
    const char *digest_name = hashlib_HMAC_get_hashlib_digest_name(self);
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
    if (hashlib_openssl_HMAC_update_with_lock(self, msg) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

#define BAD_DIGEST_SIZE 0

/*
 * Return the digest size in bytes.
 *
 * On error, set an exception and return BAD_DIGEST_SIZE.
 */
static unsigned int
hashlib_openssl_HMAC_digest_size(HMACobject *self)
{
    assert(EVP_MAX_MD_SIZE < INT_MAX);
#ifdef Py_HAS_OPENSSL3_SUPPORT
    assert(self->ctx != NULL);
    size_t digest_size = EVP_MAC_CTX_get_mac_size(self->ctx);
    assert(digest_size <= (size_t)EVP_MAX_MD_SIZE);
#else
    const EVP_MD *md = hashlib_openssl_HMAC_evp_md_borrowed(self);
    if (md == NULL) {
        return BAD_DIGEST_SIZE;
    }
    int digest_size = EVP_MD_size(md);
    /* digest_size < 0 iff EVP_MD context is NULL (which is impossible here) */
    assert(digest_size >= 0);
    assert(digest_size <= (int)EVP_MAX_MD_SIZE);
#endif
    /* digest_size == 0 means that the context is not entirely initialized */
    if (digest_size == 0) {
        raise_ssl_error(PyExc_ValueError, "missing digest size");
        return BAD_DIGEST_SIZE;
    }
    return (unsigned int)digest_size;
}

/*
 * Extract the MAC value to 'buf' and return the digest size.
 *
 * The buffer 'buf' must have at least hashlib_openssl_HMAC_digest_size(self)
 * bytes. Smaller buffers lead to undefined behaviors.
 *
 * On error, set an exception and return -1.
 */
static Py_ssize_t
hashlib_openssl_HMAC_digest_compute(HMACobject *self, unsigned char *buf)
{
    unsigned int digest_size = hashlib_openssl_HMAC_digest_size(self);
    assert(digest_size <= EVP_MAX_MD_SIZE);
    if (digest_size == BAD_DIGEST_SIZE) {
        assert(PyErr_Occurred());
        return -1;
    }
    Py_HMAC_CTX_TYPE *ctx = hashlib_openssl_HMAC_ctx_copy_with_lock(self);
    if (ctx == NULL) {
        return -1;
    }
#ifdef Py_HAS_OPENSSL3_SUPPORT
    int r = EVP_MAC_final(ctx, buf, NULL, digest_size);
#else
    int r = HMAC_Final(ctx, buf, NULL);
#endif
    PY_HMAC_CTX_free(ctx);
    if (r == 0) {
#ifdef Py_HAS_OPENSSL3_SUPPORT
        notify_ssl_error_occurred_in(Py_STRINGIFY(EVP_MAC_final));
#else
        notify_ssl_error_occurred_in(Py_STRINGIFY(HMAC_Final));
#endif
        return -1;
    }
    return digest_size;
}

/*[clinic input]
_hashlib.HMAC.digest
Return the digest of the bytes passed to the update() method so far.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_digest_impl(HMACobject *self)
/*[clinic end generated code: output=1b1424355af7a41e input=bff07f74da318fb4]*/
{
    unsigned char buf[EVP_MAX_MD_SIZE];
    Py_ssize_t n = hashlib_openssl_HMAC_digest_compute(self, buf);
    return n < 0 ? NULL : PyBytes_FromStringAndSize((const char *)buf, n);
}

/*[clinic input]
_hashlib.HMAC.hexdigest

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_hexdigest_impl(HMACobject *self)
/*[clinic end generated code: output=80d825be1eaae6a7 input=5abc42702874ddcf]*/
{
    unsigned char buf[EVP_MAX_MD_SIZE];
    Py_ssize_t n = hashlib_openssl_HMAC_digest_compute(self, buf);
    return n < 0 ? NULL : _Py_strhex((const char *)buf, n);
}

static PyObject *
_hashlib_HMAC_digest_size_getter(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
    unsigned int size = hashlib_openssl_HMAC_digest_size(self);
    return size == BAD_DIGEST_SIZE ? NULL : PyLong_FromLong(size);
}

static PyObject *
_hashlib_HMAC_block_size_getter(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
#ifdef Py_HAS_OPENSSL3_SUPPORT
    assert(self->ctx != NULL);
    return PyLong_FromSize_t(EVP_MAC_CTX_get_block_size(self->ctx));
#else
    const EVP_MD *md = hashlib_openssl_HMAC_evp_md_borrowed(self);
    return md == NULL ? NULL : PyLong_FromLong(EVP_MD_block_size(md));
#endif
}

static PyObject *
_hashlib_HMAC_name_getter(PyObject *op, void *Py_UNUSED(closure))
{
    HMACobject *self = HMACobject_CAST(op);
    const char *digest_name = hashlib_HMAC_get_hashlib_digest_name(self);
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

static PyGetSetDef HMAC_getsets[] = {
    {"digest_size", _hashlib_HMAC_digest_size_getter, NULL, NULL, NULL},
    {"block_size", _hashlib_HMAC_block_size_getter, NULL, NULL, NULL},
    {"name", _hashlib_HMAC_name_getter, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};


PyDoc_STRVAR(HMACobject_type_doc,
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

static PyType_Slot HMACobject_type_slots[] = {
    {Py_tp_doc, (char *)HMACobject_type_doc},
    {Py_tp_repr, _hashlib_HMAC_repr},
    {Py_tp_dealloc, _hashlib_HMAC_dealloc},
    {Py_tp_methods, HMAC_methods},
    {Py_tp_getset, HMAC_getsets},
    {0, NULL}
};

PyType_Spec HMACobject_type_spec = {
    .name = "_hashlib.HMAC",
    .basicsize = sizeof(HMACobject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
    ),
    .slots = HMACobject_type_slots
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
#ifdef Py_HAS_OPENSSL3_SUPPORT
    if (state->evp_hmac != NULL) {
        EVP_MAC_free(state->evp_hmac);
        state->evp_hmac = NULL;
    }
#endif

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

    state->HMAC_type = (PyTypeObject *)PyType_FromSpec(&HMACobject_type_spec);
    if (state->HMAC_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HMAC_type) < 0) {
        return -1;
    }
#ifdef Py_HAS_OPENSSL3_SUPPORT
    state->evp_hmac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (state->evp_hmac == NULL) {
        ERR_clear_error();
        PyErr_SetString(PyExc_ImportError, "cannot initialize EVP_MAC HMAC");
        return -1;
    }
#endif

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

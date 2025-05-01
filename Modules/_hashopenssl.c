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

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define PY_EVP_MD EVP_MD
#define PY_EVP_MD_fetch(algorithm, properties) EVP_MD_fetch(NULL, algorithm, properties)
#define PY_EVP_MD_up_ref(md) EVP_MD_up_ref(md)
#define PY_EVP_MD_free(md) EVP_MD_free(md)
#else
#define PY_EVP_MD const EVP_MD
#define PY_EVP_MD_fetch(algorithm, properties) EVP_get_digestbyname(algorithm)
#define PY_EVP_MD_up_ref(md) do {} while(0)
#define PY_EVP_MD_free(md) do {} while(0)
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

enum Py_hash_type {
    Py_ht_evp,            // usedforsecurity=True / default
    Py_ht_evp_nosecurity, // usedforsecurity=False
    Py_ht_mac,            // HMAC
    Py_ht_pbkdf2,         // PKBDF2
};

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

/* Module state */
static PyModuleDef _hashlibmodule;

typedef struct {
    PyTypeObject *EVPtype;
    PyTypeObject *HMACtype;
#ifdef PY_OPENSSL_HAS_SHAKE
    PyTypeObject *EVPXOFtype;
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

typedef struct {
    PyObject_HEAD
    EVP_MD_CTX          *ctx;   /* OpenSSL message digest context */
    // Prevents undefined behavior via multiple threads entering the C API.
    bool use_mutex;
    PyMutex mutex;  /* OpenSSL context lock */
} EVPobject;

#define EVPobject_CAST(op)  ((EVPobject *)(op))

typedef struct {
    PyObject_HEAD
    HMAC_CTX *ctx;            /* OpenSSL hmac context */
    // Prevents undefined behavior via multiple threads entering the C API.
    bool use_mutex;
    PyMutex mutex;  /* HMAC context lock */
} HMACobject;

#define HMACobject_CAST(op) ((HMACobject *)(op))

#include "clinic/_hashopenssl.c.h"
/*[clinic input]
module _hashlib
class _hashlib.HASH "EVPobject *" "((_hashlibstate *)PyModule_GetState(module))->EVPtype"
class _hashlib.HASHXOF "EVPobject *" "((_hashlibstate *)PyModule_GetState(module))->EVPXOFtype"
class _hashlib.HMAC "HMACobject *" "((_hashlibstate *)PyModule_GetState(module))->HMACtype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7df1bcf6f75cb8ef]*/


/* LCOV_EXCL_START */

/* Set an exception of given type using the given OpenSSL error code. */
static void
set_ssl_exception_from_errcode(PyObject *exc, unsigned long errcode)
{
    assert(errcode != 0);

    /* ERR_ERROR_STRING(3) ensures that the messages below are ASCII */
    const char *lib = ERR_lib_error_string(errcode);
    const char *func = ERR_func_error_string(errcode);
    const char *reason = ERR_reason_error_string(errcode);

    if (lib && func) {
        PyErr_Format(exc, "[%s: %s] %s", lib, func, reason);
    }
    else if (lib) {
        PyErr_Format(exc, "[%s] %s", lib, reason);
    }
    else {
        PyErr_SetString(exc, reason);
    }
}

/*
 * Set an exception of given type.
 *
 * By default, the exception's message is constructed by using the last SSL
 * error that occurred. If no error occurred, the 'fallback_format' is used
 * to create a C-style formatted fallback message.
 */
static void
raise_ssl_error(PyObject *exc, const char *fallback_format, ...)
{
    assert(fallback_format != NULL);
    unsigned long errcode = ERR_peek_last_error();
    if (errcode) {
        ERR_clear_error();
        set_ssl_exception_from_errcode(exc, errcode);
    }
    else {
        va_list vargs;
        va_start(vargs, fallback_format);
        PyErr_FormatV(exc, fallback_format, vargs);
        va_end(vargs);
    }
}

/*
 * Set an exception with a generic default message after an error occurred.
 *
 * It can also be used without previous calls to SSL built-in functions,
 * in which case a generic error message is provided.
 */
static inline void
notify_ssl_error_occurred(void)
{
    raise_ssl_error(PyExc_ValueError, "no reason supplied");
}
/* LCOV_EXCL_STOP */

static PyObject*
py_digest_name(const EVP_MD *md)
{
    assert(md != NULL);
    int nid = EVP_MD_nid(md);
    const char *name = NULL;
    const py_hashentry_t *h;

    for (h = py_hashes; h->py_name != NULL; h++) {
        if (h->ossl_nid == nid) {
            name = h->py_name;
            break;
        }
    }
    if (name == NULL) {
        /* Ignore aliased names and only use long, lowercase name. The aliases
         * pollute the list and OpenSSL appears to have its own definition of
         * alias as the resulting list still contains duplicate and alternate
         * names for several algorithms.
         */
        name = OBJ_nid2ln(nid);
        if (name == NULL)
            name = OBJ_nid2sn(nid);
    }

    return PyUnicode_FromString(name);
}

/* Get EVP_MD by HID and purpose */
static PY_EVP_MD*
py_digest_by_name(PyObject *module, const char *name, enum Py_hash_type py_ht)
{
    PY_EVP_MD *digest = NULL;
    PY_EVP_MD *other_digest = NULL;
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
        }
        // if another thread same thing at same time make sure we got same ptr
        assert(other_digest == NULL || other_digest == digest);
        if (digest != NULL) {
            if (other_digest == NULL) {
                PY_EVP_MD_up_ref(digest);
            }
        }
    } else {
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
        }
    }
    if (digest == NULL) {
        raise_ssl_error(state->unsupported_digestmod_error,
                        "unsupported hash type %s", name);
        return NULL;
    }
    return digest;
}

/* Get digest EVP from object
 *
 * * string
 * * _hashopenssl builtin function
 *
 * on error returns NULL with exception set.
 */
static PY_EVP_MD*
py_digest_by_digestmod(PyObject *module, PyObject *digestmod, enum Py_hash_type py_ht) {
    PyObject *name_obj = NULL;
    const char *name;

    if (PyUnicode_Check(digestmod)) {
        name_obj = digestmod;
    } else {
        _hashlibstate *state = get_hashlib_state(module);
        // borrowed ref
        name_obj = PyDict_GetItemWithError(state->constructs, digestmod);
    }
    if (name_obj == NULL) {
        if (!PyErr_Occurred()) {
            _hashlibstate *state = get_hashlib_state(module);
            PyErr_Format(
                state->unsupported_digestmod_error,
                "Unsupported digestmod %R", digestmod);
        }
        return NULL;
    }

    name = PyUnicode_AsUTF8(name_obj);
    if (name == NULL) {
        return NULL;
    }

    return py_digest_by_name(module, name, py_ht);
}

static EVPobject *
newEVPobject(PyTypeObject *type)
{
    EVPobject *retval = PyObject_New(EVPobject, type);
    if (retval == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(retval);

    retval->ctx = EVP_MD_CTX_new();
    if (retval->ctx == NULL) {
        Py_DECREF(retval);
        PyErr_NoMemory();
        return NULL;
    }

    return retval;
}

static int
EVP_hash(EVPobject *self, const void *vp, Py_ssize_t len)
{
    unsigned int process;
    const unsigned char *cp = (const unsigned char *)vp;
    while (0 < len) {
        if (len > (Py_ssize_t)MUNCH_SIZE)
            process = MUNCH_SIZE;
        else
            process = Py_SAFE_DOWNCAST(len, Py_ssize_t, unsigned int);
        if (!EVP_DigestUpdate(self->ctx, (const void*)cp, process)) {
            notify_ssl_error_occurred();
            return -1;
        }
        len -= process;
        cp += process;
    }
    return 0;
}

/* Internal methods for a hash object */

static void
EVP_dealloc(PyObject *op)
{
    EVPobject *self = EVPobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    EVP_MD_CTX_free(self->ctx);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static int
locked_EVP_MD_CTX_copy(EVP_MD_CTX *new_ctx_p, EVPobject *self)
{
    int result;
    ENTER_HASHLIB(self);
    result = EVP_MD_CTX_copy(new_ctx_p, self->ctx);
    LEAVE_HASHLIB(self);
    return result;
}

/* External methods for a hash object */

/*[clinic input]
_hashlib.HASH.copy as EVP_copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
EVP_copy_impl(EVPobject *self)
/*[clinic end generated code: output=b370c21cdb8ca0b4 input=31455b6a3e638069]*/
{
    EVPobject *newobj;

    if ((newobj = newEVPobject(Py_TYPE(self))) == NULL)
        return NULL;

    if (!locked_EVP_MD_CTX_copy(newobj->ctx, self)) {
        Py_DECREF(newobj);
        notify_ssl_error_occurred();
        return NULL;
    }
    return (PyObject *)newobj;
}

/*[clinic input]
_hashlib.HASH.digest as EVP_digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
EVP_digest_impl(EVPobject *self)
/*[clinic end generated code: output=0f6a3a0da46dc12d input=03561809a419bf00]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;
    unsigned int digest_size;

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        goto error;
    }
    digest_size = EVP_MD_CTX_size(temp_ctx);
    if (!EVP_DigestFinal(temp_ctx, digest, NULL)) {
        goto error;
    }

    retval = PyBytes_FromStringAndSize((const char *)digest, digest_size);
    EVP_MD_CTX_free(temp_ctx);
    return retval;

error:
    EVP_MD_CTX_free(temp_ctx);
    notify_ssl_error_occurred();
    return NULL;
}

/*[clinic input]
_hashlib.HASH.hexdigest as EVP_hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
EVP_hexdigest_impl(EVPobject *self)
/*[clinic end generated code: output=18e6decbaf197296 input=aff9cf0e4c741a9a]*/
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    EVP_MD_CTX *temp_ctx;
    unsigned int digest_size;

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    /* Get the raw (binary) digest value */
    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        goto error;
    }
    digest_size = EVP_MD_CTX_size(temp_ctx);
    if (!EVP_DigestFinal(temp_ctx, digest, NULL)) {
        goto error;
    }

    EVP_MD_CTX_free(temp_ctx);

    return _Py_strhex((const char *)digest, (Py_ssize_t)digest_size);

error:
    EVP_MD_CTX_free(temp_ctx);
    notify_ssl_error_occurred();
    return NULL;
}

/*[clinic input]
_hashlib.HASH.update as EVP_update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
EVP_update_impl(EVPobject *self, PyObject *obj)
/*[clinic end generated code: output=d56f91c68348f95f input=9b30ec848f015501]*/
{
    int result;
    Py_buffer view;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &view);

    if (!self->use_mutex && view.len >= HASHLIB_GIL_MINSIZE) {
        self->use_mutex = true;
    }
    if (self->use_mutex) {
        Py_BEGIN_ALLOW_THREADS
        PyMutex_Lock(&self->mutex);
        result = EVP_hash(self, view.buf, view.len);
        PyMutex_Unlock(&self->mutex);
        Py_END_ALLOW_THREADS
    } else {
        result = EVP_hash(self, view.buf, view.len);
    }

    PyBuffer_Release(&view);

    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}

static PyMethodDef EVP_methods[] = {
    EVP_UPDATE_METHODDEF
    EVP_DIGEST_METHODDEF
    EVP_HEXDIGEST_METHODDEF
    EVP_COPY_METHODDEF
    {NULL, NULL}  /* sentinel */
};

static PyObject *
EVP_get_block_size(PyObject *op, void *Py_UNUSED(closure))
{
    EVPobject *self = EVPobject_CAST(op);
    long block_size = EVP_MD_CTX_block_size(self->ctx);
    return PyLong_FromLong(block_size);
}

static PyObject *
EVP_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    EVPobject *self = EVPobject_CAST(op);
    long size = EVP_MD_CTX_size(self->ctx);
    return PyLong_FromLong(size);
}

static PyObject *
EVP_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    EVPobject *self = EVPobject_CAST(op);
    const EVP_MD *md = EVP_MD_CTX_md(self->ctx);
    if (md == NULL) {
        notify_ssl_error_occurred();
        return NULL;
    }
    return py_digest_name(md);
}

static PyGetSetDef EVP_getseters[] = {
    {"digest_size", EVP_get_digest_size, NULL, NULL, NULL},
    {"block_size", EVP_get_block_size, NULL, NULL, NULL},
    {"name", EVP_get_name, NULL, NULL, PyDoc_STR("algorithm name.")},
    {NULL}  /* Sentinel */
};


static PyObject *
EVP_repr(PyObject *self)
{
    PyObject *name = EVP_get_name(self, NULL);
    if (name == NULL) {
        return NULL;
    }
    PyObject *repr = PyUnicode_FromFormat("<%U %T object @ %p>",
                                          name, self, self);
    Py_DECREF(name);
    return repr;
}

PyDoc_STRVAR(hashtype_doc,
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

static PyType_Slot EVPtype_slots[] = {
    {Py_tp_dealloc, EVP_dealloc},
    {Py_tp_repr, EVP_repr},
    {Py_tp_doc, (char *)hashtype_doc},
    {Py_tp_methods, EVP_methods},
    {Py_tp_getset, EVP_getseters},
    {0, 0},
};

static PyType_Spec EVPtype_spec = {
    "_hashlib.HASH",    /*tp_name*/
    sizeof(EVPobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE,
    EVPtype_slots
};

#ifdef PY_OPENSSL_HAS_SHAKE

/*[clinic input]
_hashlib.HASHXOF.digest as EVPXOF_digest

  length: Py_ssize_t

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
EVPXOF_digest_impl(EVPobject *self, Py_ssize_t length)
/*[clinic end generated code: output=ef9320c23280efad input=816a6537cea3d1db]*/
{
    EVP_MD_CTX *temp_ctx;
    PyObject *retval = PyBytes_FromStringAndSize(NULL, length);

    if (retval == NULL) {
        return NULL;
    }

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        Py_DECREF(retval);
        PyErr_NoMemory();
        return NULL;
    }

    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        goto error;
    }
    if (!EVP_DigestFinalXOF(temp_ctx,
                            (unsigned char*)PyBytes_AS_STRING(retval),
                            length))
    {
        goto error;
    }

    EVP_MD_CTX_free(temp_ctx);
    return retval;

error:
    Py_DECREF(retval);
    EVP_MD_CTX_free(temp_ctx);
    notify_ssl_error_occurred();
    return NULL;
}

/*[clinic input]
_hashlib.HASHXOF.hexdigest as EVPXOF_hexdigest

    length: Py_ssize_t

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
EVPXOF_hexdigest_impl(EVPobject *self, Py_ssize_t length)
/*[clinic end generated code: output=eb3e6ee7788bf5b2 input=5f9d6a8f269e34df]*/
{
    unsigned char *digest;
    EVP_MD_CTX *temp_ctx;
    PyObject *retval;

    digest = (unsigned char*)PyMem_Malloc(length);
    if (digest == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    temp_ctx = EVP_MD_CTX_new();
    if (temp_ctx == NULL) {
        PyMem_Free(digest);
        PyErr_NoMemory();
        return NULL;
    }

    /* Get the raw (binary) digest value */
    if (!locked_EVP_MD_CTX_copy(temp_ctx, self)) {
        goto error;
    }
    if (!EVP_DigestFinalXOF(temp_ctx, digest, length)) {
        goto error;
    }

    EVP_MD_CTX_free(temp_ctx);

    retval = _Py_strhex((const char *)digest, length);
    PyMem_Free(digest);
    return retval;

error:
    PyMem_Free(digest);
    EVP_MD_CTX_free(temp_ctx);
    notify_ssl_error_occurred();
    return NULL;
}

static PyMethodDef EVPXOF_methods[] = {
    EVPXOF_DIGEST_METHODDEF
    EVPXOF_HEXDIGEST_METHODDEF
    {NULL, NULL}  /* sentinel */
};


static PyObject *
EVPXOF_get_digest_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(0);
}

static PyGetSetDef EVPXOF_getseters[] = {
    {"digest_size", EVPXOF_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(hashxoftype_doc,
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

static PyType_Slot EVPXOFtype_slots[] = {
    {Py_tp_doc, (char *)hashxoftype_doc},
    {Py_tp_methods, EVPXOF_methods},
    {Py_tp_getset, EVPXOF_getseters},
    {0, 0},
};

static PyType_Spec EVPXOFtype_spec = {
    "_hashlib.HASHXOF",    /*tp_name*/
    sizeof(EVPobject),  /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE,
    EVPXOFtype_slots
};


#endif

static PyObject*
py_evp_fromname(PyObject *module, const char *digestname, PyObject *data_obj,
                int usedforsecurity)
{
    Py_buffer view = { 0 };
    PY_EVP_MD *digest = NULL;
    PyTypeObject *type;
    EVPobject *self = NULL;

    if (data_obj != NULL) {
        GET_BUFFER_VIEW_OR_ERROUT(data_obj, &view);
    }

    digest = py_digest_by_name(
        module, digestname, usedforsecurity ? Py_ht_evp : Py_ht_evp_nosecurity
    );
    if (digest == NULL) {
        goto exit;
    }

    if ((EVP_MD_flags(digest) & EVP_MD_FLAG_XOF) == EVP_MD_FLAG_XOF) {
        type = get_hashlib_state(module)->EVPXOFtype;
    } else {
        type = get_hashlib_state(module)->EVPtype;
    }

    self = newEVPobject(type);
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
        notify_ssl_error_occurred();
        Py_CLEAR(self);
        goto exit;
    }

    if (view.buf && view.len) {
        if (view.len >= HASHLIB_GIL_MINSIZE) {
            /* We do not initialize self->lock here as this is the constructor
             * where it is not yet possible to have concurrent access. */
            Py_BEGIN_ALLOW_THREADS
            result = EVP_hash(self, view.buf, view.len);
            Py_END_ALLOW_THREADS
        } else {
            result = EVP_hash(self, view.buf, view.len);
        }
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


/* The module-level function: new() */

/*[clinic input]
_hashlib.new as EVP_new

    name as name_obj: object
    string as data_obj: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True

Return a new hash object using the named algorithm.

An optional string argument may be provided and will be
automatically hashed.

The MD5 and SHA1 algorithms are always supported.
[clinic start generated code]*/

static PyObject *
EVP_new_impl(PyObject *module, PyObject *name_obj, PyObject *data_obj,
             int usedforsecurity)
/*[clinic end generated code: output=ddd5053f92dffe90 input=c24554d0337be1b0]*/
{
    char *name;
    if (!PyArg_Parse(name_obj, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        return NULL;
    }
    return py_evp_fromname(module, name, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_md5

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a md5 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_md5_impl(PyObject *module, PyObject *data_obj,
                          int usedforsecurity)
/*[clinic end generated code: output=87b0186440a44f8c input=990e36d5e689b16e]*/
{
    return py_evp_fromname(module, Py_hash_md5, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha1

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha1 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha1_impl(PyObject *module, PyObject *data_obj,
                           int usedforsecurity)
/*[clinic end generated code: output=6813024cf690670d input=948f2f4b6deabc10]*/
{
    return py_evp_fromname(module, Py_hash_sha1, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha224

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha224_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=a2dfe7cc4eb14ebb input=f9272821fadca505]*/
{
    return py_evp_fromname(module, Py_hash_sha224, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha256_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=1f874a34870f0a68 input=549fad9d2930d4c5]*/
{
    return py_evp_fromname(module, Py_hash_sha256, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha384

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha384_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=58529eff9ca457b2 input=48601a6e3bf14ad7]*/
{
    return py_evp_fromname(module, Py_hash_sha384, data_obj, usedforsecurity);
}


/*[clinic input]
_hashlib.openssl_sha512

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha512_impl(PyObject *module, PyObject *data_obj,
                             int usedforsecurity)
/*[clinic end generated code: output=2c744c9e4a40d5f6 input=c5c46a2a817aa98f]*/
{
    return py_evp_fromname(module, Py_hash_sha512, data_obj, usedforsecurity);
}


#ifdef PY_OPENSSL_HAS_SHA3

/*[clinic input]
_hashlib.openssl_sha3_224

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-224 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_224_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=144641c1d144b974 input=e3a01b2888916157]*/
{
    return py_evp_fromname(module, Py_hash_sha3_224, data_obj, usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-256 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_256_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=c61f1ab772d06668 input=e2908126c1b6deed]*/
{
    return py_evp_fromname(module, Py_hash_sha3_256, data_obj , usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_384

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-384 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_384_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=f68e4846858cf0ee input=ec0edf5c792f8252]*/
{
    return py_evp_fromname(module, Py_hash_sha3_384, data_obj , usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_sha3_512

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a sha3-512 hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_sha3_512_impl(PyObject *module, PyObject *data_obj,
                               int usedforsecurity)
/*[clinic end generated code: output=2eede478c159354a input=64e2cc0c094d56f4]*/
{
    return py_evp_fromname(module, Py_hash_sha3_512, data_obj , usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHA3 */

#ifdef PY_OPENSSL_HAS_SHAKE
/*[clinic input]
_hashlib.openssl_shake_128

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a shake-128 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_128_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity)
/*[clinic end generated code: output=bc49cdd8ada1fa97 input=6c9d67440eb33ec8]*/
{
    return py_evp_fromname(module, Py_hash_shake_128, data_obj , usedforsecurity);
}

/*[clinic input]
_hashlib.openssl_shake_256

    string as data_obj: object(py_default="b''") = NULL
    *
    usedforsecurity: bool = True

Returns a shake-256 variable hash object; optionally initialized with a string

[clinic start generated code]*/

static PyObject *
_hashlib_openssl_shake_256_impl(PyObject *module, PyObject *data_obj,
                                int usedforsecurity)
/*[clinic end generated code: output=358d213be8852df7 input=479cbe9fefd4a9f8]*/
{
    return py_evp_fromname(module, Py_hash_shake_256, data_obj , usedforsecurity);
}
#endif /* PY_OPENSSL_HAS_SHAKE */

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

    PY_EVP_MD *digest = py_digest_by_name(module, hash_name, Py_ht_pbkdf2);
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
        notify_ssl_error_occurred();
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
        raise_ssl_error(PyExc_ValueError,
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
        notify_ssl_error_occurred();
        return NULL;
    }
    return key_obj;
}
#endif  /* PY_OPENSSL_HAS_SCRYPT */

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
    unsigned char md[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len = 0;
    unsigned char *result;
    PY_EVP_MD *evp;

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

    evp = py_digest_by_digestmod(module, digest, Py_ht_mac);
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

    if (result == NULL) {
        notify_ssl_error_occurred();
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char*)md, md_len);
}

/* OpenSSL-based HMAC implementation
 */

static int _hmac_update(HMACobject*, PyObject*);

static const EVP_MD *
_hashlib_hmac_get_md(HMACobject *self)
{
    const EVP_MD *md = HMAC_CTX_get_md(self->ctx);
    if (md == NULL) {
        raise_ssl_error(PyExc_ValueError, "missing EVP_MD for HMAC context");
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
    PY_EVP_MD *digest;
    HMAC_CTX *ctx = NULL;
    HMACobject *self = NULL;
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

    digest = py_digest_by_digestmod(module, digestmod, Py_ht_mac);
    if (digest == NULL) {
        return NULL;
    }

    ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        PY_EVP_MD_free(digest);
        PyErr_NoMemory();
        goto error;
    }

    r = HMAC_Init_ex(ctx, key->buf, (int)key->len, digest, NULL /* impl */);
    PY_EVP_MD_free(digest);
    if (r == 0) {
        notify_ssl_error_occurred();
        goto error;
    }

    _hashlibstate *state = get_hashlib_state(module);
    self = PyObject_New(HMACobject, state->HMACtype);
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
    ENTER_HASHLIB(self);
    result = HMAC_CTX_copy(new_ctx_p, self->ctx);
    LEAVE_HASHLIB(self);
    return result;
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
        raise_ssl_error(PyExc_ValueError, "invalid digest size");
    }
    return digest_size;
}

static int
_hmac_update(HMACobject *self, PyObject *obj)
{
    int r;
    Py_buffer view = {0};

    GET_BUFFER_VIEW_OR_ERROR(obj, &view, return 0);

    if (!self->use_mutex && view.len >= HASHLIB_GIL_MINSIZE) {
        self->use_mutex = true;
    }
    if (self->use_mutex) {
        Py_BEGIN_ALLOW_THREADS
        PyMutex_Lock(&self->mutex);
        r = HMAC_Update(self->ctx,
                        (const unsigned char *)view.buf,
                        (size_t)view.len);
        PyMutex_Unlock(&self->mutex);
        Py_END_ALLOW_THREADS
    } else {
        r = HMAC_Update(self->ctx,
                        (const unsigned char *)view.buf,
                        (size_t)view.len);
    }

    PyBuffer_Release(&view);

    if (r == 0) {
        notify_ssl_error_occurred();
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

    HMAC_CTX *ctx = HMAC_CTX_new();
    if (ctx == NULL) {
        return PyErr_NoMemory();
    }
    if (!locked_HMAC_CTX_copy(ctx, self)) {
        HMAC_CTX_free(ctx);
        notify_ssl_error_occurred();
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
    HMACobject *self = HMACobject_CAST(op);
    const EVP_MD *md = _hashlib_hmac_get_md(self);
    if (md == NULL) {
        return NULL;
    }
    PyObject *digest_name = py_digest_name(md);
    if (digest_name == NULL) {
        return NULL;
    }
    PyObject *repr = PyUnicode_FromFormat(
        "<%U HMAC object @ %p>", digest_name, self
    );
    Py_DECREF(digest_name);
    return repr;
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
    HMAC_CTX *temp_ctx = HMAC_CTX_new();
    if (temp_ctx == NULL) {
        (void)PyErr_NoMemory();
        return 0;
    }
    if (!locked_HMAC_CTX_copy(temp_ctx, self)) {
        HMAC_CTX_free(temp_ctx);
        notify_ssl_error_occurred();
        return 0;
    }
    int r = HMAC_Final(temp_ctx, buf, &len);
    HMAC_CTX_free(temp_ctx);
    if (r == 0) {
        notify_ssl_error_occurred();
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
_hashlib.HMAC.hexdigest

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.
[clinic start generated code]*/

static PyObject *
_hashlib_HMAC_hexdigest_impl(HMACobject *self)
/*[clinic end generated code: output=80d825be1eaae6a7 input=5abc42702874ddcf]*/
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
    PyObject *digest_name = py_digest_name(md);
    if (digest_name == NULL) {
        return NULL;
    }
    PyObject *name = PyUnicode_FromFormat("hmac-%U", digest_name);
    Py_DECREF(digest_name);
    return name;
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
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
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

    py_name = py_digest_name(md);
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

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
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
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    return EVP_default_properties_is_fips_enabled(NULL);
#else
    ERR_clear_error();
    int result = FIPS_mode();
    if (result == 0) {
        // "If the library was built without support of the FIPS Object Module,
        // then the function will return 0 with an error code of
        // CRYPTO_R_FIPS_MODE_NOT_SUPPORTED (0x0f06d065)."
        // But 0 is also a valid result value.
        unsigned long errcode = ERR_peek_last_error();
        if (errcode) {
            notify_ssl_error_occurred();
            return -1;
        }
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
    EVP_NEW_METHODDEF
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
    Py_VISIT(state->EVPtype);
    Py_VISIT(state->HMACtype);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_VISIT(state->EVPXOFtype);
#endif
    Py_VISIT(state->constructs);
    Py_VISIT(state->unsupported_digestmod_error);
    return 0;
}

static int
hashlib_clear(PyObject *m)
{
    _hashlibstate *state = get_hashlib_state(m);
    Py_CLEAR(state->EVPtype);
    Py_CLEAR(state->HMACtype);
#ifdef PY_OPENSSL_HAS_SHAKE
    Py_CLEAR(state->EVPXOFtype);
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
hashlib_init_evptype(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->EVPtype = (PyTypeObject *)PyType_FromSpec(&EVPtype_spec);
    if (state->EVPtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->EVPtype) < 0) {
        return -1;
    }
    return 0;
}

static int
hashlib_init_evpxoftype(PyObject *module)
{
#ifdef PY_OPENSSL_HAS_SHAKE
    _hashlibstate *state = get_hashlib_state(module);

    if (state->EVPtype == NULL) {
        return -1;
    }

    state->EVPXOFtype = (PyTypeObject *)PyType_FromSpecWithBases(
        &EVPXOFtype_spec, (PyObject *)state->EVPtype
    );
    if (state->EVPXOFtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->EVPXOFtype) < 0) {
        return -1;
    }
#endif
    return 0;
}

static int
hashlib_init_hmactype(PyObject *module)
{
    _hashlibstate *state = get_hashlib_state(module);

    state->HMACtype = (PyTypeObject *)PyType_FromSpec(&HMACtype_spec);
    if (state->HMACtype == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->HMACtype) < 0) {
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
    {Py_mod_exec, hashlib_init_evptype},
    {Py_mod_exec, hashlib_init_evpxoftype},
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

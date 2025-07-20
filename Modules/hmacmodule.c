/*
 * @author Bénédikt Tran
 *
 * Implement the HMAC algorithm as described by RFC 2104 using HACL*.
 *
 * Using HACL* implementation implicitly assumes that the caller wants
 * a formally verified implementation. In particular, only algorithms
 * given by their names will be recognized.
 *
 * Some algorithms exposed by `_hashlib` such as truncated SHA-2-512-224/256
 * are not yet implemented by the HACL* project. Nonetheless, the supported
 * HMAC algorithms form a subset of those supported by '_hashlib'.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_hashtable.h"
#include "pycore_strhex.h"              // _Py_strhex()

/*
 * Taken from blake2module.c. In the future, detection of SIMD support
 * should be delegated to https://github.com/python/cpython/pull/125011.
 */
#if defined(__x86_64__) && defined(__GNUC__)
#  include <cpuid.h>
#elif defined(_M_X64)
#  include <intrin.h>
#endif

#if defined(__APPLE__) && defined(__arm64__)
#  undef _Py_HACL_CAN_COMPILE_VEC128
#  undef _Py_HACL_CAN_COMPILE_VEC256
#endif

// HACL* expects HACL_CAN_COMPILE_VEC* macros to be set in order to enable
// the corresponding SIMD instructions so we need to "forward" the values
// we just deduced above.
#define HACL_CAN_COMPILE_VEC128 _Py_HACL_CAN_COMPILE_VEC128
#define HACL_CAN_COMPILE_VEC256 _Py_HACL_CAN_COMPILE_VEC256

#include "_hacl/Hacl_HMAC.h"
#include "_hacl/Hacl_Streaming_HMAC.h"  // Hacl_Agile_Hash_* identifiers
#include "_hacl/Hacl_Streaming_Types.h" // Hacl_Streaming_Types_error_code

#include <stdbool.h>

#include "hashlib.h"

// --- Reusable error messages ------------------------------------------------

static inline void
set_invalid_key_length_error(void)
{
    (void)PyErr_Format(PyExc_OverflowError,
                       "key length exceeds %u",
                       UINT32_MAX);
}

static inline void
set_invalid_msg_length_error(void)
{
    (void)PyErr_Format(PyExc_OverflowError,
                       "message length exceeds %u",
                       UINT32_MAX);
}

// --- HMAC underlying hash function static information -----------------------

#define UINT32_MAX_AS_SSIZE_T                   ((Py_ssize_t)UINT32_MAX)

#define Py_hmac_hash_max_block_size             144
#define Py_hmac_hash_max_digest_size            64

/* MD-5 */
// HACL_HID = md5
#define Py_hmac_md5_block_size                  64
#define Py_hmac_md5_digest_size                 16

#define Py_hmac_md5_compute_func                Hacl_HMAC_compute_md5

/* SHA-1 family */
// HACL_HID = sha1
#define Py_hmac_sha1_block_size                 64
#define Py_hmac_sha1_digest_size                20

#define Py_hmac_sha1_compute_func               Hacl_HMAC_compute_sha1

/* SHA-2 family */
// HACL_HID = sha2_224
#define Py_hmac_sha2_224_block_size             64
#define Py_hmac_sha2_224_digest_size            28

#define Py_hmac_sha2_224_compute_func           Hacl_HMAC_compute_sha2_224

// HACL_HID = sha2_256
#define Py_hmac_sha2_256_block_size             64
#define Py_hmac_sha2_256_digest_size            32

#define Py_hmac_sha2_256_compute_func           Hacl_HMAC_compute_sha2_256

// HACL_HID = sha2_384
#define Py_hmac_sha2_384_block_size             128
#define Py_hmac_sha2_384_digest_size            48

#define Py_hmac_sha2_384_compute_func           Hacl_HMAC_compute_sha2_384

// HACL_HID = sha2_512
#define Py_hmac_sha2_512_block_size             128
#define Py_hmac_sha2_512_digest_size            64

#define Py_hmac_sha2_512_compute_func           Hacl_HMAC_compute_sha2_512

/* SHA-3 family */
// HACL_HID = sha3_224
#define Py_hmac_sha3_224_block_size             144
#define Py_hmac_sha3_224_digest_size            28

#define Py_hmac_sha3_224_compute_func           Hacl_HMAC_compute_sha3_224

// HACL_HID = sha3_256
#define Py_hmac_sha3_256_block_size             136
#define Py_hmac_sha3_256_digest_size            32

#define Py_hmac_sha3_256_compute_func           Hacl_HMAC_compute_sha3_256

// HACL_HID = sha3_384
#define Py_hmac_sha3_384_block_size             104
#define Py_hmac_sha3_384_digest_size            48

#define Py_hmac_sha3_384_compute_func           Hacl_HMAC_compute_sha3_384

// HACL_HID = sha3_512
#define Py_hmac_sha3_512_block_size             72
#define Py_hmac_sha3_512_digest_size            64

#define Py_hmac_sha3_512_compute_func           Hacl_HMAC_compute_sha3_512

/* Blake2 family */
// HACL_HID = blake2s_32
#define Py_hmac_blake2s_32_block_size           64
#define Py_hmac_blake2s_32_digest_size          32

#define Py_hmac_blake2s_32_compute_func         Hacl_HMAC_compute_blake2s_32

// HACL_HID = blake2b_32
#define Py_hmac_blake2b_32_block_size           128
#define Py_hmac_blake2b_32_digest_size          64

#define Py_hmac_blake2b_32_compute_func         Hacl_HMAC_compute_blake2b_32

/* Enumeration indicating the underlying hash function used by HMAC. */
typedef enum HMAC_Hash_Kind {
    Py_hmac_kind_hash_unknown = -1,
#define DECL_HACL_HMAC_HASH_KIND(NAME, HACL_NAME)  \
    Py_hmac_kind_hmac_ ## NAME = Hacl_Agile_Hash_ ## HACL_NAME,
    /* MD5 */
    DECL_HACL_HMAC_HASH_KIND(md5, MD5)
    /* SHA-1 */
    DECL_HACL_HMAC_HASH_KIND(sha1, SHA1)
    /* SHA-2 family */
    DECL_HACL_HMAC_HASH_KIND(sha2_224, SHA2_224)
    DECL_HACL_HMAC_HASH_KIND(sha2_256, SHA2_256)
    DECL_HACL_HMAC_HASH_KIND(sha2_384, SHA2_384)
    DECL_HACL_HMAC_HASH_KIND(sha2_512, SHA2_512)
    /* SHA-3 family */
    DECL_HACL_HMAC_HASH_KIND(sha3_224, SHA3_224)
    DECL_HACL_HMAC_HASH_KIND(sha3_256, SHA3_256)
    DECL_HACL_HMAC_HASH_KIND(sha3_384, SHA3_384)
    DECL_HACL_HMAC_HASH_KIND(sha3_512, SHA3_512)
    /* Blake family */
    DECL_HACL_HMAC_HASH_KIND(blake2s_32, Blake2S_32)
    DECL_HACL_HMAC_HASH_KIND(blake2b_32, Blake2B_32)
    /* Blake runtime family (should not be used statically) */
    DECL_HACL_HMAC_HASH_KIND(vectorized_blake2s_32, Blake2S_128)
    DECL_HACL_HMAC_HASH_KIND(vectorized_blake2b_32, Blake2B_256)
#undef DECL_HACL_HMAC_HASH_KIND
} HMAC_Hash_Kind;

typedef Hacl_Streaming_Types_error_code hacl_errno_t;

/* Function pointer type for 1-shot HACL* HMAC functions. */
typedef void
(*HACL_HMAC_compute_func)(uint8_t *out,
                          uint8_t *key, uint32_t keylen,
                          uint8_t *msg, uint32_t msglen);
/* Function pointer type for 1-shot HACL* HMAC CPython AC functions. */
typedef PyObject *
(*PyAC_HMAC_compute_func)(PyObject *module, PyObject *key, PyObject *msg);

/*
 * HACL* HMAC minimal interface.
 */
typedef struct py_hmac_hacl_api {
    HACL_HMAC_compute_func compute;
    PyAC_HMAC_compute_func compute_py;
} py_hmac_hacl_api;

#if PY_SSIZE_T_MAX > UINT32_MAX
#define Py_HMAC_SSIZE_LARGER_THAN_UINT32
#endif

/*
 * Assert that 'LEN' can be safely casted to uint32_t.
 *
 * The 'LEN' parameter should be convertible to Py_ssize_t.
 */
#ifdef Py_HMAC_SSIZE_LARGER_THAN_UINT32
#define Py_CHECK_HACL_UINT32_T_LENGTH(LEN)                  \
    do {                                                    \
        assert((Py_ssize_t)(LEN) <= UINT32_MAX_AS_SSIZE_T); \
    } while (0)
#else
#define Py_CHECK_HACL_UINT32_T_LENGTH(LEN)
#endif

/*
 * HMAC underlying hash function static information.
 */
typedef struct py_hmac_hinfo {
    /*
     * Name of the hash function used by the HACL* HMAC module.
     *
     * This name may differ from the hashlib names. For instance,
     * SHA-2/224 is named "sha2_224" instead of "sha224" as it is
     * done by 'hashlib'.
     */
    const char *name;

    /* hash function information */
    HMAC_Hash_Kind kind;
    uint32_t block_size;
    uint32_t digest_size;

    /* HACL* HMAC API */
    py_hmac_hacl_api api;

    /*
     * Cached field storing the 'hashlib_name' field as a Python string.
     *
     * This field is NULL by default in the items of "py_hmac_static_hinfo"
     * but will be populated when creating the module's state "hinfo_table".
     */
    PyObject *display_name;
    const char *hashlib_name;   /* hashlib preferred name (default: name) */

    Py_ssize_t refcnt;
} py_hmac_hinfo;

// --- HMAC module state ------------------------------------------------------

typedef struct hmacmodule_state {
    _Py_hashtable_t *hinfo_table;
    PyObject *unknown_hash_error;
    /* HMAC object type */
    PyTypeObject *hmac_type;
    /* interned strings */
    PyObject *str_lower;

    bool can_run_simd128;
    bool can_run_simd256;
} hmacmodule_state;

static inline hmacmodule_state *
get_hmacmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (hmacmodule_state *)state;
}

static inline hmacmodule_state *
get_hmacmodule_state_by_cls(PyTypeObject *cls)
{
    void *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    return (hmacmodule_state *)state;
}

// --- HMAC Object ------------------------------------------------------------

typedef Hacl_Streaming_HMAC_agile_state HACL_HMAC_state;

typedef struct HMACObject {
    HASHLIB_OBJECT_HEAD
    // Hash function information
    PyObject *name;         // rendered name (exact unicode object)
    HMAC_Hash_Kind kind;    // can be used for runtime dispatch (must be known)
    uint32_t block_size;
    uint32_t digest_size;
    py_hmac_hacl_api api;

    // HMAC HACL* internal state.
    HACL_HMAC_state *state;
} HMACObject;

#define HMACObject_CAST(op) ((HMACObject *)(op))

// --- HMAC module clinic configuration ---------------------------------------

/*[clinic input]
module _hmac
class _hmac.HMAC "HMACObject *" "clinic_state()->hmac_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c8bab73fde49ba8a]*/

#define clinic_state()  (get_hmacmodule_state_by_cls(Py_TYPE(self)))
#include "clinic/hmacmodule.c.h"
#undef clinic_state

// --- Helpers ----------------------------------------------------------------
//
// The helpers have the following naming conventions:
//
// - Helpers with the "_hacl" prefix are thin wrappers around HACL* functions.
//   Buffer lengths given as inputs should fit on 32-bit integers.
//
// - Helpers with the "hmac_" prefix act on HMAC objects and accept buffers
//   whose length fits on 32-bit or 64-bit integers (depending on the host
//   machine).

/*
 * Assert that a HMAC hash kind is a static kind.
 *
 * A "static" kind is specified in the 'py_hmac_static_hinfo'
 * table and is always independent of the host CPUID features.
 */
#ifndef NDEBUG
static void
assert_is_static_hmac_hash_kind(HMAC_Hash_Kind kind)
{
    switch (kind) {
        case Py_hmac_kind_hash_unknown: {
            Py_FatalError("HMAC hash kind must be a known kind");
            return;
        }
        case Py_hmac_kind_hmac_vectorized_blake2s_32:
        case Py_hmac_kind_hmac_vectorized_blake2b_32: {
            Py_FatalError("HMAC hash kind must not be a vectorized kind");
            return;
        }
        default:
            return;
    }
}
#else
static inline void
assert_is_static_hmac_hash_kind(HMAC_Hash_Kind Py_UNUSED(kind)) {}
#endif

/*
 * Convert a HMAC hash static kind into a runtime kind.
 *
 * A "runtime" kind is derived from a static kind and depends
 * on the host CPUID features. In particular, this is the kind
 * that a HMAC object internally stores.
 */
static HMAC_Hash_Kind
narrow_hmac_hash_kind(hmacmodule_state *state, HMAC_Hash_Kind kind)
{
    switch (kind) {
        case Py_hmac_kind_hmac_blake2s_32: {
#if _Py_HACL_CAN_COMPILE_VEC128
            if (state->can_run_simd128) {
                return Py_hmac_kind_hmac_vectorized_blake2s_32;
            }
#endif
            return kind;
        }
        case Py_hmac_kind_hmac_blake2b_32: {
#if _Py_HACL_CAN_COMPILE_VEC256
            if (state->can_run_simd256) {
                return Py_hmac_kind_hmac_vectorized_blake2b_32;
            }
#endif
            return kind;
        }
        default:
            return kind;
    }
}

/*
 * Handle the HACL* exit code.
 *
 * If 'code' represents a successful operation, this returns 0.
 * Otherwise, this sets an appropriate exception and returns -1.
 */
static int
_hacl_convert_errno(hacl_errno_t code)
{
    assert(PyGILState_GetThisThreadState() != NULL);
    if (code == Hacl_Streaming_Types_Success) {
        return 0;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    switch (code) {
        case Hacl_Streaming_Types_InvalidAlgorithm: {
            PyErr_SetString(PyExc_ValueError, "invalid HACL* algorithm");
            break;
        }
        case Hacl_Streaming_Types_InvalidLength: {
            PyErr_SetString(PyExc_ValueError, "invalid length");
            break;
        }
        case Hacl_Streaming_Types_MaximumLengthExceeded: {
            PyErr_SetString(PyExc_OverflowError, "maximum length exceeded");
            break;
        }
        case Hacl_Streaming_Types_OutOfMemory: {
            PyErr_NoMemory();
            break;
        }
        default: {
            PyErr_Format(PyExc_RuntimeError,
                         "HACL* internal routine failed with error code: %u",
                         code);
            break;
        }
    }
    PyGILState_Release(gstate);
    return -1;
}

/*
 * Return a new HACL* internal state or return NULL on failure.
 *
 * An appropriate exception is set if the state cannot be created.
 */
static HACL_HMAC_state *
_hacl_hmac_state_new(HMAC_Hash_Kind kind, uint8_t *key, uint32_t len)
{
    assert(kind != Py_hmac_kind_hash_unknown);
    HACL_HMAC_state *state = NULL;
    hacl_errno_t retcode = Hacl_Streaming_HMAC_malloc_(kind, key, len, &state);
    if (_hacl_convert_errno(retcode) < 0) {
        assert(state == NULL);
        return NULL;
    }
    return state;
}

/*
 * Free the HACL* internal state.
 */
static inline void
_hacl_hmac_state_free(HACL_HMAC_state *state)
{
    if (state != NULL) {
        Hacl_Streaming_HMAC_free(state);
    }
}

/*
 * Call the HACL* HMAC-HASH update function on the given data.
 *
 * On DEBUG builds, the update() call is verified.
 *
 * Return 0 on success; otherwise, set an exception and return -1 on failure.
*/
static int
_hacl_hmac_state_update_once(HACL_HMAC_state *state,
                             uint8_t *buf, uint32_t len)
{
#ifndef NDEBUG
    hacl_errno_t code = Hacl_Streaming_HMAC_update(state, buf, len);
    return _hacl_convert_errno(code);
#else
    (void)Hacl_Streaming_HMAC_update(state, buf, len);
    return 0;
#endif
}

/*
 * Perform the HMAC-HASH update() operation in a streaming fashion.
 *
 * On DEBUG builds, each update() call is verified.
 *
 * Return 0 on success; otherwise, set an exception and return -1 on failure.
 */
static int
_hacl_hmac_state_update(HACL_HMAC_state *state, uint8_t *buf, Py_ssize_t len)
{
    assert(len >= 0);
#ifdef Py_HMAC_SSIZE_LARGER_THAN_UINT32
    while (len > UINT32_MAX_AS_SSIZE_T) {
        if (_hacl_hmac_state_update_once(state, buf, UINT32_MAX) < 0) {
            assert(PyErr_Occurred());
            return -1;
        }
        buf += UINT32_MAX;
        len -= UINT32_MAX;
    }
#endif
    Py_CHECK_HACL_UINT32_T_LENGTH(len);
    return _hacl_hmac_state_update_once(state, buf, (uint32_t)len);
}

/* Static information used to construct the hash table. */
static const py_hmac_hinfo py_hmac_static_hinfo[] = {
#define Py_HMAC_HINFO_HACL_API(HACL_HID)                                \
    {                                                                   \
        /* one-shot helpers */                                          \
        .compute = &Py_hmac_## HACL_HID ##_compute_func,                \
        .compute_py = &_hmac_compute_## HACL_HID ##_impl,               \
    }

#define Py_HMAC_HINFO_ENTRY(HACL_HID, HLIB_NAME)            \
    {                                                       \
        .name = Py_STRINGIFY(HACL_HID),                     \
        .kind = Py_hmac_kind_hmac_ ## HACL_HID,             \
        .block_size = Py_hmac_## HACL_HID ##_block_size,    \
        .digest_size = Py_hmac_## HACL_HID ##_digest_size,  \
        .api = Py_HMAC_HINFO_HACL_API(HACL_HID),            \
        .display_name = NULL,                               \
        .hashlib_name = HLIB_NAME,                          \
        .refcnt = 0,                                        \
    }
    /* MD5 */
    Py_HMAC_HINFO_ENTRY(md5, NULL),
    /* SHA-1 */
    Py_HMAC_HINFO_ENTRY(sha1, NULL),
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
    Py_HMAC_HINFO_ENTRY(blake2s_32, "blake2s"),
    Py_HMAC_HINFO_ENTRY(blake2b_32, "blake2b"),
#undef Py_HMAC_HINFO_ENTRY
#undef Py_HMAC_HINFO_HACL_API
    /* sentinel */
    {
        NULL, Py_hmac_kind_hash_unknown, 0, 0,
        {NULL, NULL},
        NULL, NULL,
        0,
    },
};

/*
 * Check whether 'name' is a known HMAC hash function name,
 * storing the corresponding static information in 'info'.
 *
 * This function always succeeds and never set an exception.
 */
static inline bool
find_hash_info_by_utf8name(hmacmodule_state *state,
                           const char *name,
                           const py_hmac_hinfo **info)
{
    assert(name != NULL);
    *info = _Py_hashtable_get(state->hinfo_table, name);
    return *info != NULL;
}

/*
 * Find the corresponding HMAC hash function static information by its name.
 *
 * On error, propagate the exception, set 'info' to NULL and return -1.
 *
 * If no correspondence exists, set 'info' to NULL and return 0.
 * Otherwise, set 'info' to the deduced information and return 1.
 *
 * Parameters
 *
 *      state           The HMAC module state.
 *      name            The hash function name.
 *      info            The deduced information, if any.
 */
static int
find_hash_info_by_name(hmacmodule_state *state,
                       PyObject *name,
                       const py_hmac_hinfo **info)
{
    const char *utf8name = PyUnicode_AsUTF8(name);
    if (utf8name == NULL) {
        goto error;
    }
    if (find_hash_info_by_utf8name(state, utf8name, info)) {
        return 1;
    }

    // try to find an alternative using the lowercase name
    PyObject *lower = PyObject_CallMethodNoArgs(name, state->str_lower);
    if (lower == NULL) {
        goto error;
    }
    const char *utf8lower = PyUnicode_AsUTF8(lower);
    if (utf8lower == NULL) {
        Py_DECREF(lower);
        goto error;
    }
    int found = find_hash_info_by_utf8name(state, utf8lower, info);
    Py_DECREF(lower);
    return found;

error:
    *info = NULL;
    return -1;
}

/*
 * Find the corresponding HMAC hash function static information.
 *
 * On error, propagate the exception, set 'info' to NULL and return -1.
 *
 * If no correspondence exists, set 'info' to NULL and return 0.
 * Otherwise, set 'info' to the deduced information and return 1.
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
    // NOTE(picnixz): For now, we only support named algorithms.
    // In the future, we need to decide whether 'hashlib.openssl_md5'
    // would make sense as an alias to 'md5' and how to remove OpenSSL.
    *info = NULL;
    return 0;
}

/*
 * Find the corresponding HMAC hash function static information.
 *
 * If nothing can be found or if an error occurred, return NULL
 * with an exception set. Otherwise return a non-NULL object.
 */
static const py_hmac_hinfo *
find_hash_info(hmacmodule_state *state, PyObject *hash_info_ref)
{
    const py_hmac_hinfo *info = NULL;
    int rc = find_hash_info_impl(state, hash_info_ref, &info);
    // The code below could be simplfied with only 'rc == 0' case,
    // but we are deliberately verbose to ease future improvements.
    if (rc < 0) {
        return NULL;
    }
    if (rc == 0) {
        PyErr_Format(state->unknown_hash_error,
                     HASHLIB_UNSUPPORTED_ALGORITHM, hash_info_ref);
        return NULL;
    }
    assert(info != NULL);
    return info;
}

/* Check that the buffer length fits on a uint32_t. */
static inline int
has_uint32_t_buffer_length(const Py_buffer *buffer)
{
#ifdef Py_HMAC_SSIZE_LARGER_THAN_UINT32
    return buffer->len <= UINT32_MAX_AS_SSIZE_T;
#else
    return 1;
#endif
}

// --- HMAC object ------------------------------------------------------------

/*
 * Use the HMAC information 'info' to populate the corresponding fields.
 *
 * The real 'kind' for BLAKE-2 is obtained once and depends on both static
 * capabilities (supported compiler flags) and runtime CPUID features.
 */
static void
hmac_set_hinfo(hmacmodule_state *state,
               HMACObject *self, const py_hmac_hinfo *info)
{
    assert(info->display_name != NULL);
    self->name = Py_NewRef(info->display_name);
    assert_is_static_hmac_hash_kind(info->kind);
    self->kind = narrow_hmac_hash_kind(state, info->kind);
    assert(info->block_size <= Py_hmac_hash_max_block_size);
    self->block_size = info->block_size;
    assert(info->digest_size <= Py_hmac_hash_max_digest_size);
    self->digest_size = info->digest_size;
    assert(info->api.compute != NULL);
    assert(info->api.compute_py != NULL);
    self->api = info->api;
}

/*
 * Create initial HACL* internal state with the given key.
 *
 * This function MUST only be called by the HMAC object constructor
 * and after hmac_set_hinfo() has been called, lest the behaviour is
 * undefined.
 *
 * Return 0 on success; otherwise, set an exception and return -1 on failure.
 */
static int
hmac_new_initial_state(HMACObject *self, uint8_t *key, Py_ssize_t len)
{
    assert(key != NULL);
#ifdef Py_HMAC_SSIZE_LARGER_THAN_UINT32
    // Technically speaking, we could hash the key to make it small
    // but it would require to call the hash functions ourselves and
    // not rely on HACL* implementation anymore. As such, we explicitly
    // reject keys that do not fit on 32 bits until HACL* handles them.
    if (len > UINT32_MAX_AS_SSIZE_T) {
        set_invalid_key_length_error();
        return -1;
    }
#endif
    assert(self->kind != Py_hmac_kind_hash_unknown);
    // cast to uint32_t is now safe even on 32-bit platforms
    self->state = _hacl_hmac_state_new(self->kind, key, (uint32_t)len);
    // _hacl_hmac_state_new() may set an exception on error
    return self->state == NULL ? -1 : 0;
}

/*[clinic input]
_hmac.new

    key as keyobj: object
    msg as msgobj: object(c_default="NULL") = None
    digestmod as hash_info_ref: object(c_default="NULL") = None

Return a new HMAC object.
[clinic start generated code]*/

static PyObject *
_hmac_new_impl(PyObject *module, PyObject *keyobj, PyObject *msgobj,
               PyObject *hash_info_ref)
/*[clinic end generated code: output=7c7573a427d58758 input=92fc7c0a00707d42]*/
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    if (hash_info_ref == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "new() missing 1 required argument 'digestmod'");
        return NULL;
    }

    const py_hmac_hinfo *info = find_hash_info(state, hash_info_ref);
    if (info == NULL) {
        return NULL;
    }

    HMACObject *self = PyObject_GC_New(HMACObject, state->hmac_type);
    if (self == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(self);
    hmac_set_hinfo(state, self, info);
    int rc;
    // Create the HACL* internal state with the given key.
    Py_buffer key;
    GET_BUFFER_VIEW_OR_ERROR(keyobj, &key, goto error_on_key);
    rc = hmac_new_initial_state(self, key.buf, key.len);
    PyBuffer_Release(&key);
    if (rc < 0) {
        goto error;
    }
    // Feed the internal state the initial message if any.
    if (msgobj != NULL && msgobj != Py_None) {
        Py_buffer msg;
        GET_BUFFER_VIEW_OR_ERROR(msgobj, &msg, goto error);
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            msg.len,
            rc = _hacl_hmac_state_update(self->state, msg.buf, msg.len)
        );
        PyBuffer_Release(&msg);
#ifndef NDEBUG
        if (rc < 0) {
            goto error;
        }
#else
        (void)rc;
#endif
    }
    assert(rc == 0);
    PyObject_GC_Track(self);
    return (PyObject *)self;

error_on_key:
    self->state = NULL;
error:
    Py_DECREF(self);
    return NULL;
}

/*
 * Copy HMAC hash information from 'src' to 'out'.
 */
static void
hmac_copy_hinfo(HMACObject *out, const HMACObject *src)
{
    assert(src->name != NULL);
    out->name = Py_NewRef(src->name);
    assert(src->kind != Py_hmac_kind_hash_unknown);
    out->kind = src->kind;
    assert(src->block_size <= Py_hmac_hash_max_block_size);
    out->block_size = src->block_size;
    assert(src->digest_size <= Py_hmac_hash_max_digest_size);
    out->digest_size = src->digest_size;
    assert(src->api.compute != NULL);
    assert(src->api.compute_py != NULL);
    out->api = src->api;
}

/*
 * Copy the HMAC internal state from 'src' to 'out'.
 *
 * The internal state of 'out' must not already exist.
 *
 * Return 0 on success; otherwise, set an exception and return -1 on failure.
 */
static int
hmac_copy_state(HMACObject *out, const HMACObject *src)
{
    assert(src->state != NULL);
    out->state = Hacl_Streaming_HMAC_copy(src->state);
    if (out->state == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

/*[clinic input]
_hmac.HMAC.copy

    cls: defining_class

Return a copy ("clone") of the HMAC object.
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_copy_impl(HMACObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=a955bfa55b65b215 input=17b2c0ad0b147e36]*/
{
    hmacmodule_state *state = get_hmacmodule_state_by_cls(cls);
    HMACObject *copy = PyObject_GC_New(HMACObject, state->hmac_type);
    if (copy == NULL) {
        return NULL;
    }

    HASHLIB_ACQUIRE_LOCK(self);
    /* copy hash information */
    hmac_copy_hinfo(copy, self);
    /* copy internal state */
    int rc = hmac_copy_state(copy, self);
    HASHLIB_RELEASE_LOCK(self);

    if (rc < 0) {
        Py_DECREF(copy);
        return NULL;
    }

    HASHLIB_INIT_MUTEX(copy);
    PyObject_GC_Track(copy);
    return (PyObject *)copy;
}

/*[clinic input]
_hmac.HMAC.update

    msg as msgobj: object

Update the HMAC object with the given message.
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_update_impl(HMACObject *self, PyObject *msgobj)
/*[clinic end generated code: output=962134ada5e55985 input=7c0ea830efb03367]*/
{
    int rc = 0;
    Py_buffer msg;
    GET_BUFFER_VIEW_OR_ERROUT(msgobj, &msg);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, msg.len,
        rc = _hacl_hmac_state_update(self->state, msg.buf, msg.len)
    );
    PyBuffer_Release(&msg);
    return rc < 0 ? NULL : Py_None;
}

/*
 * Compute the HMAC-HASH digest from the internal HACL* state.
 *
 * At least 'self->digest_size' bytes should be available
 * in the 'digest' pointed memory area.
 *
 * Return 0 on success; otherwise, set an exception and return -1 on failure.
 *
 * Note: this function may raise a MemoryError.
 */
static int
hmac_digest_compute_locked(HMACObject *self, uint8_t *digest)
{
    assert(digest != NULL);
    hacl_errno_t rc;
    HASHLIB_ACQUIRE_LOCK(self);
    rc = Hacl_Streaming_HMAC_digest(self->state, digest, self->digest_size);
    HASHLIB_RELEASE_LOCK(self);
    assert(
        rc == Hacl_Streaming_Types_Success ||
        rc == Hacl_Streaming_Types_OutOfMemory
    );
    return _hacl_convert_errno(rc);
}

/*[clinic input]
_hmac.HMAC.digest

Return the digest of the bytes passed to the update() method so far.

This method may raise a MemoryError.
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_digest_impl(HMACObject *self)
/*[clinic end generated code: output=5bf3cc5862d26ada input=a70feb0b8e2bbe7d]*/
{
    assert(self->digest_size <= Py_hmac_hash_max_digest_size);
    uint8_t digest[Py_hmac_hash_max_digest_size];
    if (hmac_digest_compute_locked(self, digest) < 0) {
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)digest, self->digest_size);
}

/*[clinic input]
_hmac.HMAC.hexdigest

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.

This method may raise a MemoryError.
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_hexdigest_impl(HMACObject *self)
/*[clinic end generated code: output=6659807a09ae14ec input=493b2db8013982b9]*/
{
    assert(self->digest_size <= Py_hmac_hash_max_digest_size);
    uint8_t digest[Py_hmac_hash_max_digest_size];
    if (hmac_digest_compute_locked(self, digest) < 0) {
        return NULL;
    }
    return _Py_strhex((const char *)digest, self->digest_size);
}

/*[clinic input]
@getter
_hmac.HMAC.name
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_name_get_impl(HMACObject *self)
/*[clinic end generated code: output=ae693f09778d96d9 input=41c2c5dd1cf47fbc]*/
{
    assert(self->name != NULL);
    return PyUnicode_FromFormat("hmac-%U", self->name);
}

/*[clinic input]
@getter
_hmac.HMAC.block_size
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_block_size_get_impl(HMACObject *self)
/*[clinic end generated code: output=52cb11dee4e80cae input=9dda6b8d43e995b4]*/
{
    return PyLong_FromUInt32(self->block_size);
}

/*[clinic input]
@getter
_hmac.HMAC.digest_size
[clinic start generated code]*/

static PyObject *
_hmac_HMAC_digest_size_get_impl(HMACObject *self)
/*[clinic end generated code: output=22eeca1010ac6255 input=5622bb2840025b5a]*/
{
    return PyLong_FromUInt32(self->digest_size);
}

static PyObject *
HMACObject_repr(PyObject *op)
{
    HMACObject *self = HMACObject_CAST(op);
    assert(self->name != NULL);
    return PyUnicode_FromFormat("<%U HMAC object @ %p>", self->name, self);
}

static int
HMACObject_clear(PyObject *op)
{
    HMACObject *self = HMACObject_CAST(op);
    Py_CLEAR(self->name);
    _hacl_hmac_state_free(self->state);
    self->state = NULL;
    return 0;
}

static void
HMACObject_dealloc(PyObject *op)
{
    PyTypeObject *type = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)HMACObject_clear(op);
    type->tp_free(op);
    Py_DECREF(type);
}

static int
HMACObject_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(op));
    return 0;
}

static PyMethodDef HMACObject_methods[] = {
    _HMAC_HMAC_COPY_METHODDEF
    _HMAC_HMAC_UPDATE_METHODDEF
    _HMAC_HMAC_DIGEST_METHODDEF
    _HMAC_HMAC_HEXDIGEST_METHODDEF
    {NULL, NULL, 0, NULL} /* sentinel */
};

static PyGetSetDef HMACObject_getsets[] = {
    _HMAC_HMAC_NAME_GETSETDEF
    _HMAC_HMAC_BLOCK_SIZE_GETSETDEF
    _HMAC_HMAC_DIGEST_SIZE_GETSETDEF
    {NULL, NULL, NULL, NULL, NULL} /* sentinel */
};

static PyType_Slot HMACObject_Type_slots[] = {
    {Py_tp_repr, HMACObject_repr},
    {Py_tp_methods, HMACObject_methods},
    {Py_tp_getset, HMACObject_getsets},
    {Py_tp_clear, HMACObject_clear},
    {Py_tp_dealloc, HMACObject_dealloc},
    {Py_tp_traverse, HMACObject_traverse},
    {0, NULL} /* sentinel */
};

static PyType_Spec HMAC_Type_spec = {
    .name = "_hmac.HMAC",
    .basicsize = sizeof(HMACObject),
    .flags = Py_TPFLAGS_DEFAULT
             | Py_TPFLAGS_DISALLOW_INSTANTIATION
             | Py_TPFLAGS_HEAPTYPE
             | Py_TPFLAGS_IMMUTABLETYPE
             | Py_TPFLAGS_HAVE_GC,
    .slots = HMACObject_Type_slots,
};

// --- One-shot HMAC-HASH interface -------------------------------------------

/*[clinic input]
_hmac.compute_digest

    key: object
    msg: object
    digest: object

[clinic start generated code]*/

static PyObject *
_hmac_compute_digest_impl(PyObject *module, PyObject *key, PyObject *msg,
                          PyObject *digest)
/*[clinic end generated code: output=c519b7c4c9f57333 input=1c2bfc2cd8598574]*/
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    const py_hmac_hinfo *info = find_hash_info(state, digest);
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
#define Py_HMAC_HACL_ONESHOT(HACL_HID, KEY, MSG)                \
    do {                                                        \
        Py_buffer keyview, msgview;                             \
        GET_BUFFER_VIEW_OR_ERROUT((KEY), &keyview);             \
        if (!has_uint32_t_buffer_length(&keyview)) {            \
            PyBuffer_Release(&keyview);                         \
            set_invalid_key_length_error();                     \
            return NULL;                                        \
        }                                                       \
        GET_BUFFER_VIEW_OR_ERROR((MSG), &msgview,               \
                                 PyBuffer_Release(&keyview);    \
                                 return NULL);                  \
        if (!has_uint32_t_buffer_length(&msgview)) {            \
            PyBuffer_Release(&msgview);                         \
            PyBuffer_Release(&keyview);                         \
            set_invalid_msg_length_error();                     \
            return NULL;                                        \
        }                                                       \
        uint8_t out[Py_hmac_## HACL_HID ##_digest_size];        \
        Py_hmac_## HACL_HID ##_compute_func(                    \
            out,                                                \
            (uint8_t *)keyview.buf, (uint32_t)keyview.len,      \
            (uint8_t *)msgview.buf, (uint32_t)msgview.len       \
        );                                                      \
        PyBuffer_Release(&msgview);                             \
        PyBuffer_Release(&keyview);                             \
        return PyBytes_FromStringAndSize(                       \
            (const char *)out,                                  \
            Py_hmac_## HACL_HID ##_digest_size                  \
        );                                                      \
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
_hmac.compute_sha224 as _hmac_compute_sha2_224

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_224_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=7f21f1613e53979e input=a1a75f25f23449af]*/
{
    Py_HMAC_HACL_ONESHOT(sha2_224, key, msg);
}

/*[clinic input]
_hmac.compute_sha256 as _hmac_compute_sha2_256

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_256_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=d4a291f7d9a82459 input=5c9ccf2df048ace3]*/
{
    Py_HMAC_HACL_ONESHOT(sha2_256, key, msg);
}

/*[clinic input]
_hmac.compute_sha384 as _hmac_compute_sha2_384

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_384_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=f211fa26e3700c27 input=2fee2c14766af231]*/
{
    Py_HMAC_HACL_ONESHOT(sha2_384, key, msg);
}

/*[clinic input]
_hmac.compute_sha512 as _hmac_compute_sha2_512

    key: object
    msg: object
    /

[clinic start generated code]*/

static PyObject *
_hmac_compute_sha2_512_impl(PyObject *module, PyObject *key, PyObject *msg)
/*[clinic end generated code: output=d5c20373762cecca input=3371eaac315c7864]*/
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
    _HMAC_NEW_METHODDEF
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
    {NULL, NULL, 0, NULL} /* sentinel */
};

// --- HMAC static information table ------------------------------------------

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
    assert(entry->display_name != NULL);
    if (--(entry->refcnt) == 0) {
        Py_CLEAR(entry->display_name);
        PyMem_Free(hinfo);
    }
}

/*
 * Equivalent to table.setdefault(key, info).
 *
 * Return 1 if a new item has been created, 0 if 'key' is NULL or
 * an entry 'table[key]' existed, and -1 if a memory error occurs.
 *
 * To reduce memory footprint, 'info' may be a borrowed reference,
 * namely, multiple keys can be associated with the same 'info'.
 *
 * In particular, resources owned by 'info' must only be released
 * when a single key associated with 'info' remains.
 */
static int
py_hmac_hinfo_ht_add(_Py_hashtable_t *table, const void *key, void *info)
{
    if (key == NULL || _Py_hashtable_get_entry(table, key) != NULL) {
        return 0;
    }
    if (_Py_hashtable_set(table, key, info) < 0) {
        assert(!PyErr_Occurred());
        PyErr_NoMemory();
        return -1;
    }
    return 1;
}

/*
 * Create a new hashtable from the static 'py_hmac_static_hinfo' object,
 * or set an exception and return NULL if an error occurs.
 */
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
        assert(!PyErr_Occurred());
        PyErr_NoMemory();
        return NULL;
    }

    for (const py_hmac_hinfo *e = py_hmac_static_hinfo; e->name != NULL; e++) {
        /*
         * The real kind of a HMAC object is obtained only once and is
         * derived from the kind of the 'py_hmac_hinfo' that could be
         * found by its name.
         *
         * Since 'vectorized_blake2{s,b}_32' depend on the runtime CPUID
         * features, we should not create 'py_hmac_hinfo' entries for them.
         */
        assert_is_static_hmac_hash_kind(e->kind);

        py_hmac_hinfo *value = PyMem_Malloc(sizeof(py_hmac_hinfo));
        if (value == NULL) {
            PyErr_NoMemory();
            goto error;
        }

        memcpy(value, e, sizeof(py_hmac_hinfo));
        assert(value->display_name == NULL);
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
#undef Py_HMAC_HINFO_LINK
        assert(value->refcnt > 0);
        assert(value->display_name == NULL);
        value->display_name = PyUnicode_FromString(
            /* display name is synchronized with hashlib's name */
            e->hashlib_name == NULL ? e->name : e->hashlib_name
        );
        if (value->display_name == NULL) {
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
    // py_hmac_hinfo_ht_new() sets an exception on error
    state->hinfo_table = py_hmac_hinfo_ht_new();
    return state->hinfo_table == NULL ? -1 : 0;
}

static int
hmacmodule_init_exceptions(PyObject *module, hmacmodule_state *state)
{
#define ADD_EXC(ATTR, NAME, BASE)                                       \
    do {                                                                \
        state->ATTR = PyErr_NewException("_hmac." NAME, BASE, NULL);    \
        if (state->ATTR == NULL) {                                      \
            return -1;                                                  \
        }                                                               \
        if (PyModule_AddObjectRef(module, NAME, state->ATTR) < 0) {     \
            return -1;                                                  \
        }                                                               \
    } while (0)
    ADD_EXC(unknown_hash_error, "UnknownHashError", PyExc_ValueError);
#undef ADD_EXC
    return 0;
}

static int
hmacmodule_init_hmac_type(PyObject *module, hmacmodule_state *state)
{
    state->hmac_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                                &HMAC_Type_spec,
                                                                NULL);
    if (state->hmac_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->hmac_type) < 0) {
        return -1;
    }
    return 0;
}

static int
hmacmodule_init_strings(hmacmodule_state *state)
{
#define ADD_STR(ATTR, STRING)                       \
    do {                                            \
        state->ATTR = PyUnicode_FromString(STRING); \
        if (state->ATTR == NULL) {                  \
            return -1;                              \
        }                                           \
    } while (0)
    ADD_STR(str_lower, "lower");
#undef ADD_STR
    return 0;
}

static int
hmacmodule_init_globals(PyObject *module, hmacmodule_state *state)
{
#define ADD_INT_CONST(NAME, VALUE)                                  \
    do {                                                            \
        if (PyModule_AddIntConstant(module, (NAME), (VALUE)) < 0) { \
            return -1;                                              \
        }                                                           \
    } while (0)
    ADD_INT_CONST("_GIL_MINSIZE", HASHLIB_GIL_MINSIZE);
#undef ADD_INT_CONST
    return 0;
}

static void
hmacmodule_init_cpu_features(hmacmodule_state *state)
{
    int eax1 = 0, ebx1 = 0, ecx1 = 0, edx1 = 0;
    int eax7 = 0, ebx7 = 0, ecx7 = 0, edx7 = 0;
#if defined(__x86_64__) && defined(__GNUC__)
    __cpuid_count(1, 0, eax1, ebx1, ecx1, edx1);
    __cpuid_count(7, 0, eax7, ebx7, ecx7, edx7);
#elif defined(_M_X64)
    int info1[4] = {0};
    __cpuidex(info1, 1, 0);
    eax1 = info1[0], ebx1 = info1[1], ecx1 = info1[2], edx1 = info1[3];

    int info7[4] = {0};
    __cpuidex(info7, 7, 0);
    eax7 = info7[0], ebx7 = info7[1], ecx7 = info7[2], edx7 = info7[3];
#endif
    // fmt: off
    (void)eax1; (void)ebx1; (void)ecx1; (void)edx1;
    (void)eax7; (void)ebx7; (void)ecx7; (void)edx7;
    // fmt: on

#define EBX_AVX2 (1 << 5)
#define ECX_SSE3 (1 << 0)
#define ECX_SSSE3 (1 << 9)
#define ECX_SSE4_1 (1 << 19)
#define ECX_SSE4_2 (1 << 20)
#define ECX_AVX (1 << 28)
#define EDX_SSE (1 << 25)
#define EDX_SSE2 (1 << 26)
#define EDX_CMOV (1 << 15)

    bool avx = (ecx1 & ECX_AVX) != 0;
    bool avx2 = (ebx7 & EBX_AVX2) != 0;

    bool sse = (edx1 & EDX_SSE) != 0;
    bool sse2 = (edx1 & EDX_SSE2) != 0;
    bool cmov = (edx1 & EDX_CMOV) != 0;

    bool sse3 = (ecx1 & ECX_SSE3) != 0;
    bool sse41 = (ecx1 & ECX_SSE4_1) != 0;
    bool sse42 = (ecx1 & ECX_SSE4_2) != 0;

#undef EDX_CMOV
#undef EDX_SSE2
#undef EDX_SSE
#undef ECX_AVX
#undef ECX_SSE4_2
#undef ECX_SSE4_1
#undef ECX_SSSE3
#undef ECX_SSE3
#undef EBX_AVX2

#if _Py_HACL_CAN_COMPILE_VEC128
    // TODO(picnixz): use py_cpuid_features (gh-125022) to improve detection
    state->can_run_simd128 = sse && sse2 && sse3 && sse41 && sse42 && cmov;
#else
    // fmt: off
    (void)sse; (void)sse2; (void)sse3; (void)sse41; (void)sse42; (void)cmov;
    // fmt: on
    state->can_run_simd128 = false;
#endif

#if _Py_HACL_CAN_COMPILE_VEC256
    // TODO(picnixz): use py_cpuid_features (gh-125022) to improve detection
    state->can_run_simd256 = state->can_run_simd128 && avx && avx2;
#else
    // fmt: off
    (void)avx; (void)avx2;
    // fmt: on
    state->can_run_simd256 = false;
#endif
}

static int
hmacmodule_exec(PyObject *module)
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    if (hmacmodule_init_hash_info_table(state) < 0) {
        return -1;
    }
    if (hmacmodule_init_exceptions(module, state) < 0) {
        return -1;
    }
    if (hmacmodule_init_hmac_type(module, state) < 0) {
        return -1;
    }
    if (hmacmodule_init_strings(state) < 0) {
        return -1;
    }
    if (hmacmodule_init_globals(module, state) < 0) {
        return -1;
    }
    hmacmodule_init_cpu_features(state);
    return 0;
}

static int
hmacmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    hmacmodule_state *state = get_hmacmodule_state(mod);
    Py_VISIT(state->unknown_hash_error);
    Py_VISIT(state->hmac_type);
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
    Py_CLEAR(state->unknown_hash_error);
    Py_CLEAR(state->hmac_type);
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
    {0, NULL} /* sentinel */
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

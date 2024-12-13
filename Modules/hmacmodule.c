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

#include "_hacl/Hacl_Streaming_HMAC.h"  // Hacl_Agile_Hash_* identifiers

// --- HMAC underlying hash function static information -----------------------

#define Py_hmac_hash_max_block_size             128
#define Py_hmac_hash_max_digest_size            64

/* MD-5 */
// HACL_HID = md5
#define Py_hmac_md5_block_size                  64
#define Py_hmac_md5_digest_size                 16

/* SHA-1 family */
// HACL_HID = sha1
#define Py_hmac_sha1_block_size                 64
#define Py_hmac_sha1_digest_size                20

/* SHA-2 family */
// HACL_HID = sha2_224
#define Py_hmac_sha2_224_block_size             64
#define Py_hmac_sha2_224_digest_size            28

// HACL_HID = sha2_256
#define Py_hmac_sha2_256_block_size             64
#define Py_hmac_sha2_256_digest_size            32

// HACL_HID = sha2_384
#define Py_hmac_sha2_384_block_size             128
#define Py_hmac_sha2_384_digest_size            48

// HACL_HID = sha2_512
#define Py_hmac_sha2_512_block_size             128
#define Py_hmac_sha2_512_digest_size            64

/* SHA-3 family */
// HACL_HID = sha3_224
#define Py_hmac_sha3_224_block_size             144
#define Py_hmac_sha3_224_digest_size            28

// HACL_HID = sha3_256
#define Py_hmac_sha3_256_block_size             136
#define Py_hmac_sha3_256_digest_size            32

// HACL_HID = sha3_384
#define Py_hmac_sha3_384_block_size             104
#define Py_hmac_sha3_384_digest_size            48

// HACL_HID = sha3_512
#define Py_hmac_sha3_512_block_size             72
#define Py_hmac_sha3_512_digest_size            64

/* Blake2 family */
// HACL_HID = blake2s_32
#define Py_hmac_blake2s_32_block_size           64
#define Py_hmac_blake2s_32_digest_size          32

// HACL_HID = blake2b_32
#define Py_hmac_blake2b_32_block_size           128
#define Py_hmac_blake2b_32_digest_size          64

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
#undef DECL_HACL_HMAC_HASH_KIND
} HMAC_Hash_Kind;

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
} hmacmodule_state;

static inline hmacmodule_state *
get_hmacmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (hmacmodule_state *)state;
}

// --- HMAC module clinic configuration ---------------------------------------

/*[clinic input]
module _hmac
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=799f0f10157d561f]*/

// --- Helpers ----------------------------------------------------------------

/* Static information used to construct the hash table. */
static const py_hmac_hinfo py_hmac_static_hinfo[] = {
#define Py_HMAC_HINFO_ENTRY(HACL_HID, HLIB_NAME)            \
    {                                                       \
        .name = Py_STRINGIFY(HACL_HID),                     \
        .kind = Py_hmac_kind_hmac_ ## HACL_HID,             \
        .block_size = Py_hmac_## HACL_HID ##_block_size,    \
        .digest_size = Py_hmac_## HACL_HID ##_digest_size,  \
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
    /* sentinel */
    {
        NULL, Py_hmac_kind_hash_unknown, 0, 0,
        NULL, NULL,
        0,
    },
};

// --- HMAC module methods ----------------------------------------------------

static PyMethodDef hmacmodule_methods[] = {
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
        assert(e->kind != Py_hmac_kind_hash_unknown);
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
hmacmodule_exec(PyObject *module)
{
    hmacmodule_state *state = get_hmacmodule_state(module);
    if (hmacmodule_init_hash_info_table(state) < 0) {
        return -1;
    }
    if (hmacmodule_init_exceptions(module, state) < 0) {
        return -1;
    }
    return 0;
}

static int
hmacmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    hmacmodule_state *state = get_hmacmodule_state(mod);
    Py_VISIT(state->unknown_hash_error);
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

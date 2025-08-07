/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 * Updated to use HACL* by Jonathan Protzenko <jonathan@protzenko.fr>
 * Additional work by Bénédikt Tran <10796600+picnixz@users.noreply.github.com>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "hashlib.h"
#include "pycore_strhex.h"       // _Py_strhex()
#include "pycore_typeobject.h"
#include "pycore_moduleobject.h"

// QUICK CPU AUTODETECTION
//
// See https://github.com/python/cpython/pull/119316 -- we only enable
// vectorized versions for Intel CPUs, even though HACL*'s "vec128" modules also
// run on ARM NEON. (We could enable them on POWER -- but I don't have access to
// a test machine to see if that speeds anything up.)
//
// Note that configure.ac and the rest of the build are written in such a way
// that if the configure script finds suitable flags to compile HACL's SIMD128
// (resp. SIMD256) files, then Hacl_Hash_Blake2b_Simd128.c (resp. ...) will be
// pulled into the build automatically, and then only the CPU autodetection will
// need to be updated here.

#if defined(__x86_64__) && defined(__GNUC__)
#include <cpuid.h>
#elif defined(_M_X64)
#include <intrin.h>
#endif

#include <stdbool.h>

// SIMD256 can't be compiled on macOS ARM64, and performance of SIMD128 isn't
// great; but when compiling a universal2 binary, autoconf will set
// _Py_HACL_CAN_COMPILE_VEC{128,256} because they *can* be compiled on x86_64.
// If we're on macOS ARM64, we however disable these preprocessor symbols.
#if defined(__APPLE__) && defined(__arm64__)
#  undef _Py_HACL_CAN_COMPILE_VEC128
#  undef _Py_HACL_CAN_COMPILE_VEC256
#endif

// HACL* expects HACL_CAN_COMPILE_VEC* macros to be set in order to enable
// the corresponding SIMD instructions so we need to "forward" the values
// we just deduced above.
#define HACL_CAN_COMPILE_VEC128 _Py_HACL_CAN_COMPILE_VEC128
#define HACL_CAN_COMPILE_VEC256 _Py_HACL_CAN_COMPILE_VEC256

#include "_hacl/Hacl_Hash_Blake2s.h"
#include "_hacl/Hacl_Hash_Blake2b.h"
#if _Py_HACL_CAN_COMPILE_VEC128
#include "_hacl/Hacl_Hash_Blake2s_Simd128.h"
#endif
#if _Py_HACL_CAN_COMPILE_VEC256
#include "_hacl/Hacl_Hash_Blake2b_Simd256.h"
#endif

// MODULE TYPE SLOTS

static PyType_Spec blake2b_type_spec;
static PyType_Spec blake2s_type_spec;

PyDoc_STRVAR(blake2mod__doc__,
             "_blake2 provides BLAKE2b and BLAKE2s for hashlib\n");

typedef struct {
    PyTypeObject *blake2b_type;
    PyTypeObject *blake2s_type;
    bool can_run_simd128;
    bool can_run_simd256;
} Blake2State;

static inline Blake2State *
blake2_get_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}

#if defined(_Py_HACL_CAN_COMPILE_VEC128) || defined(_Py_HACL_CAN_COMPILE_VEC256)
static inline Blake2State *
blake2_get_state_from_type(PyTypeObject *module)
{
    void *state = _PyType_GetModuleState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}
#endif

static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

static int
_blake2_traverse(PyObject *module, visitproc visit, void *arg)
{
    Blake2State *state = blake2_get_state(module);
    Py_VISIT(state->blake2b_type);
    Py_VISIT(state->blake2s_type);
    return 0;
}

static int
_blake2_clear(PyObject *module)
{
    Blake2State *state = blake2_get_state(module);
    Py_CLEAR(state->blake2b_type);
    Py_CLEAR(state->blake2s_type);
    return 0;
}

static void
_blake2_free(void *module)
{
    (void)_blake2_clear((PyObject *)module);
}

static void
blake2module_init_cpu_features(Blake2State *state)
{
    /* This must be kept in sync with hmacmodule_init_cpu_features()
     * in hmacmodule.c */
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
blake2_exec(PyObject *m)
{
    Blake2State *st = blake2_get_state(m);
    blake2module_init_cpu_features(st);

#define ADD_INT(DICT, NAME, VALUE)                      \
    do {                                                \
        PyObject *x = PyLong_FromLong(VALUE);           \
        if (x == NULL) {                                \
            return -1;                                  \
        }                                               \
        int rc = PyDict_SetItemString(DICT, NAME, x);   \
        Py_DECREF(x);                                   \
        if (rc < 0) {                                   \
            return -1;                                  \
        }                                               \
    } while(0)

#define ADD_INT_CONST(NAME, VALUE)                          \
    do {                                                    \
        if (PyModule_AddIntConstant(m, NAME, VALUE) < 0) {  \
            return -1;                                      \
        }                                                   \
    } while (0)

    ADD_INT_CONST("_GIL_MINSIZE", HASHLIB_GIL_MINSIZE);

    st->blake2b_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2b_type_spec, NULL);

    if (st->blake2b_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->blake2b_type) < 0) {
        return -1;
    }

    PyObject *d = st->blake2b_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", HACL_HASH_BLAKE2B_SALT_BYTES);
    ADD_INT(d, "PERSON_SIZE", HACL_HASH_BLAKE2B_PERSONAL_BYTES);
    ADD_INT(d, "MAX_KEY_SIZE", HACL_HASH_BLAKE2B_KEY_BYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", HACL_HASH_BLAKE2B_OUT_BYTES);

    ADD_INT_CONST("BLAKE2B_SALT_SIZE", HACL_HASH_BLAKE2B_SALT_BYTES);
    ADD_INT_CONST("BLAKE2B_PERSON_SIZE", HACL_HASH_BLAKE2B_PERSONAL_BYTES);
    ADD_INT_CONST("BLAKE2B_MAX_KEY_SIZE", HACL_HASH_BLAKE2B_KEY_BYTES);
    ADD_INT_CONST("BLAKE2B_MAX_DIGEST_SIZE", HACL_HASH_BLAKE2B_OUT_BYTES);

    /* BLAKE2s */
    st->blake2s_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2s_type_spec, NULL);

    if (st->blake2s_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->blake2s_type) < 0) {
        return -1;
    }

    d = st->blake2s_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", HACL_HASH_BLAKE2S_SALT_BYTES);
    ADD_INT(d, "PERSON_SIZE", HACL_HASH_BLAKE2S_PERSONAL_BYTES);
    ADD_INT(d, "MAX_KEY_SIZE", HACL_HASH_BLAKE2S_KEY_BYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", HACL_HASH_BLAKE2S_OUT_BYTES);

    ADD_INT_CONST("BLAKE2S_SALT_SIZE", HACL_HASH_BLAKE2S_SALT_BYTES);
    ADD_INT_CONST("BLAKE2S_PERSON_SIZE", HACL_HASH_BLAKE2S_PERSONAL_BYTES);
    ADD_INT_CONST("BLAKE2S_MAX_KEY_SIZE", HACL_HASH_BLAKE2S_KEY_BYTES);
    ADD_INT_CONST("BLAKE2S_MAX_DIGEST_SIZE", HACL_HASH_BLAKE2S_OUT_BYTES);

#undef ADD_INT_CONST
#undef ADD_INT
    return 0;
}

static PyModuleDef_Slot _blake2_slots[] = {
    {Py_mod_exec, blake2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef blake2_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_blake2",
    .m_doc = blake2mod__doc__,
    .m_size = sizeof(Blake2State),
    .m_methods = blake2mod_functions,
    .m_slots = _blake2_slots,
    .m_traverse = _blake2_traverse,
    .m_clear = _blake2_clear,
    .m_free = _blake2_free,
};

PyMODINIT_FUNC
PyInit__blake2(void)
{
    return PyModuleDef_Init(&blake2_module);
}

// IMPLEMENTATION OF METHODS

// The HACL* API does not offer an agile API that can deal with either Blake2S
// or Blake2B -- the reason is that the underlying states are optimized (uint32s
// for S, uint64s for B). Therefore, we use a tagged union in this module to
// correctly dispatch. Note that the previous incarnation of this code
// transformed the Blake2b implementation into the Blake2s one using a script,
// so this is an improvement.
//
// The 128 and 256 versions are only available if i) we were able to compile
// them, and ii) if the CPU we run on also happens to have the right instruction
// set.
typedef enum { Blake2s, Blake2b, Blake2s_128, Blake2b_256 } blake2_impl;

static inline bool
is_blake2b(blake2_impl impl)
{
    return impl == Blake2b || impl == Blake2b_256;
}

static inline bool
is_blake2s(blake2_impl impl)
{
    return impl == Blake2s || impl == Blake2s_128;
}

static inline blake2_impl
type_to_impl(PyTypeObject *type)
{
#if defined(_Py_HACL_CAN_COMPILE_VEC128) || defined(_Py_HACL_CAN_COMPILE_VEC256)
    Blake2State *st = blake2_get_state_from_type(type);
#endif
    if (!strcmp(type->tp_name, blake2b_type_spec.name)) {
#if _Py_HACL_CAN_COMPILE_VEC256
        return st->can_run_simd256 ? Blake2b_256 : Blake2b;
#else
        return Blake2b;
#endif
    }
    else if (!strcmp(type->tp_name, blake2s_type_spec.name)) {
#if _Py_HACL_CAN_COMPILE_VEC128
        return st->can_run_simd128 ? Blake2s_128 : Blake2s;
#else
        return Blake2s;
#endif
    }
    Py_UNREACHABLE();
}

typedef struct {
    HASHLIB_OBJECT_HEAD
    union {
        Hacl_Hash_Blake2s_state_t *blake2s_state;
        Hacl_Hash_Blake2b_state_t *blake2b_state;
#if _Py_HACL_CAN_COMPILE_VEC128
        Hacl_Hash_Blake2s_Simd128_state_t *blake2s_128_state;
#endif
#if _Py_HACL_CAN_COMPILE_VEC256
        Hacl_Hash_Blake2b_Simd256_state_t *blake2b_256_state;
#endif
    };
    blake2_impl impl;
} Blake2Object;

#define _Blake2Object_CAST(op)  ((Blake2Object *)(op))

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
module _blake2
class _blake2.blake2b "Blake2Object *" "&PyType_Type"
class _blake2.blake2s "Blake2Object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=86b0972b0c41b3d0]*/

#include "clinic/blake2module.c.h"

// --- BLAKE-2 object interface -----------------------------------------------

static Blake2Object *
new_Blake2Object(PyTypeObject *type)
{
    Blake2Object *self = PyObject_GC_New(Blake2Object, type);
    if (self == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(self);

    PyObject_GC_Track(self);
    return self;
}

/* HACL* takes a uint32_t for the length of its parameter, but Py_ssize_t can be
 * 64 bits so we loop in <4gig chunks when needed. */

#if PY_SSIZE_T_MAX > UINT32_MAX
#  define HACL_UPDATE_LOOP(UPDATE_FUNC, STATE, BUF, LEN)    \
    do {                                                    \
        while (LEN > UINT32_MAX) {                          \
            (void)UPDATE_FUNC(STATE, BUF, UINT32_MAX);      \
            LEN -= UINT32_MAX;                              \
            BUF += UINT32_MAX;                              \
        }                                                   \
    } while (0)
#else
#  define HACL_UPDATE_LOOP(...)
#endif

/*
 * Note: we explicitly ignore the error code on the basis that it would take
 * more than 1 billion years to overflow the maximum admissible length for
 * blake2b/2s (2^64 - 1).
 */
#define HACL_UPDATE(UPDATE_FUNC, STATE, BUF, LEN)               \
    do {                                                        \
        HACL_UPDATE_LOOP(UPDATE_FUNC, STATE, BUF, LEN);         \
        /* cast to uint32_t is now safe */                      \
        (void)UPDATE_FUNC(STATE, BUF, (uint32_t)LEN);           \
    } while (0)

static void
blake2_update_unlocked(Blake2Object *self, uint8_t *buf, Py_ssize_t len)
{
    switch (self->impl) {
        // blake2b_256_state and blake2s_128_state must be if'd since
        // otherwise this results in an unresolved symbol at link-time.
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            HACL_UPDATE(Hacl_Hash_Blake2b_Simd256_update,
                        self->blake2b_256_state, buf, len);
            return;
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            HACL_UPDATE(Hacl_Hash_Blake2s_Simd128_update,
                        self->blake2s_128_state, buf, len);
            return;
#endif
        case Blake2b:
            HACL_UPDATE(Hacl_Hash_Blake2b_update,
                        self->blake2b_state, buf, len);
            return;
        case Blake2s:
            HACL_UPDATE(Hacl_Hash_Blake2s_update,
                        self->blake2s_state, buf, len);
            return;
        default:
            Py_UNREACHABLE();
    }
}

#define BLAKE2_IMPLNAME(SELF)   \
    (is_blake2b((SELF)->impl) ? "blake2b" : "blake2s")
#define GET_BLAKE2_CONST(SELF, NAME)    \
    (is_blake2b((SELF)->impl)           \
        ? HACL_HASH_BLAKE2B_ ## NAME    \
        : HACL_HASH_BLAKE2S_ ## NAME)

#define MAX_OUT_BYTES(SELF)         GET_BLAKE2_CONST(SELF, OUT_BYTES)
#define MAX_SALT_LENGTH(SELF)       GET_BLAKE2_CONST(SELF, SALT_BYTES)
#define MAX_KEY_BYTES(SELF)         GET_BLAKE2_CONST(SELF, KEY_BYTES)
#define MAX_PERSONAL_BYTES(SELF)    GET_BLAKE2_CONST(SELF, PERSONAL_BYTES)

static int
py_blake2_validate_params(Blake2Object *self,
                          int digest_size,
                          Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                          int fanout, int depth, unsigned long leaf_size,
                          unsigned long long node_offset, int node_depth,
                          int inner_size)
{
    /* Validate digest size. */
    if (digest_size <= 0 || (unsigned int)digest_size > MAX_OUT_BYTES(self)) {
        PyErr_Format(
            PyExc_ValueError,
            "digest_size for %s must be between 1 and %d bytes, got %d",
            BLAKE2_IMPLNAME(self), MAX_OUT_BYTES(self), digest_size
        );
        goto error;
    }

#define CHECK_LENGTH(NAME, VALUE, MAX)                                  \
    do {                                                                \
        if ((size_t)(VALUE) > (size_t)(MAX)) {                          \
            PyErr_Format(PyExc_ValueError,                              \
                         "maximum %s length is %zu bytes, got %zd",     \
                         (NAME), (size_t)(MAX), (Py_ssize_t)(VALUE));   \
            goto error;                                                 \
        }                                                               \
    } while (0)
    /* Validate key parameter. */
    if (key->obj && key->len) {
        CHECK_LENGTH("key", key->len, MAX_KEY_BYTES(self));
    }
    /* Validate salt parameter. */
    if (salt->obj && salt->len) {
        CHECK_LENGTH("salt", salt->len, MAX_SALT_LENGTH(self));
    }
    /* Validate personalization parameter. */
    if (person->obj && person->len) {
        CHECK_LENGTH("person", person->len, MAX_PERSONAL_BYTES(self));
    }
#undef CHECK_LENGTH
#define CHECK_TREE(NAME, VALUE, MIN, MAX)                           \
    do {                                                            \
        if ((VALUE) < (MIN) || (size_t)(VALUE) > (size_t)(MAX)) {   \
            PyErr_Format(PyExc_ValueError,                          \
                         "'%s' must be between %zu and %zu",        \
                         (NAME), (size_t)(MIN), (size_t)(MAX));     \
            goto error;                                             \
        }                                                           \
    } while (0)
    /* Validate tree parameters. */
    CHECK_TREE("fanout", fanout, 0, 255);
    CHECK_TREE("depth", depth, 1, 255);
    CHECK_TREE("node_depth", node_depth, 0, 255);
    CHECK_TREE("inner_size", inner_size, 0, MAX_OUT_BYTES(self));
#undef CHECK_TREE
    if (leaf_size > 0xFFFFFFFFU) {
        /* maximum: 2**32 - 1 */
        PyErr_SetString(PyExc_OverflowError, "'leaf_size' is too large");
        goto error;
    }
    if (is_blake2s(self->impl) && node_offset > 0xFFFFFFFFFFFFULL) {
        /* maximum: 2**48 - 1 */
        PyErr_SetString(PyExc_OverflowError, "'node_offset' is too large");
        goto error;
    }
    return 0;
error:
    return -1;
}


static PyObject *
py_blake2_new(PyTypeObject *type, PyObject *data, int digest_size,
              Py_buffer *key, Py_buffer *salt, Py_buffer *person,
              int fanout, int depth, unsigned long leaf_size,
              unsigned long long node_offset, int node_depth,
              int inner_size, int last_node, int usedforsecurity)
{
    Blake2Object *self = NULL;

    self = new_Blake2Object(type);
    if (self == NULL) {
        goto error;
    }

    self->impl = type_to_impl(type);
    // Ensure that the states are NULL-initialized in case of an error.
    // See: py_blake2_clear() for more details.
    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            self->blake2b_256_state = NULL;
            break;
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            self->blake2s_128_state = NULL;
            break;
#endif
        case Blake2b:
            self->blake2b_state = NULL;
            break;
        case Blake2s:
            self->blake2s_state = NULL;
            break;
        default:
            Py_UNREACHABLE();
    }

    // Unlike the state types, the parameters share a single (client-friendly)
    // structure.
    if (py_blake2_validate_params(self,
                                  digest_size,
                                  key, salt, person,
                                  fanout, depth, leaf_size,
                                  node_offset, node_depth, inner_size) < 0)
    {
        goto error;
    }

    // Using Blake2b because we statically know that these are greater than the
    // Blake2s sizes -- this avoids a VLA.
    uint8_t salt_buffer[HACL_HASH_BLAKE2B_SALT_BYTES] = {0};
    uint8_t personal_buffer[HACL_HASH_BLAKE2B_PERSONAL_BYTES] = {0};
    if (salt->obj != NULL) {
        assert(salt->buf != NULL);
        memcpy(salt_buffer, salt->buf, salt->len);
    }
    if (person->obj != NULL) {
        assert(person->buf != NULL);
        memcpy(personal_buffer, person->buf, person->len);
    }

    Hacl_Hash_Blake2b_blake2_params params = {
        .digest_length = digest_size,
        .key_length = (uint8_t)key->len,
        .fanout = fanout,
        .depth = depth,
        .leaf_length = leaf_size,
        .node_offset = node_offset,
        .node_depth = node_depth,
        .inner_length = inner_size,
        .salt = salt_buffer,
        .personal = personal_buffer
    };

#define BLAKE2_MALLOC(TYPE, STATE)                                  \
    do {                                                            \
        STATE = Hacl_Hash_ ## TYPE ## _malloc_with_params_and_key(  \
                    &params, last_node, key->buf);                  \
        if (STATE == NULL) {                                        \
            (void)PyErr_NoMemory();                                 \
            goto error;                                             \
        }                                                           \
    } while (0)

    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            BLAKE2_MALLOC(Blake2b_Simd256, self->blake2b_256_state);
            break;
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            BLAKE2_MALLOC(Blake2s_Simd128, self->blake2s_128_state);
            break;
#endif
        case Blake2b:
            BLAKE2_MALLOC(Blake2b, self->blake2b_state);
            break;
        case Blake2s:
            BLAKE2_MALLOC(Blake2s, self->blake2s_state);
            break;
        default:
            Py_UNREACHABLE();
    }
#undef BLAKE2_MALLOC

    /* Process initial data if any. */
    if (data != NULL) {
        Py_buffer buf;
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            buf.len,
            blake2_update_unlocked(self, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return (PyObject *)self;
error:
    Py_XDECREF(self);
    return NULL;
}

/*[clinic input]
@classmethod
_blake2.blake2b.__new__ as py_blake2b_new
    data as data_obj: object(c_default="NULL") = b''
    *
    digest_size: int(c_default="HACL_HASH_BLAKE2B_OUT_BYTES") = _blake2.blake2b.MAX_DIGEST_SIZE
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    salt: Py_buffer(c_default="NULL", py_default="b''") = None
    person: Py_buffer(c_default="NULL", py_default="b''") = None
    fanout: int = 1
    depth: int = 1
    leaf_size: unsigned_long = 0
    node_offset: unsigned_long_long = 0
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Return a new BLAKE2b hash object.
[clinic start generated code]*/

static PyObject *
py_blake2b_new_impl(PyTypeObject *type, PyObject *data_obj, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity,
                    PyObject *string)
/*[clinic end generated code: output=de64bd850606b6a0 input=78cf60a2922d2f90]*/
{
    PyObject *data;
    if (_Py_hashlib_data_argument(&data, data_obj, string) < 0) {
        return NULL;
    }
    return py_blake2_new(type, data, digest_size, key, salt, person,
                         fanout, depth, leaf_size, node_offset, node_depth,
                         inner_size, last_node, usedforsecurity);
}

/*[clinic input]
@classmethod
_blake2.blake2s.__new__ as py_blake2s_new
    data as data_obj: object(c_default="NULL") = b''
    *
    digest_size: int(c_default="HACL_HASH_BLAKE2S_OUT_BYTES") = _blake2.blake2s.MAX_DIGEST_SIZE
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    salt: Py_buffer(c_default="NULL", py_default="b''") = None
    person: Py_buffer(c_default="NULL", py_default="b''") = None
    fanout: int = 1
    depth: int = 1
    leaf_size: unsigned_long = 0
    node_offset: unsigned_long_long = 0
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False
    usedforsecurity: bool = True
    string: object(c_default="NULL") = None

Return a new BLAKE2s hash object.
[clinic start generated code]*/

static PyObject *
py_blake2s_new_impl(PyTypeObject *type, PyObject *data_obj, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity,
                    PyObject *string)
/*[clinic end generated code: output=582a0c4295cc3a3c input=6843d6332eefd295]*/
{
    PyObject *data;
    if (_Py_hashlib_data_argument(&data, data_obj, string) < 0) {
        return NULL;
    }
    return py_blake2_new(type, data, digest_size, key, salt, person,
                         fanout, depth, leaf_size, node_offset, node_depth,
                         inner_size, last_node, usedforsecurity);
}

static int
blake2_blake2b_copy_unlocked(Blake2Object *self, Blake2Object *cpy)
{
    assert(cpy != NULL);
#define BLAKE2_COPY(TYPE, STATE_ATTR)                                       \
    do {                                                                    \
        cpy->STATE_ATTR = Hacl_Hash_ ## TYPE ## _copy(self->STATE_ATTR);    \
        if (cpy->STATE_ATTR == NULL) {                                      \
            goto error;                                                     \
        }                                                                   \
    } while (0)

    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            BLAKE2_COPY(Blake2b_Simd256, blake2b_256_state);
            break;
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            BLAKE2_COPY(Blake2s_Simd128, blake2s_128_state);
            break;
#endif
        case Blake2b:
            BLAKE2_COPY(Blake2b, blake2b_state);
            break;
        case Blake2s:
            BLAKE2_COPY(Blake2s, blake2s_state);
            break;
        default:
            Py_UNREACHABLE();
    }
#undef BLAKE2_COPY
    cpy->impl = self->impl;
    return 0;

error:
    (void)PyErr_NoMemory();
    return -1;
}

/*[clinic input]
_blake2.blake2b.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_copy_impl(Blake2Object *self, PyTypeObject *cls)
/*[clinic end generated code: output=5f8ea31c56c52287 input=f38f3475e9aec98d]*/
{
    int rc;
    Blake2Object *cpy;

    if ((cpy = new_Blake2Object(cls)) == NULL) {
        return NULL;
    }

    HASHLIB_ACQUIRE_LOCK(self);
    rc = blake2_blake2b_copy_unlocked(self, cpy);
    HASHLIB_RELEASE_LOCK(self);
    if (rc < 0) {
        Py_DECREF(cpy);
        return NULL;
    }
    return (PyObject *)cpy;
}

/*[clinic input]
_blake2.blake2b.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_update_impl(Blake2Object *self, PyObject *data)
/*[clinic end generated code: output=99330230068e8c99 input=ffc4aa6a6a225d31]*/
{
    Py_buffer buf;
    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        blake2_update_unlocked(self, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static uint8_t
blake2_blake2b_compute_digest(Blake2Object *self, uint8_t *digest)
{
    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            return Hacl_Hash_Blake2b_Simd256_digest(
                self->blake2b_256_state, digest);
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            return Hacl_Hash_Blake2s_Simd128_digest(
                self->blake2s_128_state, digest);
#endif
        case Blake2b:
            return Hacl_Hash_Blake2b_digest(self->blake2b_state, digest);
        case Blake2s:
            return Hacl_Hash_Blake2s_digest(self->blake2s_state, digest);
        default:
            Py_UNREACHABLE();
    }
}

/*[clinic input]
_blake2.blake2b.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_digest_impl(Blake2Object *self)
/*[clinic end generated code: output=31ab8ad477f4a2f7 input=7d21659e9c5fff02]*/
{
    uint8_t digest_length = 0, digest[HACL_HASH_BLAKE2B_OUT_BYTES];
    HASHLIB_ACQUIRE_LOCK(self);
    digest_length = blake2_blake2b_compute_digest(self, digest);
    HASHLIB_RELEASE_LOCK(self);
    return PyBytes_FromStringAndSize((const char *)digest, digest_length);
}

/*[clinic input]
_blake2.blake2b.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_hexdigest_impl(Blake2Object *self)
/*[clinic end generated code: output=5ef54b138db6610a input=76930f6946351f56]*/
{
    uint8_t digest_length = 0, digest[HACL_HASH_BLAKE2B_OUT_BYTES];
    HASHLIB_ACQUIRE_LOCK(self);
    digest_length = blake2_blake2b_compute_digest(self, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _Py_strhex((const char *)digest, digest_length);
}


static PyMethodDef py_blake2b_methods[] = {
    _BLAKE2_BLAKE2B_COPY_METHODDEF
    _BLAKE2_BLAKE2B_DIGEST_METHODDEF
    _BLAKE2_BLAKE2B_HEXDIGEST_METHODDEF
    _BLAKE2_BLAKE2B_UPDATE_METHODDEF
    {NULL, NULL}
};


static PyObject *
py_blake2b_get_name(PyObject *op, void *Py_UNUSED(closure))
{
    Blake2Object *self = _Blake2Object_CAST(op);
    return PyUnicode_FromString(BLAKE2_IMPLNAME(self));
}


static PyObject *
py_blake2b_get_block_size(PyObject *op, void *Py_UNUSED(closure))
{
    Blake2Object *self = _Blake2Object_CAST(op);
    return PyLong_FromLong(GET_BLAKE2_CONST(self, BLOCK_BYTES));
}


static Hacl_Hash_Blake2b_index
hacl_get_blake2_info(Blake2Object *self)
{
    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            return Hacl_Hash_Blake2b_Simd256_info(self->blake2b_256_state);
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            return Hacl_Hash_Blake2s_Simd128_info(self->blake2s_128_state);
#endif
        case Blake2b:
            return Hacl_Hash_Blake2b_info(self->blake2b_state);
        case Blake2s:
            return Hacl_Hash_Blake2s_info(self->blake2s_state);
        default:
            Py_UNREACHABLE();
    }
}


static PyObject *
py_blake2b_get_digest_size(PyObject *op, void *Py_UNUSED(closure))
{
    Blake2Object *self = _Blake2Object_CAST(op);
    Hacl_Hash_Blake2b_index info = hacl_get_blake2_info(self);
    return PyLong_FromLong(info.digest_length);
}


static PyGetSetDef py_blake2b_getsetters[] = {
    {"name", py_blake2b_get_name, NULL, NULL, NULL},
    {"block_size", py_blake2b_get_block_size, NULL, NULL, NULL},
    {"digest_size", py_blake2b_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};


static int
py_blake2_clear(PyObject *op)
{
    Blake2Object *self = (Blake2Object *)op;
    // The initialization function uses PyObject_GC_New() but explicitly
    // initializes the HACL* internal state to NULL before allocating
    // it. If an error occurs in the constructor, we should only free
    // states that were allocated (i.e. that are not NULL).
#define BLAKE2_FREE(TYPE, STATE)                \
    do {                                        \
        if (STATE != NULL) {                    \
            Hacl_Hash_ ## TYPE ## _free(STATE); \
            STATE = NULL;                       \
        }                                       \
    } while (0)

    switch (self->impl) {
#if _Py_HACL_CAN_COMPILE_VEC256
        case Blake2b_256:
            BLAKE2_FREE(Blake2b_Simd256, self->blake2b_256_state);
            break;
#endif
#if _Py_HACL_CAN_COMPILE_VEC128
        case Blake2s_128:
            BLAKE2_FREE(Blake2s_Simd128, self->blake2s_128_state);
            break;
#endif
        case Blake2b:
            BLAKE2_FREE(Blake2b, self->blake2b_state);
            break;
        case Blake2s:
            BLAKE2_FREE(Blake2s, self->blake2s_state);
            break;
        default:
            Py_UNREACHABLE();
    }
#undef BLAKE2_FREE
    return 0;
}

static void
py_blake2_dealloc(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)py_blake2_clear(self);
    type->tp_free(self);
    Py_DECREF(type);
}

static int
py_blake2_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyType_Slot blake2b_type_slots[] = {
    {Py_tp_clear, py_blake2_clear},
    {Py_tp_dealloc, py_blake2_dealloc},
    {Py_tp_traverse, py_blake2_traverse},
    {Py_tp_doc, (char *)py_blake2b_new__doc__},
    {Py_tp_methods, py_blake2b_methods},
    {Py_tp_getset, py_blake2b_getsetters},
    {Py_tp_new, py_blake2b_new},
    {0, 0}
};

static PyType_Slot blake2s_type_slots[] = {
    {Py_tp_clear, py_blake2_clear},
    {Py_tp_dealloc, py_blake2_dealloc},
    {Py_tp_traverse, py_blake2_traverse},
    {Py_tp_doc, (char *)py_blake2s_new__doc__},
    {Py_tp_methods, py_blake2b_methods},
    {Py_tp_getset, py_blake2b_getsetters},
    // only the constructor differs, so that it can receive a clinic-generated
    // default digest length suitable for blake2s
    {Py_tp_new, py_blake2s_new},
    {0, 0}
};

static PyType_Spec blake2b_type_spec = {
    .name = "_blake2.blake2b",
    .basicsize = sizeof(Blake2Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
             | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HEAPTYPE,
    .slots = blake2b_type_slots
};

static PyType_Spec blake2s_type_spec = {
    .name = "_blake2.blake2s",
    .basicsize = sizeof(Blake2Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
             | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HEAPTYPE,
    .slots = blake2s_type_slots
};

/*
 * Python UUID module:
 * - wraps libuuid or Windows rpcrt4.dll.
 * - implements fast version of the uuid.py:UUID class.
 * - re-implements uuid4(), uuid6(), and uuid7() functions with improved performance
 *   by virtue of them being implemented in C and better entropy fetching
 *   strategy.
 *
 * DCE compatible Universally Unique Identifier library.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "pycore_critical_section.h"  // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_long.h"          // _PyLong_AsByteArray(), _PyLong_GetOne()
#include "pycore_pylifecycle.h"   // _PyOS_URandom()
#include "pycore_runtime_init.h"  // _Py_ID()
#include "pycore_strhex.h"        // _Py_strhex()

#if defined(HAVE_UUID_H)
  // AIX, FreeBSD, libuuid with pkgconf
  #include <uuid.h>
#elif defined(HAVE_UUID_UUID_H)
  // libuuid without pkgconf
  #include <uuid/uuid.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // getpid()
#endif
#ifdef HAVE_PROCESS_H
#  include <process.h>            // getpid()
#endif
#ifdef MS_WINDOWS
#  include <windows.h>            // GetCurrentProcessId()
#endif

#ifdef MS_WINDOWS
#include <rpc.h>
#ifndef RPC_S_OK
#define RPC_S_OK 0L
#endif
#endif

#include <string.h>               // memcpy(), memset(), strncmp()

#ifndef MS_WINDOWS


/*[clinic input]
module _uuid
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7cbed123a45a3859]*/


static PyObject *
py_uuid_generate_time_safe(PyObject *Py_UNUSED(context),
                           PyObject *Py_UNUSED(ignored))
{
    uuid_t uuid;
#ifdef HAVE_UUID_GENERATE_TIME_SAFE
    int res;

    res = uuid_generate_time_safe(uuid);
    return Py_BuildValue("y#i", (const char *) uuid, sizeof(uuid), res);
#elif defined(HAVE_UUID_CREATE)
    uint32_t status;
    uuid_create(&uuid, &status);
# if defined(HAVE_UUID_ENC_BE)
    unsigned char buf[sizeof(uuid)];
    uuid_enc_be(buf, &uuid);
    return Py_BuildValue("y#i", buf, sizeof(uuid), (int) status);
# else
    return Py_BuildValue("y#i", (const char *) &uuid, sizeof(uuid), (int) status);
# endif /* HAVE_UUID_CREATE */
#else /* HAVE_UUID_GENERATE_TIME_SAFE */
    uuid_generate_time(uuid);
    return Py_BuildValue("y#O", (const char *) uuid, sizeof(uuid), Py_None);
#endif /* HAVE_UUID_GENERATE_TIME_SAFE */
}

#else /* MS_WINDOWS */

static PyObject *
py_UuidCreate(PyObject *Py_UNUSED(context),
              PyObject *Py_UNUSED(ignored))
{
    UUID uuid;
    RPC_STATUS res;

    Py_BEGIN_ALLOW_THREADS
    res = UuidCreateSequential(&uuid);
    Py_END_ALLOW_THREADS

    switch (res) {
    case RPC_S_OK:
    case RPC_S_UUID_LOCAL_ONLY:
    case RPC_S_UUID_NO_ADDRESS:
        /*
        All success codes, but the latter two indicate that the UUID is random
        rather than based on the MAC address. If the OS can't figure this out,
        neither can we, so we'll take it anyway.
        */
        return Py_BuildValue("y#", (const char *)&uuid, sizeof(uuid));
    }
    PyErr_SetFromWindowsErr(res);
    return NULL;
}

static int
py_windows_has_stable_node(void)
{
    UUID uuid;
    RPC_STATUS res;
    Py_BEGIN_ALLOW_THREADS
    res = UuidCreateSequential(&uuid);
    Py_END_ALLOW_THREADS
    return res == RPC_S_OK;
}
#endif /* MS_WINDOWS */


#define UUID_SIZE 16

typedef struct uuidobject {
    PyObject_HEAD
    uint8_t bytes[UUID_SIZE];
    Py_hash_t cached_hash;
    PyObject *is_safe;
    PyObject *weakreflist;
} uuidobject;

#define uuidobject_CAST(OP) ((uuidobject *)(OP))


// UUID Structure per RFC 9562:
//
// A UUID is 128 bits (16 bytes) represented as:
//
// String:       xx xx xx xx - xx xx - Mx xx - Nx xx - xx xx xx xx xx xx
// Byte pos:     0  1  2  3    4  5    6  7    8  9    10 11 12 13 14 15
//
// Time-based layouts in big-endian order (field widths in bits):
//
// v1: time_low(32) | time_mid(16) | version(4) | time_high(12) |
//     variant(2) | clock_seq(14) | node(48)
// v6: time_high(32) | time_mid(16) | version(4) | time_low(12) |
//     variant(2) | clock_seq(14) | node(48)
// v7: unix_ts_ms(48) | version(4) | rand_a(12) | variant(2) | rand_b(62)
//
// v1 and v6 encode the same 60-bit timestamp in 100-nanosecond intervals
// since 1582-10-15, with different field ordering. v7 uses milliseconds
// since the Unix epoch. Its rand_a and rand_b fields hold random/counter
// bits, with no clock sequence or node field.
//
// M is the version field: the most significant 4 bits of byte 6.
//
// N contains the variant: the most significant variable bits of byte 8.
//   0xxx: Reserved for NCS compatibility
//   10xx: RFC 4122/9562 (standard)
//   110x: Reserved for Microsoft compatibility
//   111x: Reserved for future definition

#define RANDOM_BUF_SIZE 256
#define MAX_FREE_LIST_SIZE 32
#define NODE_NOT_INITIALIZED UINT64_MAX

/* State of the _uuid module */
typedef struct {
    PyTypeObject *uuid_type;

    PyObject *safe_uuid;

    PyObject *uint128_max;

    PyObject *random_func;
    PyObject *random_size_int;
    PyObject *time_func;
    PyObject *node_func;

    // A freelist for uuid objects -- 15-20% performance boost.
    uuidobject *freelist;
    size_t freelist_size;

    // We overfetch entropy to speed up successive uuid generations;
    // this enables 10x peformance boost.
    uint8_t random_buf[RANDOM_BUF_SIZE];
    uint64_t random_idx;
    uint64_t random_last_pid;

    // UUID v6 state
    PyMutex node_mutex;
    uint64_t node_id;
    uint64_t last_timestamp_v6;
    uint8_t last_timestamp_v6_init;

    // UUID v7 state
    uint64_t last_timestamp_v7;
    uint64_t last_counter_v7;
    uint8_t last_timestamp_v7_init;
} uuid_state;

#include "clinic/_uuidmodule.c.h"

/*[clinic input]
class _uuid.UUID "uuidobject *" "&uuid_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b62c7d736c192881]*/

// Forward declarations
static int from_hex(uuidobject *self, PyObject *hex);
static int from_bytes_le(uuidobject *self, PyObject *bytes_le);
static int from_int(uuidobject *self, PyObject *int_value, int validate);
static int from_fields(uuidobject *self, PyObject *fields);
static PyObject *get_SafeUUID(uuid_state *state);
static PyObject *get_uuidmod_attr(const char *name);

static uint64_t
uuid_getpid(void)
{
#if !defined(MS_WINDOWS) || defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)
    return (uint64_t)getpid();
#else
    return (uint64_t)GetCurrentProcessId();
#endif
}

static struct PyModuleDef uuidmodule;

static inline uuid_state *
get_uuid_state(PyObject *mod)
{
    uuid_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static inline uuid_state *
get_uuid_state_by_cls(PyTypeObject *cls)
{
    PyObject *module = PyType_GetModuleByDef(cls, &uuidmodule);
    assert(module != NULL);
    uuid_state *state = get_uuid_state(module);
    assert(state != NULL);
    return state;
}

// Forward declaration
static PyObject *uuid_from_bytes_array(PyTypeObject *type, uint8_t bytes[UUID_SIZE]);

static int
gen_time(uuid_state *state, PyTime_t *time)
{
    if (state->time_func == NULL) {
        return PyTime_Time(time);
    }

    PyObject *time_func = Py_NewRef(state->time_func);
    PyObject *ret = PyObject_CallNoArgs(time_func);
    Py_DECREF(time_func);
    if (ret == NULL) {
        return -1;
    }

    if (!PyLong_CheckExact(ret)) {
        PyErr_SetString(PyExc_ValueError, "time_func must return int");
        Py_DECREF(ret);
        return -1;
    }

    int res = PyLong_AsInt64(ret, time);
    Py_DECREF(ret);
    return res;
}

static int
gen_random_lock_held(PyObject *module, uuid_state *state, uint8_t *bytes,
                     Py_ssize_t size)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(module);

    // Overfetching & caching entropy improves the performance 10x.
    // There's a precedent with NodeJS doing exact same thing for
    // improving performance of their UUID implementation.

    uint64_t pid = uuid_getpid();
    if (pid != state->random_last_pid) {
        // The main concern to take core of with caching entropy is handling
        // fork -- we don't want the child process to share any entropy with
        // us. Luckily getpid() is fast.
        state->random_last_pid = pid;
        state->random_idx = RANDOM_BUF_SIZE;
    }

    if (state->random_idx + size <= RANDOM_BUF_SIZE) {
        memcpy(bytes, state->random_buf + state->random_idx, size);
        state->random_idx += size;
    }
    else {
        if (state->random_idx < RANDOM_BUF_SIZE) {
            // We exhaustively consume cached entropy. We do that
            // because we have tests that compare Python and C
            // implementations and it's important that they see incoming
            // entropy as a continuous stream.
            // The overhead here must be negligible, but we want the same
            // code to be run in production and in tests.
            Py_ssize_t partial = RANDOM_BUF_SIZE - state->random_idx;
            memcpy(bytes, state->random_buf + state->random_idx, partial);
            bytes += partial;
            size -= partial;
        }

        // Reserve the cached bytes before a refill can release the module's
        // critical section. Another call must not consume the same bytes.
        state->random_idx = RANDOM_BUF_SIZE;
        uint8_t random_buf[RANDOM_BUF_SIZE];

        if (state->random_func != NULL) {
            PyObject *random_func = Py_NewRef(state->random_func);
            PyObject *buf = PyObject_CallOneArg(
                random_func, state->random_size_int);
            Py_DECREF(random_func);
            if (buf == NULL) {
                return -1;
            }

            if (!PyBytes_CheckExact(buf)) {
                PyErr_SetString(PyExc_ValueError, "random_func must return bytes");
                Py_DECREF(buf);
                return -1;
            }

            if (PyBytes_Size(buf) != (Py_ssize_t)RANDOM_BUF_SIZE) {
                PyErr_Format(
                    PyExc_ValueError,
                    "random_func must return bytes of length %zd exactly",
                    (Py_ssize_t)RANDOM_BUF_SIZE
                );
                Py_DECREF(buf);
                return -1;
            }

            memcpy(random_buf, PyBytes_AsString(buf), sizeof(random_buf));
            Py_DECREF(buf);
        }
        else {
            // _PyOS_URandom() releases the thread state. Fill a private buffer
            // so concurrent refills cannot write into the same memory.
            if (_PyOS_URandom(random_buf, sizeof(random_buf)) < 0) {
                return -1;
            }
        }

        // We hold the caller's module lock again after fetching entropy.
        // Replace the cache, discarding any unused bytes from a concurrent refill.
        memcpy(state->random_buf, random_buf, sizeof(random_buf));
        memcpy(bytes, state->random_buf, size);
        state->random_idx = size;
    }

    return 0;
}

/*[clinic input]
@critical_section
_uuid.uuid4

Generate a random UUID (version 4).
[clinic start generated code]*/

static PyObject *
_uuid_uuid4_impl(PyObject *module)
/*[clinic end generated code: output=b835af30d9d6efc5 input=9cfb3a3b71c25cdf]*/
{
    uuid_state *state = get_uuid_state(module);
    uint8_t bytes[UUID_SIZE];

    if (gen_random_lock_held(module, state, bytes, sizeof(bytes)) < 0) {
        return NULL;
    }

    // Set version (4) and variant
    bytes[6] = (uint8_t)((bytes[6] & 0x0f) | 0x40);
    bytes[8] = (uint8_t)((bytes[8] & 0x3f) | 0x80);

    return uuid_from_bytes_array(state->uuid_type, bytes);
}

static int
uuid6_mask(PyObject *value, uint64_t mask, uint64_t *result)
{
    if (PyLong_CheckExact(value)) {
        *result = PyLong_AsUnsignedLongLongMask(value) & mask;
        return 0;
    }

    // Preserve overridden bitwise operations and reject objects that only
    // implement __index__, which the Python implementation does not accept.
    PyObject *mask_obj = PyLong_FromUnsignedLongLong(mask);
    if (mask_obj == NULL) {
        return -1;
    }
    PyObject *masked = PyNumber_And(value, mask_obj);
    Py_DECREF(mask_obj);
    if (masked == NULL) {
        return -1;
    }
    int ret = PyLong_AsUInt64(masked, result);
    Py_DECREF(masked);
    return ret;
}

static int
uuid6_get_node(uuid_state *state, uint64_t *result)
{
    // Cached reads need no lock. The sentinel is outside the 48-bit node range.
    uint64_t node_id = _Py_atomic_load_uint64_relaxed(&state->node_id);
    if (node_id == NODE_NOT_INITIALIZED) {
        int ret = 0;
        Py_BEGIN_CRITICAL_SECTION_MUTEX(&state->node_mutex);
        node_id = _Py_atomic_load_uint64_relaxed(&state->node_id);
        if (node_id == NODE_NOT_INITIALIZED) {
            PyObject *getnode = state->node_func != NULL
                ? Py_NewRef(state->node_func) : get_uuidmod_attr("getnode");
            PyObject *value = NULL;
            if (getnode != NULL) {
                value = PyObject_CallNoArgs(getnode);
                Py_DECREF(getnode);
            }
            if (value == NULL) {
                ret = -1;
            }
            else {
                uint64_t resolved;
                ret = uuid6_mask(value, 0xffffffffffff, &resolved);
                Py_DECREF(value);
                if (ret == 0) {
                    // getnode() may suspend this critical section or reenter
                    // uuid6(). Keep a value cached by an intervening call.
                    node_id = _Py_atomic_load_uint64_relaxed(&state->node_id);
                    if (node_id == NODE_NOT_INITIALIZED) {
                        node_id = resolved;
                        _Py_atomic_store_uint64_relaxed(&state->node_id, node_id);
                    }
                }
            }
        }
        Py_END_CRITICAL_SECTION();
        if (ret < 0) {
            return -1;
        }
    }
    *result = node_id;
    return 0;
}

/*[clinic input]
_uuid.uuid6

    node: object = None
    clock_seq: object = None

Generate a UUID with reordered UUIDv1 fields for improved DB locality.

More precisely, given a 60-bit timestamp value as specified for UUIDv1,
for UUIDv6 the first 48 most significant bits are stored first, followed
by the 4-bit version (same position), followed by the remaining 12 bits
of the original 60-bit timestamp.
[clinic start generated code]*/

static PyObject *
_uuid_uuid6_impl(PyObject *module, PyObject *node, PyObject *clock_seq)
/*[clinic end generated code: output=ee0446987b1efc6c input=64fd6ac090b44c37]*/
{
    uuid_state *state = get_uuid_state(module);
    PyTime_t nanoseconds;
    uint8_t random[2];
    int ret;
    Py_BEGIN_CRITICAL_SECTION(module);
    ret = gen_time(state, &nanoseconds);
    if (ret == 0 && clock_seq == Py_None) {
        ret = gen_random_lock_held(module, state, random, sizeof(random));
    }
    Py_END_CRITICAL_SECTION();
    if (ret < 0) {
        return NULL;
    }

    uint64_t clock_s;
    if (clock_seq == Py_None) {
        clock_s = ((uint64_t)(random[0] & 0x3f) << 8) | random[1];
    }
    else if (uuid6_mask(clock_seq, 0x3fff, &clock_s) < 0) {
        return NULL;
    }

    uint64_t node_id;
    if (node == Py_None) {
        if (uuid6_get_node(state, &node_id) < 0) {
            return NULL;
        }
    }
    else if (uuid6_mask(node, 0xffffffffffff, &node_id) < 0) {
        return NULL;
    }

    // Match Python's floor division, including times before the Unix epoch.
    int64_t ticks = nanoseconds / 100;
    if (nanoseconds < 0 && nanoseconds % 100 != 0) {
        ticks--;
    }
    // Number of 100-ns intervals from 1582-10-15 to 1970-01-01.
    uint64_t timestamp = (uint64_t)(ticks + 0x01b21dd213814000);
    // Calls above may release the module lock or reenter uuid6(). Read and
    // advance the shared timestamp only after all Python callbacks have run.
    Py_BEGIN_CRITICAL_SECTION(module);
    if (state->last_timestamp_v6_init && timestamp <= state->last_timestamp_v6) {
        timestamp = state->last_timestamp_v6 + 1;
    }
    state->last_timestamp_v6 = timestamp;
    state->last_timestamp_v6_init = 1;
    Py_END_CRITICAL_SECTION();

    uint8_t bytes[UUID_SIZE];
    // time_high(32) | time_mid(16) | version(4) | time_low(12) |
    // variant(2) | clock_seq(14) | node(48)
    for (int i = 0; i < 6; i++) {
        bytes[i] = (uint8_t)(timestamp >> (52 - i * 8));
        bytes[10 + i] = (uint8_t)(node_id >> (40 - i * 8));
    }
    bytes[6] = (uint8_t)(((timestamp >> 8) & 0x0f) | 0x60);
    bytes[7] = (uint8_t)timestamp;
    bytes[8] = (uint8_t)((clock_s >> 8) | 0x80);
    bytes[9] = (uint8_t)clock_s;
    return uuid_from_bytes_array(state->uuid_type, bytes);
}

static inline int
uuid7_get_counter_and_tail(PyObject *module, uuid_state *state,
                          uint64_t *counter, uint8_t *tail)
{
    uint8_t rand_bytes[10];
    if (gen_random_lock_held(module, state, rand_bytes, sizeof(rand_bytes)) < 0) {
        return -1;
    }

    *counter = (((uint64_t)rand_bytes[0] & 0x01) << 40) |
                ((uint64_t)rand_bytes[1] << 32) |
                ((uint64_t)rand_bytes[2] << 24) |
                ((uint64_t)rand_bytes[3] << 16) |
                ((uint64_t)rand_bytes[4] << 8) |
                ((uint64_t)rand_bytes[5]);

    memcpy(tail, rand_bytes + 6, 4);
    return 0;
}


// The module's critical section protects the cached entropy and UUIDv7 state.
// It may be suspended during entropy fetching or a Python testing hook.
/*[clinic input]
@critical_section
_uuid.uuid7

Generate a UUID from a Unix timestamp in milliseconds and random bits.

UUIDv7 objects feature monotonicity within a millisecond.
[clinic start generated code]*/

static PyObject *
_uuid_uuid7_impl(PyObject *module)
/*[clinic end generated code: output=f301accc11162c91 input=88514d61dc785108]*/
{
    uuid_state *state = get_uuid_state(module);
    uint8_t bytes[UUID_SIZE];
    uint64_t timestamp_ms, counter;

    PyTime_t pytime;
    if (gen_time(state, &pytime) < 0) {
        return NULL;
    }
    uint64_t last_timestamp, last_counter;
    uint8_t last_timestamp_init;
    do {
        last_timestamp = state->last_timestamp_v7;
        last_counter = state->last_counter_v7;
        last_timestamp_init = state->last_timestamp_v7_init;
        timestamp_ms = (uint64_t)(pytime / 1000000);

        if (!last_timestamp_init || timestamp_ms > last_timestamp) {
            if (uuid7_get_counter_and_tail(module, state, &counter, bytes + 12) < 0) {
                return NULL;
            }
        }
        else {
            if (timestamp_ms < last_timestamp) {
                timestamp_ms = last_timestamp + 1;
            }
            // advance the 42-bit counter
            counter = last_counter + 1;
            if (counter > 0x3FFFFFFFFFF) {
                // advance the 48-bit timestamp
                timestamp_ms += 1;
                if (uuid7_get_counter_and_tail(module, state, &counter, bytes + 12) < 0) {
                    return NULL;
                }
            }
            else {
                // This is the common fast path, we only need 4 bytes of entropy
                // 32-bit random data
                if (gen_random_lock_held(module, state, bytes + 12, 4) < 0) {
                    return NULL;
                }
            }
        }
        // A refill can suspend the critical section and let another call
        // advance the state. Recompute instead of committing a stale counter.
    } while (last_timestamp != state->last_timestamp_v7 ||
             last_counter != state->last_counter_v7 ||
             last_timestamp_init != state->last_timestamp_v7_init);

    timestamp_ms &= 0xFFFFFFFFFFFF;
    bytes[0] = (uint8_t)(timestamp_ms >> 40);
    bytes[1] = (uint8_t)(timestamp_ms >> 32);
    bytes[2] = (uint8_t)(timestamp_ms >> 24);
    bytes[3] = (uint8_t)(timestamp_ms >> 16);
    bytes[4] = (uint8_t)(timestamp_ms >> 8);
    bytes[5] = (uint8_t)timestamp_ms;

    uint16_t counter_hi = (uint16_t)((counter >> 30) & 0x0FFF);
    bytes[6] = (uint8_t)(0x70 | (counter_hi >> 8));  // Version 7 = 0111
    bytes[7] = (uint8_t)counter_hi;

    uint16_t counter_mid = (uint16_t)((counter >> 16) & 0x3FFF);
    bytes[8] = (uint8_t)(0x80 | (counter_mid >> 8));  // Variant = 10
    bytes[9] = (uint8_t)counter_mid;

    uint16_t counter_lo = (uint16_t)(counter & 0xFFFF);
    bytes[10] = (uint8_t)(counter_lo >> 8);
    bytes[11] = (uint8_t)counter_lo;

    state->last_timestamp_v7_init = 1;
    state->last_timestamp_v7 = timestamp_ms;
    state->last_counter_v7 = counter;

    return uuid_from_bytes_array(state->uuid_type, bytes);
}

/*[clinic input]
@critical_section
_uuid._install_c_hooks

    *
    random_func: object
    time_func: object
    node_func: object = None

[clinic start generated code]*/

static PyObject *
_uuid__install_c_hooks_impl(PyObject *module, PyObject *random_func,
                            PyObject *time_func, PyObject *node_func)
/*[clinic end generated code: output=e742978b99d5a2ea input=beb6f73390b31d93]*/
{
    uuid_state *state = get_uuid_state(module);

    // Reset bufferred entropy -- tests need to always start fresh
    // when the random function is changed. Reset timestamps for repeatable tests.
    state->random_idx = RANDOM_BUF_SIZE;
    state->last_timestamp_v6_init = 0;
    state->last_timestamp_v7_init = 0;
    Py_BEGIN_CRITICAL_SECTION_MUTEX(&state->node_mutex);
    Py_XSETREF(state->node_func,
               node_func == Py_None ? NULL : Py_NewRef(node_func));
    _Py_atomic_store_uint64_relaxed(&state->node_id, NODE_NOT_INITIALIZED);
    Py_END_CRITICAL_SECTION();

    if (random_func == Py_None) {
        Py_CLEAR(state->random_func);
    }
    else {
        Py_XSETREF(state->random_func, Py_NewRef(random_func));
    }

    if (time_func == Py_None) {
        Py_CLEAR(state->time_func);
    }
    else {
        Py_XSETREF(state->time_func, Py_NewRef(time_func));
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_uuid.UUID.__init__

    hex: object = None
    bytes: object = None
    bytes_le: object = None
    fields: object = None
    int: object = None
    version: object = None
    *
    is_safe: object = NULL

UUID is a fast base implementation type for uuid.UUID.
[clinic start generated code]*/

static int
_uuid_UUID___init___impl(uuidobject *self, PyObject *hex, PyObject *bytes,
                         PyObject *bytes_le, PyObject *fields,
                         PyObject *int_value, PyObject *version,
                         PyObject *is_safe)
/*[clinic end generated code: output=6d100803f5ecd751 input=23c754454cc11724]*/

{
    // Explicit None values do not select a UUID input form.
    int passed = 0;
    if (hex != Py_None) passed++;
    if (bytes != Py_None) passed++;
    if (bytes_le != Py_None) passed++;
    if (fields != Py_None) passed++;
    if (int_value != Py_None) passed++;
    if (passed != 1) {
        PyErr_SetString(
            PyExc_TypeError,
            "one of the hex, bytes, bytes_le, fields, or int arguments must be given"
        );
        return -1;
    }

    if (hex != Py_None) {
        if (from_hex(self, hex) < 0) {
            return -1;
        }
    }
    else if (bytes != Py_None) {
        if (!PyBytes_Check(bytes)) {
            PyErr_SetString(PyExc_TypeError, "bytes must be a bytes object");
            return -1;
        }
        if (PyBytes_GET_SIZE(bytes) != UUID_SIZE) {
            PyErr_SetString(
                PyExc_ValueError,
                "bytes is not a 16-char string"
            );
            return -1;
        }
        memcpy(self->bytes, PyBytes_AS_STRING(bytes), sizeof(self->bytes));
    }
    else if (bytes_le != Py_None) {
        if (from_bytes_le(self, bytes_le) < 0) {
            return -1;
        }
    }
    else if (fields != Py_None) {
        if (from_fields(self, fields) < 0) {
            return -1;
        }
    }
    else if (int_value != Py_None) {
        if (from_int(self, int_value, 1) < 0) {
            return -1;
        }
    }
    else {
        Py_UNREACHABLE();
    }

    if (version != Py_None) {
        long version_num = PyLong_AsLong(version);
        if (version_num == -1 && PyErr_Occurred()) {
            return -1;
        }
        if (version_num < 1 || version_num > 8) {
            PyErr_SetString(PyExc_ValueError, "illegal version number");
            return -1;
        }

        // Clear variant bits (keep only lower 6 bits of byte 8)
        self->bytes[8] &= 0x3f;  // 0011 1111
        // Clear version bits (keep only lower 4 bits of byte 6)
        self->bytes[6] &= 0x0f;  // 0000 1111
        // Set the variant to RFC 4122/9562 (binary 10xx xxxx)
        self->bytes[8] |= 0x80;  // 1000 0000
        // Set the version number (upper 4 bits of byte 6)
        self->bytes[6] |= (uint8_t)(version_num << 4);
    }

    Py_XSETREF(self->is_safe, Py_XNewRef(is_safe));

    return 0;
}


static const uint8_t INT_TO_HEX[] = "0123456789abcdef";

// Lookup table for hex character to value conversion
// -1 for invalid characters
static const int8_t HEX_TO_INT[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

static inline void
byte_to_hex(uint8_t byte, char *hex)
{
    hex[0] = INT_TO_HEX[(byte >> 4) & 0xf];
    hex[1] = INT_TO_HEX[byte & 0xf];
}

static int
from_hex(uuidobject *self, PyObject *hex)
{
    Py_ssize_t size;
    const char *start = PyUnicode_AsUTF8AndSize(hex, &size);
    if (start == NULL) {
        return -1;
    }

    uint8_t ch;
    uint8_t acc, acc_set;
    int8_t part;
    int i, j;

    // Reimplement `hex = hex.replace('urn:', '').replace('uuid:', '')`
    if (size > 0 && start[0] == 'u') {
        if (size >= 9 && (strncmp(start, "urn:uuid:", 9) == 0 ||
                          strncmp(start, "uuid:urn:", 9) == 0)) {
            start += 9;
            size -= 9;
        }
        else if (size >= 4 && strncmp(start, "urn:", 4) == 0) {
            start += 4;
            size -= 4;
        }
        else if (size >= 5 && strncmp(start, "uuid:", 5) == 0) {
            start += 5;
            size -= 5;
        }
    }

    // Reimplement `hex = hex.strip('{}')`
    while (size >= 1 && (start[0] == '{' || start[0] == '}')) {
        start++;
        size -= 1;
    }
    while (size >= 1 && (start[size - 1] == '{' || start[size - 1] == '}')) {
        size -= 1;
    }

    if (size < 32) {
        PyErr_SetString(
            PyExc_ValueError,
            "badly formed hexadecimal UUID string"
        );
        return -1;
    }

    acc_set = 0;
    j = 0;

    for (i = 0; i < size; i++) {
        ch = (uint8_t)start[i];

        if (ch == '-') {
            continue;
        }

        part = HEX_TO_INT[ch];
        if (part == -1) {
            PyErr_SetString(
                PyExc_ValueError,
                "badly formed hexadecimal UUID string"
            );
            return -1;
        }

        if (acc_set) {
            acc |= (uint8_t)part;
            self->bytes[j] = acc;
            acc_set = 0;
            j++;
        }
        else {
            acc = (uint8_t)(part << 4);
            acc_set = 1;
        }

        if (j > UUID_SIZE || (j == UUID_SIZE && acc_set)) {
            PyErr_Format(PyExc_ValueError,
                "invalid UUID '%s': decodes to more than 16 bytes",
                hex);
            return -1;
        }
    }

    if (j != UUID_SIZE) {
        PyErr_Format(PyExc_ValueError,
            "invalid UUID '%s': decodes to less than 16 bytes",
            hex);
        return -1;
    }

    return 0;
}

static int
from_bytes_le(uuidobject *self, PyObject *bytes_le)
{
    if (!PyBytes_Check(bytes_le)) {
        PyErr_SetString(PyExc_TypeError, "bytes_le must be a bytes object");
        return -1;
    }
    if (PyBytes_GET_SIZE(bytes_le) != UUID_SIZE) {
        PyErr_SetString(PyExc_ValueError,
            "bytes_le is not a 16-char string");
        return -1;
    }

    // Convert from little-endian to big-endian UUID format
    // UUID fields in little-endian order need to be byte-swapped:
    // - time_low (4 bytes)
    // - time_mid (2 bytes)
    // - time_hi_version (2 bytes)
    // - clock_seq_hi_variant (1 byte) - no swap needed
    // - clock_seq_low (1 byte) - no swap needed
    // - node (6 bytes) - no swap needed

    const unsigned char *src = (const unsigned char *)PyBytes_AS_STRING(bytes_le);
    unsigned char *dst = (unsigned char *)self->bytes;

    // Swap time_low (bytes 0-3)
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];

    // Swap time_mid (bytes 4-5)
    dst[4] = src[5];
    dst[5] = src[4];

    // Swap time_hi_version (bytes 6-7)
    dst[6] = src[7];
    dst[7] = src[6];

    // Copy clock_seq and node as-is (bytes 8-15)
    memcpy(dst + 8, src + 8, 8);

    return 0;
}

static int
validate_int(uuid_state *state, PyObject *int_value)
{
    if (!PyLong_Check(int_value)) {
        PyErr_SetString(PyExc_TypeError, "value must be an integer");
        return -1;
    }

    int cmp = PyLong_IsNegative(int_value);
    if (cmp < 0) {
        return -1;
    }
    if (cmp == 1) {
        PyErr_SetString(PyExc_ValueError,
            "int is out of range (need a 128-bit value)");
        return -1;
    }

    // Check if it's greater than max (2^128 - 1)
    cmp = PyObject_RichCompareBool(int_value, state->uint128_max, Py_GT);
    if (cmp < 0) {
        return -1;
    }
    if (cmp == 1) {
        PyErr_SetString(PyExc_ValueError,
            "int is out of range (need a 128-bit value)");
        return -1;
    }

    return 0;
}

static int
from_int(uuidobject *self, PyObject *int_value, int validate)
{
    // Convert a 128-bit integer to UUID bytes (big-endian)

    uuid_state *state = get_uuid_state_by_cls(Py_TYPE(self));

    if (validate && validate_int(state, int_value) < 0) {
        return -1;
    }

    return _PyLong_AsByteArray(
        (PyLongObject *)int_value,
        (unsigned char *)self->bytes,
        sizeof(self->bytes),
        0,  // big-endian
        0,  // unsigned
        1   // with_exceptions
    );
}

static int
extract_field(
    PyObject *fields,
    int field_num,
    uint64_t max_value,
    const char *error_msg,
    uint64_t *result
) {
    PyObject *field = PySequence_GetItem(fields, field_num);
    if (field == NULL) {
        return -1;
    }

    if (!PyLong_Check(field)) {
        PyErr_Format(PyExc_TypeError, "field %d must be an integer", field_num);
        goto fail;
    }

    uint64_t value;
    if (PyLong_AsUInt64(field, &value) < 0) {
        goto fail;
    }

    if (value > max_value) {
        PyErr_SetString(PyExc_ValueError, error_msg);
        goto fail;
    }

    *result = value;
    Py_DECREF(field);
    return 0;

fail:
    Py_DECREF(field);
    return -1;
}

static int
from_fields(uuidobject *self, PyObject *fields)
{
    // Validate the length without replacing errors raised by __len__.
    Py_ssize_t len = PySequence_Size(fields);
    if (len < 0) {
        return -1;
    }
    if (len != 6) {
        PyErr_SetString(PyExc_ValueError, "fields is not a 6-tuple");
        return -1;
    }

#define EXTRACT_FIELD(FIELD_NUM, MAX_VALUE, ERROR_MSG, TYPE, NAME) \
    TYPE NAME;                                                     \
    do {                                                           \
        uint64_t NAME##_extracted;                                 \
        if (extract_field(fields, FIELD_NUM, MAX_VALUE, ERROR_MSG, \
                          &NAME##_extracted) < 0)                  \
        {                                                          \
            return -1;                                             \
        }                                                          \
        NAME = (TYPE)NAME##_extracted;                             \
    } while (0)

    EXTRACT_FIELD(
        0, (1ULL << 32) - 1, "field 1 out of range (need a 32-bit value)",
        uint32_t, time_low
    );
    EXTRACT_FIELD(
        1, (1ULL << 16) - 1, "field 2 out of range (need a 16-bit value)",
        uint16_t, time_mid
    );
    EXTRACT_FIELD(
        2, (1ULL << 16) - 1, "field 3 out of range (need a 16-bit value)",
        uint16_t, time_hi_version
    );
    EXTRACT_FIELD(
        3, (1ULL << 8) - 1, "field 4 out of range (need an 8-bit value)",
        uint8_t, clock_seq_hi_variant
    );
    EXTRACT_FIELD(
        4, (1ULL << 8) - 1, "field 5 out of range (need an 8-bit value)",
        uint8_t, clock_seq_low
    );
    EXTRACT_FIELD(
        5, (1ULL << 48) - 1, "field 6 out of range (need a 48-bit value)",
        uint64_t, node
    );

#undef EXTRACT_FIELD

    self->bytes[0] = (uint8_t)(time_low >> 24);
    self->bytes[1] = (uint8_t)(time_low >> 16);
    self->bytes[2] = (uint8_t)(time_low >> 8);
    self->bytes[3] = (uint8_t)time_low;

    self->bytes[4] = (uint8_t)(time_mid >> 8);
    self->bytes[5] = (uint8_t)time_mid;

    self->bytes[6] = (uint8_t)(time_hi_version >> 8);
    self->bytes[7] = (uint8_t)time_hi_version;

    self->bytes[8] = clock_seq_hi_variant;

    self->bytes[9] = clock_seq_low;

    self->bytes[10] = (uint8_t)(node >> 40);
    self->bytes[11] = (uint8_t)(node >> 32);
    self->bytes[12] = (uint8_t)(node >> 24);
    self->bytes[13] = (uint8_t)(node >> 16);
    self->bytes[14] = (uint8_t)(node >> 8);
    self->bytes[15] = (uint8_t)node;

    return 0;
}

static PyObject *
get_int(uuidobject *self)
{
    return _PyLong_FromByteArray(
        (unsigned char *)self->bytes, sizeof(self->bytes), 0, 0);
}

static PyObject *
get_uuidmod_attr(const char *name)
{
    return PyImport_ImportModuleAttrString("uuid", name);
}

static PyObject *
get_SafeUUID(uuid_state *state)
{
    if (state->safe_uuid != NULL) {
        return Py_NewRef(state->safe_uuid);
    }

    state->safe_uuid = get_uuidmod_attr("SafeUUID");
    return Py_XNewRef(state->safe_uuid);
}

static uuidobject *
make_uuid(PyTypeObject *type)
{
    uuidobject *self = NULL;
    uuid_state *state = get_uuid_state_by_cls(type);

    if (type != state->uuid_type) {
        // Allocate the subclass's full instance size and enable GC tracking.
        self = uuidobject_CAST(PyType_GenericAlloc(type, 0));
        if (self == NULL) {
            return NULL;
        }
        goto init;
    }

    Py_BEGIN_CRITICAL_SECTION(type);
    if (state->freelist_size > 0) {
        self = state->freelist;
        state->freelist = uuidobject_CAST(self->weakreflist);
        state->freelist_size--;
    }
    Py_END_CRITICAL_SECTION();

    if (self != NULL) {
        // Deallocation releases the type reference even when caching the object,
        // so the freelist does not keep the type and its module alive. Restore
        // that reference and reinitialize the object before returning it.
        PyObject_Init((PyObject *)self, type);
    }
    else {
        self = PyObject_New(uuidobject, type);
        if (self == NULL) {
            return NULL;
        }
    }

init:
    self->is_safe = NULL;

    self->weakreflist = NULL;
    self->cached_hash = -1;

    return self;
}

static PyObject *
Uuid_alloc(PyTypeObject *type, Py_ssize_t Py_UNUSED(nitems))
{
    uuidobject *self = make_uuid(type);
    if (self == NULL) {
        return NULL;
    }
    memset(self->bytes, 0, sizeof(self->bytes));
    return (PyObject *)self;
}

static void
Uuid_dealloc(PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);

    uuidobject *uuid = uuidobject_CAST(obj);
    if (uuid->weakreflist != NULL) {
        PyObject_ClearWeakRefs(obj);
    }
    Py_CLEAR(uuid->is_safe);

    int skip_dealloc = 0;
    // Only the base UUID type uses the freelist. Do not search a subclass's
    // MRO: GC may already have cleared its base type's module reference.
    PyObject *module = PyType_GetModule_DuringGC(type);
    if (module == NULL) {
        goto dealloc;
    }
    assert(PyModule_Check(module));
    if (PyModule_GetDef(module) != &uuidmodule) {
        goto dealloc;
    }
    uuid_state *state = get_uuid_state(module);
    if (type != state->uuid_type) {
        goto dealloc;
    }

    Py_BEGIN_CRITICAL_SECTION(type);
    if (state->freelist_size < MAX_FREE_LIST_SIZE) {
        uuidobject *head = state->freelist;
        state->freelist = uuid;
        uuid->weakreflist = (PyObject *)head;
        state->freelist_size++;
        skip_dealloc = 1;
    }
    Py_END_CRITICAL_SECTION();

dealloc:
    if (!skip_dealloc) {
        type->tp_free(uuid);
    }
    // Only live instances own a reference to the heap type. Keeping that
    // reference in the freelist would prevent the module from being collected.
    Py_DECREF(type);
}


static PyObject *
Uuid_get_int(PyObject *op, void *Py_UNUSED(closure))
{
    return get_int(uuidobject_CAST(op));
}

static PyObject *
Uuid_get_is_safe(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    uuid_state *state = get_uuid_state_by_cls(Py_TYPE(self));
    if (self->is_safe == NULL) {
        PyObject *SafeUUID = get_SafeUUID(state);
        if (SafeUUID == NULL) {
            return NULL;
        }
        PyObject *ret = PyObject_GetAttrString(SafeUUID, "unknown");
        Py_DECREF(SafeUUID);
        return ret;
    }
    return Py_NewRef(self->is_safe);
}

static PyObject *
Uuid_get_hex(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return _Py_strhex((const char *)self->bytes, sizeof(self->bytes));
}

static PyObject *
Uuid_get_variant(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    uint8_t variant_byte = self->bytes[8];

    // xxx - three high bits of variant_byte are unknown
    if (!(variant_byte & 0x80)) {   // & 0b1000_0000
        // 0xx - RESERVED_NCS
        return get_uuidmod_attr("RESERVED_NCS");
    }

    // 1xx -- we know that high bit must be 1
    if (!(variant_byte & 0x40)) {   // & 0b0100_0000
        // 10x - RFC_4122
        return get_uuidmod_attr("RFC_4122");
    }

    // 11x -- we know that two high bits are 1
    if (!(variant_byte & 0x20)) {   // & 0b0010_0000
        // 110 - RESERVED_MICROSOFT
        return get_uuidmod_attr("RESERVED_MICROSOFT");
    }

    // 111 -- we know that all three high bits are 1 - RESERVED_FUTURE
    return get_uuidmod_attr("RESERVED_FUTURE");
}

static int
is_rfc_4122(uuidobject *self)
{
    return (self->bytes[8] & 0xc0) == 0x80;
}

static long
get_version(uuidobject *self)
{
    // RFC_4122 is when bit 7 is set (0x80) and bit 6 is not set (0x40)
    // 0xc0 = 0b11000000
    // 0x80 = 0b10000000
    if (!is_rfc_4122(self)) {
        return 0;
    }
    return (self->bytes[6] >> 4) & 0xf;
}

static PyObject *
Uuid_get_version(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    if (!is_rfc_4122(self)) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(get_version(self));
}

static inline uint32_t
get_time_low(uuidobject *self)
{
    return ((uint32_t)self->bytes[0] << 24) |
           ((uint32_t)self->bytes[1] << 16) |
           ((uint32_t)self->bytes[2] << 8) |
           ((uint32_t)self->bytes[3]);
}

static inline uint16_t
get_time_mid(uuidobject *self)
{
    return (uint16_t)((self->bytes[4] << 8) | self->bytes[5]);
}

static inline uint16_t
get_time_hi_version(uuidobject *self)
{
    return (uint16_t)((self->bytes[6] << 8) | self->bytes[7]);
}

static inline uint8_t
get_clock_seq_hi_variant(uuidobject *self)
{
    return self->bytes[8];
}

static inline uint8_t
get_clock_seq_low(uuidobject *self)
{
    return self->bytes[9];
}

static inline uint64_t
get_node(uuidobject *self)
{
    return ((uint64_t)self->bytes[10] << 40) |
           ((uint64_t)self->bytes[11] << 32) |
           ((uint64_t)self->bytes[12] << 24) |
           ((uint64_t)self->bytes[13] << 16) |
           ((uint64_t)self->bytes[14] << 8) |
           ((uint64_t)self->bytes[15]);
}

static PyObject *
Uuid_get_time_low(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLong(get_time_low(self));
}

static PyObject *
Uuid_get_time_mid(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLong(get_time_mid(self));
}

static PyObject *
Uuid_get_time_hi_version(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLong(get_time_hi_version(self));
}

static PyObject *
Uuid_get_clock_seq_hi_variant(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLong(get_clock_seq_hi_variant(self));
}

static PyObject *
Uuid_get_clock_seq_low(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLong(get_clock_seq_low(self));
}

static PyObject *
Uuid_get_time(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);

    long version = get_version(self);

    if (version == 6) {
        // UUID v6: time_hi (32) | time_mid (16) | ver (4) | time_lo (12) | ... (64)
        uint32_t time_hi = get_time_low(self);
        uint16_t time_mid = get_time_mid(self);
        uint16_t time_lo = (uint16_t)(((self->bytes[6] & 0x0f) << 8) |
                                    self->bytes[7]);

        uint64_t time = ((uint64_t)time_hi << 28) |
                        ((uint64_t)time_mid << 12) |
                        (uint64_t)time_lo;
        return PyLong_FromUnsignedLongLong(time);
    }
    else if (version == 7) {
        // UUID v7: unix_ts_ms (48) | ... (80)
        // First 6 bytes are the 48-bit timestamp
        uint64_t unix_ts_ms = ((uint64_t)self->bytes[0] << 40) |
                             ((uint64_t)self->bytes[1] << 32) |
                             ((uint64_t)self->bytes[2] << 24) |
                             ((uint64_t)self->bytes[3] << 16) |
                             ((uint64_t)self->bytes[4] << 8) |
                             ((uint64_t)self->bytes[5]);
        return PyLong_FromUnsignedLongLong(unix_ts_ms);
    }
    else {
        // UUID v1 and others: time_lo (32) | time_mid (16) | ver (4)
        //                     | time_hi (12) | ... (64)
        uint32_t time_lo = get_time_low(self);
        uint16_t time_mid = get_time_mid(self);
        uint16_t time_hi = (uint16_t)(((self->bytes[6] & 0x0f) << 8) |
                                    self->bytes[7]);

        uint64_t time = ((uint64_t)time_hi << 48) |
                        ((uint64_t)time_mid << 32) |
                        (uint64_t)time_lo;
        return PyLong_FromUnsignedLongLong(time);
    }
}

static PyObject *
Uuid_get_clock_seq(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    // clock_seq_hi_variant (byte 8) & 0x3f, then clock_seq_low (byte 9)
    uint16_t clock_seq = (uint16_t)(((get_clock_seq_hi_variant(self) & 0x3f) << 8) |
                                    get_clock_seq_low(self));
    return PyLong_FromUnsignedLong(clock_seq);
}

static PyObject *
Uuid_get_node(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyLong_FromUnsignedLongLong(get_node(self));
}

static PyObject *
Uuid_get_bytes(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    return PyBytes_FromStringAndSize((const char *)self->bytes, sizeof(self->bytes));
}

static PyObject *
Uuid_get_bytes_le(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    // UUID fields in little-endian order need to be byte-swapped:
    // - time_low (4 bytes) - reversed
    // - time_mid (2 bytes) - reversed
    // - time_hi_version (2 bytes) - reversed
    // - clock_seq and node (8 bytes) - unchanged

    unsigned char bytes_le[UUID_SIZE];

    // Reverse time_low (bytes 0-3)
    bytes_le[0] = self->bytes[3];
    bytes_le[1] = self->bytes[2];
    bytes_le[2] = self->bytes[1];
    bytes_le[3] = self->bytes[0];

    // Reverse time_mid (bytes 4-5)
    bytes_le[4] = self->bytes[5];
    bytes_le[5] = self->bytes[4];

    // Reverse time_hi_version (bytes 6-7)
    bytes_le[6] = self->bytes[7];
    bytes_le[7] = self->bytes[6];

    // Copy clock_seq and node as-is (bytes 8-15)
    memcpy(bytes_le + 8, self->bytes + 8, 8);

    return PyBytes_FromStringAndSize((const char *)bytes_le, sizeof(bytes_le));
}

static PyObject *
Uuid_get_fields(PyObject *op, void *Py_UNUSED(closure))
{
    uuidobject *self = uuidobject_CAST(op);
    uint32_t time_low = get_time_low(self);
    uint16_t time_mid = get_time_mid(self);
    uint16_t time_hi_version = get_time_hi_version(self);
    uint8_t clock_seq_hi_variant = get_clock_seq_hi_variant(self);
    uint8_t clock_seq_low = get_clock_seq_low(self);
    uint64_t node = get_node(self);

    // Build and return the tuple
    return Py_BuildValue(
        "(kHHBBK)",
        (unsigned long)time_low,
        (unsigned short)time_mid,
        (unsigned short)time_hi_version,
        (unsigned char)clock_seq_hi_variant,
        (unsigned char)clock_seq_low,
        (unsigned long long)node
    );
}

static PyObject *
uuid_from_bytes_array(PyTypeObject *type, uint8_t bytes[UUID_SIZE])
{
    uuidobject *self = make_uuid(type);
    if (self == NULL) {
        return NULL;
    }
    memcpy(self->bytes, bytes, sizeof(self->bytes));
    return (PyObject *)self;
}

static PyObject *
Uuid_nb_int(PyObject *self)
{
    return get_int(uuidobject_CAST(self));
}

static PyObject *
Uuid_richcompare(PyObject *self, PyObject *other, int op)
{
    uuid_state *state = get_uuid_state_by_cls(Py_TYPE(self));

    if (!PyObject_TypeCheck(other, state->uuid_type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    uuidobject *uuid_self = uuidobject_CAST(self);
    uuidobject *uuid_other = uuidobject_CAST(other);

    int cmp = memcmp(uuid_self->bytes, uuid_other->bytes, sizeof(uuid_self->bytes));

    int result;
    switch (op) {
        case Py_EQ:
            result = (cmp == 0);
            break;
        case Py_NE:
            result = (cmp != 0);
            break;
        case Py_LT:
            result = (cmp < 0);
            break;
        case Py_LE:
            result = (cmp <= 0);
            break;
        case Py_GT:
            result = (cmp > 0);
            break;
        case Py_GE:
            result = (cmp >= 0);
            break;
        default:
            Py_RETURN_NOTIMPLEMENTED;
    }

    if (result) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
Uuid_str(PyObject *self)
{
    uuidobject *uuid = uuidobject_CAST(self);

    // UUID string format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
    PyObject *result = PyUnicode_New(36, 127);
    if (result == NULL) {
        return NULL;
    }
    char *str = (char *)PyUnicode_1BYTE_DATA(result);

    for (int i = 0; i < 4; i++) {
        byte_to_hex(uuid->bytes[i], &str[i * 2]);
    }
    str[8] = '-';

    for (int i = 4; i < 6; i++) {
        byte_to_hex(uuid->bytes[i], &str[9 + (i - 4) * 2]);
    }
    str[13] = '-';

    for (int i = 6; i < 8; i++) {
        byte_to_hex(uuid->bytes[i], &str[14 + (i - 6) * 2]);
    }
    str[18] = '-';

    for (int i = 8; i < 10; i++) {
        byte_to_hex(uuid->bytes[i], &str[19 + (i - 8) * 2]);
    }
    str[23] = '-';

    for (int i = 10; i < UUID_SIZE; i++) {
        byte_to_hex(uuid->bytes[i], &str[24 + (i - 10) * 2]);
    }

    return result;
}

static PyObject *
Uuid_repr(PyObject *self)
{
    // Get the class name (sadly can't use tp_name -- we don't need the full name)
    PyObject *cls_name = PyObject_GetAttrString((PyObject *)Py_TYPE(self), "__name__");
    if (cls_name == NULL) {
        return NULL;
    }

    PyObject *repr = PyUnicode_FromFormat("%U('%S')", cls_name, self);
    Py_DECREF(cls_name);
    return repr;
}

static int
Uuid_setattr(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(name),
             PyObject *Py_UNUSED(value))
{
    PyErr_SetString(PyExc_TypeError, "UUID objects are immutable");
    return -1;
}

static PyObject *
Uuid_get_urn(PyObject *op, void *Py_UNUSED(closure))
{
    PyObject *str_obj = Uuid_str(op);
    if (str_obj == NULL) {
        return NULL;
    }

    PyObject *urn = PyUnicode_FromFormat("urn:uuid:%U", str_obj);
    Py_DECREF(str_obj);
    return urn;
}

static Py_hash_t
Uuid_hash(PyObject *op)
{
    uuidobject *uuid = uuidobject_CAST(op);
    // UUIDs are very often used in dicts/sets, so it makes
    // sense to cache the computed hash (like we do for str).
    if (uuid->cached_hash == -1) {
        uuid->cached_hash = Py_HashBuffer(uuid->bytes, sizeof(uuid->bytes));
    }
    return uuid->cached_hash;
}


/*[clinic input]
@classmethod
_uuid.UUID._from_int

    value: object
    /

Create a UUID from an integer value. Internal use only.
[clinic start generated code]*/

static PyObject *
_uuid_UUID__from_int_impl(PyTypeObject *type, PyObject *value)
/*[clinic end generated code: output=05af0cfa4805fcae input=3f472ebfd07bbf50]*/
{
    uuid_state *state = get_uuid_state_by_cls(type);

    if (validate_int(state, value) < 0) {
        return NULL;
    }

    uuidobject *self = make_uuid(type);
    if (self == NULL) {
        return NULL;
    }

    if (from_int(self, value, 0) < 0) {
        // We validated before creating an instance, so now we don't need to
        // validate again
        return NULL;
    }

    return (PyObject *)self;
}

static PyGetSetDef Uuid_getset[] = {
    {"int", Uuid_get_int, NULL, "UUID as a 128-bit integer", NULL},
    {"is_safe", Uuid_get_is_safe, NULL, "UUID safety status", NULL},
    {"fields", Uuid_get_fields, NULL, "UUID as a 6-tuple", NULL},
    {"hex", Uuid_get_hex, NULL, "UUID as a 32-character hex string", NULL},
    {"urn", Uuid_get_urn, NULL, "UUID as a URN", NULL},
    {"variant", Uuid_get_variant, NULL, "UUID variant", NULL},
    {"version", Uuid_get_version, NULL, "UUID version", NULL},
    {"time_low", Uuid_get_time_low, NULL, "Time low field (32 bits)", NULL},
    {"time_mid", Uuid_get_time_mid, NULL, "Time mid field (16 bits)", NULL},
    {"bytes", Uuid_get_bytes, NULL, "UUID as a 16-byte string", NULL},
    {"bytes_le", Uuid_get_bytes_le, NULL,
        "UUID as a 16-byte string in little-endian byte order", NULL},
    {"time_hi_version", Uuid_get_time_hi_version, NULL,
        "Time high and version field (16 bits)", NULL},
    {"clock_seq_hi_variant", Uuid_get_clock_seq_hi_variant, NULL,
        "Clock sequence high and variant field (8 bits)", NULL},
    {"clock_seq_low", Uuid_get_clock_seq_low, NULL,
        "Clock sequence low field (8 bits)", NULL},
    {"time", Uuid_get_time, NULL,
        "Time field (60 bits for v1/v6, 48 bits for v7)", NULL},
    {"clock_seq", Uuid_get_clock_seq, NULL, "Clock sequence field (14 bits)", NULL},
    {"node", Uuid_get_node, NULL, "Node field (48 bits)", NULL},
    {NULL}
};

/*[clinic input]
_uuid.UUID.__getstate__

Return the UUID's state for pickling.
[clinic start generated code]*/

static PyObject *
_uuid_UUID___getstate___impl(uuidobject *self)
/*[clinic end generated code: output=f9278a4d28ccac91 input=4b471ae24b705e8e]*/
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

    // Always add 'int' key
    PyObject *int_value = get_int(self);
    if (int_value == NULL) {
        Py_DECREF(dict);
        return NULL;
    }
    if (PyDict_SetItem(dict, &_Py_ID(int), int_value) < 0) {
        Py_DECREF(int_value);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(int_value);

    if (PyDict_SetItem(dict, &_Py_ID(is_safe), self->is_safe) < 0) {
        Py_DECREF(dict);
        return NULL;
    }

    return dict;
}

/*[clinic input]
_uuid.UUID.__setstate__

    state: object
    /

Restore the UUID's state from pickling.

Expects a mapping with 'int' and optionally 'is_safe' keys.
[clinic start generated code]*/

static PyObject *
_uuid_UUID___setstate___impl(uuidobject *self, PyObject *state)
/*[clinic end generated code: output=cdf6bd4a2a680b3f input=fc65bdad0a8d6211]*/
{
    // Get and set the 'int' value
    PyObject *int_value = PyObject_GetItem(state, &_Py_ID(int));
    if (int_value == NULL) {
        return NULL;
    }

    int result = from_int(self, int_value, 1);
    Py_DECREF(int_value);
    if (result < 0) {
        return NULL;
    }

    int has_is_safe = PySequence_Contains(state, &_Py_ID(is_safe));
    if (has_is_safe < 0) {
        return NULL;
    }
    PyObject *is_safe = has_is_safe
        ? PyObject_GetItem(state, &_Py_ID(is_safe))
        : Py_NewRef(Py_None);
    if (is_safe == NULL) {
        return NULL;
    }

    uuid_state *module_state = get_uuid_state_by_cls(Py_TYPE(self));
    PyObject *SafeUUID = get_SafeUUID(module_state);
    if (SafeUUID == NULL) {
        Py_DECREF(is_safe);
        return NULL;
    }
    PyObject *safety = PyObject_CallOneArg(SafeUUID, is_safe);
    Py_DECREF(SafeUUID);
    Py_DECREF(is_safe);
    if (safety == NULL) {
        return NULL;
    }
    Py_XSETREF(self->is_safe, safety);

    Py_RETURN_NONE;
}

static PyObject *
compute_uuid_max(void)
{
    // Compute `(1 << 128) - 1`

    PyObject *shift = PyLong_FromLong(128);
    if (shift == NULL) {
        return NULL;
    }

    PyObject *shifted = PyNumber_Lshift(_PyLong_GetOne(), shift);
    Py_DECREF(shift);
    if (shifted == NULL) {
        return NULL;
    }

    PyObject *result = PyNumber_Subtract(shifted, _PyLong_GetOne());
    Py_DECREF(shifted);
    return result;
}

static PyMethodDef Uuid_methods[] = {
    _UUID_UUID__FROM_INT_METHODDEF
    _UUID_UUID___GETSTATE___METHODDEF
    _UUID_UUID___SETSTATE___METHODDEF
    {NULL, NULL}
};

static PyMemberDef Uuid_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(uuidobject, weakreflist), Py_READONLY},
    {NULL}
};

static PyType_Slot Uuid_slots[] = {
    {Py_tp_alloc, Uuid_alloc},
    {Py_tp_dealloc, Uuid_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_setattro, Uuid_setattr},
    {Py_tp_getset, Uuid_getset},
    {Py_tp_members, Uuid_members},
    {Py_tp_init, _uuid_UUID___init__},
    {Py_tp_doc, (void *)_uuid_UUID___init____doc__},
    {Py_tp_str, Uuid_str},
    {Py_tp_repr, Uuid_repr},
    {Py_tp_hash, Uuid_hash},
    {Py_tp_richcompare, Uuid_richcompare},
    {Py_nb_int, Uuid_nb_int},
    {Py_tp_methods, Uuid_methods},
    {0, NULL},
};


static PyType_Spec Uuid_spec = {
    // We use "uuid.UUID" here and not "_uuid.UUID" to have full
    // compatibility with old pickled UUIDs. There's no workaround
    // if we want both to produce compatible pickles that can be read
    // by older Pythons (using ancient pickle protocol verions) and
    // restore from pickles produced by old Python versions.
    .name = "uuid.UUID",

    .basicsize = sizeof(uuidobject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HEAPTYPE
        | Py_TPFLAGS_IMMUTABLETYPE
    ),
    .slots = Uuid_slots,
};


static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    uuid_state *state = get_uuid_state(mod);
    Py_VISIT(state->uuid_type);
    Py_VISIT(state->safe_uuid);
    Py_VISIT(state->uint128_max);
    Py_VISIT(state->random_func);
    Py_VISIT(state->time_func);
    Py_VISIT(state->node_func);
    Py_VISIT(state->random_size_int);
    return 0;
}

static int
module_clear(PyObject *mod)
{
    uuid_state *state = get_uuid_state(mod);

    Py_CLEAR(state->uuid_type);
    Py_CLEAR(state->safe_uuid);
    Py_CLEAR(state->uint128_max);
    Py_CLEAR(state->random_func);
    Py_CLEAR(state->time_func);
    Py_CLEAR(state->node_func);
    Py_CLEAR(state->random_size_int);

    if (state->freelist != NULL) {
        while (state->freelist != NULL) {
            uuidobject *cur = state->freelist;
            state->freelist = uuidobject_CAST(cur->weakreflist);
            PyObject_Free(cur);
        }
        state->freelist = NULL;
        state->freelist_size = 0;
    }

    return 0;
}

static void
module_free(void *mod)
{
    (void)module_clear((PyObject *)mod);
}


static int
uuid_exec(PyObject *module)
{
    uuid_state *state = get_uuid_state(module);

#define ADD_INT(NAME, VALUE)                                        \
    do {                                                            \
        if (PyModule_AddIntConstant(module, (NAME), (VALUE)) < 0) { \
            return -1;                                              \
        }                                                           \
    } while (0)

    assert(sizeof(uuid_t) == UUID_SIZE);
#if defined(MS_WINDOWS)
    ADD_INT("has_uuid_generate_time_safe", 0);
#elif defined(HAVE_UUID_GENERATE_TIME_SAFE)
    ADD_INT("has_uuid_generate_time_safe", 1);
#else
    ADD_INT("has_uuid_generate_time_safe", 0);
#endif

#if defined(MS_WINDOWS)
    ADD_INT("has_stable_extractable_node", py_windows_has_stable_node());
#elif defined(HAVE_UUID_GENERATE_TIME_SAFE_STABLE_MAC)
    ADD_INT("has_stable_extractable_node", 1);
#else
    ADD_INT("has_stable_extractable_node", 0);
#endif

#undef ADD_INT

    state->uuid_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module,
        &Uuid_spec,
        NULL
    );
    if (state->uuid_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->uuid_type) < 0) {
        return -1;
    }

    state->safe_uuid = NULL;

    state->uint128_max = compute_uuid_max();
    if (state->uint128_max == NULL) {
        return -1;
    }

    state->node_mutex = (PyMutex){0};
    _Py_atomic_store_uint64_relaxed(&state->node_id, NODE_NOT_INITIALIZED);
    state->last_timestamp_v6 = 0;
    state->last_timestamp_v6_init = 0;
    state->last_timestamp_v7 = 0;
    state->last_timestamp_v7_init = 0;
    state->last_counter_v7 = 0;
    state->random_last_pid = uuid_getpid();

    state->time_func = NULL;
    state->node_func = NULL;
    state->random_func = NULL;
    state->random_size_int = PyLong_FromSize_t((Py_ssize_t)RANDOM_BUF_SIZE);
    if (state->random_size_int == NULL) {
        return -1;
    }
    state->random_idx = RANDOM_BUF_SIZE;

    state->freelist = NULL;
    state->freelist_size = 0;

    return 0;
}

static PyMethodDef uuid_methods[] = {
    _UUID_UUID4_METHODDEF
    _UUID_UUID6_METHODDEF
    _UUID_UUID7_METHODDEF
    _UUID__INSTALL_C_HOOKS_METHODDEF
#if defined(HAVE_UUID_UUID_H) || defined(HAVE_UUID_H)
    {"generate_time_safe", py_uuid_generate_time_safe, METH_NOARGS, NULL},
#endif
#if defined(MS_WINDOWS)
    {"UuidCreate", py_UuidCreate, METH_NOARGS, NULL},
#endif
    {NULL, NULL, 0, NULL}
};

static PyModuleDef_Slot uuid_slots[] = {
    {Py_mod_exec, uuid_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef uuidmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_uuid",
    .m_size = sizeof(uuid_state),
    .m_methods = uuid_methods,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_slots = uuid_slots,
    .m_free = module_free,
};

PyMODINIT_FUNC
PyInit__uuid(void)
{
    return PyModuleDef_Init(&uuidmodule);
}

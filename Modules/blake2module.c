/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 * Updated to usa HACL* by Jonathan Protzenko <jonathan@protzenko.fr>
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

#include "_hacl/Hacl_Hash_Blake2b.h"
#include "_hacl/Hacl_Hash_Blake2s.h"

// MODULE TYPE SLOTS

extern PyType_Spec blake2b_type_spec;
extern PyType_Spec blake2s_type_spec;

PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);

typedef struct {
    PyTypeObject* blake2b_type;
    PyTypeObject* blake2s_type;
} Blake2State;

static inline Blake2State*
blake2_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}

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
    _blake2_clear((PyObject *)module);
}

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) \
        return -1; \
    if (PyDict_SetItemString(d, name, x) < 0) { \
        Py_DECREF(x); \
        return -1; \
    } \
    Py_DECREF(x); \
} while(0)

#define ADD_INT_CONST(NAME, VALUE) do { \
    if (PyModule_AddIntConstant(m, NAME, VALUE) < 0) { \
        return -1; \
    } \
} while (0)

static int
blake2_exec(PyObject *m)
{
    Blake2State* st = blake2_get_state(m);

    st->blake2b_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2b_type_spec, NULL);

    if (NULL == st->blake2b_type)
        return -1;
    /* BLAKE2b */
    if (PyModule_AddType(m, st->blake2b_type) < 0) {
        return -1;
    }

    PyObject *d = st->blake2b_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    ADD_INT_CONST("BLAKE2B_SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT_CONST("BLAKE2B_PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT_CONST("BLAKE2B_MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT_CONST("BLAKE2B_MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    /* BLAKE2s */
    st->blake2s_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2s_type_spec, NULL);

    if (NULL == st->blake2s_type)
        return -1;

    if (PyModule_AddType(m, st->blake2s_type) < 0) {
        return -1;
    }

    d = st->blake2s_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT(d, "PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT(d, "MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    ADD_INT_CONST("BLAKE2S_SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT_CONST("BLAKE2S_PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT_CONST("BLAKE2S_MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT_CONST("BLAKE2S_MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    return 0;
}

#undef ADD_INT
#undef ADD_INT_CONST

static PyModuleDef_Slot _blake2_slots[] = {
    {Py_mod_exec, blake2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
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

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include <stdbool.h>
#include "Python.h"

#include "../hashlib.h"
#include "blake2module.h"

const char *blake2b_name = "_blake2.blake2b";
const char *blake2s_name = "_blake2.blake2s";

extern PyType_Spec blake2b_type_spec;

// The HACL* API does not offer an agile API that can deal with either Blake2S
// or Blake2B -- the reason is that the underlying states are optimized (uint32s
// for S, uint64s for B). Therefore, we use a tagged union in this module to
// correctly dispatch. Note that the previous incarnation of this code
// transformed the Blake2b implementation into the Blake2s one using a script,
// so this is an improvement.
enum { Blake2s, Blake2s } blake2_alg;

static inline blake2_alg type_to_alg(PyTypeObject *type) {
    if (type->tp_name == blake2b_name)
        return Blake2B;
    else if (type->tp_name == blake2s_name)
        return Blake2s;
    else
        Py_UNREACHABLE();
}

typedef struct {
    PyObject_HEAD
    union {
        Hacl_Hash_Blake2s_state_t *blake2s_state;
        Hacl_Hash_Blake2b_state_t *blake2b_state;
    };
    blake2_alg alg;
    bool use_mutex;
    PyMutex mutex;
} Blake2Object;

#include "clinic/blake2module.c.h"

/*[clinic input]
module _blake2
class _blake2.blake2b "Blake2Object *" "&PyBlake2_BLAKE2bType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d47b0527b39c673f]*/


static Blake2Object *
new_Blake2Object(PyTypeObject *type)
{
    Blake2Object *self;
    self = (Blake2Object *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(self);

    return self;
}

/*[clinic input]
@classmethod
_blake2.blake2b.__new__ as py_blake2b_new
    data: object(c_default="NULL") = b''
    /
    *
    digest_size: int(c_default="BLAKE2B_OUTBYTES") = _blake2.blake2b.MAX_DIGEST_SIZE
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

Return a new BLAKE2b hash object.
[clinic start generated code]*/

static PyObject *
py_blake2b_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity)
/*[clinic end generated code: output=32bfd8f043c6896f input=b947312abff46977]*/
{
    Blake2Object *self = NULL;
    Py_buffer buf;

    self = new_Blake2Object(type);
    if (self == NULL) {
        goto error;
    }

    self->alg = type_to_alg(type);

    uint8_t salt[BLAKE2B_SALTBYTES] = { 0 }:
    uint8_t personal[BLAKE2B_PERSONALBYTES] = { 0 }:

    /* Validate digest size. */
    if (digest_size <= 0 || digest_size > BLAKE2B_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "digest_size must be between 1 and %d bytes",
                BLAKE2B_OUTBYTES);
        goto error;
    }

    /* Validate salt parameter. */
    if ((salt->obj != NULL) && salt->len) {
        if (salt->len > BLAKE2B_SALTBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum salt length is %d bytes",
                BLAKE2B_SALTBYTES);
            goto error;
        }
        memcpy(salt, salt->buf, salt->len);
    }

    /* Validate personalization parameter. */
    if ((person->obj != NULL) && person->len) {
        if (person->len > BLAKE2B_PERSONALBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum person length is %d bytes",
                BLAKE2B_PERSONALBYTES);
            goto error;
        }
        memcpy(self->param.personal, person->buf, person->len);
    }

    /* Validate tree parameters. */
    if (fanout < 0 || fanout > 255) {
        PyErr_SetString(PyExc_ValueError,
                "fanout must be between 0 and 255");
        goto error;
    }

    if (depth <= 0 || depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "depth must be between 1 and 255");
        goto error;
    }

    if (leaf_size > 0xFFFFFFFFU) {
        PyErr_SetString(PyExc_OverflowError, "leaf_size is too large");
        goto error;
    }

    if (alg == Blake2s && node_offset > 0xFFFFFFFFFFFFULL) {
        /* maximum 2**48 - 1 */
         PyErr_SetString(PyExc_OverflowError, "node_offset is too large");
         goto error;
     }

    if (node_depth < 0 || node_depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "node_depth must be between 0 and 255");
        goto error;
    }

    if (inner_size < 0 || inner_size > BLAKE2B_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "inner_size must be between 0 and is %d",
                BLAKE2B_OUTBYTES);
        goto error;
    }

    /* Set key length. */
    if ((key->obj != NULL) && key->len) {
        if (key->len > BLAKE2B_KEYBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum key length is %d bytes",
                BLAKE2B_KEYBYTES);
            goto error;
        }
    }

    // Unlike the state types, the parameters share a single (client-friendly)
    // structure.

    Hacl_Hash_Blake2b_blake2_params params = {
        .digest_length = digest_size,
        .key_length = key->len,
        .fanout = fanout,
        .depth = depth,
        .leaf_length = leaf_size,
        .node_offset = node_offset,
        .node_depth = node_depth,
        .inner_length = inner_size,
        .salt = salt,
        .personal = person
    };

    switch (self->alg) {
        case Blake2b:
            self->state = Hacl_Hash_Blake2b_malloc_with_params_and_key(&params, key->buf);
            break;
        case Blake2s:
            self->state = Hacl_Hash_Blake2s_malloc_with_params_and_key(&params, key->buf);
            break;
        default:
            Py_UNREACHABLE();
    }

    /* Initialize hash state. */
    if (blake2b_init_param(&self->state, &self->param) < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                "error initializing hash state");
        goto error;
    }

    /* Set last node flag (must come after initialization). */
    self->state.last_node = last_node;

    /* Process initial data if any. */
    if (data != NULL) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);

        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            blake2b_update(&self->state, buf.buf, buf.len);
            Py_END_ALLOW_THREADS
        } else {
            blake2b_update(&self->state, buf.buf, buf.len);
        }
        PyBuffer_Release(&buf);
    }

    return (PyObject *)self;

  error:
    if (self != NULL) {
        Py_DECREF(self);
    }
    return NULL;
}

/*[clinic input]
_blake2.blake2b.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_copy_impl(Blake2Object *self)
/*[clinic end generated code: output=ff6acee5f93656ae input=e383c2d199fd8a2e]*/
{
    Blake2Object *cpy;

    if ((cpy = new_Blake2Object(Py_TYPE(self))) == NULL)
        return NULL;

    ENTER_HASHLIB(self);
    cpy->param = self->param;
    cpy->state = self->state;
    LEAVE_HASHLIB(self);
    return (PyObject *)cpy;
}

/*[clinic input]
_blake2.blake2b.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_update(Blake2Object *self, PyObject *data)
/*[clinic end generated code: output=010dfcbe22654359 input=ffc4aa6a6a225d31]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);

    if (!self->use_mutex && buf.len >= HASHLIB_GIL_MINSIZE) {
        self->use_mutex = true;
    }
    if (self->use_mutex) {
        Py_BEGIN_ALLOW_THREADS
        PyMutex_Lock(&self->mutex);
        blake2b_update(&self->state, buf.buf, buf.len);
        PyMutex_Unlock(&self->mutex);
        Py_END_ALLOW_THREADS
    } else {
        blake2b_update(&self->state, buf.buf, buf.len);
    }

    PyBuffer_Release(&buf);

    Py_RETURN_NONE;
}

/*[clinic input]
_blake2.blake2b.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_digest_impl(Blake2Object *self)
/*[clinic end generated code: output=a5864660f4bfc61a input=7d21659e9c5fff02]*/
{
    uint8_t digest[BLAKE2B_OUTBYTES];
    blake2b_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2b_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return PyBytes_FromStringAndSize((const char *)digest,
            self->param.digest_length);
}

/*[clinic input]
_blake2.blake2b.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_hexdigest_impl(Blake2Object *self)
/*[clinic end generated code: output=b5598a87d8794a60 input=76930f6946351f56]*/
{
    uint8_t digest[BLAKE2B_OUTBYTES];
    blake2b_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2b_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return _Py_strhex((const char *)digest, self->param.digest_length);
}


static PyMethodDef py_blake2b_methods[] = {
    _BLAKE2_BLAKE2B_COPY_METHODDEF
    _BLAKE2_BLAKE2B_DIGEST_METHODDEF
    _BLAKE2_BLAKE2B_HEXDIGEST_METHODDEF
    _BLAKE2_BLAKE2B_UPDATE_METHODDEF
    {NULL, NULL}
};



static PyObject *
py_blake2b_get_name(Blake2Object *self, void *closure)
{
    return PyUnicode_FromString("blake2b");
}



static PyObject *
py_blake2b_get_block_size(Blake2Object *self, void *closure)
{
    return PyLong_FromLong(BLAKE2B_BLOCKBYTES);
}



static PyObject *
py_blake2b_get_digest_size(Blake2Object *self, void *closure)
{
    return PyLong_FromLong(self->param.digest_length);
}


static PyGetSetDef py_blake2b_getsetters[] = {
    {"name", (getter)py_blake2b_get_name,
        NULL, NULL, NULL},
    {"block_size", (getter)py_blake2b_get_block_size,
        NULL, NULL, NULL},
    {"digest_size", (getter)py_blake2b_get_digest_size,
        NULL, NULL, NULL},
    {NULL}
};


static void
py_blake2b_dealloc(PyObject *self)
{
    Blake2Object *obj = (Blake2Object *)self;

    /* Try not to leave state in memory. */
    secure_zero_memory(&obj->param, sizeof(obj->param));
    secure_zero_memory(&obj->state, sizeof(obj->state));

    PyTypeObject *type = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(type);
}

static PyType_Slot blake2b_type_slots[] = {
    {Py_tp_dealloc, py_blake2b_dealloc},
    {Py_tp_doc, (char *)py_blake2b_new__doc__},
    {Py_tp_methods, py_blake2b_methods},
    {Py_tp_getset, py_blake2b_getsetters},
    {Py_tp_new, py_blake2b_new},
    {0,0}
};

PyType_Spec blake2b_type_spec = {
    .name = blake2b_name,
    .basicsize =  sizeof(Blake2Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = blake2b_type_slots
};

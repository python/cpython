/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * Modified for BLAKE3 by Larry Hastings <larry@hastings.org>
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
#include "pycore_strhex.h"        // _Py_strhex()

#include "../hashlib.h"

#include "impl/blake3.h"


extern PyType_Spec blake3_type_spec;

typedef struct {
    PyObject_HEAD
    blake3_hasher self;
    PyThread_type_lock lock;
} BLAKE3Object;

#include "clinic/blake3_impl.c.h"

/*[clinic input]
module _blake3
class _blake3.blake3 "BLAKE3Object *" "&PyBlake3_Blake3Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=22d8bd47b7e2b6ed]*/


static BLAKE3Object *
new_BLAKE3Object(PyTypeObject *type)
{
    BLAKE3Object *self;
    self = (BLAKE3Object *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->lock = NULL;
    }
    return self;
}

/*[clinic input]
@classmethod
_blake3.blake3.__new__ as py_blake3_new
    data: object(c_default="NULL") = b''
    /
    *
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    derive_key_context: unicode(c_default="NULL") = None
    max_threads: Py_ssize_t = -1
    usedforsecurity: bool = True

Return a new BLAKE3 hash object.
[clinic start generated code]*/

static PyObject *
py_blake3_new_impl(PyTypeObject *type, PyObject *data, Py_buffer *key,
                   PyObject *derive_key_context, Py_ssize_t max_threads,
                   int usedforsecurity)
/*[clinic end generated code: output=5520c7ccfcc0ea6c input=484a9211624e61a2]*/
{
    if ((derive_key_context != NULL) && !PyUnicode_GetLength(derive_key_context)) {
        PyErr_SetString(PyExc_ValueError, "empty derive_key_context string is invalid");
        return NULL;
    }

    if ((key->obj != NULL) && (derive_key_context != NULL)) {
        PyErr_SetString(PyExc_ValueError, "key and derive_key_context can't be used together");
        return NULL;
    }

    if ((max_threads < 1) && (max_threads != -1)) {
        PyErr_SetString(PyExc_ValueError, "invalid value for max_threads");
        return NULL;
    }

    BLAKE3Object *self = new_BLAKE3Object(type);
    if (self == NULL) {
        return PyErr_NoMemory();
    }

    /* Initialize with key */
    if ((key->obj != NULL) && key->len) {
        if (key->len != BLAKE3_KEY_LEN) {
            PyErr_SetString(PyExc_ValueError, "key must be exactly 32 bytes");
            goto error;
        }

        uint8_t key_array[BLAKE3_KEY_LEN];
        memset(key_array, 0, sizeof(key_array));
        memcpy(key_array, key->buf, key->len);
        blake3_hasher_init_keyed(&self->self, key_array);
    } else if (derive_key_context != NULL) {
        Py_ssize_t length;
        const char *utf8_key_context = PyUnicode_AsUTF8AndSize(derive_key_context, &length);
        if (!utf8_key_context) {
            goto error;
        }
        blake3_hasher_init_derive_key_raw(&self->self, utf8_key_context, length);
    } else {
        blake3_hasher_init(&self->self);
    }

    if (data != NULL) {
        Py_buffer data_buffer;
        GET_BUFFER_VIEW_OR_ERROR(data, &data_buffer, goto error);

        if (data_buffer.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            blake3_hasher_update(&self->self, data_buffer.buf, data_buffer.len);
            Py_END_ALLOW_THREADS
        } else {
            blake3_hasher_update(&self->self, data_buffer.buf, data_buffer.len);
        }
        PyBuffer_Release(&data_buffer);
    }

    return (PyObject *)self;

  error:
    if (self != NULL) {
        Py_DECREF(self);
    }
    return NULL;
}

/*[clinic input]
_blake3.blake3.reset

Reset this hash object to its initial state.
[clinic start generated code]*/

static PyObject *
_blake3_blake3_reset_impl(BLAKE3Object *self)
/*[clinic end generated code: output=d03fd37d58b87017 input=ba958463850a68df]*/
{
    ENTER_HASHLIB(self);
    blake3_hasher_reset(&self->self);
    LEAVE_HASHLIB(self);
    Py_RETURN_NONE;
}

/*[clinic input]
_blake3.blake3.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake3_blake3_copy_impl(BLAKE3Object *self)
/*[clinic end generated code: output=c09d0574b2c26cc4 input=b613f4ad414fbe84]*/
{
    BLAKE3Object *copy;

    if ((copy = new_BLAKE3Object(Py_TYPE(self))) == NULL)
        return NULL;

    ENTER_HASHLIB(self);
    copy->self = self->self;
    LEAVE_HASHLIB(self);
    return (PyObject *)copy;
}

/*[clinic input]
_blake3.blake3.update

    data: object

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_blake3_blake3_update_impl(BLAKE3Object *self, PyObject *data)
/*[clinic end generated code: output=0f1070953b310c1b input=51d8a9a205414a95]*/
{
    Py_buffer data_buffer;
    GET_BUFFER_VIEW_OR_ERROUT(data, &data_buffer);

    if ((self->lock == NULL)
        && (data_buffer.len >= HASHLIB_GIL_MINSIZE)) {
        self->lock = PyThread_allocate_lock();
    }

    if (self->lock != NULL) {
       Py_BEGIN_ALLOW_THREADS
       PyThread_acquire_lock(self->lock, 1);
       blake3_hasher_update(&self->self, data_buffer.buf, data_buffer.len);
       PyThread_release_lock(self->lock);
       Py_END_ALLOW_THREADS
    } else {
       blake3_hasher_update(&self->self, data_buffer.buf, data_buffer.len);
    }
    PyBuffer_Release(&data_buffer);

    Py_RETURN_NONE;
}

static PyObject *
_blake3_digest(BLAKE3Object *self, size_t length, size_t seek, int hexdigest)
{
    uint8_t _digest[BLAKE3_OUT_LEN];
    uint8_t *digest;
    int free_digest = 0;

    if (!length) {
        return PyErr_Format(PyExc_ValueError, "digest length cannot be 0");
    }

    if (length > BLAKE3_OUT_LEN) {
        digest = PyMem_Malloc(length);
        if (!digest) {
            return PyErr_NoMemory();
        }
        free_digest = 1;
    } else {
        digest = _digest;
    }

    ENTER_HASHLIB(self);
    if (seek) {
        blake3_hasher_finalize_seek(&self->self, seek, digest, length);
    } else {
        blake3_hasher_finalize(&self->self, digest, length);
    }
    LEAVE_HASHLIB(self);

    PyObject *return_value;
    if (hexdigest) {
        return_value = _Py_strhex((const char *)digest, length);
    } else {
        return_value = PyBytes_FromStringAndSize((const char *)digest, length);
    }

    if (free_digest) {
        PyMem_Free(digest);
    }

    return return_value;
}


/*[clinic input]
_blake3.blake3.digest

    length: size_t(c_default="BLAKE3_OUT_LEN") = _blake3.OUT_LEN
    seek: size_t = 0

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_blake3_blake3_digest_impl(BLAKE3Object *self, size_t length, size_t seek)
/*[clinic end generated code: output=59a6fdc2867733ca input=7500b1ff013fbf54]*/
{
    return _blake3_digest(self, length, seek, 0);
}

/*[clinic input]
_blake3.blake3.hexdigest

    length: size_t(c_default="BLAKE3_OUT_LEN") = _blake3.OUT_LEN
    seek: size_t = 0

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake3_blake3_hexdigest_impl(BLAKE3Object *self, size_t length, size_t seek)
/*[clinic end generated code: output=4367001fe935ee7e input=d85c09aa508f217e]*/
{
    return _blake3_digest(self, length, seek, 1);
}


static PyMethodDef py_blake3_methods[] = {
    _BLAKE3_BLAKE3_UPDATE_METHODDEF
    _BLAKE3_BLAKE3_HEXDIGEST_METHODDEF
    _BLAKE3_BLAKE3_DIGEST_METHODDEF
    _BLAKE3_BLAKE3_COPY_METHODDEF
    _BLAKE3_BLAKE3_RESET_METHODDEF
    {NULL, NULL}
};



static PyObject *
py_blake3_get_name(BLAKE3Object *self, void *closure)
{
    return PyUnicode_FromString("blake3");
}



static PyObject *
py_blake3_get_block_size(BLAKE3Object *self, void *closure)
{
    return PyLong_FromLong(BLAKE3_BLOCK_LEN);
}



static PyObject *
py_blake3_get_digest_size(BLAKE3Object *self, void *closure)
{
    return PyLong_FromLong(BLAKE3_OUT_LEN);
}


static PyGetSetDef py_blake3_getsetters[] = {
    {"name", (getter)py_blake3_get_name,
        NULL, NULL, NULL},
    {"block_size", (getter)py_blake3_get_block_size,
        NULL, NULL, NULL},
    {"digest_size", (getter)py_blake3_get_digest_size,
        NULL, NULL, NULL},
    {NULL}
};

/* prevents compiler optimizing out memset() */
static inline void secure_zero_memory(void *v, size_t n)
{
#if defined(_WIN32) || defined(WIN32)
  SecureZeroMemory(v, n);
#elif defined(__hpux)
  static void *(*const volatile memset_v)(void *, int, size_t) = &memset;
  memset_v(v, 0, n);
#else
// prioritize first the general C11 call
#if defined(HAVE_MEMSET_S)
  memset_s(v, n, 0, n);
#elif defined(HAVE_EXPLICIT_BZERO)
  explicit_bzero(v, n);
#elif defined(HAVE_EXPLICIT_MEMSET)
  explicit_memset(v, 0, n);
#else
  memset(v, 0, n);
  __asm__ __volatile__("" :: "r"(v) : "memory");
#endif
#endif
}


static void
py_blake3_dealloc(PyObject *o)
{
    BLAKE3Object *self = (BLAKE3Object *)o;

    /* Don't leave state in memory. */
    secure_zero_memory(&self->self, sizeof(self->self));
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }

    PyTypeObject *type = Py_TYPE(o);
    PyObject_Free(o);
    Py_DECREF(type);
}

static PyType_Slot blake3_type_slots[] = {
    {Py_tp_dealloc, py_blake3_dealloc},
    {Py_tp_doc, (char *)py_blake3_new__doc__},
    {Py_tp_methods, py_blake3_methods},
    {Py_tp_getset, py_blake3_getsetters},
    {Py_tp_new, py_blake3_new},
    {0,0}
};

PyType_Spec blake3_type_spec = {
    .name = "_blake3.blake3",
    .basicsize =  sizeof(BLAKE3Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = blake3_type_slots
};


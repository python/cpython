/* PyByteArray (bytearray) implementation */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_bytes_methods.h"
#include "pycore_object.h"
#include "bytesobject.h"
#include "pystrhex.h"

/*[clinic input]
class bytearray "PyByteArrayObject *" "&PyByteArray_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=5535b77c37a119e0]*/

char _PyByteArray_empty_string[] = "";

/* end nullbytes support */

/* Helpers */

static int
_getbytevalue(PyObject* arg, int *value)
{
    long face_value;

    if (PyLong_Check(arg)) {
        face_value = PyLong_AsLong(arg);
    } else {
        PyObject *index = PyNumber_Index(arg);
        if (index == NULL) {
            *value = -1;
            return 0;
        }
        face_value = PyLong_AsLong(index);
        Py_DECREF(index);
    }

    if (face_value < 0 || face_value >= 256) {
        /* this includes the OverflowError in case the long is too large */
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        *value = -1;
        return 0;
    }

    *value = face_value;
    return 1;
}

static int
bytearray_getbuffer(PyByteArrayObject *obj, Py_buffer *view, int flags)
{
    void *ptr;
    if (view == NULL) {
        PyErr_SetString(PyExc_BufferError,
            "bytearray_getbuffer: view==NULL argument is obsolete");
        return -1;
    }
    ptr = (void *) PyByteArray_AS_STRING(obj);
    /* cannot fail if view != NULL and readonly == 0 */
    (void)PyBuffer_FillInfo(view, (PyObject*)obj, ptr, Py_SIZE(obj), 0, flags);
    obj->ob_exports++;
    return 0;
}

static void
bytearray_releasebuffer(PyByteArrayObject *obj, Py_buffer *view)
{
    obj->ob_exports--;
}

static int
_canresize(PyByteArrayObject *self)
{
    if (self->ob_exports > 0) {
        PyErr_SetString(PyExc_BufferError,
                "Existing exports of data: object cannot be re-sized");
        return 0;
    }
    return 1;
}

#include "clinic/bytearrayobject.c.h"

/* Direct API functions */

PyObject *
PyByteArray_FromObject(PyObject *input)
{
    return PyObject_CallOneArg((PyObject *)&PyByteArray_Type, input);
}

static PyObject *
_PyByteArray_FromBufferObject(PyObject *obj)
{
    PyObject *result;
    Py_buffer view;

    if (PyObject_GetBuffer(obj, &view, PyBUF_FULL_RO) < 0) {
        return NULL;
    }
    result = PyByteArray_FromStringAndSize(NULL, view.len);
    if (result != NULL &&
        PyBuffer_ToContiguous(PyByteArray_AS_STRING(result),
                              &view, view.len, 'C') < 0)
    {
        Py_CLEAR(result);
    }
    PyBuffer_Release(&view);
    return result;
}

PyObject *
PyByteArray_FromStringAndSize(const char *bytes, Py_ssize_t size)
{
    PyByteArrayObject *new;
    Py_ssize_t alloc;

    if (size < 0) {
        PyErr_SetString(PyExc_SystemError,
            "Negative size passed to PyByteArray_FromStringAndSize");
        return NULL;
    }

    /* Prevent buffer overflow when setting alloc to size+1. */
    if (size == PY_SSIZE_T_MAX) {
        return PyErr_NoMemory();
    }

    new = PyObject_New(PyByteArrayObject, &PyByteArray_Type);
    if (new == NULL)
        return NULL;

    if (size == 0) {
        new->ob_bytes = NULL;
        alloc = 0;
    }
    else {
        alloc = size + 1;
        new->ob_bytes = PyObject_Malloc(alloc);
        if (new->ob_bytes == NULL) {
            Py_DECREF(new);
            return PyErr_NoMemory();
        }
        if (bytes != NULL && size > 0)
            memcpy(new->ob_bytes, bytes, size);
        new->ob_bytes[size] = '\0';  /* Trailing null byte */
    }
    Py_SET_SIZE(new, size);
    new->ob_alloc = alloc;
    new->ob_start = new->ob_bytes;
    new->ob_exports = 0;

    return (PyObject *)new;
}

Py_ssize_t
PyByteArray_Size(PyObject *self)
{
    assert(self != NULL);
    assert(PyByteArray_Check(self));

    return PyByteArray_GET_SIZE(self);
}

char  *
PyByteArray_AsString(PyObject *self)
{
    assert(self != NULL);
    assert(PyByteArray_Check(self));

    return PyByteArray_AS_STRING(self);
}

int
PyByteArray_Resize(PyObject *self, Py_ssize_t requested_size)
{
    void *sval;
    PyByteArrayObject *obj = ((PyByteArrayObject *)self);
    /* All computations are done unsigned to avoid integer overflows
       (see issue #22335). */
    size_t alloc = (size_t) obj->ob_alloc;
    size_t logical_offset = (size_t) (obj->ob_start - obj->ob_bytes);
    size_t size = (size_t) requested_size;

    assert(self != NULL);
    assert(PyByteArray_Check(self));
    assert(logical_offset <= alloc);
    assert(requested_size >= 0);

    if (requested_size == Py_SIZE(self)) {
        return 0;
    }
    if (!_canresize(obj)) {
        return -1;
    }

    if (size + logical_offset + 1 <= alloc) {
        /* Current buffer is large enough to host the requested size,
           decide on a strategy. */
        if (size < alloc / 2) {
            /* Major downsize; resize down to exact size */
            alloc = size + 1;
        }
        else {
            /* Minor downsize; quick exit */
            Py_SET_SIZE(self, size);
            PyByteArray_AS_STRING(self)[size] = '\0'; /* Trailing null */
            return 0;
        }
    }
    else {
        /* Need growing, decide on a strategy */
        if (size <= alloc * 1.125) {
            /* Moderate upsize; overallocate similar to list_resize() */
            alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
        }
        else {
            /* Major upsize; resize up to exact size */
            alloc = size + 1;
        }
    }
    if (alloc > PY_SSIZE_T_MAX) {
        PyErr_NoMemory();
        return -1;
    }

    if (logical_offset > 0) {
        sval = PyObject_Malloc(alloc);
        if (sval == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        memcpy(sval, PyByteArray_AS_STRING(self),
               Py_MIN((size_t)requested_size, (size_t)Py_SIZE(self)));
        PyObject_Free(obj->ob_bytes);
    }
    else {
        sval = PyObject_Realloc(obj->ob_bytes, alloc);
        if (sval == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }

    obj->ob_bytes = obj->ob_start = sval;
    Py_SET_SIZE(self, size);
    obj->ob_alloc = alloc;
    obj->ob_bytes[size] = '\0'; /* Trailing null byte */

    return 0;
}

PyObject *
PyByteArray_Concat(PyObject *a, PyObject *b)
{
    Py_buffer va, vb;
    PyByteArrayObject *result = NULL;

    va.len = -1;
    vb.len = -1;
    if (PyObject_GetBuffer(a, &va, PyBUF_SIMPLE) != 0 ||
        PyObject_GetBuffer(b, &vb, PyBUF_SIMPLE) != 0) {
            PyErr_Format(PyExc_TypeError, "can't concat %.100s to %.100s",
                         Py_TYPE(b)->tp_name, Py_TYPE(a)->tp_name);
            goto done;
    }

    if (va.len > PY_SSIZE_T_MAX - vb.len) {
        PyErr_NoMemory();
        goto done;
    }

    result = (PyByteArrayObject *) \
        PyByteArray_FromStringAndSize(NULL, va.len + vb.len);
    // result->ob_bytes is NULL if result is an empty string:
    // if va.len + vb.len equals zero.
    if (result != NULL && result->ob_bytes != NULL) {
        memcpy(result->ob_bytes, va.buf, va.len);
        memcpy(result->ob_bytes + va.len, vb.buf, vb.len);
    }

  done:
    if (va.len != -1)
        PyBuffer_Release(&va);
    if (vb.len != -1)
        PyBuffer_Release(&vb);
    return (PyObject *)result;
}

/* Functions stuffed into the type object */

static Py_ssize_t
bytearray_length(PyByteArrayObject *self)
{
    return Py_SIZE(self);
}

static PyObject *
bytearray_iconcat(PyByteArrayObject *self, PyObject *other)
{
    Py_ssize_t size;
    Py_buffer vo;

    if (PyObject_GetBuffer(other, &vo, PyBUF_SIMPLE) != 0) {
        PyErr_Format(PyExc_TypeError, "can't concat %.100s to %.100s",
                     Py_TYPE(other)->tp_name, Py_TYPE(self)->tp_name);
        return NULL;
    }

    size = Py_SIZE(self);
    if (size > PY_SSIZE_T_MAX - vo.len) {
        PyBuffer_Release(&vo);
        return PyErr_NoMemory();
    }
    if (PyByteArray_Resize((PyObject *)self, size + vo.len) < 0) {
        PyBuffer_Release(&vo);
        return NULL;
    }
    memcpy(PyByteArray_AS_STRING(self) + size, vo.buf, vo.len);
    PyBuffer_Release(&vo);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
bytearray_repeat(PyByteArrayObject *self, Py_ssize_t count)
{
    PyByteArrayObject *result;
    Py_ssize_t mysize;
    Py_ssize_t size;
    const char *buf;

    if (count < 0)
        count = 0;
    mysize = Py_SIZE(self);
    if (count > 0 && mysize > PY_SSIZE_T_MAX / count)
        return PyErr_NoMemory();
    size = mysize * count;
    result = (PyByteArrayObject *)PyByteArray_FromStringAndSize(NULL, size);
    buf = PyByteArray_AS_STRING(self);
    if (result != NULL && size != 0) {
        if (mysize == 1)
            memset(result->ob_bytes, buf[0], size);
        else {
            Py_ssize_t i;
            for (i = 0; i < count; i++)
                memcpy(result->ob_bytes + i*mysize, buf, mysize);
        }
    }
    return (PyObject *)result;
}

static PyObject *
bytearray_irepeat(PyByteArrayObject *self, Py_ssize_t count)
{
    Py_ssize_t mysize;
    Py_ssize_t size;
    char *buf;

    if (count < 0)
        count = 0;
    mysize = Py_SIZE(self);
    if (count > 0 && mysize > PY_SSIZE_T_MAX / count)
        return PyErr_NoMemory();
    size = mysize * count;
    if (PyByteArray_Resize((PyObject *)self, size) < 0)
        return NULL;

    buf = PyByteArray_AS_STRING(self);
    if (mysize == 1)
        memset(buf, buf[0], size);
    else {
        Py_ssize_t i;
        for (i = 1; i < count; i++)
            memcpy(buf + i*mysize, buf, mysize);
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
bytearray_getitem(PyByteArrayObject *self, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
        return NULL;
    }
    return PyLong_FromLong((unsigned char)(PyByteArray_AS_STRING(self)[i]));
}

static PyObject *
bytearray_subscript(PyByteArrayObject *self, PyObject *index)
{
    if (_PyIndex_Check(index)) {
        Py_ssize_t i = PyNumber_AsSsize_t(index, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return NULL;

        if (i < 0)
            i += PyByteArray_GET_SIZE(self);

        if (i < 0 || i >= Py_SIZE(self)) {
            PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
            return NULL;
        }
        return PyLong_FromLong((unsigned char)(PyByteArray_AS_STRING(self)[i]));
    }
    else if (PySlice_Check(index)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        if (PySlice_Unpack(index, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(PyByteArray_GET_SIZE(self),
                                            &start, &stop, step);

        if (slicelength <= 0)
            return PyByteArray_FromStringAndSize("", 0);
        else if (step == 1) {
            return PyByteArray_FromStringAndSize(
                PyByteArray_AS_STRING(self) + start, slicelength);
        }
        else {
            char *source_buf = PyByteArray_AS_STRING(self);
            char *result_buf;
            PyObject *result;

            result = PyByteArray_FromStringAndSize(NULL, slicelength);
            if (result == NULL)
                return NULL;

            result_buf = PyByteArray_AS_STRING(result);
            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                     result_buf[i] = source_buf[cur];
            }
            return result;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "bytearray indices must be integers or slices, not %.200s",
                     Py_TYPE(index)->tp_name);
        return NULL;
    }
}

static int
bytearray_setslice_linear(PyByteArrayObject *self,
                          Py_ssize_t lo, Py_ssize_t hi,
                          char *bytes, Py_ssize_t bytes_len)
{
    Py_ssize_t avail = hi - lo;
    char *buf = PyByteArray_AS_STRING(self);
    Py_ssize_t growth = bytes_len - avail;
    int res = 0;
    assert(avail >= 0);

    if (growth < 0) {
        if (!_canresize(self))
            return -1;

        if (lo == 0) {
            /* Shrink the buffer by advancing its logical start */
            self->ob_start -= growth;
            /*
              0   lo               hi             old_size
              |   |<----avail----->|<-----tail------>|
              |      |<-bytes_len->|<-----tail------>|
              0    new_lo         new_hi          new_size
            */
        }
        else {
            /*
              0   lo               hi               old_size
              |   |<----avail----->|<-----tomove------>|
              |   |<-bytes_len->|<-----tomove------>|
              0   lo         new_hi              new_size
            */
            memmove(buf + lo + bytes_len, buf + hi,
                    Py_SIZE(self) - hi);
        }
        if (PyByteArray_Resize((PyObject *)self,
                               Py_SIZE(self) + growth) < 0) {
            /* Issue #19578: Handling the memory allocation failure here is
               tricky here because the bytearray object has already been
               modified. Depending on growth and lo, the behaviour is
               different.

               If growth < 0 and lo != 0, the operation is completed, but a
               MemoryError is still raised and the memory block is not
               shrunk. Otherwise, the bytearray is restored in its previous
               state and a MemoryError is raised. */
            if (lo == 0) {
                self->ob_start += growth;
                return -1;
            }
            /* memmove() removed bytes, the bytearray object cannot be
               restored in its previous state. */
            Py_SET_SIZE(self, Py_SIZE(self) + growth);
            res = -1;
        }
        buf = PyByteArray_AS_STRING(self);
    }
    else if (growth > 0) {
        if (Py_SIZE(self) > (Py_ssize_t)PY_SSIZE_T_MAX - growth) {
            PyErr_NoMemory();
            return -1;
        }

        if (PyByteArray_Resize((PyObject *)self,
                               Py_SIZE(self) + growth) < 0) {
            return -1;
        }
        buf = PyByteArray_AS_STRING(self);
        /* Make the place for the additional bytes */
        /*
          0   lo        hi               old_size
          |   |<-avail->|<-----tomove------>|
          |   |<---bytes_len-->|<-----tomove------>|
          0   lo            new_hi              new_size
         */
        memmove(buf + lo + bytes_len, buf + hi,
                Py_SIZE(self) - lo - bytes_len);
    }

    if (bytes_len > 0)
        memcpy(buf + lo, bytes, bytes_len);
    return res;
}

static int
bytearray_setslice(PyByteArrayObject *self, Py_ssize_t lo, Py_ssize_t hi,
               PyObject *values)
{
    Py_ssize_t needed;
    void *bytes;
    Py_buffer vbytes;
    int res = 0;

    vbytes.len = -1;
    if (values == (PyObject *)self) {
        /* Make a copy and call this function recursively */
        int err;
        values = PyByteArray_FromStringAndSize(PyByteArray_AS_STRING(values),
                                               PyByteArray_GET_SIZE(values));
        if (values == NULL)
            return -1;
        err = bytearray_setslice(self, lo, hi, values);
        Py_DECREF(values);
        return err;
    }
    if (values == NULL) {
        /* del b[lo:hi] */
        bytes = NULL;
        needed = 0;
    }
    else {
        if (PyObject_GetBuffer(values, &vbytes, PyBUF_SIMPLE) != 0) {
            PyErr_Format(PyExc_TypeError,
                         "can't set bytearray slice from %.100s",
                         Py_TYPE(values)->tp_name);
            return -1;
        }
        needed = vbytes.len;
        bytes = vbytes.buf;
    }

    if (lo < 0)
        lo = 0;
    if (hi < lo)
        hi = lo;
    if (hi > Py_SIZE(self))
        hi = Py_SIZE(self);

    res = bytearray_setslice_linear(self, lo, hi, bytes, needed);
    if (vbytes.len != -1)
        PyBuffer_Release(&vbytes);
    return res;
}

static int
bytearray_setitem(PyByteArrayObject *self, Py_ssize_t i, PyObject *value)
{
    int ival;

    if (i < 0)
        i += Py_SIZE(self);

    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
        return -1;
    }

    if (value == NULL)
        return bytearray_setslice(self, i, i+1, NULL);

    if (!_getbytevalue(value, &ival))
        return -1;

    PyByteArray_AS_STRING(self)[i] = ival;
    return 0;
}

static int
bytearray_ass_subscript(PyByteArrayObject *self, PyObject *index, PyObject *values)
{
    Py_ssize_t start, stop, step, slicelen, needed;
    char *buf, *bytes;
    buf = PyByteArray_AS_STRING(self);

    if (_PyIndex_Check(index)) {
        Py_ssize_t i = PyNumber_AsSsize_t(index, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return -1;

        if (i < 0)
            i += PyByteArray_GET_SIZE(self);

        if (i < 0 || i >= Py_SIZE(self)) {
            PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
            return -1;
        }

        if (values == NULL) {
            /* Fall through to slice assignment */
            start = i;
            stop = i + 1;
            step = 1;
            slicelen = 1;
        }
        else {
            int ival;
            if (!_getbytevalue(values, &ival))
                return -1;
            buf[i] = (char)ival;
            return 0;
        }
    }
    else if (PySlice_Check(index)) {
        if (PySlice_Unpack(index, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelen = PySlice_AdjustIndices(PyByteArray_GET_SIZE(self), &start,
                                         &stop, step);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "bytearray indices must be integers or slices, not %.200s",
                      Py_TYPE(index)->tp_name);
        return -1;
    }

    if (values == NULL) {
        bytes = NULL;
        needed = 0;
    }
    else if (values == (PyObject *)self || !PyByteArray_Check(values)) {
        int err;
        if (PyNumber_Check(values) || PyUnicode_Check(values)) {
            PyErr_SetString(PyExc_TypeError,
                            "can assign only bytes, buffers, or iterables "
                            "of ints in range(0, 256)");
            return -1;
        }
        /* Make a copy and call this function recursively */
        values = PyByteArray_FromObject(values);
        if (values == NULL)
            return -1;
        err = bytearray_ass_subscript(self, index, values);
        Py_DECREF(values);
        return err;
    }
    else {
        assert(PyByteArray_Check(values));
        bytes = PyByteArray_AS_STRING(values);
        needed = Py_SIZE(values);
    }
    /* Make sure b[5:2] = ... inserts before 5, not before 2. */
    if ((step < 0 && start < stop) ||
        (step > 0 && start > stop))
        stop = start;
    if (step == 1) {
        return bytearray_setslice_linear(self, start, stop, bytes, needed);
    }
    else {
        if (needed == 0) {
            /* Delete slice */
            size_t cur;
            Py_ssize_t i;

            if (!_canresize(self))
                return -1;

            if (slicelen == 0)
                /* Nothing to do here. */
                return 0;

            if (step < 0) {
                stop = start + 1;
                start = stop + step * (slicelen - 1) - 1;
                step = -step;
            }
            for (cur = start, i = 0;
                 i < slicelen; cur += step, i++) {
                Py_ssize_t lim = step - 1;

                if (cur + step >= (size_t)PyByteArray_GET_SIZE(self))
                    lim = PyByteArray_GET_SIZE(self) - cur - 1;

                memmove(buf + cur - i,
                        buf + cur + 1, lim);
            }
            /* Move the tail of the bytes, in one chunk */
            cur = start + (size_t)slicelen*step;
            if (cur < (size_t)PyByteArray_GET_SIZE(self)) {
                memmove(buf + cur - slicelen,
                        buf + cur,
                        PyByteArray_GET_SIZE(self) - cur);
            }
            if (PyByteArray_Resize((PyObject *)self,
                               PyByteArray_GET_SIZE(self) - slicelen) < 0)
                return -1;

            return 0;
        }
        else {
            /* Assign slice */
            Py_ssize_t i;
            size_t cur;

            if (needed != slicelen) {
                PyErr_Format(PyExc_ValueError,
                             "attempt to assign bytes of size %zd "
                             "to extended slice of size %zd",
                             needed, slicelen);
                return -1;
            }
            for (cur = start, i = 0; i < slicelen; cur += step, i++)
                buf[cur] = bytes[i];
            return 0;
        }
    }
}

static int
bytearray_init(PyByteArrayObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"source", "encoding", "errors", 0};
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;
    Py_ssize_t count;
    PyObject *it;
    PyObject *(*iternext)(PyObject *);

    if (Py_SIZE(self) != 0) {
        /* Empty previous contents (yes, do this first of all!) */
        if (PyByteArray_Resize((PyObject *)self, 0) < 0)
            return -1;
    }

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:bytearray", kwlist,
                                     &arg, &encoding, &errors))
        return -1;

    /* Make a quick exit if no first argument */
    if (arg == NULL) {
        if (encoding != NULL || errors != NULL) {
            PyErr_SetString(PyExc_TypeError,
                            encoding != NULL ?
                            "encoding without a string argument" :
                            "errors without a string argument");
            return -1;
        }
        return 0;
    }

    if (PyUnicode_Check(arg)) {
        /* Encode via the codec registry */
        PyObject *encoded, *new;
        if (encoding == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "string argument without an encoding");
            return -1;
        }
        encoded = PyUnicode_AsEncodedString(arg, encoding, errors);
        if (encoded == NULL)
            return -1;
        assert(PyBytes_Check(encoded));
        new = bytearray_iconcat(self, encoded);
        Py_DECREF(encoded);
        if (new == NULL)
            return -1;
        Py_DECREF(new);
        return 0;
    }

    /* If it's not unicode, there can't be encoding or errors */
    if (encoding != NULL || errors != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        encoding != NULL ?
                        "encoding without a string argument" :
                        "errors without a string argument");
        return -1;
    }

    /* Is it an int? */
    if (_PyIndex_Check(arg)) {
        count = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
        if (count == -1 && PyErr_Occurred()) {
            if (!PyErr_ExceptionMatches(PyExc_TypeError))
                return -1;
            PyErr_Clear();  /* fall through */
        }
        else {
            if (count < 0) {
                PyErr_SetString(PyExc_ValueError, "negative count");
                return -1;
            }
            if (count > 0) {
                if (PyByteArray_Resize((PyObject *)self, count))
                    return -1;
                memset(PyByteArray_AS_STRING(self), 0, count);
            }
            return 0;
        }
    }

    /* Use the buffer API */
    if (PyObject_CheckBuffer(arg)) {
        Py_ssize_t size;
        Py_buffer view;
        if (PyObject_GetBuffer(arg, &view, PyBUF_FULL_RO) < 0)
            return -1;
        size = view.len;
        if (PyByteArray_Resize((PyObject *)self, size) < 0) goto fail;
        if (PyBuffer_ToContiguous(PyByteArray_AS_STRING(self),
            &view, size, 'C') < 0)
            goto fail;
        PyBuffer_Release(&view);
        return 0;
    fail:
        PyBuffer_Release(&view);
        return -1;
    }

    /* XXX Optimize this if the arguments is a list, tuple */

    /* Get the iterator */
    it = PyObject_GetIter(arg);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Format(PyExc_TypeError,
                         "cannot convert '%.200s' object to bytearray",
                         Py_TYPE(arg)->tp_name);
        }
        return -1;
    }
    iternext = *Py_TYPE(it)->tp_iternext;

    /* Run the iterator to exhaustion */
    for (;;) {
        PyObject *item;
        int rc, value;

        /* Get the next item */
        item = iternext(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                if (!PyErr_ExceptionMatches(PyExc_StopIteration))
                    goto error;
                PyErr_Clear();
            }
            break;
        }

        /* Interpret it as an int (__index__) */
        rc = _getbytevalue(item, &value);
        Py_DECREF(item);
        if (!rc)
            goto error;

        /* Append the byte */
        if (Py_SIZE(self) + 1 < self->ob_alloc) {
            Py_SET_SIZE(self, Py_SIZE(self) + 1);
            PyByteArray_AS_STRING(self)[Py_SIZE(self)] = '\0';
        }
        else if (PyByteArray_Resize((PyObject *)self, Py_SIZE(self)+1) < 0)
            goto error;
        PyByteArray_AS_STRING(self)[Py_SIZE(self)-1] = value;
    }

    /* Clean up and return success */
    Py_DECREF(it);
    return 0;

 error:
    /* Error handling when it != NULL */
    Py_DECREF(it);
    return -1;
}

/* Mostly copied from string_repr, but without the
   "smart quote" functionality. */
static PyObject *
bytearray_repr(PyByteArrayObject *self)
{
    const char *className = _PyType_Name(Py_TYPE(self));
    const char *quote_prefix = "(b";
    const char *quote_postfix = ")";
    Py_ssize_t length = Py_SIZE(self);
    /* 6 == strlen(quote_prefix) + 2 + strlen(quote_postfix) + 1 */
    Py_ssize_t newsize;
    PyObject *v;
    Py_ssize_t i;
    char *bytes;
    char c;
    char *p;
    int quote;
    char *test, *start;
    char *buffer;

    newsize = strlen(className);
    if (length > (PY_SSIZE_T_MAX - 6 - newsize) / 4) {
        PyErr_SetString(PyExc_OverflowError,
            "bytearray object is too large to make repr");
        return NULL;
    }

    newsize += 6 + length * 4;
    buffer = PyObject_Malloc(newsize);
    if (buffer == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    /* Figure out which quote to use; single is preferred */
    quote = '\'';
    start = PyByteArray_AS_STRING(self);
    for (test = start; test < start+length; ++test) {
        if (*test == '"') {
            quote = '\''; /* back to single */
            break;
        }
        else if (*test == '\'')
            quote = '"';
    }

    p = buffer;
    while (*className)
        *p++ = *className++;
    while (*quote_prefix)
        *p++ = *quote_prefix++;
    *p++ = quote;

    bytes = PyByteArray_AS_STRING(self);
    for (i = 0; i < length; i++) {
        /* There's at least enough room for a hex escape
           and a closing quote. */
        assert(newsize - (p - buffer) >= 5);
        c = bytes[i];
        if (c == '\'' || c == '\\')
            *p++ = '\\', *p++ = c;
        else if (c == '\t')
            *p++ = '\\', *p++ = 't';
        else if (c == '\n')
            *p++ = '\\', *p++ = 'n';
        else if (c == '\r')
            *p++ = '\\', *p++ = 'r';
        else if (c == 0)
            *p++ = '\\', *p++ = 'x', *p++ = '0', *p++ = '0';
        else if (c < ' ' || c >= 0x7f) {
            *p++ = '\\';
            *p++ = 'x';
            *p++ = Py_hexdigits[(c & 0xf0) >> 4];
            *p++ = Py_hexdigits[c & 0xf];
        }
        else
            *p++ = c;
    }
    assert(newsize - (p - buffer) >= 1);
    *p++ = quote;
    while (*quote_postfix) {
       *p++ = *quote_postfix++;
    }

    v = PyUnicode_FromStringAndSize(buffer, p - buffer);
    PyObject_Free(buffer);
    return v;
}

static PyObject *
bytearray_str(PyObject *op)
{
    if (_Py_GetConfig()->bytes_warning) {
        if (PyErr_WarnEx(PyExc_BytesWarning,
                         "str() on a bytearray instance", 1)) {
                return NULL;
        }
    }
    return bytearray_repr((PyByteArrayObject*)op);
}

static PyObject *
bytearray_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_ssize_t self_size, other_size;
    Py_buffer self_bytes, other_bytes;
    int cmp, rc;

    /* Bytes can be compared to anything that supports the (binary)
       buffer API.  Except that a comparison with Unicode is always an
       error, even if the comparison is for equality. */
    rc = PyObject_IsInstance(self, (PyObject*)&PyUnicode_Type);
    if (!rc)
        rc = PyObject_IsInstance(other, (PyObject*)&PyUnicode_Type);
    if (rc < 0)
        return NULL;
    if (rc) {
        if (_Py_GetConfig()->bytes_warning && (op == Py_EQ || op == Py_NE)) {
            if (PyErr_WarnEx(PyExc_BytesWarning,
                            "Comparison between bytearray and string", 1))
                return NULL;
        }

        Py_RETURN_NOTIMPLEMENTED;
    }

    if (PyObject_GetBuffer(self, &self_bytes, PyBUF_SIMPLE) != 0) {
        PyErr_Clear();
        Py_RETURN_NOTIMPLEMENTED;
    }
    self_size = self_bytes.len;

    if (PyObject_GetBuffer(other, &other_bytes, PyBUF_SIMPLE) != 0) {
        PyErr_Clear();
        PyBuffer_Release(&self_bytes);
        Py_RETURN_NOTIMPLEMENTED;
    }
    other_size = other_bytes.len;

    if (self_size != other_size && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the objects differ */
        PyBuffer_Release(&self_bytes);
        PyBuffer_Release(&other_bytes);
        return PyBool_FromLong((op == Py_NE));
    }
    else {
        cmp = memcmp(self_bytes.buf, other_bytes.buf,
                     Py_MIN(self_size, other_size));
        /* In ISO C, memcmp() guarantees to use unsigned bytes! */

        PyBuffer_Release(&self_bytes);
        PyBuffer_Release(&other_bytes);

        if (cmp != 0) {
            Py_RETURN_RICHCOMPARE(cmp, 0, op);
        }

        Py_RETURN_RICHCOMPARE(self_size, other_size, op);
    }

}

static void
bytearray_dealloc(PyByteArrayObject *self)
{
    if (self->ob_exports > 0) {
        PyErr_SetString(PyExc_SystemError,
                        "deallocated bytearray object has exported buffers");
        PyErr_Print();
    }
    if (self->ob_bytes != 0) {
        PyObject_Free(self->ob_bytes);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* -------------------------------------------------------------------- */
/* Methods */

#define FASTSEARCH fastsearch
#define STRINGLIB(F) stringlib_##F
#define STRINGLIB_CHAR char
#define STRINGLIB_SIZEOF_CHAR 1
#define STRINGLIB_LEN PyByteArray_GET_SIZE
#define STRINGLIB_STR PyByteArray_AS_STRING
#define STRINGLIB_NEW PyByteArray_FromStringAndSize
#define STRINGLIB_ISSPACE Py_ISSPACE
#define STRINGLIB_ISLINEBREAK(x) ((x == '\n') || (x == '\r'))
#define STRINGLIB_CHECK_EXACT PyByteArray_CheckExact
#define STRINGLIB_MUTABLE 1

#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/join.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/ctype.h"
#include "stringlib/transmogrify.h"


static PyObject *
bytearray_find(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_find(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

static PyObject *
bytearray_count(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_count(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

/*[clinic input]
bytearray.clear

Remove all items from the bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_clear_impl(PyByteArrayObject *self)
/*[clinic end generated code: output=85c2fe6aede0956c input=ed6edae9de447ac4]*/
{
    if (PyByteArray_Resize((PyObject *)self, 0) < 0)
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
bytearray.copy

Return a copy of B.
[clinic start generated code]*/

static PyObject *
bytearray_copy_impl(PyByteArrayObject *self)
/*[clinic end generated code: output=68cfbcfed484c132 input=6597b0c01bccaa9e]*/
{
    return PyByteArray_FromStringAndSize(PyByteArray_AS_STRING((PyObject *)self),
                                         PyByteArray_GET_SIZE(self));
}

static PyObject *
bytearray_index(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_index(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

static PyObject *
bytearray_rfind(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_rfind(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

static PyObject *
bytearray_rindex(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_rindex(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

static int
bytearray_contains(PyObject *self, PyObject *arg)
{
    return _Py_bytes_contains(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), arg);
}

static PyObject *
bytearray_startswith(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_startswith(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

static PyObject *
bytearray_endswith(PyByteArrayObject *self, PyObject *args)
{
    return _Py_bytes_endswith(PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self), args);
}

/*[clinic input]
bytearray.removeprefix as bytearray_removeprefix

    prefix: Py_buffer
    /

Return a bytearray with the given prefix string removed if present.

If the bytearray starts with the prefix string, return
bytearray[len(prefix):].  Otherwise, return a copy of the original
bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_removeprefix_impl(PyByteArrayObject *self, Py_buffer *prefix)
/*[clinic end generated code: output=6cabc585e7f502e0 input=968aada38aedd262]*/
{
    const char *self_start = PyByteArray_AS_STRING(self);
    Py_ssize_t self_len = PyByteArray_GET_SIZE(self);
    const char *prefix_start = prefix->buf;
    Py_ssize_t prefix_len = prefix->len;

    if (self_len >= prefix_len
        && memcmp(self_start, prefix_start, prefix_len) == 0)
    {
        return PyByteArray_FromStringAndSize(self_start + prefix_len,
                                             self_len - prefix_len);
    }

    return PyByteArray_FromStringAndSize(self_start, self_len);
}

/*[clinic input]
bytearray.removesuffix as bytearray_removesuffix

    suffix: Py_buffer
    /

Return a bytearray with the given suffix string removed if present.

If the bytearray ends with the suffix string and that suffix is not
empty, return bytearray[:-len(suffix)].  Otherwise, return a copy of
the original bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_removesuffix_impl(PyByteArrayObject *self, Py_buffer *suffix)
/*[clinic end generated code: output=2bc8cfb79de793d3 input=c1827e810b2f6b99]*/
{
    const char *self_start = PyByteArray_AS_STRING(self);
    Py_ssize_t self_len = PyByteArray_GET_SIZE(self);
    const char *suffix_start = suffix->buf;
    Py_ssize_t suffix_len = suffix->len;

    if (self_len >= suffix_len
        && memcmp(self_start + self_len - suffix_len,
                  suffix_start, suffix_len) == 0)
    {
        return PyByteArray_FromStringAndSize(self_start,
                                             self_len - suffix_len);
    }

    return PyByteArray_FromStringAndSize(self_start, self_len);
}


/*[clinic input]
bytearray.translate

    table: object
        Translation table, which must be a bytes object of length 256.
    /
    delete as deletechars: object(c_default="NULL") = b''

Return a copy with each character mapped by the given translation table.

All characters occurring in the optional argument delete are removed.
The remaining characters are mapped through the given translation table.
[clinic start generated code]*/

static PyObject *
bytearray_translate_impl(PyByteArrayObject *self, PyObject *table,
                         PyObject *deletechars)
/*[clinic end generated code: output=b6a8f01c2a74e446 input=cfff956d4d127a9b]*/
{
    char *input, *output;
    const char *table_chars;
    Py_ssize_t i, c;
    PyObject *input_obj = (PyObject*)self;
    const char *output_start;
    Py_ssize_t inlen;
    PyObject *result = NULL;
    int trans_table[256];
    Py_buffer vtable, vdel;

    if (table == Py_None) {
        table_chars = NULL;
        table = NULL;
    } else if (PyObject_GetBuffer(table, &vtable, PyBUF_SIMPLE) != 0) {
        return NULL;
    } else {
        if (vtable.len != 256) {
            PyErr_SetString(PyExc_ValueError,
                            "translation table must be 256 characters long");
            PyBuffer_Release(&vtable);
            return NULL;
        }
        table_chars = (const char*)vtable.buf;
    }

    if (deletechars != NULL) {
        if (PyObject_GetBuffer(deletechars, &vdel, PyBUF_SIMPLE) != 0) {
            if (table != NULL)
                PyBuffer_Release(&vtable);
            return NULL;
        }
    }
    else {
        vdel.buf = NULL;
        vdel.len = 0;
    }

    inlen = PyByteArray_GET_SIZE(input_obj);
    result = PyByteArray_FromStringAndSize((char *)NULL, inlen);
    if (result == NULL)
        goto done;
    output_start = output = PyByteArray_AS_STRING(result);
    input = PyByteArray_AS_STRING(input_obj);

    if (vdel.len == 0 && table_chars != NULL) {
        /* If no deletions are required, use faster code */
        for (i = inlen; --i >= 0; ) {
            c = Py_CHARMASK(*input++);
            *output++ = table_chars[c];
        }
        goto done;
    }

    if (table_chars == NULL) {
        for (i = 0; i < 256; i++)
            trans_table[i] = Py_CHARMASK(i);
    } else {
        for (i = 0; i < 256; i++)
            trans_table[i] = Py_CHARMASK(table_chars[i]);
    }

    for (i = 0; i < vdel.len; i++)
        trans_table[(int) Py_CHARMASK( ((unsigned char*)vdel.buf)[i] )] = -1;

    for (i = inlen; --i >= 0; ) {
        c = Py_CHARMASK(*input++);
        if (trans_table[c] != -1)
            *output++ = (char)trans_table[c];
    }
    /* Fix the size of the resulting string */
    if (inlen > 0)
        if (PyByteArray_Resize(result, output - output_start) < 0) {
            Py_CLEAR(result);
            goto done;
        }

done:
    if (table != NULL)
        PyBuffer_Release(&vtable);
    if (deletechars != NULL)
        PyBuffer_Release(&vdel);
    return result;
}


/*[clinic input]

@staticmethod
bytearray.maketrans

    frm: Py_buffer
    to: Py_buffer
    /

Return a translation table useable for the bytes or bytearray translate method.

The returned table will be one where each byte in frm is mapped to the byte at
the same position in to.

The bytes objects frm and to must be of the same length.
[clinic start generated code]*/

static PyObject *
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to)
/*[clinic end generated code: output=1df267d99f56b15e input=5925a81d2fbbf151]*/
{
    return _Py_bytes_maketrans(frm, to);
}


/*[clinic input]
bytearray.replace

    old: Py_buffer
    new: Py_buffer
    count: Py_ssize_t = -1
        Maximum number of occurrences to replace.
        -1 (the default value) means replace all occurrences.
    /

Return a copy with all occurrences of substring old replaced by new.

If the optional argument count is given, only the first count occurrences are
replaced.
[clinic start generated code]*/

static PyObject *
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count)
/*[clinic end generated code: output=d39884c4dc59412a input=aa379d988637c7fb]*/
{
    return stringlib_replace((PyObject *)self,
                             (const char *)old->buf, old->len,
                             (const char *)new->buf, new->len, count);
}

/*[clinic input]
bytearray.split

    sep: object = None
        The delimiter according which to split the bytearray.
        None (the default value) means split on ASCII whitespace characters
        (space, tab, return, newline, formfeed, vertical tab).
    maxsplit: Py_ssize_t = -1
        Maximum number of splits to do.
        -1 (the default value) means no limit.

Return a list of the sections in the bytearray, using sep as the delimiter.
[clinic start generated code]*/

static PyObject *
bytearray_split_impl(PyByteArrayObject *self, PyObject *sep,
                     Py_ssize_t maxsplit)
/*[clinic end generated code: output=833e2cf385d9a04d input=24f82669f41bf523]*/
{
    Py_ssize_t len = PyByteArray_GET_SIZE(self), n;
    const char *s = PyByteArray_AS_STRING(self), *sub;
    PyObject *list;
    Py_buffer vsub;

    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;

    if (sep == Py_None)
        return stringlib_split_whitespace((PyObject*) self, s, len, maxsplit);

    if (PyObject_GetBuffer(sep, &vsub, PyBUF_SIMPLE) != 0)
        return NULL;
    sub = vsub.buf;
    n = vsub.len;

    list = stringlib_split(
        (PyObject*) self, s, len, sub, n, maxsplit
        );
    PyBuffer_Release(&vsub);
    return list;
}

/*[clinic input]
bytearray.partition

    sep: object
    /

Partition the bytearray into three parts using the given separator.

This will search for the separator sep in the bytearray. If the separator is
found, returns a 3-tuple containing the part before the separator, the
separator itself, and the part after it as new bytearray objects.

If the separator is not found, returns a 3-tuple containing the copy of the
original bytearray object and two empty bytearray objects.
[clinic start generated code]*/

static PyObject *
bytearray_partition(PyByteArrayObject *self, PyObject *sep)
/*[clinic end generated code: output=45d2525ddd35f957 input=8f644749ee4fc83a]*/
{
    PyObject *bytesep, *result;

    bytesep = _PyByteArray_FromBufferObject(sep);
    if (! bytesep)
        return NULL;

    result = stringlib_partition(
            (PyObject*) self,
            PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self),
            bytesep,
            PyByteArray_AS_STRING(bytesep), PyByteArray_GET_SIZE(bytesep)
            );

    Py_DECREF(bytesep);
    return result;
}

/*[clinic input]
bytearray.rpartition

    sep: object
    /

Partition the bytearray into three parts using the given separator.

This will search for the separator sep in the bytearray, starting at the end.
If the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it as new bytearray
objects.

If the separator is not found, returns a 3-tuple containing two empty bytearray
objects and the copy of the original bytearray object.
[clinic start generated code]*/

static PyObject *
bytearray_rpartition(PyByteArrayObject *self, PyObject *sep)
/*[clinic end generated code: output=440de3c9426115e8 input=7e3df3e6cb8fa0ac]*/
{
    PyObject *bytesep, *result;

    bytesep = _PyByteArray_FromBufferObject(sep);
    if (! bytesep)
        return NULL;

    result = stringlib_rpartition(
            (PyObject*) self,
            PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self),
            bytesep,
            PyByteArray_AS_STRING(bytesep), PyByteArray_GET_SIZE(bytesep)
            );

    Py_DECREF(bytesep);
    return result;
}

/*[clinic input]
bytearray.rsplit = bytearray.split

Return a list of the sections in the bytearray, using sep as the delimiter.

Splitting is done starting at the end of the bytearray and working to the front.
[clinic start generated code]*/

static PyObject *
bytearray_rsplit_impl(PyByteArrayObject *self, PyObject *sep,
                      Py_ssize_t maxsplit)
/*[clinic end generated code: output=a55e0b5a03cb6190 input=a68286e4dd692ffe]*/
{
    Py_ssize_t len = PyByteArray_GET_SIZE(self), n;
    const char *s = PyByteArray_AS_STRING(self), *sub;
    PyObject *list;
    Py_buffer vsub;

    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;

    if (sep == Py_None)
        return stringlib_rsplit_whitespace((PyObject*) self, s, len, maxsplit);

    if (PyObject_GetBuffer(sep, &vsub, PyBUF_SIMPLE) != 0)
        return NULL;
    sub = vsub.buf;
    n = vsub.len;

    list = stringlib_rsplit(
        (PyObject*) self, s, len, sub, n, maxsplit
        );
    PyBuffer_Release(&vsub);
    return list;
}

/*[clinic input]
bytearray.reverse

Reverse the order of the values in B in place.
[clinic start generated code]*/

static PyObject *
bytearray_reverse_impl(PyByteArrayObject *self)
/*[clinic end generated code: output=9f7616f29ab309d3 input=543356319fc78557]*/
{
    char swap, *head, *tail;
    Py_ssize_t i, j, n = Py_SIZE(self);

    j = n / 2;
    head = PyByteArray_AS_STRING(self);
    tail = head + n - 1;
    for (i = 0; i < j; i++) {
        swap = *head;
        *head++ = *tail;
        *tail-- = swap;
    }

    Py_RETURN_NONE;
}


/*[python input]
class bytesvalue_converter(CConverter):
    type = 'int'
    converter = '_getbytevalue'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=29c2e7c26c212812]*/


/*[clinic input]
bytearray.insert

    index: Py_ssize_t
        The index where the value is to be inserted.
    item: bytesvalue
        The item to be inserted.
    /

Insert a single item into the bytearray before the given index.
[clinic start generated code]*/

static PyObject *
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item)
/*[clinic end generated code: output=76c775a70e7b07b7 input=b2b5d07e9de6c070]*/
{
    Py_ssize_t n = Py_SIZE(self);
    char *buf;

    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytearray");
        return NULL;
    }
    if (PyByteArray_Resize((PyObject *)self, n + 1) < 0)
        return NULL;
    buf = PyByteArray_AS_STRING(self);

    if (index < 0) {
        index += n;
        if (index < 0)
            index = 0;
    }
    if (index > n)
        index = n;
    memmove(buf + index + 1, buf + index, n - index);
    buf[index] = item;

    Py_RETURN_NONE;
}

/*[clinic input]
bytearray.append

    item: bytesvalue
        The item to be appended.
    /

Append a single item to the end of the bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_append_impl(PyByteArrayObject *self, int item)
/*[clinic end generated code: output=a154e19ed1886cb6 input=20d6bec3d1340593]*/
{
    Py_ssize_t n = Py_SIZE(self);

    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytearray");
        return NULL;
    }
    if (PyByteArray_Resize((PyObject *)self, n + 1) < 0)
        return NULL;

    PyByteArray_AS_STRING(self)[n] = item;

    Py_RETURN_NONE;
}

/*[clinic input]
bytearray.extend

    iterable_of_ints: object
        The iterable of items to append.
    /

Append all the items from the iterator or sequence to the end of the bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_extend(PyByteArrayObject *self, PyObject *iterable_of_ints)
/*[clinic end generated code: output=98155dbe249170b1 input=c617b3a93249ba28]*/
{
    PyObject *it, *item, *bytearray_obj;
    Py_ssize_t buf_size = 0, len = 0;
    int value;
    char *buf;

    /* bytearray_setslice code only accepts something supporting PEP 3118. */
    if (PyObject_CheckBuffer(iterable_of_ints)) {
        if (bytearray_setslice(self, Py_SIZE(self), Py_SIZE(self), iterable_of_ints) == -1)
            return NULL;

        Py_RETURN_NONE;
    }

    it = PyObject_GetIter(iterable_of_ints);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Format(PyExc_TypeError,
                         "can't extend bytearray with %.100s",
                         Py_TYPE(iterable_of_ints)->tp_name);
        }
        return NULL;
    }

    /* Try to determine the length of the argument. 32 is arbitrary. */
    buf_size = PyObject_LengthHint(iterable_of_ints, 32);
    if (buf_size == -1) {
        Py_DECREF(it);
        return NULL;
    }

    bytearray_obj = PyByteArray_FromStringAndSize(NULL, buf_size);
    if (bytearray_obj == NULL) {
        Py_DECREF(it);
        return NULL;
    }
    buf = PyByteArray_AS_STRING(bytearray_obj);

    while ((item = PyIter_Next(it)) != NULL) {
        if (! _getbytevalue(item, &value)) {
            Py_DECREF(item);
            Py_DECREF(it);
            Py_DECREF(bytearray_obj);
            return NULL;
        }
        buf[len++] = value;
        Py_DECREF(item);

        if (len >= buf_size) {
            Py_ssize_t addition;
            if (len == PY_SSIZE_T_MAX) {
                Py_DECREF(it);
                Py_DECREF(bytearray_obj);
                return PyErr_NoMemory();
            }
            addition = len >> 1;
            if (addition > PY_SSIZE_T_MAX - len - 1)
                buf_size = PY_SSIZE_T_MAX;
            else
                buf_size = len + addition + 1;
            if (PyByteArray_Resize((PyObject *)bytearray_obj, buf_size) < 0) {
                Py_DECREF(it);
                Py_DECREF(bytearray_obj);
                return NULL;
            }
            /* Recompute the `buf' pointer, since the resizing operation may
               have invalidated it. */
            buf = PyByteArray_AS_STRING(bytearray_obj);
        }
    }
    Py_DECREF(it);

    /* Resize down to exact size. */
    if (PyByteArray_Resize((PyObject *)bytearray_obj, len) < 0) {
        Py_DECREF(bytearray_obj);
        return NULL;
    }

    if (bytearray_setslice(self, Py_SIZE(self), Py_SIZE(self), bytearray_obj) == -1) {
        Py_DECREF(bytearray_obj);
        return NULL;
    }
    Py_DECREF(bytearray_obj);

    if (PyErr_Occurred()) {
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
bytearray.pop

    index: Py_ssize_t = -1
        The index from where to remove the item.
        -1 (the default value) means remove the last item.
    /

Remove and return a single item from B.

If no index argument is given, will pop the last item.
[clinic start generated code]*/

static PyObject *
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index)
/*[clinic end generated code: output=e0ccd401f8021da8 input=3591df2d06c0d237]*/
{
    int value;
    Py_ssize_t n = Py_SIZE(self);
    char *buf;

    if (n == 0) {
        PyErr_SetString(PyExc_IndexError,
                        "pop from empty bytearray");
        return NULL;
    }
    if (index < 0)
        index += Py_SIZE(self);
    if (index < 0 || index >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    if (!_canresize(self))
        return NULL;

    buf = PyByteArray_AS_STRING(self);
    value = buf[index];
    memmove(buf + index, buf + index + 1, n - index);
    if (PyByteArray_Resize((PyObject *)self, n - 1) < 0)
        return NULL;

    return PyLong_FromLong((unsigned char)value);
}

/*[clinic input]
bytearray.remove

    value: bytesvalue
        The value to remove.
    /

Remove the first occurrence of a value in the bytearray.
[clinic start generated code]*/

static PyObject *
bytearray_remove_impl(PyByteArrayObject *self, int value)
/*[clinic end generated code: output=d659e37866709c13 input=121831240cd51ddf]*/
{
    Py_ssize_t where, n = Py_SIZE(self);
    char *buf = PyByteArray_AS_STRING(self);

    where = stringlib_find_char(buf, n, value);
    if (where < 0) {
        PyErr_SetString(PyExc_ValueError, "value not found in bytearray");
        return NULL;
    }
    if (!_canresize(self))
        return NULL;

    memmove(buf + where, buf + where + 1, n - where);
    if (PyByteArray_Resize((PyObject *)self, n - 1) < 0)
        return NULL;

    Py_RETURN_NONE;
}

/* XXX These two helpers could be optimized if argsize == 1 */

static Py_ssize_t
lstrip_helper(const char *myptr, Py_ssize_t mysize,
              const void *argptr, Py_ssize_t argsize)
{
    Py_ssize_t i = 0;
    while (i < mysize && memchr(argptr, (unsigned char) myptr[i], argsize))
        i++;
    return i;
}

static Py_ssize_t
rstrip_helper(const char *myptr, Py_ssize_t mysize,
              const void *argptr, Py_ssize_t argsize)
{
    Py_ssize_t i = mysize - 1;
    while (i >= 0 && memchr(argptr, (unsigned char) myptr[i], argsize))
        i--;
    return i + 1;
}

/*[clinic input]
bytearray.strip

    bytes: object = None
    /

Strip leading and trailing bytes contained in the argument.

If the argument is omitted or None, strip leading and trailing ASCII whitespace.
[clinic start generated code]*/

static PyObject *
bytearray_strip_impl(PyByteArrayObject *self, PyObject *bytes)
/*[clinic end generated code: output=760412661a34ad5a input=ef7bb59b09c21d62]*/
{
    Py_ssize_t left, right, mysize, byteslen;
    char *myptr;
    const char *bytesptr;
    Py_buffer vbytes;

    if (bytes == Py_None) {
        bytesptr = "\t\n\r\f\v ";
        byteslen = 6;
    }
    else {
        if (PyObject_GetBuffer(bytes, &vbytes, PyBUF_SIMPLE) != 0)
            return NULL;
        bytesptr = (const char *) vbytes.buf;
        byteslen = vbytes.len;
    }
    myptr = PyByteArray_AS_STRING(self);
    mysize = Py_SIZE(self);
    left = lstrip_helper(myptr, mysize, bytesptr, byteslen);
    if (left == mysize)
        right = left;
    else
        right = rstrip_helper(myptr, mysize, bytesptr, byteslen);
    if (bytes != Py_None)
        PyBuffer_Release(&vbytes);
    return PyByteArray_FromStringAndSize(myptr + left, right - left);
}

/*[clinic input]
bytearray.lstrip

    bytes: object = None
    /

Strip leading bytes contained in the argument.

If the argument is omitted or None, strip leading ASCII whitespace.
[clinic start generated code]*/

static PyObject *
bytearray_lstrip_impl(PyByteArrayObject *self, PyObject *bytes)
/*[clinic end generated code: output=d005c9d0ab909e66 input=80843f975dd7c480]*/
{
    Py_ssize_t left, right, mysize, byteslen;
    char *myptr;
    const char *bytesptr;
    Py_buffer vbytes;

    if (bytes == Py_None) {
        bytesptr = "\t\n\r\f\v ";
        byteslen = 6;
    }
    else {
        if (PyObject_GetBuffer(bytes, &vbytes, PyBUF_SIMPLE) != 0)
            return NULL;
        bytesptr = (const char *) vbytes.buf;
        byteslen = vbytes.len;
    }
    myptr = PyByteArray_AS_STRING(self);
    mysize = Py_SIZE(self);
    left = lstrip_helper(myptr, mysize, bytesptr, byteslen);
    right = mysize;
    if (bytes != Py_None)
        PyBuffer_Release(&vbytes);
    return PyByteArray_FromStringAndSize(myptr + left, right - left);
}

/*[clinic input]
bytearray.rstrip

    bytes: object = None
    /

Strip trailing bytes contained in the argument.

If the argument is omitted or None, strip trailing ASCII whitespace.
[clinic start generated code]*/

static PyObject *
bytearray_rstrip_impl(PyByteArrayObject *self, PyObject *bytes)
/*[clinic end generated code: output=030e2fbd2f7276bd input=e728b994954cfd91]*/
{
    Py_ssize_t right, mysize, byteslen;
    char *myptr;
    const char *bytesptr;
    Py_buffer vbytes;

    if (bytes == Py_None) {
        bytesptr = "\t\n\r\f\v ";
        byteslen = 6;
    }
    else {
        if (PyObject_GetBuffer(bytes, &vbytes, PyBUF_SIMPLE) != 0)
            return NULL;
        bytesptr = (const char *) vbytes.buf;
        byteslen = vbytes.len;
    }
    myptr = PyByteArray_AS_STRING(self);
    mysize = Py_SIZE(self);
    right = rstrip_helper(myptr, mysize, bytesptr, byteslen);
    if (bytes != Py_None)
        PyBuffer_Release(&vbytes);
    return PyByteArray_FromStringAndSize(myptr, right);
}

/*[clinic input]
bytearray.decode

    encoding: str(c_default="NULL") = 'utf-8'
        The encoding with which to decode the bytearray.
    errors: str(c_default="NULL") = 'strict'
        The error handling scheme to use for the handling of decoding errors.
        The default is 'strict' meaning that decoding errors raise a
        UnicodeDecodeError. Other possible values are 'ignore' and 'replace'
        as well as any other name registered with codecs.register_error that
        can handle UnicodeDecodeErrors.

Decode the bytearray using the codec registered for encoding.
[clinic start generated code]*/

static PyObject *
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors)
/*[clinic end generated code: output=f57d43f4a00b42c5 input=f28d8f903020257b]*/
{
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();
    return PyUnicode_FromEncodedObject((PyObject*)self, encoding, errors);
}

PyDoc_STRVAR(alloc_doc,
"B.__alloc__() -> int\n\
\n\
Return the number of bytes actually allocated.");

static PyObject *
bytearray_alloc(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromSsize_t(self->ob_alloc);
}

/*[clinic input]
bytearray.join

    iterable_of_bytes: object
    /

Concatenate any number of bytes/bytearray objects.

The bytearray whose method is called is inserted in between each pair.

The result is returned as a new bytearray object.
[clinic start generated code]*/

static PyObject *
bytearray_join(PyByteArrayObject *self, PyObject *iterable_of_bytes)
/*[clinic end generated code: output=a8516370bf68ae08 input=aba6b1f9b30fcb8e]*/
{
    return stringlib_bytes_join((PyObject*)self, iterable_of_bytes);
}

/*[clinic input]
bytearray.splitlines

    keepends: bool(accept={int}) = False

Return a list of the lines in the bytearray, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.
[clinic start generated code]*/

static PyObject *
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends)
/*[clinic end generated code: output=4223c94b895f6ad9 input=99a27ad959b9cf6b]*/
{
    return stringlib_splitlines(
        (PyObject*) self, PyByteArray_AS_STRING(self),
        PyByteArray_GET_SIZE(self), keepends
        );
}

/*[clinic input]
@classmethod
bytearray.fromhex

    string: unicode
    /

Create a bytearray object from a string of hexadecimal numbers.

Spaces between two numbers are accepted.
Example: bytearray.fromhex('B9 01EF') -> bytearray(b'\\xb9\\x01\\xef')
[clinic start generated code]*/

static PyObject *
bytearray_fromhex_impl(PyTypeObject *type, PyObject *string)
/*[clinic end generated code: output=8f0f0b6d30fb3ba0 input=f033a16d1fb21f48]*/
{
    PyObject *result = _PyBytes_FromHex(string, type == &PyByteArray_Type);
    if (type != &PyByteArray_Type && result != NULL) {
        Py_SETREF(result, PyObject_CallOneArg((PyObject *)type, result));
    }
    return result;
}

/*[clinic input]
bytearray.hex

    sep: object = NULL
        An optional single character or byte to separate hex bytes.
    bytes_per_sep: int = 1
        How many bytes between separators.  Positive values count from the
        right, negative values count from the left.

Create a str of hexadecimal numbers from a bytearray object.

Example:
>>> value = bytearray([0xb9, 0x01, 0xef])
>>> value.hex()
'b901ef'
>>> value.hex(':')
'b9:01:ef'
>>> value.hex(':', 2)
'b9:01ef'
>>> value.hex(':', -2)
'b901:ef'
[clinic start generated code]*/

static PyObject *
bytearray_hex_impl(PyByteArrayObject *self, PyObject *sep, int bytes_per_sep)
/*[clinic end generated code: output=29c4e5ef72c565a0 input=814c15830ac8c4b5]*/
{
    char* argbuf = PyByteArray_AS_STRING(self);
    Py_ssize_t arglen = PyByteArray_GET_SIZE(self);
    return _Py_strhex_with_sep(argbuf, arglen, sep, bytes_per_sep);
}

static PyObject *
_common_reduce(PyByteArrayObject *self, int proto)
{
    PyObject *dict;
    _Py_IDENTIFIER(__dict__);
    char *buf;

    if (_PyObject_LookupAttrId((PyObject *)self, &PyId___dict__, &dict) < 0) {
        return NULL;
    }
    if (dict == NULL) {
        dict = Py_None;
        Py_INCREF(dict);
    }

    buf = PyByteArray_AS_STRING(self);
    if (proto < 3) {
        /* use str based reduction for backwards compatibility with Python 2.x */
        PyObject *latin1;
        if (Py_SIZE(self))
            latin1 = PyUnicode_DecodeLatin1(buf, Py_SIZE(self), NULL);
        else
            latin1 = PyUnicode_FromString("");
        return Py_BuildValue("(O(Ns)N)", Py_TYPE(self), latin1, "latin-1", dict);
    }
    else {
        /* use more efficient byte based reduction */
        if (Py_SIZE(self)) {
            return Py_BuildValue("(O(y#)N)", Py_TYPE(self), buf, Py_SIZE(self), dict);
        }
        else {
            return Py_BuildValue("(O()N)", Py_TYPE(self), dict);
        }
    }
}

/*[clinic input]
bytearray.__reduce__ as bytearray_reduce

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
bytearray_reduce_impl(PyByteArrayObject *self)
/*[clinic end generated code: output=52bf304086464cab input=44b5737ada62dd3f]*/
{
    return _common_reduce(self, 2);
}

/*[clinic input]
bytearray.__reduce_ex__ as bytearray_reduce_ex

    proto: int = 0
    /

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto)
/*[clinic end generated code: output=52eac33377197520 input=f129bc1a1aa151ee]*/
{
    return _common_reduce(self, proto);
}

/*[clinic input]
bytearray.__sizeof__ as bytearray_sizeof

Returns the size of the bytearray object in memory, in bytes.
[clinic start generated code]*/

static PyObject *
bytearray_sizeof_impl(PyByteArrayObject *self)
/*[clinic end generated code: output=738abdd17951c427 input=e27320fd98a4bc5a]*/
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self)) + self->ob_alloc * sizeof(char);
    return PyLong_FromSsize_t(res);
}

static PySequenceMethods bytearray_as_sequence = {
    (lenfunc)bytearray_length,              /* sq_length */
    (binaryfunc)PyByteArray_Concat,         /* sq_concat */
    (ssizeargfunc)bytearray_repeat,         /* sq_repeat */
    (ssizeargfunc)bytearray_getitem,        /* sq_item */
    0,                                      /* sq_slice */
    (ssizeobjargproc)bytearray_setitem,     /* sq_ass_item */
    0,                                      /* sq_ass_slice */
    (objobjproc)bytearray_contains,         /* sq_contains */
    (binaryfunc)bytearray_iconcat,          /* sq_inplace_concat */
    (ssizeargfunc)bytearray_irepeat,        /* sq_inplace_repeat */
};

static PyMappingMethods bytearray_as_mapping = {
    (lenfunc)bytearray_length,
    (binaryfunc)bytearray_subscript,
    (objobjargproc)bytearray_ass_subscript,
};

static PyBufferProcs bytearray_as_buffer = {
    (getbufferproc)bytearray_getbuffer,
    (releasebufferproc)bytearray_releasebuffer,
};

static PyMethodDef
bytearray_methods[] = {
    {"__alloc__", (PyCFunction)bytearray_alloc, METH_NOARGS, alloc_doc},
    BYTEARRAY_REDUCE_METHODDEF
    BYTEARRAY_REDUCE_EX_METHODDEF
    BYTEARRAY_SIZEOF_METHODDEF
    BYTEARRAY_APPEND_METHODDEF
    {"capitalize", stringlib_capitalize, METH_NOARGS,
     _Py_capitalize__doc__},
    STRINGLIB_CENTER_METHODDEF
    BYTEARRAY_CLEAR_METHODDEF
    BYTEARRAY_COPY_METHODDEF
    {"count", (PyCFunction)bytearray_count, METH_VARARGS,
     _Py_count__doc__},
    BYTEARRAY_DECODE_METHODDEF
    {"endswith", (PyCFunction)bytearray_endswith, METH_VARARGS,
     _Py_endswith__doc__},
    STRINGLIB_EXPANDTABS_METHODDEF
    BYTEARRAY_EXTEND_METHODDEF
    {"find", (PyCFunction)bytearray_find, METH_VARARGS,
     _Py_find__doc__},
    BYTEARRAY_FROMHEX_METHODDEF
    BYTEARRAY_HEX_METHODDEF
    {"index", (PyCFunction)bytearray_index, METH_VARARGS, _Py_index__doc__},
    BYTEARRAY_INSERT_METHODDEF
    {"isalnum", stringlib_isalnum, METH_NOARGS,
     _Py_isalnum__doc__},
    {"isalpha", stringlib_isalpha, METH_NOARGS,
     _Py_isalpha__doc__},
    {"isascii", stringlib_isascii, METH_NOARGS,
     _Py_isascii__doc__},
    {"isdigit", stringlib_isdigit, METH_NOARGS,
     _Py_isdigit__doc__},
    {"islower", stringlib_islower, METH_NOARGS,
     _Py_islower__doc__},
    {"isspace", stringlib_isspace, METH_NOARGS,
     _Py_isspace__doc__},
    {"istitle", stringlib_istitle, METH_NOARGS,
     _Py_istitle__doc__},
    {"isupper", stringlib_isupper, METH_NOARGS,
     _Py_isupper__doc__},
    BYTEARRAY_JOIN_METHODDEF
    STRINGLIB_LJUST_METHODDEF
    {"lower", stringlib_lower, METH_NOARGS, _Py_lower__doc__},
    BYTEARRAY_LSTRIP_METHODDEF
    BYTEARRAY_MAKETRANS_METHODDEF
    BYTEARRAY_PARTITION_METHODDEF
    BYTEARRAY_POP_METHODDEF
    BYTEARRAY_REMOVE_METHODDEF
    BYTEARRAY_REPLACE_METHODDEF
    BYTEARRAY_REMOVEPREFIX_METHODDEF
    BYTEARRAY_REMOVESUFFIX_METHODDEF
    BYTEARRAY_REVERSE_METHODDEF
    {"rfind", (PyCFunction)bytearray_rfind, METH_VARARGS, _Py_rfind__doc__},
    {"rindex", (PyCFunction)bytearray_rindex, METH_VARARGS, _Py_rindex__doc__},
    STRINGLIB_RJUST_METHODDEF
    BYTEARRAY_RPARTITION_METHODDEF
    BYTEARRAY_RSPLIT_METHODDEF
    BYTEARRAY_RSTRIP_METHODDEF
    BYTEARRAY_SPLIT_METHODDEF
    BYTEARRAY_SPLITLINES_METHODDEF
    {"startswith", (PyCFunction)bytearray_startswith, METH_VARARGS ,
     _Py_startswith__doc__},
    BYTEARRAY_STRIP_METHODDEF
    {"swapcase", stringlib_swapcase, METH_NOARGS,
     _Py_swapcase__doc__},
    {"title", stringlib_title, METH_NOARGS, _Py_title__doc__},
    BYTEARRAY_TRANSLATE_METHODDEF
    {"upper", stringlib_upper, METH_NOARGS, _Py_upper__doc__},
    STRINGLIB_ZFILL_METHODDEF
    {NULL}
};

static PyObject *
bytearray_mod(PyObject *v, PyObject *w)
{
    if (!PyByteArray_Check(v))
        Py_RETURN_NOTIMPLEMENTED;
    return _PyBytes_FormatEx(PyByteArray_AS_STRING(v), PyByteArray_GET_SIZE(v), w, 1);
}

static PyNumberMethods bytearray_as_number = {
    0,              /*nb_add*/
    0,              /*nb_subtract*/
    0,              /*nb_multiply*/
    bytearray_mod,  /*nb_remainder*/
};

PyDoc_STRVAR(bytearray_doc,
"bytearray(iterable_of_ints) -> bytearray\n\
bytearray(string, encoding[, errors]) -> bytearray\n\
bytearray(bytes_or_buffer) -> mutable copy of bytes_or_buffer\n\
bytearray(int) -> bytes array of size given by the parameter initialized with null bytes\n\
bytearray() -> empty bytes array\n\
\n\
Construct a mutable bytearray object from:\n\
  - an iterable yielding integers in range(256)\n\
  - a text string encoded using the specified encoding\n\
  - a bytes or a buffer object\n\
  - any object implementing the buffer API.\n\
  - an integer");


static PyObject *bytearray_iter(PyObject *seq);

PyTypeObject PyByteArray_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bytearray",
    sizeof(PyByteArrayObject),
    0,
    (destructor)bytearray_dealloc,       /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)bytearray_repr,           /* tp_repr */
    &bytearray_as_number,               /* tp_as_number */
    &bytearray_as_sequence,             /* tp_as_sequence */
    &bytearray_as_mapping,              /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    bytearray_str,                      /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    &bytearray_as_buffer,               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    bytearray_doc,                      /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    (richcmpfunc)bytearray_richcompare, /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    bytearray_iter,                     /* tp_iter */
    0,                                  /* tp_iternext */
    bytearray_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)bytearray_init,           /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};

/*********************** Bytes Iterator ****************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyByteArrayObject *it_seq; /* Set to NULL when iterator is exhausted */
} bytesiterobject;

static void
bytearrayiter_dealloc(bytesiterobject *it)
{
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
bytearrayiter_traverse(bytesiterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
bytearrayiter_next(bytesiterobject *it)
{
    PyByteArrayObject *seq;
    PyObject *item;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    assert(PyByteArray_Check(seq));

    if (it->it_index < PyByteArray_GET_SIZE(seq)) {
        item = PyLong_FromLong(
            (unsigned char)PyByteArray_AS_STRING(seq)[it->it_index]);
        if (item != NULL)
            ++it->it_index;
        return item;
    }

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
bytearrayiter_length_hint(bytesiterobject *it, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t len = 0;
    if (it->it_seq) {
        len = PyByteArray_GET_SIZE(it->it_seq) - it->it_index;
        if (len < 0) {
            len = 0;
        }
    }
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
    "Private method returning an estimate of len(list(it)).");

static PyObject *
bytearrayiter_reduce(bytesiterobject *it, PyObject *Py_UNUSED(ignored))
{
    _Py_IDENTIFIER(iter);
    if (it->it_seq != NULL) {
        return Py_BuildValue("N(O)n", _PyEval_GetBuiltinId(&PyId_iter),
                             it->it_seq, it->it_index);
    } else {
        return Py_BuildValue("N(())", _PyEval_GetBuiltinId(&PyId_iter));
    }
}

static PyObject *
bytearrayiter_setstate(bytesiterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyByteArray_GET_SIZE(it->it_seq))
            index = PyByteArray_GET_SIZE(it->it_seq); /* iterator exhausted */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef bytearrayiter_methods[] = {
    {"__length_hint__", (PyCFunction)bytearrayiter_length_hint, METH_NOARGS,
     length_hint_doc},
     {"__reduce__",      (PyCFunction)bytearrayiter_reduce, METH_NOARGS,
     bytearray_reduce__doc__},
    {"__setstate__",    (PyCFunction)bytearrayiter_setstate, METH_O,
     setstate_doc},
    {NULL, NULL} /* sentinel */
};

PyTypeObject PyByteArrayIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bytearray_iterator",              /* tp_name */
    sizeof(bytesiterobject),           /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)bytearrayiter_dealloc, /* tp_dealloc */
    0,                                 /* tp_vectorcall_offset */
    0,                                 /* tp_getattr */
    0,                                 /* tp_setattr */
    0,                                 /* tp_as_async */
    0,                                 /* tp_repr */
    0,                                 /* tp_as_number */
    0,                                 /* tp_as_sequence */
    0,                                 /* tp_as_mapping */
    0,                                 /* tp_hash */
    0,                                 /* tp_call */
    0,                                 /* tp_str */
    PyObject_GenericGetAttr,           /* tp_getattro */
    0,                                 /* tp_setattro */
    0,                                 /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                 /* tp_doc */
    (traverseproc)bytearrayiter_traverse,  /* tp_traverse */
    0,                                 /* tp_clear */
    0,                                 /* tp_richcompare */
    0,                                 /* tp_weaklistoffset */
    PyObject_SelfIter,                 /* tp_iter */
    (iternextfunc)bytearrayiter_next,  /* tp_iternext */
    bytearrayiter_methods,             /* tp_methods */
    0,
};

static PyObject *
bytearray_iter(PyObject *seq)
{
    bytesiterobject *it;

    if (!PyByteArray_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(bytesiterobject, &PyByteArrayIter_Type);
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    Py_INCREF(seq);
    it->it_seq = (PyByteArrayObject *)seq;
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

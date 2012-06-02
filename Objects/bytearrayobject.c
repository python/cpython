/* PyBytes (bytearray) implementation */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "bytes_methods.h"

char _PyByteArray_empty_string[] = "";

void
PyByteArray_Fini(void)
{
}

int
PyByteArray_Init(void)
{
    return 1;
}

/* end nullbytes support */

/* Helpers */

static int
_getbytevalue(PyObject* arg, int *value)
{
    long face_value;

    if (PyBytes_CheckExact(arg)) {
        if (Py_SIZE(arg) != 1) {
            PyErr_SetString(PyExc_ValueError, "string must be of size 1");
            return 0;
        }
        *value = Py_CHARMASK(((PyBytesObject*)arg)->ob_sval[0]);
        return 1;
    }
    else if (PyInt_Check(arg) || PyLong_Check(arg)) {
        face_value = PyLong_AsLong(arg);
    }
    else {
        PyObject *index = PyNumber_Index(arg);
        if (index == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "an integer or string of size 1 is required");
            return 0;
        }
        face_value = PyLong_AsLong(index);
        Py_DECREF(index);
    }

    if (face_value < 0 || face_value >= 256) {
        /* this includes the OverflowError in case the long is too large */
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return 0;
    }

    *value = face_value;
    return 1;
}

static Py_ssize_t
bytearray_buffer_getreadbuf(PyByteArrayObject *self, Py_ssize_t index, const void **ptr)
{
    if ( index != 0 ) {
        PyErr_SetString(PyExc_SystemError,
                "accessing non-existent bytes segment");
        return -1;
    }
    *ptr = (void *)PyByteArray_AS_STRING(self);
    return Py_SIZE(self);
}

static Py_ssize_t
bytearray_buffer_getwritebuf(PyByteArrayObject *self, Py_ssize_t index, const void **ptr)
{
    if ( index != 0 ) {
        PyErr_SetString(PyExc_SystemError,
                "accessing non-existent bytes segment");
        return -1;
    }
    *ptr = (void *)PyByteArray_AS_STRING(self);
    return Py_SIZE(self);
}

static Py_ssize_t
bytearray_buffer_getsegcount(PyByteArrayObject *self, Py_ssize_t *lenp)
{
    if ( lenp )
        *lenp = Py_SIZE(self);
    return 1;
}

static Py_ssize_t
bytearray_buffer_getcharbuf(PyByteArrayObject *self, Py_ssize_t index, const char **ptr)
{
    if ( index != 0 ) {
        PyErr_SetString(PyExc_SystemError,
                "accessing non-existent bytes segment");
        return -1;
    }
    *ptr = PyByteArray_AS_STRING(self);
    return Py_SIZE(self);
}

static int
bytearray_getbuffer(PyByteArrayObject *obj, Py_buffer *view, int flags)
{
    int ret;
    void *ptr;
    if (view == NULL) {
        obj->ob_exports++;
        return 0;
    }
    ptr = (void *) PyByteArray_AS_STRING(obj);
    ret = PyBuffer_FillInfo(view, (PyObject*)obj, ptr, Py_SIZE(obj), 0, flags);
    if (ret >= 0) {
        obj->ob_exports++;
    }
    return ret;
}

static void
bytearray_releasebuffer(PyByteArrayObject *obj, Py_buffer *view)
{
    obj->ob_exports--;
}

static Py_ssize_t
_getbuffer(PyObject *obj, Py_buffer *view)
{
    PyBufferProcs *buffer = Py_TYPE(obj)->tp_as_buffer;

    if (buffer == NULL || buffer->bf_getbuffer == NULL)
    {
        PyErr_Format(PyExc_TypeError,
                     "Type %.100s doesn't support the buffer API",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }

    if (buffer->bf_getbuffer(obj, view, PyBUF_SIMPLE) < 0)
            return -1;
    return view->len;
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

/* Direct API functions */

PyObject *
PyByteArray_FromObject(PyObject *input)
{
    return PyObject_CallFunctionObjArgs((PyObject *)&PyByteArray_Type,
                                        input, NULL);
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

    new = PyObject_New(PyByteArrayObject, &PyByteArray_Type);
    if (new == NULL)
        return NULL;

    if (size == 0) {
        new->ob_bytes = NULL;
        alloc = 0;
    }
    else {
        alloc = size + 1;
        new->ob_bytes = PyMem_Malloc(alloc);
        if (new->ob_bytes == NULL) {
            Py_DECREF(new);
            return PyErr_NoMemory();
        }
        if (bytes != NULL && size > 0)
            memcpy(new->ob_bytes, bytes, size);
        new->ob_bytes[size] = '\0';  /* Trailing null byte */
    }
    Py_SIZE(new) = size;
    new->ob_alloc = alloc;
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
PyByteArray_Resize(PyObject *self, Py_ssize_t size)
{
    void *sval;
    Py_ssize_t alloc = ((PyByteArrayObject *)self)->ob_alloc;

    assert(self != NULL);
    assert(PyByteArray_Check(self));
    assert(size >= 0);

    if (size == Py_SIZE(self)) {
        return 0;
    }
    if (!_canresize((PyByteArrayObject *)self)) {
        return -1;
    }

    if (size < alloc / 2) {
        /* Major downsize; resize down to exact size */
        alloc = size + 1;
    }
    else if (size < alloc) {
        /* Within allocated size; quick exit */
        Py_SIZE(self) = size;
        ((PyByteArrayObject *)self)->ob_bytes[size] = '\0'; /* Trailing null */
        return 0;
    }
    else if (size <= alloc * 1.125) {
        /* Moderate upsize; overallocate similar to list_resize() */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize; resize up to exact size */
        alloc = size + 1;
    }

    sval = PyMem_Realloc(((PyByteArrayObject *)self)->ob_bytes, alloc);
    if (sval == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    ((PyByteArrayObject *)self)->ob_bytes = sval;
    Py_SIZE(self) = size;
    ((PyByteArrayObject *)self)->ob_alloc = alloc;
    ((PyByteArrayObject *)self)->ob_bytes[size] = '\0'; /* Trailing null byte */

    return 0;
}

PyObject *
PyByteArray_Concat(PyObject *a, PyObject *b)
{
    Py_ssize_t size;
    Py_buffer va, vb;
    PyByteArrayObject *result = NULL;

    va.len = -1;
    vb.len = -1;
    if (_getbuffer(a, &va) < 0  ||
        _getbuffer(b, &vb) < 0) {
            PyErr_Format(PyExc_TypeError, "can't concat %.100s to %.100s",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            goto done;
    }

    size = va.len + vb.len;
    if (size < 0) {
            PyErr_NoMemory();
            goto done;
    }

    result = (PyByteArrayObject *) PyByteArray_FromStringAndSize(NULL, size);
    if (result != NULL) {
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
    Py_ssize_t mysize;
    Py_ssize_t size;
    Py_buffer vo;

    if (_getbuffer(other, &vo) < 0) {
        PyErr_Format(PyExc_TypeError, "can't concat %.100s to %.100s",
                     Py_TYPE(other)->tp_name, Py_TYPE(self)->tp_name);
        return NULL;
    }

    mysize = Py_SIZE(self);
    size = mysize + vo.len;
    if (size < 0) {
        PyBuffer_Release(&vo);
        return PyErr_NoMemory();
    }
    if (size < self->ob_alloc) {
        Py_SIZE(self) = size;
        self->ob_bytes[Py_SIZE(self)] = '\0'; /* Trailing null byte */
    }
    else if (PyByteArray_Resize((PyObject *)self, size) < 0) {
        PyBuffer_Release(&vo);
        return NULL;
    }
    memcpy(self->ob_bytes + mysize, vo.buf, vo.len);
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

    if (count < 0)
        count = 0;
    mysize = Py_SIZE(self);
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    result = (PyByteArrayObject *)PyByteArray_FromStringAndSize(NULL, size);
    if (result != NULL && size != 0) {
        if (mysize == 1)
            memset(result->ob_bytes, self->ob_bytes[0], size);
        else {
            Py_ssize_t i;
            for (i = 0; i < count; i++)
                memcpy(result->ob_bytes + i*mysize, self->ob_bytes, mysize);
        }
    }
    return (PyObject *)result;
}

static PyObject *
bytearray_irepeat(PyByteArrayObject *self, Py_ssize_t count)
{
    Py_ssize_t mysize;
    Py_ssize_t size;

    if (count < 0)
        count = 0;
    mysize = Py_SIZE(self);
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    if (size < self->ob_alloc) {
        Py_SIZE(self) = size;
        self->ob_bytes[Py_SIZE(self)] = '\0'; /* Trailing null byte */
    }
    else if (PyByteArray_Resize((PyObject *)self, size) < 0)
        return NULL;

    if (mysize == 1)
        memset(self->ob_bytes, self->ob_bytes[0], size);
    else {
        Py_ssize_t i;
        for (i = 1; i < count; i++)
            memcpy(self->ob_bytes + i*mysize, self->ob_bytes, mysize);
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
bytearray_getitem(PyByteArrayObject *self, Py_ssize_t i)
{
    if (i < 0)
        i += Py_SIZE(self);
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
        return NULL;
    }
    return PyInt_FromLong((unsigned char)(self->ob_bytes[i]));
}

static PyObject *
bytearray_subscript(PyByteArrayObject *self, PyObject *index)
{
    if (PyIndex_Check(index)) {
        Py_ssize_t i = PyNumber_AsSsize_t(index, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return NULL;

        if (i < 0)
            i += PyByteArray_GET_SIZE(self);

        if (i < 0 || i >= Py_SIZE(self)) {
            PyErr_SetString(PyExc_IndexError, "bytearray index out of range");
            return NULL;
        }
        return PyInt_FromLong((unsigned char)(self->ob_bytes[i]));
    }
    else if (PySlice_Check(index)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        if (PySlice_GetIndicesEx((PySliceObject *)index,
                                 PyByteArray_GET_SIZE(self),
                                 &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0)
            return PyByteArray_FromStringAndSize("", 0);
        else if (step == 1) {
            return PyByteArray_FromStringAndSize(self->ob_bytes + start,
                                             slicelength);
        }
        else {
            char *source_buf = PyByteArray_AS_STRING(self);
            char *result_buf = (char *)PyMem_Malloc(slicelength);
            PyObject *result;

            if (result_buf == NULL)
                return PyErr_NoMemory();

            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                     result_buf[i] = source_buf[cur];
            }
            result = PyByteArray_FromStringAndSize(result_buf, slicelength);
            PyMem_Free(result_buf);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "bytearray indices must be integers");
        return NULL;
    }
}

static int
bytearray_setslice(PyByteArrayObject *self, Py_ssize_t lo, Py_ssize_t hi,
               PyObject *values)
{
    Py_ssize_t avail, needed;
    void *bytes;
    Py_buffer vbytes;
    int res = 0;

    vbytes.len = -1;
    if (values == (PyObject *)self) {
        /* Make a copy and call this function recursively */
        int err;
        values = PyByteArray_FromObject(values);
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
            if (_getbuffer(values, &vbytes) < 0) {
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

    avail = hi - lo;
    if (avail < 0)
        lo = hi = avail = 0;

    if (avail != needed) {
        if (avail > needed) {
            if (!_canresize(self)) {
                res = -1;
                goto finish;
            }
            /*
              0   lo               hi               old_size
              |   |<----avail----->|<-----tomove------>|
              |   |<-needed->|<-----tomove------>|
              0   lo      new_hi              new_size
            */
            memmove(self->ob_bytes + lo + needed, self->ob_bytes + hi,
                    Py_SIZE(self) - hi);
        }
        /* XXX(nnorwitz): need to verify this can't overflow! */
        if (PyByteArray_Resize((PyObject *)self,
                           Py_SIZE(self) + needed - avail) < 0) {
                res = -1;
                goto finish;
        }
        if (avail < needed) {
            /*
              0   lo        hi               old_size
              |   |<-avail->|<-----tomove------>|
              |   |<----needed---->|<-----tomove------>|
              0   lo            new_hi              new_size
             */
            memmove(self->ob_bytes + lo + needed, self->ob_bytes + hi,
                    Py_SIZE(self) - lo - needed);
        }
    }

    if (needed > 0)
        memcpy(self->ob_bytes + lo, bytes, needed);


 finish:
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

    self->ob_bytes[i] = ival;
    return 0;
}

static int
bytearray_ass_subscript(PyByteArrayObject *self, PyObject *index, PyObject *values)
{
    Py_ssize_t start, stop, step, slicelen, needed;
    char *bytes;

    if (PyIndex_Check(index)) {
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
            self->ob_bytes[i] = (char)ival;
            return 0;
        }
    }
    else if (PySlice_Check(index)) {
        if (PySlice_GetIndicesEx((PySliceObject *)index,
                                 PyByteArray_GET_SIZE(self),
                                 &start, &stop, &step, &slicelen) < 0) {
            return -1;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "bytearray indices must be integer");
        return -1;
    }

    if (values == NULL) {
        bytes = NULL;
        needed = 0;
    }
    else if (values == (PyObject *)self || !PyByteArray_Check(values)) {
        /* Make a copy and call this function recursively */
        int err;
        values = PyByteArray_FromObject(values);
        if (values == NULL)
            return -1;
        err = bytearray_ass_subscript(self, index, values);
        Py_DECREF(values);
        return err;
    }
    else {
        assert(PyByteArray_Check(values));
        bytes = ((PyByteArrayObject *)values)->ob_bytes;
        needed = Py_SIZE(values);
    }
    /* Make sure b[5:2] = ... inserts before 5, not before 2. */
    if ((step < 0 && start < stop) ||
        (step > 0 && start > stop))
        stop = start;
    if (step == 1) {
        if (slicelen != needed) {
            if (!_canresize(self))
                return -1;
            if (slicelen > needed) {
                /*
                  0   start           stop              old_size
                  |   |<---slicelen--->|<-----tomove------>|
                  |   |<-needed->|<-----tomove------>|
                  0   lo      new_hi              new_size
                */
                memmove(self->ob_bytes + start + needed, self->ob_bytes + stop,
                        Py_SIZE(self) - stop);
            }
            if (PyByteArray_Resize((PyObject *)self,
                               Py_SIZE(self) + needed - slicelen) < 0)
                return -1;
            if (slicelen < needed) {
                /*
                  0   lo        hi               old_size
                  |   |<-avail->|<-----tomove------>|
                  |   |<----needed---->|<-----tomove------>|
                  0   lo            new_hi              new_size
                 */
                memmove(self->ob_bytes + start + needed, self->ob_bytes + stop,
                        Py_SIZE(self) - start - needed);
            }
        }

        if (needed > 0)
            memcpy(self->ob_bytes + start, bytes, needed);

        return 0;
    }
    else {
        if (needed == 0) {
            /* Delete slice */
            size_t cur;
            Py_ssize_t i;

            if (!_canresize(self))
                return -1;
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

                memmove(self->ob_bytes + cur - i,
                        self->ob_bytes + cur + 1, lim);
            }
            /* Move the tail of the bytes, in one chunk */
            cur = start + slicelen*step;
            if (cur < (size_t)PyByteArray_GET_SIZE(self)) {
                memmove(self->ob_bytes + cur - slicelen,
                        self->ob_bytes + cur,
                        PyByteArray_GET_SIZE(self) - cur);
            }
            if (PyByteArray_Resize((PyObject *)self,
                               PyByteArray_GET_SIZE(self) - slicelen) < 0)
                return -1;

            return 0;
        }
        else {
            /* Assign slice */
            Py_ssize_t cur, i;

            if (needed != slicelen) {
                PyErr_Format(PyExc_ValueError,
                             "attempt to assign bytes of size %zd "
                             "to extended slice of size %zd",
                             needed, slicelen);
                return -1;
            }
            for (cur = start, i = 0; i < slicelen; cur += step, i++)
                self->ob_bytes[cur] = bytes[i];
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
                            "encoding or errors without sequence argument");
            return -1;
        }
        return 0;
    }

    if (PyBytes_Check(arg)) {
        PyObject *new, *encoded;
        if (encoding != NULL) {
            encoded = PyCodec_Encode(arg, encoding, errors);
            if (encoded == NULL)
                return -1;
            assert(PyBytes_Check(encoded));
        }
        else {
            encoded = arg;
            Py_INCREF(arg);
        }
        new = bytearray_iconcat(self, arg);
        Py_DECREF(encoded);
        if (new == NULL)
            return -1;
        Py_DECREF(new);
        return 0;
    }

#ifdef Py_USING_UNICODE
    if (PyUnicode_Check(arg)) {
        /* Encode via the codec registry */
        PyObject *encoded, *new;
        if (encoding == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "unicode argument without an encoding");
            return -1;
        }
        encoded = PyCodec_Encode(arg, encoding, errors);
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
#endif

    /* If it's not unicode, there can't be encoding or errors */
    if (encoding != NULL || errors != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding or errors without a string argument");
        return -1;
    }

    /* Is it an int? */
    count = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
    if (count == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            return -1;
        PyErr_Clear();
    }
    else if (count < 0) {
        PyErr_SetString(PyExc_ValueError, "negative count");
        return -1;
    }
    else {
        if (count > 0) {
            if (PyByteArray_Resize((PyObject *)self, count))
                return -1;
            memset(self->ob_bytes, 0, count);
        }
        return 0;
    }

    /* Use the buffer API */
    if (PyObject_CheckBuffer(arg)) {
        Py_ssize_t size;
        Py_buffer view;
        if (PyObject_GetBuffer(arg, &view, PyBUF_FULL_RO) < 0)
            return -1;
        size = view.len;
        if (PyByteArray_Resize((PyObject *)self, size) < 0) goto fail;
        if (PyBuffer_ToContiguous(self->ob_bytes, &view, size, 'C') < 0)
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
    if (it == NULL)
        return -1;
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
        if (Py_SIZE(self) < self->ob_alloc)
            Py_SIZE(self)++;
        else if (PyByteArray_Resize((PyObject *)self, Py_SIZE(self)+1) < 0)
            goto error;
        self->ob_bytes[Py_SIZE(self)-1] = value;
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
    static const char *hexdigits = "0123456789abcdef";
    const char *quote_prefix = "bytearray(b";
    const char *quote_postfix = ")";
    Py_ssize_t length = Py_SIZE(self);
    /* 14 == strlen(quote_prefix) + 2 + strlen(quote_postfix) */
    size_t newsize;
    PyObject *v;
    if (length > (PY_SSIZE_T_MAX - 14) / 4) {
        PyErr_SetString(PyExc_OverflowError,
            "bytearray object is too large to make repr");
        return NULL;
    }
    newsize = 14 + 4 * length;
    v = PyString_FromStringAndSize(NULL, newsize);
    if (v == NULL) {
        return NULL;
    }
    else {
        register Py_ssize_t i;
        register char c;
        register char *p;
        int quote;

        /* Figure out which quote to use; single is preferred */
        quote = '\'';
        {
            char *test, *start;
            start = PyByteArray_AS_STRING(self);
            for (test = start; test < start+length; ++test) {
                if (*test == '"') {
                    quote = '\''; /* back to single */
                    goto decided;
                }
                else if (*test == '\'')
                    quote = '"';
            }
          decided:
            ;
        }

        p = PyString_AS_STRING(v);
        while (*quote_prefix)
            *p++ = *quote_prefix++;
        *p++ = quote;

        for (i = 0; i < length; i++) {
            /* There's at least enough room for a hex escape
               and a closing quote. */
            assert(newsize - (p - PyString_AS_STRING(v)) >= 5);
            c = self->ob_bytes[i];
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
                *p++ = hexdigits[(c & 0xf0) >> 4];
                *p++ = hexdigits[c & 0xf];
            }
            else
                *p++ = c;
        }
        assert(newsize - (p - PyString_AS_STRING(v)) >= 1);
        *p++ = quote;
        while (*quote_postfix) {
           *p++ = *quote_postfix++;
        }
        *p = '\0';
        if (_PyString_Resize(&v, (p - PyString_AS_STRING(v)))) {
            Py_DECREF(v);
            return NULL;
        }
        return v;
    }
}

static PyObject *
bytearray_str(PyObject *op)
{
#if 0
    if (Py_BytesWarningFlag) {
        if (PyErr_WarnEx(PyExc_BytesWarning,
                 "str() on a bytearray instance", 1))
            return NULL;
    }
    return bytearray_repr((PyByteArrayObject*)op);
#endif
    return PyBytes_FromStringAndSize(((PyByteArrayObject*)op)->ob_bytes, Py_SIZE(op));
}

static PyObject *
bytearray_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_ssize_t self_size, other_size;
    Py_buffer self_bytes, other_bytes;
    PyObject *res;
    Py_ssize_t minsize;
    int cmp;

    /* Bytes can be compared to anything that supports the (binary)
       buffer API.  Except that a comparison with Unicode is always an
       error, even if the comparison is for equality. */
#ifdef Py_USING_UNICODE
    if (PyObject_IsInstance(self, (PyObject*)&PyUnicode_Type) ||
        PyObject_IsInstance(other, (PyObject*)&PyUnicode_Type)) {
        if (Py_BytesWarningFlag && op == Py_EQ) {
            if (PyErr_WarnEx(PyExc_BytesWarning,
                            "Comparison between bytearray and string", 1))
                return NULL;
        }

        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
#endif

    self_size = _getbuffer(self, &self_bytes);
    if (self_size < 0) {
        PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    other_size = _getbuffer(other, &other_bytes);
    if (other_size < 0) {
        PyErr_Clear();
        PyBuffer_Release(&self_bytes);
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    if (self_size != other_size && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the objects differ */
        cmp = (op == Py_NE);
    }
    else {
        minsize = self_size;
        if (other_size < minsize)
            minsize = other_size;

        cmp = memcmp(self_bytes.buf, other_bytes.buf, minsize);
        /* In ISO C, memcmp() guarantees to use unsigned bytes! */

        if (cmp == 0) {
            if (self_size < other_size)
                cmp = -1;
            else if (self_size > other_size)
                cmp = 1;
        }

        switch (op) {
        case Py_LT: cmp = cmp <  0; break;
        case Py_LE: cmp = cmp <= 0; break;
        case Py_EQ: cmp = cmp == 0; break;
        case Py_NE: cmp = cmp != 0; break;
        case Py_GT: cmp = cmp >  0; break;
        case Py_GE: cmp = cmp >= 0; break;
        }
    }

    res = cmp ? Py_True : Py_False;
    PyBuffer_Release(&self_bytes);
    PyBuffer_Release(&other_bytes);
    Py_INCREF(res);
    return res;
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
        PyMem_Free(self->ob_bytes);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* -------------------------------------------------------------------- */
/* Methods */

#define STRINGLIB_CHAR char
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
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/ctype.h"
#include "stringlib/transmogrify.h"


/* The following Py_LOCAL_INLINE and Py_LOCAL functions
were copied from the old char* style string object. */

/* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                              \
        end = len;                              \
    else if (end < 0) {                         \
        end += len;                             \
        if (end < 0)                            \
            end = 0;                            \
    }                                           \
    if (start < 0) {                            \
        start += len;                           \
        if (start < 0)                          \
            start = 0;                          \
    }

Py_LOCAL_INLINE(Py_ssize_t)
bytearray_find_internal(PyByteArrayObject *self, PyObject *args, int dir)
{
    PyObject *subobj;
    Py_buffer subbuf;
    Py_ssize_t start=0, end=PY_SSIZE_T_MAX;
    Py_ssize_t res;

    if (!stringlib_parse_args_finds("find/rfind/index/rindex",
                                    args, &subobj, &start, &end))
        return -2;
    if (_getbuffer(subobj, &subbuf) < 0)
        return -2;
    if (dir > 0)
        res = stringlib_find_slice(
            PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self),
            subbuf.buf, subbuf.len, start, end);
    else
        res = stringlib_rfind_slice(
            PyByteArray_AS_STRING(self), PyByteArray_GET_SIZE(self),
            subbuf.buf, subbuf.len, start, end);
    PyBuffer_Release(&subbuf);
    return res;
}

PyDoc_STRVAR(find__doc__,
"B.find(sub [,start [,end]]) -> int\n\
\n\
Return the lowest index in B where subsection sub is found,\n\
such that sub is contained within B[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
bytearray_find(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t result = bytearray_find_internal(self, args, +1);
    if (result == -2)
        return NULL;
    return PyInt_FromSsize_t(result);
}

PyDoc_STRVAR(count__doc__,
"B.count(sub [,start [,end]]) -> int\n\
\n\
Return the number of non-overlapping occurrences of subsection sub in\n\
bytes B[start:end].  Optional arguments start and end are interpreted\n\
as in slice notation.");

static PyObject *
bytearray_count(PyByteArrayObject *self, PyObject *args)
{
    PyObject *sub_obj;
    const char *str = PyByteArray_AS_STRING(self);
    Py_ssize_t start = 0, end = PY_SSIZE_T_MAX;
    Py_buffer vsub;
    PyObject *count_obj;

    if (!stringlib_parse_args_finds("count", args, &sub_obj, &start, &end))
        return NULL;

    if (_getbuffer(sub_obj, &vsub) < 0)
        return NULL;

    ADJUST_INDICES(start, end, PyByteArray_GET_SIZE(self));

    count_obj = PyInt_FromSsize_t(
        stringlib_count(str + start, end - start, vsub.buf, vsub.len, PY_SSIZE_T_MAX)
        );
    PyBuffer_Release(&vsub);
    return count_obj;
}


PyDoc_STRVAR(index__doc__,
"B.index(sub [,start [,end]]) -> int\n\
\n\
Like B.find() but raise ValueError when the subsection is not found.");

static PyObject *
bytearray_index(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t result = bytearray_find_internal(self, args, +1);
    if (result == -2)
        return NULL;
    if (result == -1) {
        PyErr_SetString(PyExc_ValueError,
                        "subsection not found");
        return NULL;
    }
    return PyInt_FromSsize_t(result);
}


PyDoc_STRVAR(rfind__doc__,
"B.rfind(sub [,start [,end]]) -> int\n\
\n\
Return the highest index in B where subsection sub is found,\n\
such that sub is contained within B[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
bytearray_rfind(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t result = bytearray_find_internal(self, args, -1);
    if (result == -2)
        return NULL;
    return PyInt_FromSsize_t(result);
}


PyDoc_STRVAR(rindex__doc__,
"B.rindex(sub [,start [,end]]) -> int\n\
\n\
Like B.rfind() but raise ValueError when the subsection is not found.");

static PyObject *
bytearray_rindex(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t result = bytearray_find_internal(self, args, -1);
    if (result == -2)
        return NULL;
    if (result == -1) {
        PyErr_SetString(PyExc_ValueError,
                        "subsection not found");
        return NULL;
    }
    return PyInt_FromSsize_t(result);
}


static int
bytearray_contains(PyObject *self, PyObject *arg)
{
    Py_ssize_t ival = PyNumber_AsSsize_t(arg, PyExc_ValueError);
    if (ival == -1 && PyErr_Occurred()) {
        Py_buffer varg;
        int pos;
        PyErr_Clear();
        if (_getbuffer(arg, &varg) < 0)
            return -1;
        pos = stringlib_find(PyByteArray_AS_STRING(self), Py_SIZE(self),
                             varg.buf, varg.len, 0);
        PyBuffer_Release(&varg);
        return pos >= 0;
    }
    if (ival < 0 || ival >= 256) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return -1;
    }

    return memchr(PyByteArray_AS_STRING(self), ival, Py_SIZE(self)) != NULL;
}


/* Matches the end (direction >= 0) or start (direction < 0) of self
 * against substr, using the start and end arguments. Returns
 * -1 on error, 0 if not found and 1 if found.
 */
Py_LOCAL(int)
_bytearray_tailmatch(PyByteArrayObject *self, PyObject *substr, Py_ssize_t start,
                 Py_ssize_t end, int direction)
{
    Py_ssize_t len = PyByteArray_GET_SIZE(self);
    const char* str;
    Py_buffer vsubstr;
    int rv = 0;

    str = PyByteArray_AS_STRING(self);

    if (_getbuffer(substr, &vsubstr) < 0)
        return -1;

    ADJUST_INDICES(start, end, len);

    if (direction < 0) {
        /* startswith */
        if (start+vsubstr.len > len) {
            goto done;
        }
    } else {
        /* endswith */
        if (end-start < vsubstr.len || start > len) {
            goto done;
        }

        if (end-vsubstr.len > start)
            start = end - vsubstr.len;
    }
    if (end-start >= vsubstr.len)
        rv = ! memcmp(str+start, vsubstr.buf, vsubstr.len);

done:
    PyBuffer_Release(&vsubstr);
    return rv;
}


PyDoc_STRVAR(startswith__doc__,
"B.startswith(prefix [,start [,end]]) -> bool\n\
\n\
Return True if B starts with the specified prefix, False otherwise.\n\
With optional start, test B beginning at that position.\n\
With optional end, stop comparing B at that position.\n\
prefix can also be a tuple of strings to try.");

static PyObject *
bytearray_startswith(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *subobj;
    int result;

    if (!stringlib_parse_args_finds("startswith", args, &subobj, &start, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            result = _bytearray_tailmatch(self,
                                      PyTuple_GET_ITEM(subobj, i),
                                      start, end, -1);
            if (result == -1)
                return NULL;
            else if (result) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    result = _bytearray_tailmatch(self, subobj, start, end, -1);
    if (result == -1)
        return NULL;
    else
        return PyBool_FromLong(result);
}

PyDoc_STRVAR(endswith__doc__,
"B.endswith(suffix [,start [,end]]) -> bool\n\
\n\
Return True if B ends with the specified suffix, False otherwise.\n\
With optional start, test B beginning at that position.\n\
With optional end, stop comparing B at that position.\n\
suffix can also be a tuple of strings to try.");

static PyObject *
bytearray_endswith(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *subobj;
    int result;

    if (!stringlib_parse_args_finds("endswith", args, &subobj, &start, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            result = _bytearray_tailmatch(self,
                                      PyTuple_GET_ITEM(subobj, i),
                                      start, end, +1);
            if (result == -1)
                return NULL;
            else if (result) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    result = _bytearray_tailmatch(self, subobj, start, end, +1);
    if (result == -1)
        return NULL;
    else
        return PyBool_FromLong(result);
}


PyDoc_STRVAR(translate__doc__,
"B.translate(table[, deletechars]) -> bytearray\n\
\n\
Return a copy of B, where all characters occurring in the\n\
optional argument deletechars are removed, and the remaining\n\
characters have been mapped through the given translation\n\
table, which must be a bytes object of length 256.");

static PyObject *
bytearray_translate(PyByteArrayObject *self, PyObject *args)
{
    register char *input, *output;
    register const char *table;
    register Py_ssize_t i, c;
    PyObject *input_obj = (PyObject*)self;
    const char *output_start;
    Py_ssize_t inlen;
    PyObject *result = NULL;
    int trans_table[256];
    PyObject *tableobj = NULL, *delobj = NULL;
    Py_buffer vtable, vdel;

    if (!PyArg_UnpackTuple(args, "translate", 1, 2,
                           &tableobj, &delobj))
          return NULL;

    if (tableobj == Py_None) {
        table = NULL;
        tableobj = NULL;
    } else if (_getbuffer(tableobj, &vtable) < 0) {
        return NULL;
    } else {
        if (vtable.len != 256) {
            PyErr_SetString(PyExc_ValueError,
                            "translation table must be 256 characters long");
            PyBuffer_Release(&vtable);
            return NULL;
        }
        table = (const char*)vtable.buf;
    }

    if (delobj != NULL) {
        if (_getbuffer(delobj, &vdel) < 0) {
            if (tableobj != NULL)
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
    output_start = output = PyByteArray_AsString(result);
    input = PyByteArray_AS_STRING(input_obj);

    if (vdel.len == 0 && table != NULL) {
        /* If no deletions are required, use faster code */
        for (i = inlen; --i >= 0; ) {
            c = Py_CHARMASK(*input++);
            *output++ = table[c];
        }
        goto done;
    }

    if (table == NULL) {
        for (i = 0; i < 256; i++)
            trans_table[i] = Py_CHARMASK(i);
    } else {
        for (i = 0; i < 256; i++)
            trans_table[i] = Py_CHARMASK(table[i]);
    }

    for (i = 0; i < vdel.len; i++)
        trans_table[(int) Py_CHARMASK( ((unsigned char*)vdel.buf)[i] )] = -1;

    for (i = inlen; --i >= 0; ) {
        c = Py_CHARMASK(*input++);
        if (trans_table[c] != -1)
            if (Py_CHARMASK(*output++ = (char)trans_table[c]) == c)
                    continue;
    }
    /* Fix the size of the resulting string */
    if (inlen > 0)
        PyByteArray_Resize(result, output - output_start);

done:
    if (tableobj != NULL)
        PyBuffer_Release(&vtable);
    if (delobj != NULL)
        PyBuffer_Release(&vdel);
    return result;
}


/* find and count characters and substrings */

#define findchar(target, target_len, c)                         \
  ((char *)memchr((const void *)(target), c, target_len))


/* Bytes ops must return a string, create a copy */
Py_LOCAL(PyByteArrayObject *)
return_self(PyByteArrayObject *self)
{
    return (PyByteArrayObject *)PyByteArray_FromStringAndSize(
            PyByteArray_AS_STRING(self),
            PyByteArray_GET_SIZE(self));
}

Py_LOCAL_INLINE(Py_ssize_t)
countchar(const char *target, Py_ssize_t target_len, char c, Py_ssize_t maxcount)
{
    Py_ssize_t count=0;
    const char *start=target;
    const char *end=target+target_len;

    while ( (start=findchar(start, end-start, c)) != NULL ) {
        count++;
        if (count >= maxcount)
            break;
        start += 1;
    }
    return count;
}


/* Algorithms for different cases of string replacement */

/* len(self)>=1, from="", len(to)>=1, maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_interleave(PyByteArrayObject *self,
                   const char *to_s, Py_ssize_t to_len,
                   Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, i, product;
    PyByteArrayObject *result;

    self_len = PyByteArray_GET_SIZE(self);

    /* 1 at the end plus 1 after every character */
    count = self_len+1;
    if (maxcount < count)
        count = maxcount;

    /* Check for overflow */
    /*   result_len = count * to_len + self_len; */
    product = count * to_len;
    if (product / to_len != count) {
        PyErr_SetString(PyExc_OverflowError,
                        "replace string is too long");
        return NULL;
    }
    result_len = product + self_len;
    if (result_len < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "replace string is too long");
        return NULL;
    }

    if (! (result = (PyByteArrayObject *)
                     PyByteArray_FromStringAndSize(NULL, result_len)) )
        return NULL;

    self_s = PyByteArray_AS_STRING(self);
    result_s = PyByteArray_AS_STRING(result);

    /* TODO: special case single character, which doesn't need memcpy */

    /* Lay the first one down (guaranteed this will occur) */
    Py_MEMCPY(result_s, to_s, to_len);
    result_s += to_len;
    count -= 1;

    for (i=0; i<count; i++) {
        *result_s++ = *self_s++;
        Py_MEMCPY(result_s, to_s, to_len);
        result_s += to_len;
    }

    /* Copy the rest of the original string */
    Py_MEMCPY(result_s, self_s, self_len-i);

    return result;
}

/* Special case for deleting a single character */
/* len(self)>=1, len(from)==1, to="", maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_delete_single_character(PyByteArrayObject *self,
                                char from_c, Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count;
    PyByteArrayObject *result;

    self_len = PyByteArray_GET_SIZE(self);
    self_s = PyByteArray_AS_STRING(self);

    count = countchar(self_s, self_len, from_c, maxcount);
    if (count == 0) {
        return return_self(self);
    }

    result_len = self_len - count;  /* from_len == 1 */
    assert(result_len>=0);

    if ( (result = (PyByteArrayObject *)
                    PyByteArray_FromStringAndSize(NULL, result_len)) == NULL)
        return NULL;
    result_s = PyByteArray_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        next = findchar(start, end-start, from_c);
        if (next == NULL)
            break;
        Py_MEMCPY(result_s, start, next-start);
        result_s += (next-start);
        start = next+1;
    }
    Py_MEMCPY(result_s, start, end-start);

    return result;
}

/* len(self)>=1, len(from)>=2, to="", maxcount>=1 */

Py_LOCAL(PyByteArrayObject *)
replace_delete_substring(PyByteArrayObject *self,
                         const char *from_s, Py_ssize_t from_len,
                         Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset;
    PyByteArrayObject *result;

    self_len = PyByteArray_GET_SIZE(self);
    self_s = PyByteArray_AS_STRING(self);

    count = stringlib_count(self_s, self_len,
                            from_s, from_len,
                            maxcount);

    if (count == 0) {
        /* no matches */
        return return_self(self);
    }

    result_len = self_len - (count * from_len);
    assert (result_len>=0);

    if ( (result = (PyByteArrayObject *)
        PyByteArray_FromStringAndSize(NULL, result_len)) == NULL )
            return NULL;

    result_s = PyByteArray_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = stringlib_find(start, end-start,
                                from_s, from_len,
                                0);
        if (offset == -1)
            break;
        next = start + offset;

        Py_MEMCPY(result_s, start, next-start);

        result_s += (next-start);
        start = next+from_len;
    }
    Py_MEMCPY(result_s, start, end-start);
    return result;
}

/* len(self)>=1, len(from)==len(to)==1, maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_single_character_in_place(PyByteArrayObject *self,
                                  char from_c, char to_c,
                                  Py_ssize_t maxcount)
{
    char *self_s, *result_s, *start, *end, *next;
    Py_ssize_t self_len;
    PyByteArrayObject *result;

    /* The result string will be the same size */
    self_s = PyByteArray_AS_STRING(self);
    self_len = PyByteArray_GET_SIZE(self);

    next = findchar(self_s, self_len, from_c);

    if (next == NULL) {
        /* No matches; return the original bytes */
        return return_self(self);
    }

    /* Need to make a new bytes */
    result = (PyByteArrayObject *) PyByteArray_FromStringAndSize(NULL, self_len);
    if (result == NULL)
        return NULL;
    result_s = PyByteArray_AS_STRING(result);
    Py_MEMCPY(result_s, self_s, self_len);

    /* change everything in-place, starting with this one */
    start =  result_s + (next-self_s);
    *start = to_c;
    start++;
    end = result_s + self_len;

    while (--maxcount > 0) {
        next = findchar(start, end-start, from_c);
        if (next == NULL)
            break;
        *next = to_c;
        start = next+1;
    }

    return result;
}

/* len(self)>=1, len(from)==len(to)>=2, maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_substring_in_place(PyByteArrayObject *self,
                           const char *from_s, Py_ssize_t from_len,
                           const char *to_s, Py_ssize_t to_len,
                           Py_ssize_t maxcount)
{
    char *result_s, *start, *end;
    char *self_s;
    Py_ssize_t self_len, offset;
    PyByteArrayObject *result;

    /* The result bytes will be the same size */

    self_s = PyByteArray_AS_STRING(self);
    self_len = PyByteArray_GET_SIZE(self);

    offset = stringlib_find(self_s, self_len,
                            from_s, from_len,
                            0);
    if (offset == -1) {
        /* No matches; return the original bytes */
        return return_self(self);
    }

    /* Need to make a new bytes */
    result = (PyByteArrayObject *) PyByteArray_FromStringAndSize(NULL, self_len);
    if (result == NULL)
        return NULL;
    result_s = PyByteArray_AS_STRING(result);
    Py_MEMCPY(result_s, self_s, self_len);

    /* change everything in-place, starting with this one */
    start =  result_s + offset;
    Py_MEMCPY(start, to_s, from_len);
    start += from_len;
    end = result_s + self_len;

    while ( --maxcount > 0) {
        offset = stringlib_find(start, end-start,
                                from_s, from_len,
                                0);
        if (offset==-1)
            break;
        Py_MEMCPY(start+offset, to_s, from_len);
        start += offset+from_len;
    }

    return result;
}

/* len(self)>=1, len(from)==1, len(to)>=2, maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_single_character(PyByteArrayObject *self,
                         char from_c,
                         const char *to_s, Py_ssize_t to_len,
                         Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, product;
    PyByteArrayObject *result;

    self_s = PyByteArray_AS_STRING(self);
    self_len = PyByteArray_GET_SIZE(self);

    count = countchar(self_s, self_len, from_c, maxcount);
    if (count == 0) {
        /* no matches, return unchanged */
        return return_self(self);
    }

    /* use the difference between current and new, hence the "-1" */
    /*   result_len = self_len + count * (to_len-1)  */
    product = count * (to_len-1);
    if (product / (to_len-1) != count) {
        PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
        return NULL;
    }
    result_len = self_len + product;
    if (result_len < 0) {
            PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
            return NULL;
    }

    if ( (result = (PyByteArrayObject *)
          PyByteArray_FromStringAndSize(NULL, result_len)) == NULL)
            return NULL;
    result_s = PyByteArray_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        next = findchar(start, end-start, from_c);
        if (next == NULL)
            break;

        if (next == start) {
            /* replace with the 'to' */
            Py_MEMCPY(result_s, to_s, to_len);
            result_s += to_len;
            start += 1;
        } else {
            /* copy the unchanged old then the 'to' */
            Py_MEMCPY(result_s, start, next-start);
            result_s += (next-start);
            Py_MEMCPY(result_s, to_s, to_len);
            result_s += to_len;
            start = next+1;
        }
    }
    /* Copy the remainder of the remaining bytes */
    Py_MEMCPY(result_s, start, end-start);

    return result;
}

/* len(self)>=1, len(from)>=2, len(to)>=2, maxcount>=1 */
Py_LOCAL(PyByteArrayObject *)
replace_substring(PyByteArrayObject *self,
                  const char *from_s, Py_ssize_t from_len,
                  const char *to_s, Py_ssize_t to_len,
                  Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset, product;
    PyByteArrayObject *result;

    self_s = PyByteArray_AS_STRING(self);
    self_len = PyByteArray_GET_SIZE(self);

    count = stringlib_count(self_s, self_len,
                            from_s, from_len,
                            maxcount);

    if (count == 0) {
        /* no matches, return unchanged */
        return return_self(self);
    }

    /* Check for overflow */
    /*    result_len = self_len + count * (to_len-from_len) */
    product = count * (to_len-from_len);
    if (product / (to_len-from_len) != count) {
        PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
        return NULL;
    }
    result_len = self_len + product;
    if (result_len < 0) {
        PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
        return NULL;
    }

    if ( (result = (PyByteArrayObject *)
          PyByteArray_FromStringAndSize(NULL, result_len)) == NULL)
        return NULL;
    result_s = PyByteArray_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = stringlib_find(start, end-start,
                                from_s, from_len,
                                0);
        if (offset == -1)
            break;
        next = start+offset;
        if (next == start) {
            /* replace with the 'to' */
            Py_MEMCPY(result_s, to_s, to_len);
            result_s += to_len;
            start += from_len;
        } else {
            /* copy the unchanged old then the 'to' */
            Py_MEMCPY(result_s, start, next-start);
            result_s += (next-start);
            Py_MEMCPY(result_s, to_s, to_len);
            result_s += to_len;
            start = next+from_len;
        }
    }
    /* Copy the remainder of the remaining bytes */
    Py_MEMCPY(result_s, start, end-start);

    return result;
}


Py_LOCAL(PyByteArrayObject *)
replace(PyByteArrayObject *self,
        const char *from_s, Py_ssize_t from_len,
        const char *to_s, Py_ssize_t to_len,
        Py_ssize_t maxcount)
{
    if (maxcount < 0) {
        maxcount = PY_SSIZE_T_MAX;
    } else if (maxcount == 0 || PyByteArray_GET_SIZE(self) == 0) {
        /* nothing to do; return the original bytes */
        return return_self(self);
    }

    if (maxcount == 0 ||
        (from_len == 0 && to_len == 0)) {
        /* nothing to do; return the original bytes */
        return return_self(self);
    }

    /* Handle zero-length special cases */

    if (from_len == 0) {
        /* insert the 'to' bytes everywhere.   */
        /*    >>> "Python".replace("", ".")     */
        /*    '.P.y.t.h.o.n.'                   */
        return replace_interleave(self, to_s, to_len, maxcount);
    }

    /* Except for "".replace("", "A") == "A" there is no way beyond this */
    /* point for an empty self bytes to generate a non-empty bytes */
    /* Special case so the remaining code always gets a non-empty bytes */
    if (PyByteArray_GET_SIZE(self) == 0) {
        return return_self(self);
    }

    if (to_len == 0) {
        /* delete all occurances of 'from' bytes */
        if (from_len == 1) {
            return replace_delete_single_character(
                    self, from_s[0], maxcount);
        } else {
            return replace_delete_substring(self, from_s, from_len, maxcount);
        }
    }

    /* Handle special case where both bytes have the same length */

    if (from_len == to_len) {
        if (from_len == 1) {
            return replace_single_character_in_place(
                    self,
                    from_s[0],
                    to_s[0],
                    maxcount);
        } else {
            return replace_substring_in_place(
                self, from_s, from_len, to_s, to_len, maxcount);
        }
    }

    /* Otherwise use the more generic algorithms */
    if (from_len == 1) {
        return replace_single_character(self, from_s[0],
                                        to_s, to_len, maxcount);
    } else {
        /* len('from')>=2, len('to')>=1 */
        return replace_substring(self, from_s, from_len, to_s, to_len, maxcount);
    }
}


PyDoc_STRVAR(replace__doc__,
"B.replace(old, new[, count]) -> bytes\n\
\n\
Return a copy of B with all occurrences of subsection\n\
old replaced by new.  If the optional argument count is\n\
given, only the first count occurrences are replaced.");

static PyObject *
bytearray_replace(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t count = -1;
    PyObject *from, *to, *res;
    Py_buffer vfrom, vto;

    if (!PyArg_ParseTuple(args, "OO|n:replace", &from, &to, &count))
        return NULL;

    if (_getbuffer(from, &vfrom) < 0)
        return NULL;
    if (_getbuffer(to, &vto) < 0) {
        PyBuffer_Release(&vfrom);
        return NULL;
    }

    res = (PyObject *)replace((PyByteArrayObject *) self,
                              vfrom.buf, vfrom.len,
                              vto.buf, vto.len, count);

    PyBuffer_Release(&vfrom);
    PyBuffer_Release(&vto);
    return res;
}

PyDoc_STRVAR(split__doc__,
"B.split([sep[, maxsplit]]) -> list of bytearray\n\
\n\
Return a list of the sections in B, using sep as the delimiter.\n\
If sep is not given, B is split on ASCII whitespace characters\n\
(space, tab, return, newline, formfeed, vertical tab).\n\
If maxsplit is given, at most maxsplit splits are done.");

static PyObject *
bytearray_split(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t len = PyByteArray_GET_SIZE(self), n;
    Py_ssize_t maxsplit = -1;
    const char *s = PyByteArray_AS_STRING(self), *sub;
    PyObject *list, *subobj = Py_None;
    Py_buffer vsub;

    if (!PyArg_ParseTuple(args, "|On:split", &subobj, &maxsplit))
        return NULL;
    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;

    if (subobj == Py_None)
        return stringlib_split_whitespace((PyObject*) self, s, len, maxsplit);

    if (_getbuffer(subobj, &vsub) < 0)
        return NULL;
    sub = vsub.buf;
    n = vsub.len;

    list = stringlib_split(
        (PyObject*) self, s, len, sub, n, maxsplit
        );
    PyBuffer_Release(&vsub);
    return list;
}

PyDoc_STRVAR(partition__doc__,
"B.partition(sep) -> (head, sep, tail)\n\
\n\
Searches for the separator sep in B, and returns the part before it,\n\
the separator itself, and the part after it.  If the separator is not\n\
found, returns B and two empty bytearray objects.");

static PyObject *
bytearray_partition(PyByteArrayObject *self, PyObject *sep_obj)
{
    PyObject *bytesep, *result;

    bytesep = PyByteArray_FromObject(sep_obj);
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

PyDoc_STRVAR(rpartition__doc__,
"B.rpartition(sep) -> (head, sep, tail)\n\
\n\
Searches for the separator sep in B, starting at the end of B,\n\
and returns the part before it, the separator itself, and the\n\
part after it.  If the separator is not found, returns two empty\n\
bytearray objects and B.");

static PyObject *
bytearray_rpartition(PyByteArrayObject *self, PyObject *sep_obj)
{
    PyObject *bytesep, *result;

    bytesep = PyByteArray_FromObject(sep_obj);
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

PyDoc_STRVAR(rsplit__doc__,
"B.rsplit(sep[, maxsplit]) -> list of bytearray\n\
\n\
Return a list of the sections in B, using sep as the delimiter,\n\
starting at the end of B and working to the front.\n\
If sep is not given, B is split on ASCII whitespace characters\n\
(space, tab, return, newline, formfeed, vertical tab).\n\
If maxsplit is given, at most maxsplit splits are done.");

static PyObject *
bytearray_rsplit(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t len = PyByteArray_GET_SIZE(self), n;
    Py_ssize_t maxsplit = -1;
    const char *s = PyByteArray_AS_STRING(self), *sub;
    PyObject *list, *subobj = Py_None;
    Py_buffer vsub;

    if (!PyArg_ParseTuple(args, "|On:rsplit", &subobj, &maxsplit))
        return NULL;
    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;

    if (subobj == Py_None)
        return stringlib_rsplit_whitespace((PyObject*) self, s, len, maxsplit);

    if (_getbuffer(subobj, &vsub) < 0)
        return NULL;
    sub = vsub.buf;
    n = vsub.len;

    list = stringlib_rsplit(
        (PyObject*) self, s, len, sub, n, maxsplit
        );
    PyBuffer_Release(&vsub);
    return list;
}

PyDoc_STRVAR(reverse__doc__,
"B.reverse() -> None\n\
\n\
Reverse the order of the values in B in place.");
static PyObject *
bytearray_reverse(PyByteArrayObject *self, PyObject *unused)
{
    char swap, *head, *tail;
    Py_ssize_t i, j, n = Py_SIZE(self);

    j = n / 2;
    head = self->ob_bytes;
    tail = head + n - 1;
    for (i = 0; i < j; i++) {
        swap = *head;
        *head++ = *tail;
        *tail-- = swap;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(insert__doc__,
"B.insert(index, int) -> None\n\
\n\
Insert a single item into the bytearray before the given index.");
static PyObject *
bytearray_insert(PyByteArrayObject *self, PyObject *args)
{
    PyObject *value;
    int ival;
    Py_ssize_t where, n = Py_SIZE(self);

    if (!PyArg_ParseTuple(args, "nO:insert", &where, &value))
        return NULL;

    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytearray");
        return NULL;
    }
    if (!_getbytevalue(value, &ival))
        return NULL;
    if (PyByteArray_Resize((PyObject *)self, n + 1) < 0)
        return NULL;

    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    memmove(self->ob_bytes + where + 1, self->ob_bytes + where, n - where);
    self->ob_bytes[where] = ival;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(append__doc__,
"B.append(int) -> None\n\
\n\
Append a single item to the end of B.");
static PyObject *
bytearray_append(PyByteArrayObject *self, PyObject *arg)
{
    int value;
    Py_ssize_t n = Py_SIZE(self);

    if (! _getbytevalue(arg, &value))
        return NULL;
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytearray");
        return NULL;
    }
    if (PyByteArray_Resize((PyObject *)self, n + 1) < 0)
        return NULL;

    self->ob_bytes[n] = value;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(extend__doc__,
"B.extend(iterable int) -> None\n\
\n\
Append all the elements from the iterator or sequence to the\n\
end of B.");
static PyObject *
bytearray_extend(PyByteArrayObject *self, PyObject *arg)
{
    PyObject *it, *item, *bytearray_obj;
    Py_ssize_t buf_size = 0, len = 0;
    int value;
    char *buf;

    /* bytearray_setslice code only accepts something supporting PEP 3118. */
    if (PyObject_CheckBuffer(arg)) {
        if (bytearray_setslice(self, Py_SIZE(self), Py_SIZE(self), arg) == -1)
            return NULL;

        Py_RETURN_NONE;
    }

    it = PyObject_GetIter(arg);
    if (it == NULL)
        return NULL;

    /* Try to determine the length of the argument. 32 is arbitrary. */
    buf_size = _PyObject_LengthHint(arg, 32);
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
            buf_size = len + (len >> 1) + 1;
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

    Py_RETURN_NONE;
}

PyDoc_STRVAR(pop__doc__,
"B.pop([index]) -> int\n\
\n\
Remove and return a single item from B. If no index\n\
argument is given, will pop the last value.");
static PyObject *
bytearray_pop(PyByteArrayObject *self, PyObject *args)
{
    int value;
    Py_ssize_t where = -1, n = Py_SIZE(self);

    if (!PyArg_ParseTuple(args, "|n:pop", &where))
        return NULL;

    if (n == 0) {
        PyErr_SetString(PyExc_IndexError,
                        "pop from empty bytearray");
        return NULL;
    }
    if (where < 0)
        where += Py_SIZE(self);
    if (where < 0 || where >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    if (!_canresize(self))
        return NULL;

    value = self->ob_bytes[where];
    memmove(self->ob_bytes + where, self->ob_bytes + where + 1, n - where);
    if (PyByteArray_Resize((PyObject *)self, n - 1) < 0)
        return NULL;

    return PyInt_FromLong((unsigned char)value);
}

PyDoc_STRVAR(remove__doc__,
"B.remove(int) -> None\n\
\n\
Remove the first occurance of a value in B.");
static PyObject *
bytearray_remove(PyByteArrayObject *self, PyObject *arg)
{
    int value;
    Py_ssize_t where, n = Py_SIZE(self);

    if (! _getbytevalue(arg, &value))
        return NULL;

    for (where = 0; where < n; where++) {
        if (self->ob_bytes[where] == value)
            break;
    }
    if (where == n) {
        PyErr_SetString(PyExc_ValueError, "value not found in bytearray");
        return NULL;
    }
    if (!_canresize(self))
        return NULL;

    memmove(self->ob_bytes + where, self->ob_bytes + where + 1, n - where);
    if (PyByteArray_Resize((PyObject *)self, n - 1) < 0)
        return NULL;

    Py_RETURN_NONE;
}

/* XXX These two helpers could be optimized if argsize == 1 */

static Py_ssize_t
lstrip_helper(unsigned char *myptr, Py_ssize_t mysize,
              void *argptr, Py_ssize_t argsize)
{
    Py_ssize_t i = 0;
    while (i < mysize && memchr(argptr, myptr[i], argsize))
        i++;
    return i;
}

static Py_ssize_t
rstrip_helper(unsigned char *myptr, Py_ssize_t mysize,
              void *argptr, Py_ssize_t argsize)
{
    Py_ssize_t i = mysize - 1;
    while (i >= 0 && memchr(argptr, myptr[i], argsize))
        i--;
    return i + 1;
}

PyDoc_STRVAR(strip__doc__,
"B.strip([bytes]) -> bytearray\n\
\n\
Strip leading and trailing bytes contained in the argument.\n\
If the argument is omitted, strip ASCII whitespace.");
static PyObject *
bytearray_strip(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    PyObject *arg = Py_None;
    Py_buffer varg;
    if (!PyArg_ParseTuple(args, "|O:strip", &arg))
        return NULL;
    if (arg == Py_None) {
        argptr = "\t\n\r\f\v ";
        argsize = 6;
    }
    else {
        if (_getbuffer(arg, &varg) < 0)
            return NULL;
        argptr = varg.buf;
        argsize = varg.len;
    }
    myptr = self->ob_bytes;
    mysize = Py_SIZE(self);
    left = lstrip_helper(myptr, mysize, argptr, argsize);
    if (left == mysize)
        right = left;
    else
        right = rstrip_helper(myptr, mysize, argptr, argsize);
    if (arg != Py_None)
        PyBuffer_Release(&varg);
    return PyByteArray_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(lstrip__doc__,
"B.lstrip([bytes]) -> bytearray\n\
\n\
Strip leading bytes contained in the argument.\n\
If the argument is omitted, strip leading ASCII whitespace.");
static PyObject *
bytearray_lstrip(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    PyObject *arg = Py_None;
    Py_buffer varg;
    if (!PyArg_ParseTuple(args, "|O:lstrip", &arg))
        return NULL;
    if (arg == Py_None) {
        argptr = "\t\n\r\f\v ";
        argsize = 6;
    }
    else {
        if (_getbuffer(arg, &varg) < 0)
            return NULL;
        argptr = varg.buf;
        argsize = varg.len;
    }
    myptr = self->ob_bytes;
    mysize = Py_SIZE(self);
    left = lstrip_helper(myptr, mysize, argptr, argsize);
    right = mysize;
    if (arg != Py_None)
        PyBuffer_Release(&varg);
    return PyByteArray_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(rstrip__doc__,
"B.rstrip([bytes]) -> bytearray\n\
\n\
Strip trailing bytes contained in the argument.\n\
If the argument is omitted, strip trailing ASCII whitespace.");
static PyObject *
bytearray_rstrip(PyByteArrayObject *self, PyObject *args)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    PyObject *arg = Py_None;
    Py_buffer varg;
    if (!PyArg_ParseTuple(args, "|O:rstrip", &arg))
        return NULL;
    if (arg == Py_None) {
        argptr = "\t\n\r\f\v ";
        argsize = 6;
    }
    else {
        if (_getbuffer(arg, &varg) < 0)
            return NULL;
        argptr = varg.buf;
        argsize = varg.len;
    }
    myptr = self->ob_bytes;
    mysize = Py_SIZE(self);
    left = 0;
    right = rstrip_helper(myptr, mysize, argptr, argsize);
    if (arg != Py_None)
        PyBuffer_Release(&varg);
    return PyByteArray_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(decode_doc,
"B.decode([encoding[, errors]]) -> unicode object.\n\
\n\
Decodes B using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme.  Default is 'strict' meaning that encoding errors raise\n\
a UnicodeDecodeError.  Other possible values are 'ignore' and 'replace'\n\
as well as any other name registered with codecs.register_error that is\n\
able to handle UnicodeDecodeErrors.");

static PyObject *
bytearray_decode(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *encoding = NULL;
    const char *errors = NULL;
    static char *kwlist[] = {"encoding", "errors", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ss:decode", kwlist, &encoding, &errors))
        return NULL;
    if (encoding == NULL) {
#ifdef Py_USING_UNICODE
        encoding = PyUnicode_GetDefaultEncoding();
#else
        PyErr_SetString(PyExc_ValueError, "no encoding specified");
        return NULL;
#endif
    }
    return PyCodec_Decode(self, encoding, errors);
}

PyDoc_STRVAR(alloc_doc,
"B.__alloc__() -> int\n\
\n\
Returns the number of bytes actually allocated.");

static PyObject *
bytearray_alloc(PyByteArrayObject *self)
{
    return PyInt_FromSsize_t(self->ob_alloc);
}

PyDoc_STRVAR(join_doc,
"B.join(iterable_of_bytes) -> bytes\n\
\n\
Concatenates any number of bytearray objects, with B in between each pair.");

static PyObject *
bytearray_join(PyByteArrayObject *self, PyObject *it)
{
    PyObject *seq;
    Py_ssize_t mysize = Py_SIZE(self);
    Py_ssize_t i;
    Py_ssize_t n;
    PyObject **items;
    Py_ssize_t totalsize = 0;
    PyObject *result;
    char *dest;

    seq = PySequence_Fast(it, "can only join an iterable");
    if (seq == NULL)
        return NULL;
    n = PySequence_Fast_GET_SIZE(seq);
    items = PySequence_Fast_ITEMS(seq);

    /* Compute the total size, and check that they are all bytes */
    /* XXX Shouldn't we use _getbuffer() on these items instead? */
    for (i = 0; i < n; i++) {
        PyObject *obj = items[i];
        if (!PyByteArray_Check(obj) && !PyBytes_Check(obj)) {
            PyErr_Format(PyExc_TypeError,
                         "can only join an iterable of bytes "
                         "(item %ld has type '%.100s')",
                         /* XXX %ld isn't right on Win64 */
                         (long)i, Py_TYPE(obj)->tp_name);
            goto error;
        }
        if (i > 0)
            totalsize += mysize;
        totalsize += Py_SIZE(obj);
        if (totalsize < 0) {
            PyErr_NoMemory();
            goto error;
        }
    }

    /* Allocate the result, and copy the bytes */
    result = PyByteArray_FromStringAndSize(NULL, totalsize);
    if (result == NULL)
        goto error;
    dest = PyByteArray_AS_STRING(result);
    for (i = 0; i < n; i++) {
        PyObject *obj = items[i];
        Py_ssize_t size = Py_SIZE(obj);
        char *buf;
        if (PyByteArray_Check(obj))
           buf = PyByteArray_AS_STRING(obj);
        else
           buf = PyBytes_AS_STRING(obj);
        if (i) {
            memcpy(dest, self->ob_bytes, mysize);
            dest += mysize;
        }
        memcpy(dest, buf, size);
        dest += size;
    }

    /* Done */
    Py_DECREF(seq);
    return result;

    /* Error handling */
  error:
    Py_DECREF(seq);
    return NULL;
}

PyDoc_STRVAR(splitlines__doc__,
"B.splitlines(keepends=False) -> list of lines\n\
\n\
Return a list of the lines in B, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.");

static PyObject*
bytearray_splitlines(PyObject *self, PyObject *args)
{
    int keepends = 0;

    if (!PyArg_ParseTuple(args, "|i:splitlines", &keepends))
        return NULL;

    return stringlib_splitlines(
        (PyObject*) self, PyByteArray_AS_STRING(self),
        PyByteArray_GET_SIZE(self), keepends
        );
}

PyDoc_STRVAR(fromhex_doc,
"bytearray.fromhex(string) -> bytearray\n\
\n\
Create a bytearray object from a string of hexadecimal numbers.\n\
Spaces between two numbers are accepted.\n\
Example: bytearray.fromhex('B9 01EF') -> bytearray(b'\\xb9\\x01\\xef').");

static int
hex_digit_to_int(char c)
{
    if (Py_ISDIGIT(c))
        return c - '0';
    else {
        if (Py_ISUPPER(c))
            c = Py_TOLOWER(c);
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
    }
    return -1;
}

static PyObject *
bytearray_fromhex(PyObject *cls, PyObject *args)
{
    PyObject *newbytes;
    char *buf;
    char *hex;
    Py_ssize_t hexlen, byteslen, i, j;
    int top, bot;

    if (!PyArg_ParseTuple(args, "s#:fromhex", &hex, &hexlen))
        return NULL;
    byteslen = hexlen/2; /* This overestimates if there are spaces */
    newbytes = PyByteArray_FromStringAndSize(NULL, byteslen);
    if (!newbytes)
        return NULL;
    buf = PyByteArray_AS_STRING(newbytes);
    for (i = j = 0; i < hexlen; i += 2) {
        /* skip over spaces in the input */
        while (hex[i] == ' ')
            i++;
        if (i >= hexlen)
            break;
        top = hex_digit_to_int(hex[i]);
        bot = hex_digit_to_int(hex[i+1]);
        if (top == -1 || bot == -1) {
            PyErr_Format(PyExc_ValueError,
                         "non-hexadecimal number found in "
                         "fromhex() arg at position %zd", i);
            goto error;
        }
        buf[j++] = (top << 4) + bot;
    }
    if (PyByteArray_Resize(newbytes, j) < 0)
        goto error;
    return newbytes;

  error:
    Py_DECREF(newbytes);
    return NULL;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
bytearray_reduce(PyByteArrayObject *self)
{
    PyObject *latin1, *dict;
    if (self->ob_bytes)
#ifdef Py_USING_UNICODE
        latin1 = PyUnicode_DecodeLatin1(self->ob_bytes,
                                        Py_SIZE(self), NULL);
#else
        latin1 = PyString_FromStringAndSize(self->ob_bytes, Py_SIZE(self));
#endif
    else
#ifdef Py_USING_UNICODE
        latin1 = PyUnicode_FromString("");
#else
        latin1 = PyString_FromString("");
#endif

    dict = PyObject_GetAttrString((PyObject *)self, "__dict__");
    if (dict == NULL) {
        PyErr_Clear();
        dict = Py_None;
        Py_INCREF(dict);
    }

    return Py_BuildValue("(O(Ns)N)", Py_TYPE(self), latin1, "latin-1", dict);
}

PyDoc_STRVAR(sizeof_doc,
"B.__sizeof__() -> int\n\
 \n\
Returns the size of B in memory, in bytes");
static PyObject *
bytearray_sizeof(PyByteArrayObject *self)
{
    Py_ssize_t res;

    res = sizeof(PyByteArrayObject) + self->ob_alloc * sizeof(char);
    return PyInt_FromSsize_t(res);
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
    (readbufferproc)bytearray_buffer_getreadbuf,
    (writebufferproc)bytearray_buffer_getwritebuf,
    (segcountproc)bytearray_buffer_getsegcount,
    (charbufferproc)bytearray_buffer_getcharbuf,
    (getbufferproc)bytearray_getbuffer,
    (releasebufferproc)bytearray_releasebuffer,
};

static PyMethodDef
bytearray_methods[] = {
    {"__alloc__", (PyCFunction)bytearray_alloc, METH_NOARGS, alloc_doc},
    {"__reduce__", (PyCFunction)bytearray_reduce, METH_NOARGS, reduce_doc},
    {"__sizeof__", (PyCFunction)bytearray_sizeof, METH_NOARGS, sizeof_doc},
    {"append", (PyCFunction)bytearray_append, METH_O, append__doc__},
    {"capitalize", (PyCFunction)stringlib_capitalize, METH_NOARGS,
     _Py_capitalize__doc__},
    {"center", (PyCFunction)stringlib_center, METH_VARARGS, center__doc__},
    {"count", (PyCFunction)bytearray_count, METH_VARARGS, count__doc__},
    {"decode", (PyCFunction)bytearray_decode, METH_VARARGS | METH_KEYWORDS, decode_doc},
    {"endswith", (PyCFunction)bytearray_endswith, METH_VARARGS, endswith__doc__},
    {"expandtabs", (PyCFunction)stringlib_expandtabs, METH_VARARGS,
     expandtabs__doc__},
    {"extend", (PyCFunction)bytearray_extend, METH_O, extend__doc__},
    {"find", (PyCFunction)bytearray_find, METH_VARARGS, find__doc__},
    {"fromhex", (PyCFunction)bytearray_fromhex, METH_VARARGS|METH_CLASS,
     fromhex_doc},
    {"index", (PyCFunction)bytearray_index, METH_VARARGS, index__doc__},
    {"insert", (PyCFunction)bytearray_insert, METH_VARARGS, insert__doc__},
    {"isalnum", (PyCFunction)stringlib_isalnum, METH_NOARGS,
     _Py_isalnum__doc__},
    {"isalpha", (PyCFunction)stringlib_isalpha, METH_NOARGS,
     _Py_isalpha__doc__},
    {"isdigit", (PyCFunction)stringlib_isdigit, METH_NOARGS,
     _Py_isdigit__doc__},
    {"islower", (PyCFunction)stringlib_islower, METH_NOARGS,
     _Py_islower__doc__},
    {"isspace", (PyCFunction)stringlib_isspace, METH_NOARGS,
     _Py_isspace__doc__},
    {"istitle", (PyCFunction)stringlib_istitle, METH_NOARGS,
     _Py_istitle__doc__},
    {"isupper", (PyCFunction)stringlib_isupper, METH_NOARGS,
     _Py_isupper__doc__},
    {"join", (PyCFunction)bytearray_join, METH_O, join_doc},
    {"ljust", (PyCFunction)stringlib_ljust, METH_VARARGS, ljust__doc__},
    {"lower", (PyCFunction)stringlib_lower, METH_NOARGS, _Py_lower__doc__},
    {"lstrip", (PyCFunction)bytearray_lstrip, METH_VARARGS, lstrip__doc__},
    {"partition", (PyCFunction)bytearray_partition, METH_O, partition__doc__},
    {"pop", (PyCFunction)bytearray_pop, METH_VARARGS, pop__doc__},
    {"remove", (PyCFunction)bytearray_remove, METH_O, remove__doc__},
    {"replace", (PyCFunction)bytearray_replace, METH_VARARGS, replace__doc__},
    {"reverse", (PyCFunction)bytearray_reverse, METH_NOARGS, reverse__doc__},
    {"rfind", (PyCFunction)bytearray_rfind, METH_VARARGS, rfind__doc__},
    {"rindex", (PyCFunction)bytearray_rindex, METH_VARARGS, rindex__doc__},
    {"rjust", (PyCFunction)stringlib_rjust, METH_VARARGS, rjust__doc__},
    {"rpartition", (PyCFunction)bytearray_rpartition, METH_O, rpartition__doc__},
    {"rsplit", (PyCFunction)bytearray_rsplit, METH_VARARGS, rsplit__doc__},
    {"rstrip", (PyCFunction)bytearray_rstrip, METH_VARARGS, rstrip__doc__},
    {"split", (PyCFunction)bytearray_split, METH_VARARGS, split__doc__},
    {"splitlines", (PyCFunction)bytearray_splitlines, METH_VARARGS,
     splitlines__doc__},
    {"startswith", (PyCFunction)bytearray_startswith, METH_VARARGS ,
     startswith__doc__},
    {"strip", (PyCFunction)bytearray_strip, METH_VARARGS, strip__doc__},
    {"swapcase", (PyCFunction)stringlib_swapcase, METH_NOARGS,
     _Py_swapcase__doc__},
    {"title", (PyCFunction)stringlib_title, METH_NOARGS, _Py_title__doc__},
    {"translate", (PyCFunction)bytearray_translate, METH_VARARGS,
     translate__doc__},
    {"upper", (PyCFunction)stringlib_upper, METH_NOARGS, _Py_upper__doc__},
    {"zfill", (PyCFunction)stringlib_zfill, METH_VARARGS, zfill__doc__},
    {NULL}
};

PyDoc_STRVAR(bytearray_doc,
"bytearray(iterable_of_ints) -> bytearray.\n\
bytearray(string, encoding[, errors]) -> bytearray.\n\
bytearray(bytes_or_bytearray) -> mutable copy of bytes_or_bytearray.\n\
bytearray(memory_view) -> bytearray.\n\
\n\
Construct an mutable bytearray object from:\n\
  - an iterable yielding integers in range(256)\n\
  - a text string encoded using the specified encoding\n\
  - a bytes or a bytearray object\n\
  - any object implementing the buffer API.\n\
\n\
bytearray(int) -> bytearray.\n\
\n\
Construct a zero-initialized bytearray of the given length.");


static PyObject *bytearray_iter(PyObject *seq);

PyTypeObject PyByteArray_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bytearray",
    sizeof(PyByteArrayObject),
    0,
    (destructor)bytearray_dealloc,       /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    (reprfunc)bytearray_repr,           /* tp_repr */
    0,                                  /* tp_as_number */
    &bytearray_as_sequence,             /* tp_as_sequence */
    &bytearray_as_mapping,              /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    bytearray_str,                      /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    &bytearray_as_buffer,               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_NEWBUFFER,          /* tp_flags */
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
        item = PyInt_FromLong(
            (unsigned char)seq->ob_bytes[it->it_index]);
        if (item != NULL)
            ++it->it_index;
        return item;
    }

    Py_DECREF(seq);
    it->it_seq = NULL;
    return NULL;
}

static PyObject *
bytesarrayiter_length_hint(bytesiterobject *it)
{
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyByteArray_GET_SIZE(it->it_seq) - it->it_index;
    return PyInt_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
    "Private method returning an estimate of len(list(it)).");

static PyMethodDef bytearrayiter_methods[] = {
    {"__length_hint__", (PyCFunction)bytesarrayiter_length_hint, METH_NOARGS,
     length_hint_doc},
    {NULL, NULL} /* sentinel */
};

PyTypeObject PyByteArrayIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bytearray_iterator",              /* tp_name */
    sizeof(bytesiterobject),           /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)bytearrayiter_dealloc, /* tp_dealloc */
    0,                                 /* tp_print */
    0,                                 /* tp_getattr */
    0,                                 /* tp_setattr */
    0,                                 /* tp_compare */
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

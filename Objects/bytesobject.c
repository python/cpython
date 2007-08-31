/* Bytes object implementation */

/* XXX TO DO: optimizations */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

/* The nullbytes are used by the stringlib during partition.
 * If partition is removed from bytes, nullbytes and its helper
 * Init/Fini should also be removed.
 */
static PyBytesObject *nullbytes = NULL;

void
PyBytes_Fini(void)
{
    Py_CLEAR(nullbytes);
}

int
PyBytes_Init(void)
{
    nullbytes = PyObject_New(PyBytesObject, &PyBytes_Type);
    if (nullbytes == NULL)
        return 0;
    nullbytes->ob_bytes = NULL;
    Py_Size(nullbytes) = nullbytes->ob_alloc = 0;
    nullbytes->ob_exports = 0;
    return 1;
}

/* end nullbytes support */

/* Helpers */

static int
_getbytevalue(PyObject* arg, int *value)
{
    PyObject *intarg = PyNumber_Int(arg);
    if (! intarg)
        return 0;
    *value = PyInt_AsLong(intarg);
    Py_DECREF(intarg);
    if (*value < 0 || *value >= 256) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return 0;
    }
    return 1;
}

static int
bytes_getbuffer(PyBytesObject *obj, PyBuffer *view, int flags)
{
        int ret;
        void *ptr;
        if (view == NULL) {
                obj->ob_exports++;
                return 0;
        }
        if (obj->ob_bytes == NULL)
                ptr = "";
        else
                ptr = obj->ob_bytes;
        ret = PyBuffer_FillInfo(view, ptr, Py_Size(obj), 0, flags);
        if (ret >= 0) {
                obj->ob_exports++;
        }
        return ret;
}

static void
bytes_releasebuffer(PyBytesObject *obj, PyBuffer *view)
{
        obj->ob_exports--;
}

static Py_ssize_t
_getbuffer(PyObject *obj, PyBuffer *view)
{
    PyBufferProcs *buffer = Py_Type(obj)->tp_as_buffer;

    if (buffer == NULL ||
        PyUnicode_Check(obj) ||
        buffer->bf_getbuffer == NULL)
    {
        PyErr_Format(PyExc_TypeError,
                     "Type %.100s doesn't support the buffer API",
                     Py_Type(obj)->tp_name);
        return -1;
    }

    if (buffer->bf_getbuffer(obj, view, PyBUF_SIMPLE) < 0)
            return -1;
    return view->len;
}

/* Direct API functions */

PyObject *
PyBytes_FromObject(PyObject *input)
{
    return PyObject_CallFunctionObjArgs((PyObject *)&PyBytes_Type,
                                        input, NULL);
}

PyObject *
PyBytes_FromStringAndSize(const char *bytes, Py_ssize_t size)
{
    PyBytesObject *new;
    int alloc;

    assert(size >= 0);

    new = PyObject_New(PyBytesObject, &PyBytes_Type);
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
        if (bytes != NULL)
            memcpy(new->ob_bytes, bytes, size);
        new->ob_bytes[size] = '\0';  /* Trailing null byte */
    }
    Py_Size(new) = size;
    new->ob_alloc = alloc;
    new->ob_exports = 0;

    return (PyObject *)new;
}

Py_ssize_t
PyBytes_Size(PyObject *self)
{
    assert(self != NULL);
    assert(PyBytes_Check(self));

    return PyBytes_GET_SIZE(self);
}

char  *
PyBytes_AsString(PyObject *self)
{
    assert(self != NULL);
    assert(PyBytes_Check(self));

    return PyBytes_AS_STRING(self);
}

int
PyBytes_Resize(PyObject *self, Py_ssize_t size)
{
    void *sval;
    Py_ssize_t alloc = ((PyBytesObject *)self)->ob_alloc;

    assert(self != NULL);
    assert(PyBytes_Check(self));
    assert(size >= 0);

    if (size < alloc / 2) {
        /* Major downsize; resize down to exact size */
        alloc = size + 1;
    }
    else if (size < alloc) {
        /* Within allocated size; quick exit */
        Py_Size(self) = size;
        ((PyBytesObject *)self)->ob_bytes[size] = '\0'; /* Trailing null */
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

    if (((PyBytesObject *)self)->ob_exports > 0) {
            /*
            fprintf(stderr, "%d: %s", ((PyBytesObject *)self)->ob_exports,
                    ((PyBytesObject *)self)->ob_bytes);
            */
            PyErr_SetString(PyExc_BufferError,
                    "Existing exports of data: object cannot be re-sized");
            return -1;
    }

    sval = PyMem_Realloc(((PyBytesObject *)self)->ob_bytes, alloc);
    if (sval == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    ((PyBytesObject *)self)->ob_bytes = sval;
    Py_Size(self) = size;
    ((PyBytesObject *)self)->ob_alloc = alloc;
    ((PyBytesObject *)self)->ob_bytes[size] = '\0'; /* Trailing null byte */

    return 0;
}

PyObject *
PyBytes_Concat(PyObject *a, PyObject *b)
{
    Py_ssize_t size;
    PyBuffer va, vb;
    PyBytesObject *result;

    va.len = -1;
    vb.len = -1;
    if (_getbuffer(a, &va) < 0  ||
        _getbuffer(b, &vb) < 0) {
            if (va.len != -1)
                    PyObject_ReleaseBuffer(a, &va);
            if (vb.len != -1)
                    PyObject_ReleaseBuffer(b, &vb);
            PyErr_Format(PyExc_TypeError, "can't concat %.100s to %.100s",
                         Py_Type(a)->tp_name, Py_Type(b)->tp_name);
            return NULL;
    }

    size = va.len + vb.len;
    if (size < 0) {
            PyObject_ReleaseBuffer(a, &va);
            PyObject_ReleaseBuffer(b, &vb);
            return PyErr_NoMemory();
    }

    result = (PyBytesObject *) PyBytes_FromStringAndSize(NULL, size);
    if (result != NULL) {
        memcpy(result->ob_bytes, va.buf, va.len);
        memcpy(result->ob_bytes + va.len, vb.buf, vb.len);
    }

    PyObject_ReleaseBuffer(a, &va);
    PyObject_ReleaseBuffer(b, &vb);
    return (PyObject *)result;
}

/* Functions stuffed into the type object */

static Py_ssize_t
bytes_length(PyBytesObject *self)
{
    return Py_Size(self);
}

static PyObject *
bytes_concat(PyBytesObject *self, PyObject *other)
{
    return PyBytes_Concat((PyObject *)self, other);
}

static PyObject *
bytes_iconcat(PyBytesObject *self, PyObject *other)
{
    Py_ssize_t mysize;
    Py_ssize_t size;
    PyBuffer vo;

    if (_getbuffer(other, &vo) < 0) {
        PyErr_Format(PyExc_TypeError, "can't concat bytes to %.100s",
                     Py_Type(self)->tp_name);
        return NULL;
    }

    mysize = Py_Size(self);
    size = mysize + vo.len;
    if (size < 0) {
        PyObject_ReleaseBuffer(other, &vo);
        return PyErr_NoMemory();
    }
    if (size < self->ob_alloc) {
        Py_Size(self) = size;
        self->ob_bytes[Py_Size(self)] = '\0'; /* Trailing null byte */
    }
    else if (PyBytes_Resize((PyObject *)self, size) < 0) {
        PyObject_ReleaseBuffer(other, &vo);
        return NULL;
    }
    memcpy(self->ob_bytes + mysize, vo.buf, vo.len);
    PyObject_ReleaseBuffer(other, &vo);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
bytes_repeat(PyBytesObject *self, Py_ssize_t count)
{
    PyBytesObject *result;
    Py_ssize_t mysize;
    Py_ssize_t size;

    if (count < 0)
        count = 0;
    mysize = Py_Size(self);
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    result = (PyBytesObject *)PyBytes_FromStringAndSize(NULL, size);
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
bytes_irepeat(PyBytesObject *self, Py_ssize_t count)
{
    Py_ssize_t mysize;
    Py_ssize_t size;

    if (count < 0)
        count = 0;
    mysize = Py_Size(self);
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    if (size < self->ob_alloc) {
        Py_Size(self) = size;
        self->ob_bytes[Py_Size(self)] = '\0'; /* Trailing null byte */
    }
    else if (PyBytes_Resize((PyObject *)self, size) < 0)
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

static int
bytes_substring(PyBytesObject *self, PyBytesObject *other)
{
    Py_ssize_t i;

    if (Py_Size(other) == 1) {
        return memchr(self->ob_bytes, other->ob_bytes[0],
                      Py_Size(self)) != NULL;
    }
    if (Py_Size(other) == 0)
        return 1; /* Edge case */
    for (i = 0; i + Py_Size(other) <= Py_Size(self); i++) {
        /* XXX Yeah, yeah, lots of optimizations possible... */
        if (memcmp(self->ob_bytes + i, other->ob_bytes, Py_Size(other)) == 0)
            return 1;
    }
    return 0;
}

static int
bytes_contains(PyBytesObject *self, PyObject *value)
{
    Py_ssize_t ival;

    if (PyBytes_Check(value))
        return bytes_substring(self, (PyBytesObject *)value);

    ival = PyNumber_AsSsize_t(value, PyExc_ValueError);
    if (ival == -1 && PyErr_Occurred())
        return -1;
    if (ival < 0 || ival >= 256) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return -1;
    }

    return memchr(self->ob_bytes, ival, Py_Size(self)) != NULL;
}

static PyObject *
bytes_getitem(PyBytesObject *self, Py_ssize_t i)
{
    if (i < 0)
        i += Py_Size(self);
    if (i < 0 || i >= Py_Size(self)) {
        PyErr_SetString(PyExc_IndexError, "bytes index out of range");
        return NULL;
    }
    return PyInt_FromLong((unsigned char)(self->ob_bytes[i]));
}

static PyObject *
bytes_subscript(PyBytesObject *self, PyObject *item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return NULL;

        if (i < 0)
            i += PyBytes_GET_SIZE(self);

        if (i < 0 || i >= Py_Size(self)) {
            PyErr_SetString(PyExc_IndexError, "bytes index out of range");
            return NULL;
        }
        return PyInt_FromLong((unsigned char)(self->ob_bytes[i]));
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        if (PySlice_GetIndicesEx((PySliceObject *)item,
                                 PyBytes_GET_SIZE(self),
                                 &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0)
            return PyBytes_FromStringAndSize("", 0);
        else if (step == 1) {
            return PyBytes_FromStringAndSize(self->ob_bytes + start,
                                             slicelength);
        }
        else {
            char *source_buf = PyBytes_AS_STRING(self);
            char *result_buf = (char *)PyMem_Malloc(slicelength);
            PyObject *result;

            if (result_buf == NULL)
                return PyErr_NoMemory();

            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                     result_buf[i] = source_buf[cur];
            }
            result = PyBytes_FromStringAndSize(result_buf, slicelength);
            PyMem_Free(result_buf);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "bytes indices must be integers");
        return NULL;
    }
}

static int
bytes_setslice(PyBytesObject *self, Py_ssize_t lo, Py_ssize_t hi,
               PyObject *values)
{
    Py_ssize_t avail, needed;
    void *bytes;
    PyBuffer vbytes;
    int res = 0;

    vbytes.len = -1;
    if (values == (PyObject *)self) {
        /* Make a copy and call this function recursively */
        int err;
        values = PyBytes_FromObject(values);
        if (values == NULL)
            return -1;
        err = bytes_setslice(self, lo, hi, values);
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
                                 "can't set bytes slice from %.100s",
                                 Py_Type(values)->tp_name);
                    return -1;
            }
            needed = vbytes.len;
            bytes = vbytes.buf;
    }

    if (lo < 0)
        lo = 0;
    if (hi < lo)
        hi = lo;
    if (hi > Py_Size(self))
        hi = Py_Size(self);

    avail = hi - lo;
    if (avail < 0)
        lo = hi = avail = 0;

    if (avail != needed) {
        if (avail > needed) {
            /*
              0   lo               hi               old_size
              |   |<----avail----->|<-----tomove------>|
              |   |<-needed->|<-----tomove------>|
              0   lo      new_hi              new_size
            */
            memmove(self->ob_bytes + lo + needed, self->ob_bytes + hi,
                    Py_Size(self) - hi);
        }
        /* XXX(nnorwitz): need to verify this can't overflow! */
        if (PyBytes_Resize((PyObject *)self,
                           Py_Size(self) + needed - avail) < 0) {
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
                    Py_Size(self) - lo - needed);
        }
    }

    if (needed > 0)
        memcpy(self->ob_bytes + lo, bytes, needed);


 finish:
    if (vbytes.len != -1)
            PyObject_ReleaseBuffer(values, &vbytes);
    return res;
}

static int
bytes_setitem(PyBytesObject *self, Py_ssize_t i, PyObject *value)
{
    Py_ssize_t ival;

    if (i < 0)
        i += Py_Size(self);

    if (i < 0 || i >= Py_Size(self)) {
        PyErr_SetString(PyExc_IndexError, "bytes index out of range");
        return -1;
    }

    if (value == NULL)
        return bytes_setslice(self, i, i+1, NULL);

    ival = PyNumber_AsSsize_t(value, PyExc_ValueError);
    if (ival == -1 && PyErr_Occurred())
        return -1;

    if (ival < 0 || ival >= 256) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return -1;
    }

    self->ob_bytes[i] = ival;
    return 0;
}

static int
bytes_ass_subscript(PyBytesObject *self, PyObject *item, PyObject *values)
{
    Py_ssize_t start, stop, step, slicelen, needed;
    char *bytes;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return -1;

        if (i < 0)
            i += PyBytes_GET_SIZE(self);

        if (i < 0 || i >= Py_Size(self)) {
            PyErr_SetString(PyExc_IndexError, "bytes index out of range");
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
            Py_ssize_t ival = PyNumber_AsSsize_t(values, PyExc_ValueError);
            if (ival == -1 && PyErr_Occurred())
                return -1;
            if (ival < 0 || ival >= 256) {
                PyErr_SetString(PyExc_ValueError,
                                "byte must be in range(0, 256)");
                return -1;
            }
            self->ob_bytes[i] = (char)ival;
            return 0;
        }
    }
    else if (PySlice_Check(item)) {
        if (PySlice_GetIndicesEx((PySliceObject *)item,
                                 PyBytes_GET_SIZE(self),
                                 &start, &stop, &step, &slicelen) < 0) {
            return -1;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "bytes indices must be integer");
        return -1;
    }

    if (values == NULL) {
        bytes = NULL;
        needed = 0;
    }
    else if (values == (PyObject *)self || !PyBytes_Check(values)) {
        /* Make a copy an call this function recursively */
        int err;
        values = PyBytes_FromObject(values);
        if (values == NULL)
            return -1;
        err = bytes_ass_subscript(self, item, values);
        Py_DECREF(values);
        return err;
    }
    else {
        assert(PyBytes_Check(values));
        bytes = ((PyBytesObject *)values)->ob_bytes;
        needed = Py_Size(values);
    }
    /* Make sure b[5:2] = ... inserts before 5, not before 2. */
    if ((step < 0 && start < stop) ||
        (step > 0 && start > stop))
        stop = start;
    if (step == 1) {
        if (slicelen != needed) {
            if (slicelen > needed) {
                /*
                  0   start           stop              old_size
                  |   |<---slicelen--->|<-----tomove------>|
                  |   |<-needed->|<-----tomove------>|
                  0   lo      new_hi              new_size
                */
                memmove(self->ob_bytes + start + needed, self->ob_bytes + stop,
                        Py_Size(self) - stop);
            }
            if (PyBytes_Resize((PyObject *)self,
                               Py_Size(self) + needed - slicelen) < 0)
                return -1;
            if (slicelen < needed) {
                /*
                  0   lo        hi               old_size
                  |   |<-avail->|<-----tomove------>|
                  |   |<----needed---->|<-----tomove------>|
                  0   lo            new_hi              new_size
                 */
                memmove(self->ob_bytes + start + needed, self->ob_bytes + stop,
                        Py_Size(self) - start - needed);
            }
        }

        if (needed > 0)
            memcpy(self->ob_bytes + start, bytes, needed);

        return 0;
    }
    else {
        if (needed == 0) {
            /* Delete slice */
            Py_ssize_t cur, i;

            if (step < 0) {
                stop = start + 1;
                start = stop + step * (slicelen - 1) - 1;
                step = -step;
            }
            for (cur = start, i = 0;
                 i < slicelen; cur += step, i++) {
                Py_ssize_t lim = step - 1;

                if (cur + step >= PyBytes_GET_SIZE(self))
                    lim = PyBytes_GET_SIZE(self) - cur - 1;

                memmove(self->ob_bytes + cur - i,
                        self->ob_bytes + cur + 1, lim);
            }
            /* Move the tail of the bytes, in one chunk */
            cur = start + slicelen*step;
            if (cur < PyBytes_GET_SIZE(self)) {
                memmove(self->ob_bytes + cur - slicelen,
                        self->ob_bytes + cur,
                        PyBytes_GET_SIZE(self) - cur);
            }
            if (PyBytes_Resize((PyObject *)self,
                               PyBytes_GET_SIZE(self) - slicelen) < 0)
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
bytes_init(PyBytesObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"source", "encoding", "errors", 0};
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;
    Py_ssize_t count;
    PyObject *it;
    PyObject *(*iternext)(PyObject *);

    if (Py_Size(self) != 0) {
        /* Empty previous contents (yes, do this first of all!) */
        if (PyBytes_Resize((PyObject *)self, 0) < 0)
            return -1;
    }

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:bytes", kwlist,
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

    if (PyUnicode_Check(arg)) {
        /* Encode via the codec registry */
        PyObject *encoded, *new;
        if (encoding == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "string argument without an encoding");
            return -1;
        }
        encoded = PyCodec_Encode(arg, encoding, errors);
        if (encoded == NULL)
            return -1;
        if (!PyBytes_Check(encoded) && !PyString_Check(encoded)) {
            PyErr_Format(PyExc_TypeError,
                "encoder did not return a str8 or bytes object (type=%.400s)",
                Py_Type(encoded)->tp_name);
            Py_DECREF(encoded);
            return -1;
        }
        new = bytes_iconcat(self, encoded);
        Py_DECREF(encoded);
        if (new == NULL)
            return -1;
        Py_DECREF(new);
        return 0;
    }

    /* If it's not unicode, there can't be encoding or errors */
    if (encoding != NULL || errors != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding or errors without a string argument");
        return -1;
    }

    /* Is it an int? */
    count = PyNumber_AsSsize_t(arg, PyExc_ValueError);
    if (count == -1 && PyErr_Occurred())
        PyErr_Clear();
    else {
        if (count < 0) {
            PyErr_SetString(PyExc_ValueError, "negative count");
            return -1;
        }
        if (count > 0) {
            if (PyBytes_Resize((PyObject *)self, count))
                return -1;
            memset(self->ob_bytes, 0, count);
        }
        return 0;
    }

    /* Use the modern buffer interface */
    if (PyObject_CheckBuffer(arg)) {
        Py_ssize_t size;
        PyBuffer view;
        if (PyObject_GetBuffer(arg, &view, PyBUF_FULL_RO) < 0)
            return -1;
        size = view.len;
        if (PyBytes_Resize((PyObject *)self, size) < 0) goto fail;
        if (PyBuffer_ToContiguous(self->ob_bytes, &view, size, 'C') < 0)
                goto fail;
        PyObject_ReleaseBuffer(arg, &view);
        return 0;
    fail:
        PyObject_ReleaseBuffer(arg, &view);
        return -1;
    }

    /* XXX Optimize this if the arguments is a list, tuple */

    /* Get the iterator */
    it = PyObject_GetIter(arg);
    if (it == NULL)
        return -1;
    iternext = *Py_Type(it)->tp_iternext;

    /* Run the iterator to exhaustion */
    for (;;) {
        PyObject *item;
        Py_ssize_t value;

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
        value = PyNumber_AsSsize_t(item, PyExc_ValueError);
        Py_DECREF(item);
        if (value == -1 && PyErr_Occurred())
            goto error;

        /* Range check */
        if (value < 0 || value >= 256) {
            PyErr_SetString(PyExc_ValueError,
                            "bytes must be in range(0, 256)");
            goto error;
        }

        /* Append the byte */
        if (Py_Size(self) < self->ob_alloc)
            Py_Size(self)++;
        else if (PyBytes_Resize((PyObject *)self, Py_Size(self)+1) < 0)
            goto error;
        self->ob_bytes[Py_Size(self)-1] = value;
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
bytes_repr(PyBytesObject *self)
{
    static const char *hexdigits = "0123456789abcdef";
    size_t newsize = 3 + 4 * Py_Size(self);
    PyObject *v;
    if (newsize > PY_SSIZE_T_MAX || newsize / 4 != Py_Size(self)) {
        PyErr_SetString(PyExc_OverflowError,
            "bytes object is too large to make repr");
        return NULL;
    }
    v = PyUnicode_FromUnicode(NULL, newsize);
    if (v == NULL) {
        return NULL;
    }
    else {
        register Py_ssize_t i;
        register Py_UNICODE c;
        register Py_UNICODE *p;
        int quote = '\'';

        p = PyUnicode_AS_UNICODE(v);
        *p++ = 'b';
        *p++ = quote;
        for (i = 0; i < Py_Size(self); i++) {
            /* There's at least enough room for a hex escape
               and a closing quote. */
            assert(newsize - (p - PyUnicode_AS_UNICODE(v)) >= 5);
            c = self->ob_bytes[i];
            if (c == quote || c == '\\')
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
        assert(newsize - (p - PyUnicode_AS_UNICODE(v)) >= 1);
        *p++ = quote;
        *p = '\0';
        if (PyUnicode_Resize(&v, (p - PyUnicode_AS_UNICODE(v)))) {
            Py_DECREF(v);
            return NULL;
        }
        return v;
    }
}

static PyObject *
bytes_str(PyBytesObject *self)
{
    return PyString_FromStringAndSize(self->ob_bytes, Py_Size(self));
}

static PyObject *
bytes_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_ssize_t self_size, other_size;
    PyBuffer self_bytes, other_bytes;
    PyObject *res;
    Py_ssize_t minsize;
    int cmp;

    /* Bytes can be compared to anything that supports the (binary)
       buffer API.  Except that a comparison with Unicode is always an
       error, even if the comparison is for equality. */
    if (PyObject_IsInstance(self, (PyObject*)&PyUnicode_Type) ||
        PyObject_IsInstance(other, (PyObject*)&PyUnicode_Type)) {
            PyErr_SetString(PyExc_TypeError, "can't compare bytes and str");
            return NULL;
    }

    self_size = _getbuffer(self, &self_bytes);
    if (self_size < 0) {
        PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    other_size = _getbuffer(other, &other_bytes);
    if (other_size < 0) {
        PyErr_Clear();
        PyObject_ReleaseBuffer(self, &self_bytes);
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
    PyObject_ReleaseBuffer(self, &self_bytes);
    PyObject_ReleaseBuffer(other, &other_bytes);
    Py_INCREF(res);
    return res;
}

static void
bytes_dealloc(PyBytesObject *self)
{
    if (self->ob_bytes != 0) {
        PyMem_Free(self->ob_bytes);
    }
    Py_Type(self)->tp_free((PyObject *)self);
}


/* -------------------------------------------------------------------- */
/* Methods */

#define STRINGLIB_CHAR char
#define STRINGLIB_CMP memcmp
#define STRINGLIB_LEN PyBytes_GET_SIZE
#define STRINGLIB_NEW PyBytes_FromStringAndSize
#define STRINGLIB_EMPTY nullbytes

#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/partition.h"


/* The following Py_LOCAL_INLINE and Py_LOCAL functions
were copied from the old char* style string object. */

Py_LOCAL_INLINE(void)
_adjust_indices(Py_ssize_t *start, Py_ssize_t *end, Py_ssize_t len)
{
    if (*end > len)
        *end = len;
    else if (*end < 0)
        *end += len;
    if (*end < 0)
        *end = 0;
    if (*start < 0)
        *start += len;
    if (*start < 0)
        *start = 0;
}


Py_LOCAL_INLINE(Py_ssize_t)
bytes_find_internal(PyBytesObject *self, PyObject *args, int dir)
{
    PyObject *subobj;
    PyBuffer subbuf;
    Py_ssize_t start=0, end=PY_SSIZE_T_MAX;
    Py_ssize_t res;

    if (!PyArg_ParseTuple(args, "O|O&O&:find/rfind/index/rindex", &subobj,
        _PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return -2;
    if (_getbuffer(subobj, &subbuf) < 0)
        return -2;
    if (dir > 0)
        res = stringlib_find_slice(
            PyBytes_AS_STRING(self), PyBytes_GET_SIZE(self),
            subbuf.buf, subbuf.len, start, end);
    else
        res = stringlib_rfind_slice(
            PyBytes_AS_STRING(self), PyBytes_GET_SIZE(self),
            subbuf.buf, subbuf.len, start, end);
    PyObject_ReleaseBuffer(subobj, &subbuf);
    return res;
}


PyDoc_STRVAR(find__doc__,
"B.find(sub [,start [,end]]) -> int\n\
\n\
Return the lowest index in B where subsection sub is found,\n\
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
bytes_find(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t result = bytes_find_internal(self, args, +1);
    if (result == -2)
        return NULL;
    return PyInt_FromSsize_t(result);
}

PyDoc_STRVAR(count__doc__,
"B.count(sub[, start[, end]]) -> int\n\
\n\
Return the number of non-overlapping occurrences of subsection sub in\n\
bytes B[start:end].  Optional arguments start and end are interpreted\n\
as in slice notation.");

static PyObject *
bytes_count(PyBytesObject *self, PyObject *args)
{
    PyObject *sub_obj;
    const char *str = PyBytes_AS_STRING(self), *sub;
    Py_ssize_t sub_len;
    Py_ssize_t start = 0, end = PY_SSIZE_T_MAX;

    if (!PyArg_ParseTuple(args, "O|O&O&:count", &sub_obj,
        _PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;

    if (PyBytes_Check(sub_obj)) {
        sub = PyBytes_AS_STRING(sub_obj);
        sub_len = PyBytes_GET_SIZE(sub_obj);
    }
    /* XXX --> use the modern buffer interface */
    else if (PyObject_AsCharBuffer(sub_obj, &sub, &sub_len))
        return NULL;

    _adjust_indices(&start, &end, PyBytes_GET_SIZE(self));

    return PyInt_FromSsize_t(
        stringlib_count(str + start, end - start, sub, sub_len)
        );
}


PyDoc_STRVAR(index__doc__,
"B.index(sub [,start [,end]]) -> int\n\
\n\
Like B.find() but raise ValueError when the subsection is not found.");

static PyObject *
bytes_index(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t result = bytes_find_internal(self, args, +1);
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
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
bytes_rfind(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t result = bytes_find_internal(self, args, -1);
    if (result == -2)
        return NULL;
    return PyInt_FromSsize_t(result);
}


PyDoc_STRVAR(rindex__doc__,
"B.rindex(sub [,start [,end]]) -> int\n\
\n\
Like B.rfind() but raise ValueError when the subsection is not found.");

static PyObject *
bytes_rindex(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t result = bytes_find_internal(self, args, -1);
    if (result == -2)
        return NULL;
    if (result == -1) {
        PyErr_SetString(PyExc_ValueError,
                        "subsection not found");
        return NULL;
    }
    return PyInt_FromSsize_t(result);
}


/* Matches the end (direction >= 0) or start (direction < 0) of self
 * against substr, using the start and end arguments. Returns
 * -1 on error, 0 if not found and 1 if found.
 */
Py_LOCAL(int)
_bytes_tailmatch(PyBytesObject *self, PyObject *substr, Py_ssize_t start,
                 Py_ssize_t end, int direction)
{
    Py_ssize_t len = PyBytes_GET_SIZE(self);
    Py_ssize_t slen;
    const char* sub;
    const char* str;

    if (PyBytes_Check(substr)) {
        sub = PyBytes_AS_STRING(substr);
        slen = PyBytes_GET_SIZE(substr);
    }
    /* XXX --> Use the modern buffer interface */
    else if (PyObject_AsCharBuffer(substr, &sub, &slen))
        return -1;
    str = PyBytes_AS_STRING(self);

    _adjust_indices(&start, &end, len);

    if (direction < 0) {
        /* startswith */
        if (start+slen > len)
            return 0;
    } else {
        /* endswith */
        if (end-start < slen || start > len)
            return 0;

        if (end-slen > start)
            start = end - slen;
    }
    if (end-start >= slen)
        return ! memcmp(str+start, sub, slen);
    return 0;
}


PyDoc_STRVAR(startswith__doc__,
"B.startswith(prefix[, start[, end]]) -> bool\n\
\n\
Return True if B starts with the specified prefix, False otherwise.\n\
With optional start, test B beginning at that position.\n\
With optional end, stop comparing B at that position.\n\
prefix can also be a tuple of strings to try.");

static PyObject *
bytes_startswith(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *subobj;
    int result;

    if (!PyArg_ParseTuple(args, "O|O&O&:startswith", &subobj,
        _PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            result = _bytes_tailmatch(self,
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
    result = _bytes_tailmatch(self, subobj, start, end, -1);
    if (result == -1)
        return NULL;
    else
        return PyBool_FromLong(result);
}

PyDoc_STRVAR(endswith__doc__,
"B.endswith(suffix[, start[, end]]) -> bool\n\
\n\
Return True if B ends with the specified suffix, False otherwise.\n\
With optional start, test B beginning at that position.\n\
With optional end, stop comparing B at that position.\n\
suffix can also be a tuple of strings to try.");

static PyObject *
bytes_endswith(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *subobj;
    int result;

    if (!PyArg_ParseTuple(args, "O|O&O&:endswith", &subobj,
        _PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            result = _bytes_tailmatch(self,
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
    result = _bytes_tailmatch(self, subobj, start, end, +1);
    if (result == -1)
        return NULL;
    else
        return PyBool_FromLong(result);
}



PyDoc_STRVAR(translate__doc__,
"B.translate(table [,deletechars]) -> bytes\n\
\n\
Return a copy of the bytes B, where all characters occurring\n\
in the optional argument deletechars are removed, and the\n\
remaining characters have been mapped through the given\n\
translation table, which must be a bytes of length 256.");

static PyObject *
bytes_translate(PyBytesObject *self, PyObject *args)
{
    register char *input, *output;
    register const char *table;
    register Py_ssize_t i, c, changed = 0;
    PyObject *input_obj = (PyObject*)self;
    const char *table1, *output_start, *del_table=NULL;
    Py_ssize_t inlen, tablen, dellen = 0;
    PyObject *result;
    int trans_table[256];
    PyObject *tableobj, *delobj = NULL;

    if (!PyArg_UnpackTuple(args, "translate", 1, 2,
                           &tableobj, &delobj))
          return NULL;

    if (PyBytes_Check(tableobj)) {
        table1 = PyBytes_AS_STRING(tableobj);
        tablen = PyBytes_GET_SIZE(tableobj);
    }
    /* XXX -> Use the modern buffer interface */
    else if (PyObject_AsCharBuffer(tableobj, &table1, &tablen))
        return NULL;

    if (tablen != 256) {
        PyErr_SetString(PyExc_ValueError,
                        "translation table must be 256 characters long");
        return NULL;
    }

    if (delobj != NULL) {
        if (PyBytes_Check(delobj)) {
            del_table = PyBytes_AS_STRING(delobj);
            dellen = PyBytes_GET_SIZE(delobj);
        }
        /* XXX -> use the modern buffer interface */
        else if (PyObject_AsCharBuffer(delobj, &del_table, &dellen))
            return NULL;
    }
    else {
        del_table = NULL;
        dellen = 0;
    }

    table = table1;
    inlen = PyBytes_GET_SIZE(input_obj);
    result = PyBytes_FromStringAndSize((char *)NULL, inlen);
    if (result == NULL)
        return NULL;
    output_start = output = PyBytes_AsString(result);
    input = PyBytes_AS_STRING(input_obj);

    if (dellen == 0) {
        /* If no deletions are required, use faster code */
        for (i = inlen; --i >= 0; ) {
            c = Py_CHARMASK(*input++);
            if (Py_CHARMASK((*output++ = table[c])) != c)
                changed = 1;
        }
        if (changed || !PyBytes_CheckExact(input_obj))
            return result;
        Py_DECREF(result);
        Py_INCREF(input_obj);
        return input_obj;
    }

    for (i = 0; i < 256; i++)
        trans_table[i] = Py_CHARMASK(table[i]);

    for (i = 0; i < dellen; i++)
        trans_table[(int) Py_CHARMASK(del_table[i])] = -1;

    for (i = inlen; --i >= 0; ) {
        c = Py_CHARMASK(*input++);
        if (trans_table[c] != -1)
            if (Py_CHARMASK(*output++ = (char)trans_table[c]) == c)
                    continue;
        changed = 1;
    }
    if (!changed && PyBytes_CheckExact(input_obj)) {
        Py_DECREF(result);
        Py_INCREF(input_obj);
        return input_obj;
    }
    /* Fix the size of the resulting string */
    if (inlen > 0)
        PyBytes_Resize(result, output - output_start);
    return result;
}


#define FORWARD 1
#define REVERSE -1

/* find and count characters and substrings */

#define findchar(target, target_len, c)                         \
  ((char *)memchr((const void *)(target), c, target_len))

/* Don't call if length < 2 */
#define Py_STRING_MATCH(target, offset, pattern, length)        \
  (target[offset] == pattern[0] &&                              \
   target[offset+length-1] == pattern[length-1] &&              \
   !memcmp(target+offset+1, pattern+1, length-2) )


/* Bytes ops must return a string.  */
/* If the object is subclass of bytes, create a copy */
Py_LOCAL(PyBytesObject *)
return_self(PyBytesObject *self)
{
    if (PyBytes_CheckExact(self)) {
        Py_INCREF(self);
        return (PyBytesObject *)self;
    }
    return (PyBytesObject *)PyBytes_FromStringAndSize(
            PyBytes_AS_STRING(self),
            PyBytes_GET_SIZE(self));
}

Py_LOCAL_INLINE(Py_ssize_t)
countchar(const char *target, int target_len, char c, Py_ssize_t maxcount)
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

Py_LOCAL(Py_ssize_t)
findstring(const char *target, Py_ssize_t target_len,
           const char *pattern, Py_ssize_t pattern_len,
           Py_ssize_t start,
           Py_ssize_t end,
           int direction)
{
    if (start < 0) {
        start += target_len;
        if (start < 0)
            start = 0;
    }
    if (end > target_len) {
        end = target_len;
    } else if (end < 0) {
        end += target_len;
        if (end < 0)
            end = 0;
    }

    /* zero-length substrings always match at the first attempt */
    if (pattern_len == 0)
        return (direction > 0) ? start : end;

    end -= pattern_len;

    if (direction < 0) {
        for (; end >= start; end--)
            if (Py_STRING_MATCH(target, end, pattern, pattern_len))
                return end;
    } else {
        for (; start <= end; start++)
            if (Py_STRING_MATCH(target, start, pattern, pattern_len))
                return start;
    }
    return -1;
}

Py_LOCAL_INLINE(Py_ssize_t)
countstring(const char *target, Py_ssize_t target_len,
            const char *pattern, Py_ssize_t pattern_len,
            Py_ssize_t start,
            Py_ssize_t end,
            int direction, Py_ssize_t maxcount)
{
    Py_ssize_t count=0;

    if (start < 0) {
        start += target_len;
        if (start < 0)
            start = 0;
    }
    if (end > target_len) {
        end = target_len;
    } else if (end < 0) {
        end += target_len;
        if (end < 0)
            end = 0;
    }

    /* zero-length substrings match everywhere */
    if (pattern_len == 0 || maxcount == 0) {
        if (target_len+1 < maxcount)
            return target_len+1;
        return maxcount;
    }

    end -= pattern_len;
    if (direction < 0) {
        for (; (end >= start); end--)
            if (Py_STRING_MATCH(target, end, pattern, pattern_len)) {
                count++;
                if (--maxcount <= 0) break;
                end -= pattern_len-1;
            }
    } else {
        for (; (start <= end); start++)
            if (Py_STRING_MATCH(target, start, pattern, pattern_len)) {
                count++;
                if (--maxcount <= 0)
                    break;
                start += pattern_len-1;
            }
    }
    return count;
}


/* Algorithms for different cases of string replacement */

/* len(self)>=1, from="", len(to)>=1, maxcount>=1 */
Py_LOCAL(PyBytesObject *)
replace_interleave(PyBytesObject *self,
                   const char *to_s, Py_ssize_t to_len,
                   Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, i, product;
    PyBytesObject *result;

    self_len = PyBytes_GET_SIZE(self);

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

    if (! (result = (PyBytesObject *)
                     PyBytes_FromStringAndSize(NULL, result_len)) )
        return NULL;

    self_s = PyBytes_AS_STRING(self);
    result_s = PyBytes_AS_STRING(result);

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
Py_LOCAL(PyBytesObject *)
replace_delete_single_character(PyBytesObject *self,
                                char from_c, Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count;
    PyBytesObject *result;

    self_len = PyBytes_GET_SIZE(self);
    self_s = PyBytes_AS_STRING(self);

    count = countchar(self_s, self_len, from_c, maxcount);
    if (count == 0) {
        return return_self(self);
    }

    result_len = self_len - count;  /* from_len == 1 */
    assert(result_len>=0);

    if ( (result = (PyBytesObject *)
                    PyBytes_FromStringAndSize(NULL, result_len)) == NULL)
        return NULL;
    result_s = PyBytes_AS_STRING(result);

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

Py_LOCAL(PyBytesObject *)
replace_delete_substring(PyBytesObject *self,
                         const char *from_s, Py_ssize_t from_len,
                         Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset;
    PyBytesObject *result;

    self_len = PyBytes_GET_SIZE(self);
    self_s = PyBytes_AS_STRING(self);

    count = countstring(self_s, self_len,
                        from_s, from_len,
                        0, self_len, 1,
                        maxcount);

    if (count == 0) {
        /* no matches */
        return return_self(self);
    }

    result_len = self_len - (count * from_len);
    assert (result_len>=0);

    if ( (result = (PyBytesObject *)
        PyBytes_FromStringAndSize(NULL, result_len)) == NULL )
            return NULL;

    result_s = PyBytes_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = findstring(start, end-start,
                            from_s, from_len,
                            0, end-start, FORWARD);
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
Py_LOCAL(PyBytesObject *)
replace_single_character_in_place(PyBytesObject *self,
                                  char from_c, char to_c,
                                  Py_ssize_t maxcount)
{
        char *self_s, *result_s, *start, *end, *next;
        Py_ssize_t self_len;
        PyBytesObject *result;

        /* The result string will be the same size */
        self_s = PyBytes_AS_STRING(self);
        self_len = PyBytes_GET_SIZE(self);

        next = findchar(self_s, self_len, from_c);

        if (next == NULL) {
                /* No matches; return the original bytes */
                return return_self(self);
        }

        /* Need to make a new bytes */
        result = (PyBytesObject *) PyBytes_FromStringAndSize(NULL, self_len);
        if (result == NULL)
                return NULL;
        result_s = PyBytes_AS_STRING(result);
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
Py_LOCAL(PyBytesObject *)
replace_substring_in_place(PyBytesObject *self,
                           const char *from_s, Py_ssize_t from_len,
                           const char *to_s, Py_ssize_t to_len,
                           Py_ssize_t maxcount)
{
    char *result_s, *start, *end;
    char *self_s;
    Py_ssize_t self_len, offset;
    PyBytesObject *result;

    /* The result bytes will be the same size */

    self_s = PyBytes_AS_STRING(self);
    self_len = PyBytes_GET_SIZE(self);

    offset = findstring(self_s, self_len,
                        from_s, from_len,
                        0, self_len, FORWARD);
    if (offset == -1) {
        /* No matches; return the original bytes */
        return return_self(self);
    }

    /* Need to make a new bytes */
    result = (PyBytesObject *) PyBytes_FromStringAndSize(NULL, self_len);
    if (result == NULL)
        return NULL;
    result_s = PyBytes_AS_STRING(result);
    Py_MEMCPY(result_s, self_s, self_len);

    /* change everything in-place, starting with this one */
    start =  result_s + offset;
    Py_MEMCPY(start, to_s, from_len);
    start += from_len;
    end = result_s + self_len;

    while ( --maxcount > 0) {
        offset = findstring(start, end-start,
                            from_s, from_len,
                            0, end-start, FORWARD);
        if (offset==-1)
            break;
        Py_MEMCPY(start+offset, to_s, from_len);
        start += offset+from_len;
    }

    return result;
}

/* len(self)>=1, len(from)==1, len(to)>=2, maxcount>=1 */
Py_LOCAL(PyBytesObject *)
replace_single_character(PyBytesObject *self,
                         char from_c,
                         const char *to_s, Py_ssize_t to_len,
                         Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, product;
    PyBytesObject *result;

    self_s = PyBytes_AS_STRING(self);
    self_len = PyBytes_GET_SIZE(self);

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

    if ( (result = (PyBytesObject *)
          PyBytes_FromStringAndSize(NULL, result_len)) == NULL)
            return NULL;
    result_s = PyBytes_AS_STRING(result);

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
Py_LOCAL(PyBytesObject *)
replace_substring(PyBytesObject *self,
                  const char *from_s, Py_ssize_t from_len,
                  const char *to_s, Py_ssize_t to_len,
                  Py_ssize_t maxcount)
{
    char *self_s, *result_s;
    char *start, *next, *end;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset, product;
    PyBytesObject *result;

    self_s = PyBytes_AS_STRING(self);
    self_len = PyBytes_GET_SIZE(self);

    count = countstring(self_s, self_len,
                        from_s, from_len,
                        0, self_len, FORWARD, maxcount);
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

    if ( (result = (PyBytesObject *)
          PyBytes_FromStringAndSize(NULL, result_len)) == NULL)
        return NULL;
    result_s = PyBytes_AS_STRING(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = findstring(start, end-start,
                            from_s, from_len,
                            0, end-start, FORWARD);
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


Py_LOCAL(PyBytesObject *)
replace(PyBytesObject *self,
        const char *from_s, Py_ssize_t from_len,
        const char *to_s, Py_ssize_t to_len,
        Py_ssize_t maxcount)
{
    if (maxcount < 0) {
        maxcount = PY_SSIZE_T_MAX;
    } else if (maxcount == 0 || PyBytes_GET_SIZE(self) == 0) {
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
    if (PyBytes_GET_SIZE(self) == 0) {
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
"B.replace (old, new[, count]) -> bytes\n\
\n\
Return a copy of bytes B with all occurrences of subsection\n\
old replaced by new.  If the optional argument count is\n\
given, only the first count occurrences are replaced.");

static PyObject *
bytes_replace(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t count = -1;
    PyObject *from, *to, *res;
    PyBuffer vfrom, vto;

    if (!PyArg_ParseTuple(args, "OO|n:replace", &from, &to, &count))
        return NULL;

    if (_getbuffer(from, &vfrom) < 0)
        return NULL;
    if (_getbuffer(to, &vto) < 0) {
        PyObject_ReleaseBuffer(from, &vfrom);
        return NULL;
    }

    res = (PyObject *)replace((PyBytesObject *) self,
                              vfrom.buf, vfrom.len,
                              vto.buf, vto.len, count);

    PyObject_ReleaseBuffer(from, &vfrom);
    PyObject_ReleaseBuffer(to, &vto);
    return res;
}


/* Overallocate the initial list to reduce the number of reallocs for small
   split sizes.  Eg, "A A A A A A A A A A".split() (10 elements) has three
   resizes, to sizes 4, 8, then 16.  Most observed string splits are for human
   text (roughly 11 words per line) and field delimited data (usually 1-10
   fields).  For large strings the split algorithms are bandwidth limited
   so increasing the preallocation likely will not improve things.*/

#define MAX_PREALLOC 12

/* 5 splits gives 6 elements */
#define PREALLOC_SIZE(maxsplit) \
    (maxsplit >= MAX_PREALLOC ? MAX_PREALLOC : maxsplit+1)

#define SPLIT_APPEND(data, left, right)                         \
    str = PyBytes_FromStringAndSize((data) + (left),       \
                                     (right) - (left));     \
    if (str == NULL)                                        \
        goto onError;                                   \
    if (PyList_Append(list, str)) {                         \
        Py_DECREF(str);                                 \
        goto onError;                                   \
    }                                                       \
    else                                                    \
        Py_DECREF(str);

#define SPLIT_ADD(data, left, right) {                          \
    str = PyBytes_FromStringAndSize((data) + (left),       \
                                     (right) - (left));     \
    if (str == NULL)                                        \
        goto onError;                                   \
    if (count < MAX_PREALLOC) {                             \
        PyList_SET_ITEM(list, count, str);              \
    } else {                                                \
        if (PyList_Append(list, str)) {                 \
            Py_DECREF(str);                         \
            goto onError;                           \
        }                                               \
        else                                            \
            Py_DECREF(str);                         \
    }                                                       \
    count++; }

/* Always force the list to the expected size. */
#define FIX_PREALLOC_SIZE(list) Py_Size(list) = count


Py_LOCAL_INLINE(PyObject *)
split_char(const char *s, Py_ssize_t len, char ch, Py_ssize_t maxcount)
{
    register Py_ssize_t i, j, count=0;
    PyObject *str;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));

    if (list == NULL)
        return NULL;

    i = j = 0;
    while ((j < len) && (maxcount-- > 0)) {
        for(; j<len; j++) {
            /* I found that using memchr makes no difference */
            if (s[j] == ch) {
                SPLIT_ADD(s, i, j);
                i = j = j + 1;
                break;
            }
        }
    }
    if (i <= len) {
        SPLIT_ADD(s, i, len);
    }
    FIX_PREALLOC_SIZE(list);
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

PyDoc_STRVAR(split__doc__,
"B.split(sep [,maxsplit]) -> list of bytes\n\
\n\
Return a list of the bytes in the string B, using sep as the\n\
delimiter.  If maxsplit is given, at most maxsplit\n\
splits are done.");

static PyObject *
bytes_split(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t len = PyBytes_GET_SIZE(self), n, i, j;
    Py_ssize_t maxsplit = -1, count=0;
    const char *s = PyBytes_AS_STRING(self), *sub;
    PyObject *list, *str, *subobj;
#ifdef USE_FAST
    Py_ssize_t pos;
#endif

    if (!PyArg_ParseTuple(args, "O|n:split", &subobj, &maxsplit))
        return NULL;
    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;
    if (PyBytes_Check(subobj)) {
        sub = PyBytes_AS_STRING(subobj);
        n = PyBytes_GET_SIZE(subobj);
    }
    /* XXX -> use the modern buffer interface */
    else if (PyObject_AsCharBuffer(subobj, &sub, &n))
        return NULL;

    if (n == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }
    else if (n == 1)
        return split_char(s, len, sub[0], maxsplit);

    list = PyList_New(PREALLOC_SIZE(maxsplit));
    if (list == NULL)
        return NULL;

#ifdef USE_FAST
    i = j = 0;
    while (maxsplit-- > 0) {
        pos = fastsearch(s+i, len-i, sub, n, FAST_SEARCH);
        if (pos < 0)
                break;
        j = i+pos;
        SPLIT_ADD(s, i, j);
        i = j + n;
    }
#else
    i = j = 0;
    while ((j+n <= len) && (maxsplit-- > 0)) {
        for (; j+n <= len; j++) {
            if (Py_STRING_MATCH(s, j, sub, n)) {
                SPLIT_ADD(s, i, j);
                i = j = j + n;
                break;
            }
        }
    }
#endif
    SPLIT_ADD(s, i, len);
    FIX_PREALLOC_SIZE(list);
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

PyDoc_STRVAR(partition__doc__,
"B.partition(sep) -> (head, sep, tail)\n\
\n\
Searches for the separator sep in B, and returns the part before it,\n\
the separator itself, and the part after it.  If the separator is not\n\
found, returns B and two empty bytes.");

static PyObject *
bytes_partition(PyBytesObject *self, PyObject *sep_obj)
{
    PyObject *bytesep, *result;

    bytesep = PyBytes_FromObject(sep_obj);
    if (! bytesep)
        return NULL;

    result = stringlib_partition(
            (PyObject*) self,
            PyBytes_AS_STRING(self), PyBytes_GET_SIZE(self),
            bytesep,
            PyBytes_AS_STRING(bytesep), PyBytes_GET_SIZE(bytesep)
            );

    Py_DECREF(bytesep);
    return result;
}

PyDoc_STRVAR(rpartition__doc__,
"B.rpartition(sep) -> (tail, sep, head)\n\
\n\
Searches for the separator sep in B, starting at the end of B, and returns\n\
the part before it, the separator itself, and the part after it.  If the\n\
separator is not found, returns two empty bytes and B.");

static PyObject *
bytes_rpartition(PyBytesObject *self, PyObject *sep_obj)
{
    PyObject *bytesep, *result;

    bytesep = PyBytes_FromObject(sep_obj);
    if (! bytesep)
        return NULL;

    result = stringlib_rpartition(
            (PyObject*) self,
            PyBytes_AS_STRING(self), PyBytes_GET_SIZE(self),
            bytesep,
            PyBytes_AS_STRING(bytesep), PyBytes_GET_SIZE(bytesep)
            );

    Py_DECREF(bytesep);
    return result;
}

Py_LOCAL_INLINE(PyObject *)
rsplit_char(const char *s, Py_ssize_t len, char ch, Py_ssize_t maxcount)
{
    register Py_ssize_t i, j, count=0;
    PyObject *str;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));

    if (list == NULL)
        return NULL;

    i = j = len - 1;
    while ((i >= 0) && (maxcount-- > 0)) {
        for (; i >= 0; i--) {
            if (s[i] == ch) {
                SPLIT_ADD(s, i + 1, j + 1);
                j = i = i - 1;
                break;
            }
        }
    }
    if (j >= -1) {
        SPLIT_ADD(s, 0, j + 1);
    }
    FIX_PREALLOC_SIZE(list);
    if (PyList_Reverse(list) < 0)
        goto onError;

    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

PyDoc_STRVAR(rsplit__doc__,
"B.rsplit(sep [,maxsplit]) -> list of bytes\n\
\n\
Return a list of the sections in the byte B, using sep as the\n\
delimiter, starting at the end of the bytes and working\n\
to the front.  If maxsplit is given, at most maxsplit splits are\n\
done.");

static PyObject *
bytes_rsplit(PyBytesObject *self, PyObject *args)
{
    Py_ssize_t len = PyBytes_GET_SIZE(self), n, i, j;
    Py_ssize_t maxsplit = -1, count=0;
    const char *s = PyBytes_AS_STRING(self), *sub;
    PyObject *list, *str, *subobj;

    if (!PyArg_ParseTuple(args, "O|n:rsplit", &subobj, &maxsplit))
        return NULL;
    if (maxsplit < 0)
        maxsplit = PY_SSIZE_T_MAX;
    if (PyBytes_Check(subobj)) {
        sub = PyBytes_AS_STRING(subobj);
        n = PyBytes_GET_SIZE(subobj);
    }
    /* XXX -> Use the modern buffer interface */
    else if (PyObject_AsCharBuffer(subobj, &sub, &n))
        return NULL;

    if (n == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }
    else if (n == 1)
        return rsplit_char(s, len, sub[0], maxsplit);

    list = PyList_New(PREALLOC_SIZE(maxsplit));
    if (list == NULL)
        return NULL;

    j = len;
    i = j - n;

    while ( (i >= 0) && (maxsplit-- > 0) ) {
        for (; i>=0; i--) {
            if (Py_STRING_MATCH(s, i, sub, n)) {
                SPLIT_ADD(s, i + n, j);
                j = i;
                i -= n;
                break;
            }
        }
    }
    SPLIT_ADD(s, 0, j);
    FIX_PREALLOC_SIZE(list);
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

onError:
    Py_DECREF(list);
    return NULL;
}

PyDoc_STRVAR(extend__doc__,
"B.extend(iterable int) -> None\n\
\n\
Append all the elements from the iterator or sequence to the\n\
end of the bytes.");
static PyObject *
bytes_extend(PyBytesObject *self, PyObject *arg)
{
    if (bytes_setslice(self, Py_Size(self), Py_Size(self), arg) == -1)
        return NULL;
    Py_RETURN_NONE;
}


PyDoc_STRVAR(reverse__doc__,
"B.reverse() -> None\n\
\n\
Reverse the order of the values in bytes in place.");
static PyObject *
bytes_reverse(PyBytesObject *self, PyObject *unused)
{
    char swap, *head, *tail;
    Py_ssize_t i, j, n = Py_Size(self);

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
Insert a single item into the bytes before the given index.");
static PyObject *
bytes_insert(PyBytesObject *self, PyObject *args)
{
    int value;
    Py_ssize_t where, n = Py_Size(self);

    if (!PyArg_ParseTuple(args, "ni:insert", &where, &value))
        return NULL;

    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytes");
        return NULL;
    }
    if (value < 0 || value >= 256) {
        PyErr_SetString(PyExc_ValueError,
                        "byte must be in range(0, 256)");
        return NULL;
    }
    if (PyBytes_Resize((PyObject *)self, n + 1) < 0)
        return NULL;

    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    memmove(self->ob_bytes + where + 1, self->ob_bytes + where, n - where);
    self->ob_bytes[where] = value;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(append__doc__,
"B.append(int) -> None\n\
\n\
Append a single item to the end of the bytes.");
static PyObject *
bytes_append(PyBytesObject *self, PyObject *arg)
{
    int value;
    Py_ssize_t n = Py_Size(self);

    if (! _getbytevalue(arg, &value))
        return NULL;
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot add more objects to bytes");
        return NULL;
    }
    if (PyBytes_Resize((PyObject *)self, n + 1) < 0)
        return NULL;

    self->ob_bytes[n] = value;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(pop__doc__,
"B.pop([index]) -> int\n\
\n\
Remove and return a single item from the bytes. If no index\n\
argument is give, will pop the last value.");
static PyObject *
bytes_pop(PyBytesObject *self, PyObject *args)
{
    int value;
    Py_ssize_t where = -1, n = Py_Size(self);

    if (!PyArg_ParseTuple(args, "|n:pop", &where))
        return NULL;

    if (n == 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot pop an empty bytes");
        return NULL;
    }
    if (where < 0)
        where += Py_Size(self);
    if (where < 0 || where >= Py_Size(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }

    value = self->ob_bytes[where];
    memmove(self->ob_bytes + where, self->ob_bytes + where + 1, n - where);
    if (PyBytes_Resize((PyObject *)self, n - 1) < 0)
        return NULL;

    return PyInt_FromLong(value);
}

PyDoc_STRVAR(remove__doc__,
"B.remove(int) -> None\n\
\n\
Remove the first occurance of a value in bytes");
static PyObject *
bytes_remove(PyBytesObject *self, PyObject *arg)
{
    int value;
    Py_ssize_t where, n = Py_Size(self);

    if (! _getbytevalue(arg, &value))
        return NULL;

    for (where = 0; where < n; where++) {
        if (self->ob_bytes[where] == value)
            break;
    }
    if (where == n) {
        PyErr_SetString(PyExc_ValueError, "value not found in bytes");
        return NULL;
    }

    memmove(self->ob_bytes + where, self->ob_bytes + where + 1, n - where);
    if (PyBytes_Resize((PyObject *)self, n - 1) < 0)
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
"B.strip(bytes) -> bytes\n\
\n\
Strip leading and trailing bytes contained in the argument.");
static PyObject *
bytes_strip(PyBytesObject *self, PyObject *arg)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    if (arg == NULL || !PyBytes_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "strip() requires a bytes argument");
        return NULL;
    }
    myptr = self->ob_bytes;
    mysize = Py_Size(self);
    argptr = ((PyBytesObject *)arg)->ob_bytes;
    argsize = Py_Size(arg);
    left = lstrip_helper(myptr, mysize, argptr, argsize);
    if (left == mysize)
        right = left;
    else
        right = rstrip_helper(myptr, mysize, argptr, argsize);
    return PyBytes_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(lstrip__doc__,
"B.lstrip(bytes) -> bytes\n\
\n\
Strip leading bytes contained in the argument.");
static PyObject *
bytes_lstrip(PyBytesObject *self, PyObject *arg)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    if (arg == NULL || !PyBytes_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "strip() requires a bytes argument");
        return NULL;
    }
    myptr = self->ob_bytes;
    mysize = Py_Size(self);
    argptr = ((PyBytesObject *)arg)->ob_bytes;
    argsize = Py_Size(arg);
    left = lstrip_helper(myptr, mysize, argptr, argsize);
    right = mysize;
    return PyBytes_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(rstrip__doc__,
"B.rstrip(bytes) -> bytes\n\
\n\
Strip trailing bytes contained in the argument.");
static PyObject *
bytes_rstrip(PyBytesObject *self, PyObject *arg)
{
    Py_ssize_t left, right, mysize, argsize;
    void *myptr, *argptr;
    if (arg == NULL || !PyBytes_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "strip() requires a bytes argument");
        return NULL;
    }
    myptr = self->ob_bytes;
    mysize = Py_Size(self);
    argptr = ((PyBytesObject *)arg)->ob_bytes;
    argsize = Py_Size(arg);
    left = 0;
    right = rstrip_helper(myptr, mysize, argptr, argsize);
    return PyBytes_FromStringAndSize(self->ob_bytes + left, right - left);
}

PyDoc_STRVAR(decode_doc,
"B.decode([encoding[,errors]]) -> unicode obect.\n\
\n\
Decodes B using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a UnicodeDecodeError. Other possible values are 'ignore' and 'replace'\n\
as well as any other name registerd with codecs.register_error that is\n\
able to handle UnicodeDecodeErrors.");

static PyObject *
bytes_decode(PyObject *self, PyObject *args)
{
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "|ss:decode", &encoding, &errors))
        return NULL;
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();
    return PyCodec_Decode(self, encoding, errors);
}

PyDoc_STRVAR(alloc_doc,
"B.__alloc__() -> int\n\
\n\
Returns the number of bytes actually allocated.");

static PyObject *
bytes_alloc(PyBytesObject *self)
{
    return PyInt_FromSsize_t(self->ob_alloc);
}

PyDoc_STRVAR(join_doc,
"B.join(iterable_of_bytes) -> bytes\n\
\n\
Concatenates any number of bytes objects, with B in between each pair.\n\
Example: b'.'.join([b'ab', b'pq', b'rs']) -> b'ab.pq.rs'.");

static PyObject *
bytes_join(PyBytesObject *self, PyObject *it)
{
    PyObject *seq;
    Py_ssize_t mysize = Py_Size(self);
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
    for (i = 0; i < n; i++) {
        PyObject *obj = items[i];
        if (!PyBytes_Check(obj)) {
            PyErr_Format(PyExc_TypeError,
                         "can only join an iterable of bytes "
                         "(item %ld has type '%.100s')",
                         /* XXX %ld isn't right on Win64 */
                         (long)i, Py_Type(obj)->tp_name);
            goto error;
        }
        if (i > 0)
            totalsize += mysize;
        totalsize += PyBytes_GET_SIZE(obj);
        if (totalsize < 0) {
            PyErr_NoMemory();
            goto error;
        }
    }

    /* Allocate the result, and copy the bytes */
    result = PyBytes_FromStringAndSize(NULL, totalsize);
    if (result == NULL)
        goto error;
    dest = PyBytes_AS_STRING(result);
    for (i = 0; i < n; i++) {
        PyObject *obj = items[i];
        Py_ssize_t size = PyBytes_GET_SIZE(obj);
        if (i > 0) {
            memcpy(dest, self->ob_bytes, mysize);
            dest += mysize;
        }
        memcpy(dest, PyBytes_AS_STRING(obj), size);
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

PyDoc_STRVAR(fromhex_doc,
"bytes.fromhex(string) -> bytes\n\
\n\
Create a bytes object from a string of hexadecimal numbers.\n\
Spaces between two numbers are accepted. Example:\n\
bytes.fromhex('10 2030') -> bytes([0x10, 0x20, 0x30]).");

static int
hex_digit_to_int(int c)
{
    if (isdigit(c))
        return c - '0';
    else {
        if (isupper(c))
            c = tolower(c);
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
    }
    return -1;
}

static PyObject *
bytes_fromhex(PyObject *cls, PyObject *args)
{
    PyObject *newbytes;
    char *hex, *buf;
    Py_ssize_t len, byteslen, i, j;
    int top, bot;

    if (!PyArg_ParseTuple(args, "s#:fromhex", &hex, &len))
        return NULL;

    byteslen = len / 2; /* max length if there are no spaces */

    newbytes = PyBytes_FromStringAndSize(NULL, byteslen);
    if (!newbytes)
        return NULL;
    buf = PyBytes_AS_STRING(newbytes);

    for (i = j = 0; i < len; i += 2) {
        /* skip over spaces in the input */
        while (Py_CHARMASK(hex[i]) == ' ')
            i++;
        if (i >= len)
            break;
        top = hex_digit_to_int(Py_CHARMASK(hex[i]));
        bot = hex_digit_to_int(Py_CHARMASK(hex[i+1]));
        if (top == -1 || bot == -1) {
            PyErr_Format(PyExc_ValueError,
                         "non-hexadecimal number string '%c%c' found in "
                         "fromhex() arg at position %zd",
                         hex[i], hex[i+1], i);
            goto error;
        }
        buf[j++] = (top << 4) + bot;
    }
    if (PyBytes_Resize(newbytes, j) < 0)
        goto error;
    return newbytes;

  error:
    Py_DECREF(newbytes);
    return NULL;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
bytes_reduce(PyBytesObject *self)
{
    PyObject *latin1;
    if (self->ob_bytes)
        latin1 = PyUnicode_DecodeLatin1(self->ob_bytes,
                                        Py_Size(self), NULL);
    else
        latin1 = PyUnicode_FromString("");
    return Py_BuildValue("(O(Ns))", Py_Type(self), latin1, "latin-1");
}

static PySequenceMethods bytes_as_sequence = {
    (lenfunc)bytes_length,              /* sq_length */
    (binaryfunc)bytes_concat,           /* sq_concat */
    (ssizeargfunc)bytes_repeat,         /* sq_repeat */
    (ssizeargfunc)bytes_getitem,        /* sq_item */
    0,                                  /* sq_slice */
    (ssizeobjargproc)bytes_setitem,     /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)bytes_contains,         /* sq_contains */
    (binaryfunc)bytes_iconcat,          /* sq_inplace_concat */
    (ssizeargfunc)bytes_irepeat,        /* sq_inplace_repeat */
};

static PyMappingMethods bytes_as_mapping = {
    (lenfunc)bytes_length,
    (binaryfunc)bytes_subscript,
    (objobjargproc)bytes_ass_subscript,
};

static PyBufferProcs bytes_as_buffer = {
    (getbufferproc)bytes_getbuffer,
    (releasebufferproc)bytes_releasebuffer,
};

static PyMethodDef
bytes_methods[] = {
    {"find", (PyCFunction)bytes_find, METH_VARARGS, find__doc__},
    {"count", (PyCFunction)bytes_count, METH_VARARGS, count__doc__},
    {"index", (PyCFunction)bytes_index, METH_VARARGS, index__doc__},
    {"rfind", (PyCFunction)bytes_rfind, METH_VARARGS, rfind__doc__},
    {"rindex", (PyCFunction)bytes_rindex, METH_VARARGS, rindex__doc__},
    {"endswith", (PyCFunction)bytes_endswith, METH_VARARGS, endswith__doc__},
    {"startswith", (PyCFunction)bytes_startswith, METH_VARARGS,
                startswith__doc__},
    {"replace", (PyCFunction)bytes_replace, METH_VARARGS, replace__doc__},
    {"translate", (PyCFunction)bytes_translate, METH_VARARGS, translate__doc__},
    {"partition", (PyCFunction)bytes_partition, METH_O, partition__doc__},
    {"rpartition", (PyCFunction)bytes_rpartition, METH_O, rpartition__doc__},
    {"split", (PyCFunction)bytes_split, METH_VARARGS, split__doc__},
    {"rsplit", (PyCFunction)bytes_rsplit, METH_VARARGS, rsplit__doc__},
    {"extend", (PyCFunction)bytes_extend, METH_O, extend__doc__},
    {"insert", (PyCFunction)bytes_insert, METH_VARARGS, insert__doc__},
    {"append", (PyCFunction)bytes_append, METH_O, append__doc__},
    {"reverse", (PyCFunction)bytes_reverse, METH_NOARGS, reverse__doc__},
    {"pop", (PyCFunction)bytes_pop, METH_VARARGS, pop__doc__},
    {"remove", (PyCFunction)bytes_remove, METH_O, remove__doc__},
    {"strip", (PyCFunction)bytes_strip, METH_O, strip__doc__},
    {"lstrip", (PyCFunction)bytes_lstrip, METH_O, lstrip__doc__},
    {"rstrip", (PyCFunction)bytes_rstrip, METH_O, rstrip__doc__},
    {"decode", (PyCFunction)bytes_decode, METH_VARARGS, decode_doc},
    {"__alloc__", (PyCFunction)bytes_alloc, METH_NOARGS, alloc_doc},
    {"fromhex", (PyCFunction)bytes_fromhex, METH_VARARGS|METH_CLASS,
     fromhex_doc},
    {"join", (PyCFunction)bytes_join, METH_O, join_doc},
    {"__reduce__", (PyCFunction)bytes_reduce, METH_NOARGS, reduce_doc},
    {NULL}
};

PyDoc_STRVAR(bytes_doc,
"bytes([iterable]) -> new array of bytes.\n\
\n\
If an argument is given it must be an iterable yielding ints in range(256).");

PyTypeObject PyBytes_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bytes",
    sizeof(PyBytesObject),
    0,
    (destructor)bytes_dealloc,          /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    (reprfunc)bytes_repr,               /* tp_repr */
    0,                                  /* tp_as_number */
    &bytes_as_sequence,                 /* tp_as_sequence */
    &bytes_as_mapping,                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    (reprfunc)bytes_str,                /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    &bytes_as_buffer,                   /* tp_as_buffer */
    /* bytes is 'final' or 'sealed' */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    bytes_doc,                          /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    (richcmpfunc)bytes_richcompare,     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    bytes_methods,                      /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)bytes_init,               /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};

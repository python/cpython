/* Bytes object implementation */

/* XXX TO DO: optimizations */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

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

    assert(size >= 0);

    new = PyObject_New(PyBytesObject, &PyBytes_Type);
    if (new == NULL)
        return NULL;

    if (size == 0)
        new->ob_bytes = NULL;
    else {
        new->ob_bytes = PyMem_Malloc(size);
        if (new->ob_bytes == NULL) {
            Py_DECREF(new);
            return NULL;
        }
        if (bytes != NULL)
            memcpy(new->ob_bytes, bytes, size);
    }
    new->ob_size = new->ob_alloc = size;
    
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
        alloc = size;
    }
    else if (size <= alloc) {
        /* Within allocated size; quick exit */
        ((PyBytesObject *)self)->ob_size = size;
        return 0;
    }
    else if (size <= alloc * 1.125) {
        /* Moderate upsize; overallocate similar to list_resize() */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize; resize up to exact size */
        alloc = size;
    }

    sval = PyMem_Realloc(((PyBytesObject *)self)->ob_bytes, alloc);
    if (sval == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    ((PyBytesObject *)self)->ob_bytes = sval;
    ((PyBytesObject *)self)->ob_size = size;
    ((PyBytesObject *)self)->ob_alloc = alloc;

    return 0;
}

/* Functions stuffed into the type object */

static Py_ssize_t
bytes_length(PyBytesObject *self)
{
    return self->ob_size;
}

static PyObject *
bytes_concat(PyBytesObject *self, PyObject *other)
{
    PyBytesObject *result;
    Py_ssize_t mysize;
    Py_ssize_t size;

    if (!PyBytes_Check(other)) {
        PyErr_Format(PyExc_TypeError,
                     "can't concat bytes to %.100s", other->ob_type->tp_name);
        return NULL;
    }
    
    mysize = self->ob_size;
    size = mysize + ((PyBytesObject *)other)->ob_size;
    if (size < 0)
        return PyErr_NoMemory();
    result = (PyBytesObject *) PyBytes_FromStringAndSize(NULL, size);
    if (result != NULL) {
        memcpy(result->ob_bytes, self->ob_bytes, self->ob_size);
        memcpy(result->ob_bytes + self->ob_size,
               ((PyBytesObject *)other)->ob_bytes,
               ((PyBytesObject *)other)->ob_size);
    }
    return (PyObject *)result;
}

static PyObject *
bytes_iconcat(PyBytesObject *self, PyObject *other)
{
    Py_ssize_t mysize;
    Py_ssize_t osize;
    Py_ssize_t size;

    if (!PyBytes_Check(other)) {
        PyErr_Format(PyExc_TypeError,
                     "can't concat bytes to %.100s", other->ob_type->tp_name);
        return NULL;
    }

    mysize = self->ob_size;
    osize = ((PyBytesObject *)other)->ob_size;
    size = mysize + osize;
    if (size < 0)
        return PyErr_NoMemory();
    if (size <= self->ob_alloc)
        self->ob_size = size;
    else if (PyBytes_Resize((PyObject *)self, size) < 0)
        return NULL;
    memcpy(self->ob_bytes + mysize, ((PyBytesObject *)other)->ob_bytes, osize);
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
    mysize = self->ob_size;
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    result = (PyBytesObject *)PyBytes_FromStringAndSize(NULL,  size);
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
    mysize = self->ob_size;
    size = mysize * count;
    if (count != 0 && size / count != mysize)
        return PyErr_NoMemory();
    if (size <= self->ob_alloc)
        self->ob_size = size;
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

    if (other->ob_size == 1) {
        return memchr(self->ob_bytes, other->ob_bytes[0], 
                      self->ob_size) != NULL;
    }
    if (other->ob_size == 0)
        return 1; /* Edge case */
    for (i = 0; i + other->ob_size <= self->ob_size; i++) {
        /* XXX Yeah, yeah, lots of optimizations possible... */
        if (memcmp(self->ob_bytes + i, other->ob_bytes, other->ob_size) == 0)
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

    return memchr(self->ob_bytes, ival, self->ob_size) != NULL;
}

static PyObject *
bytes_getitem(PyBytesObject *self, Py_ssize_t i)
{
    if (i < 0)
        i += self->ob_size;
    if (i < 0 || i >= self->ob_size) {
        PyErr_SetString(PyExc_IndexError, "bytes index out of range");
        return NULL;
    }
    return PyInt_FromLong((unsigned char)(self->ob_bytes[i]));
}

static PyObject *
bytes_getslice(PyBytesObject *self, Py_ssize_t lo, Py_ssize_t hi)
{
    if (lo < 0)
        lo = 0;
    if (hi > self->ob_size)
        hi = self->ob_size;
    if (lo >= hi)
        lo = hi = 0;
    return PyBytes_FromStringAndSize(self->ob_bytes + lo, hi - lo);
}

static int
bytes_setslice(PyBytesObject *self, Py_ssize_t lo, Py_ssize_t hi, 
               PyObject *values)
{
    int avail;
    int needed;
    char *bytes;

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
        err = bytes_setslice(self, lo, hi, values);
        Py_DECREF(values);
        return err;
    }
    else {
        assert(PyBytes_Check(values));
        bytes = ((PyBytesObject *)values)->ob_bytes;
        needed = ((PyBytesObject *)values)->ob_size;
    }

    if (lo < 0)
        lo = 0;
    if (hi > self->ob_size)
        hi = self->ob_size;

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
                    self->ob_size - hi);
        }
        if (PyBytes_Resize((PyObject *)self, 
                           self->ob_size + needed - avail) < 0)
            return -1;
        if (avail < needed) {
            /*
              0   lo        hi               old_size
              |   |<-avail->|<-----tomove------>|
              |   |<----needed---->|<-----tomove------>|
              0   lo            new_hi              new_size
             */
            memmove(self->ob_bytes + lo + needed, self->ob_bytes + hi,
                    self->ob_size - lo - needed);
        }
    }

    if (needed > 0)
        memcpy(self->ob_bytes + lo, bytes, needed);

    return 0;
}

static int
bytes_setitem(PyBytesObject *self, Py_ssize_t i, PyObject *value)
{
    Py_ssize_t ival;

    if (i < 0)
        i += self->ob_size;

    if (i < 0 || i >= self->ob_size) {
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
bytes_init(PyBytesObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"source", "encoding", "errors", 0};
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;
    Py_ssize_t count;
    PyObject *it;
    PyObject *(*iternext)(PyObject *);

    if (self->ob_size != 0) {
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
        PyObject *encoded;
        char *bytes;
        Py_ssize_t size;
        if (encoding == NULL)
            encoding = PyUnicode_GetDefaultEncoding();
        encoded = PyCodec_Encode(arg, encoding, errors);
        if (encoded == NULL)
            return -1;
        if (!PyString_Check(encoded)) {
            PyErr_Format(PyExc_TypeError,
                "encoder did not return a string object (type=%.400s)",
                encoded->ob_type->tp_name);
            Py_DECREF(encoded);
            return -1;
        }
        bytes = PyString_AS_STRING(encoded);
        size = PyString_GET_SIZE(encoded);
        if (size <= self->ob_alloc)
            self->ob_size = size;
        else if (PyBytes_Resize((PyObject *)self, size) < 0) {
            Py_DECREF(encoded);
            return -1;
        }
        memcpy(self->ob_bytes, bytes, size);
        Py_DECREF(encoded);
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

    if (PyObject_CheckReadBuffer(arg)) {
        const void *bytes;
        Py_ssize_t size;
        if (PyObject_AsReadBuffer(arg, &bytes, &size) < 0)
            return -1;
        if (PyBytes_Resize((PyObject *)self, size) < 0)
            return -1;
        memcpy(self->ob_bytes, bytes, size);
        return 0;
    }

    /* XXX Optimize this if the arguments is a list, tuple */

    /* Get the iterator */
    it = PyObject_GetIter(arg);
    if (it == NULL)
        return -1;
    iternext = *it->ob_type->tp_iternext;

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
        if (self->ob_size < self->ob_alloc)
            self->ob_size++;
        else if (PyBytes_Resize((PyObject *)self, self->ob_size+1) < 0)
            goto error;
        self->ob_bytes[self->ob_size-1] = value;
    }

    /* Clean up and return success */
    Py_DECREF(it);
    return 0;

 error:
    /* Error handling when it != NULL */
    Py_DECREF(it);
    return -1;
}

static PyObject *
bytes_repr(PyBytesObject *self)
{
    PyObject *list;
    PyObject *str;
    PyObject *result;
    int err;
    int i;

    if (self->ob_size == 0)
        return PyString_FromString("bytes()");

    list = PyList_New(0);
    if (list == NULL)
        return NULL;

    str = PyString_FromString("bytes([");
    if (str == NULL)
        goto error;

    err = PyList_Append(list, str);
    Py_DECREF(str);
    if (err < 0)
        goto error;

    for (i = 0; i < self->ob_size; i++) {
        char buffer[20];
        sprintf(buffer, ", 0x%02x", (unsigned char) (self->ob_bytes[i]));
        str = PyString_FromString((i == 0) ? buffer+2 : buffer);
        if (str == NULL)
            goto error;
        err = PyList_Append(list, str);
        Py_DECREF(str);
        if (err < 0)
            goto error;
    }

    str = PyString_FromString("])");
    if (str == NULL)
        goto error;

    err = PyList_Append(list, str);
    Py_DECREF(str);
    if (err < 0)
        goto error;
    
    str = PyString_FromString("");
    if (str == NULL)
        goto error;

    result = _PyString_Join(str, list);
    Py_DECREF(str);
    Py_DECREF(list);
    return result;

 error:
    /* Error handling when list != NULL  */
    Py_DECREF(list);
    return NULL;
}

static PyObject *
bytes_str(PyBytesObject *self)
{
    return PyString_FromStringAndSize(self->ob_bytes, self->ob_size);
}

static PyObject *
bytes_richcompare(PyBytesObject *self, PyBytesObject *other, int op)
{
    PyObject *res;
    int minsize;
    int cmp;

    if (!PyBytes_Check(self) || !PyBytes_Check(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    if (self->ob_size != other->ob_size && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the objects differ */
        cmp = (op == Py_NE);
    }
    else {
        minsize = self->ob_size;
        if (other->ob_size < minsize)
            minsize = other->ob_size;

        cmp = memcmp(self->ob_bytes, other->ob_bytes, minsize);
        /* In ISO C, memcmp() guarantees to use unsigned bytes! */

        if (cmp == 0) {
            if (self->ob_size < other->ob_size)
                cmp = -1;
            else if (self->ob_size > other->ob_size)
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
    Py_INCREF(res);
    return res;
}

static void
bytes_dealloc(PyBytesObject *self)
{
    if (self->ob_bytes != 0) {
        PyMem_Free(self->ob_bytes);
    }
    self->ob_type->tp_free((PyObject *)self);
}

static Py_ssize_t
bytes_getbuffer(PyBytesObject *self, Py_ssize_t index, const void **ptr)
{
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "accessing non-existent string segment");
        return -1;
    }
    *ptr = (void *)self->ob_bytes;
    return self->ob_size;
}

static Py_ssize_t
bytes_getsegcount(PyStringObject *self, Py_ssize_t *lenp)
{
    if (lenp)
        *lenp = self->ob_size;
    return 1;
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
"bytes.join(iterable_of_bytes) -> bytes\n\
\n\
Concatenates any number of bytes objects.  Example:\n\
bytes.join([bytes('ab'), bytes('pq'), bytes('rs')]) -> bytes('abpqrs').");

static PyObject *
bytes_join(PyObject *cls, PyObject *it)
{
    PyObject *seq;
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
			 (long)i, obj->ob_type->tp_name);
	    goto error;
	}
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

static PySequenceMethods bytes_as_sequence = {
    (lenfunc)bytes_length,              /*sq_length*/
    (binaryfunc)bytes_concat,           /*sq_concat*/
    (ssizeargfunc)bytes_repeat,         /*sq_repeat*/
    (ssizeargfunc)bytes_getitem,        /*sq_item*/
    (ssizessizeargfunc)bytes_getslice,  /*sq_slice*/
    (ssizeobjargproc)bytes_setitem,     /*sq_ass_item*/
    (ssizessizeobjargproc)bytes_setslice, /* sq_ass_slice */
    (objobjproc)bytes_contains,         /* sq_contains */
    (binaryfunc)bytes_iconcat,   /* sq_inplace_concat */
    (ssizeargfunc)bytes_irepeat, /* sq_inplace_repeat */
};

static PyMappingMethods bytes_as_mapping = {
    (lenfunc)bytes_length,
    (binaryfunc)0,
    0,
};

static PyBufferProcs bytes_as_buffer = {
    (readbufferproc)bytes_getbuffer,
    (writebufferproc)bytes_getbuffer,
    (segcountproc)bytes_getsegcount,
    /* XXX Bytes are not characters! But we need to implement
       bf_getcharbuffer() so we can be used as 't#' argument to codecs. */
    (charbufferproc)bytes_getbuffer,
};

static PyMethodDef
bytes_methods[] = {
    {"decode", (PyCFunction)bytes_decode, METH_VARARGS, decode_doc},
    {"__alloc__", (PyCFunction)bytes_alloc, METH_NOARGS, alloc_doc},
    {"join", (PyCFunction)bytes_join, METH_O|METH_CLASS, join_doc},
    {NULL}
};

PyDoc_STRVAR(bytes_doc,
"bytes([iterable]) -> new array of bytes.\n\
\n\
If an argument is given it must be an iterable yielding ints in range(256).");

PyTypeObject PyBytes_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
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
    0,		                       /* tp_hash */
    0,                                  /* tp_call */
    (reprfunc)bytes_str,                /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    &bytes_as_buffer,                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */ 
                                        /* bytes is 'final' or 'sealed' */
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

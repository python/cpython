
/* Memoryview object implementation */

#include "Python.h"

static Py_ssize_t
get_shape0(Py_buffer *buf)
{
    if (buf->shape != NULL)
        return buf->shape[0];
    if (buf->ndim == 0)
        return 1;
    PyErr_SetString(PyExc_TypeError,
        "exported buffer does not have any shape information associated "
        "to it");
    return -1;
}

static void
dup_buffer(Py_buffer *dest, Py_buffer *src)
{
    *dest = *src;
    if (src->ndim == 1 && src->shape != NULL) {
        dest->shape = &(dest->smalltable[0]);
        dest->shape[0] = get_shape0(src);
    }
    if (src->ndim == 1 && src->strides != NULL) {
        dest->strides = &(dest->smalltable[1]);
        dest->strides[0] = src->strides[0];
    }
}

static int
memory_getbuf(PyMemoryViewObject *self, Py_buffer *view, int flags)
{
    int res = 0;
    if (self->view.obj != NULL)
        res = PyObject_GetBuffer(self->view.obj, view, flags);
    if (view)
        dup_buffer(view, &self->view);
    return res;
}

static void
memory_releasebuf(PyMemoryViewObject *self, Py_buffer *view)
{
    PyBuffer_Release(view);
}

PyDoc_STRVAR(memory_doc,
"memoryview(object)\n\
\n\
Create a new memoryview object which references the given object.");

PyObject *
PyMemoryView_FromBuffer(Py_buffer *info)
{
    PyMemoryViewObject *mview;

    mview = (PyMemoryViewObject *)
        PyObject_GC_New(PyMemoryViewObject, &PyMemoryView_Type);
    if (mview == NULL)
        return NULL;
    mview->base = NULL;
    dup_buffer(&mview->view, info);
    /* NOTE: mview->view.obj should already have been incref'ed as
       part of PyBuffer_FillInfo(). */
    _PyObject_GC_TRACK(mview);
    return (PyObject *)mview;
}

PyObject *
PyMemoryView_FromObject(PyObject *base)
{
    PyMemoryViewObject *mview;
    Py_buffer view;

    if (!PyObject_CheckBuffer(base)) {
        PyErr_SetString(PyExc_TypeError,
            "cannot make memory view because object does "
            "not have the buffer interface");
        return NULL;
    }

    if (PyObject_GetBuffer(base, &view, PyBUF_FULL_RO) < 0)
        return NULL;

    mview = (PyMemoryViewObject *)PyMemoryView_FromBuffer(&view);
    if (mview == NULL) {
        PyBuffer_Release(&view);
        return NULL;
    }

    mview->base = base;
    Py_INCREF(base);
    return (PyObject *)mview;
}

static PyObject *
memory_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    PyObject *obj;
    static char *kwlist[] = {"object", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:memoryview", kwlist,
                                     &obj)) {
        return NULL;
    }

    return PyMemoryView_FromObject(obj);
}


static void
_strided_copy_nd(char *dest, char *src, int nd, Py_ssize_t *shape,
                 Py_ssize_t *strides, Py_ssize_t itemsize, char fort)
{
    int k;
    Py_ssize_t outstride;

    if (nd==0) {
        memcpy(dest, src, itemsize);
    }
    else if (nd == 1) {
        for (k = 0; k<shape[0]; k++) {
            memcpy(dest, src, itemsize);
            dest += itemsize;
            src += strides[0];
        }
    }
    else {
        if (fort == 'F') {
            /* Copy first dimension first,
               second dimension second, etc...
               Set up the recursive loop backwards so that final
               dimension is actually copied last.
            */
            outstride = itemsize;
            for (k=1; k<nd-1;k++) {
                outstride *= shape[k];
            }
            for (k=0; k<shape[nd-1]; k++) {
                _strided_copy_nd(dest, src, nd-1, shape,
                                 strides, itemsize, fort);
                dest += outstride;
                src += strides[nd-1];
            }
        }

        else {
            /* Copy last dimension first,
               second-to-last dimension second, etc.
               Set up the recursion so that the
               first dimension is copied last
            */
            outstride = itemsize;
            for (k=1; k < nd; k++) {
                outstride *= shape[k];
            }
            for (k=0; k<shape[0]; k++) {
                _strided_copy_nd(dest, src, nd-1, shape+1,
                                 strides+1, itemsize,
                                 fort);
                dest += outstride;
                src += strides[0];
            }
        }
    }
    return;
}

static int
_indirect_copy_nd(char *dest, Py_buffer *view, char fort)
{
    Py_ssize_t *indices;
    int k;
    Py_ssize_t elements;
    char *ptr;
    void (*func)(int, Py_ssize_t *, const Py_ssize_t *);

    if (view->ndim > PY_SSIZE_T_MAX / sizeof(Py_ssize_t)) {
        PyErr_NoMemory();
        return -1;
    }

    indices = (Py_ssize_t *)PyMem_Malloc(sizeof(Py_ssize_t)*view->ndim);
    if (indices == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (k=0; k<view->ndim;k++) {
        indices[k] = 0;
    }

    elements = 1;
    for (k=0; k<view->ndim; k++) {
        elements *= view->shape[k];
    }
    if (fort == 'F') {
        func = _Py_add_one_to_index_F;
    }
    else {
        func = _Py_add_one_to_index_C;
    }
    while (elements--) {
        func(view->ndim, indices, view->shape);
        ptr = PyBuffer_GetPointer(view, indices);
        memcpy(dest, ptr, view->itemsize);
        dest += view->itemsize;
    }

    PyMem_Free(indices);
    return 0;
}

/*
   Get a the data from an object as a contiguous chunk of memory (in
   either 'C' or 'F'ortran order) even if it means copying it into a
   separate memory area.

   Returns a new reference to a Memory view object.  If no copy is needed,
   the memory view object points to the original memory and holds a
   lock on the original.  If a copy is needed, then the memory view object
   points to a brand-new Bytes object (and holds a memory lock on it).

   buffertype

   PyBUF_READ  buffer only needs to be read-only
   PyBUF_WRITE buffer needs to be writable (give error if not contiguous)
   PyBUF_SHADOW buffer needs to be writable so shadow it with
                a contiguous buffer if it is not. The view will point to
                the shadow buffer which can be written to and then
                will be copied back into the other buffer when the memory
                view is de-allocated.  While the shadow buffer is
                being used, it will have an exclusive write lock on
                the original buffer.
 */

PyObject *
PyMemoryView_GetContiguous(PyObject *obj, int buffertype, char fort)
{
    PyMemoryViewObject *mem;
    PyObject *bytes;
    Py_buffer *view;
    int flags;
    char *dest;

    if (!PyObject_CheckBuffer(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "object does not have the buffer interface");
        return NULL;
    }

    mem = PyObject_GC_New(PyMemoryViewObject, &PyMemoryView_Type);
    if (mem == NULL)
        return NULL;

    view = &mem->view;
    flags = PyBUF_FULL_RO;
    switch(buffertype) {
    case PyBUF_WRITE:
        flags = PyBUF_FULL;
        break;
    }

    if (PyObject_GetBuffer(obj, view, flags) != 0) {
        Py_DECREF(mem);
        return NULL;
    }

    if (PyBuffer_IsContiguous(view, fort)) {
        /* no copy needed */
        Py_INCREF(obj);
        mem->base = obj;
        _PyObject_GC_TRACK(mem);
        return (PyObject *)mem;
    }
    /* otherwise a copy is needed */
    if (buffertype == PyBUF_WRITE) {
        Py_DECREF(mem);
        PyErr_SetString(PyExc_BufferError,
                        "writable contiguous buffer requested "
                        "for a non-contiguousobject.");
        return NULL;
    }
    bytes = PyBytes_FromStringAndSize(NULL, view->len);
    if (bytes == NULL) {
        Py_DECREF(mem);
        return NULL;
    }
    dest = PyBytes_AS_STRING(bytes);
    /* different copying strategy depending on whether
       or not any pointer de-referencing is needed
    */
    /* strided or in-direct copy */
    if (view->suboffsets==NULL) {
        _strided_copy_nd(dest, view->buf, view->ndim, view->shape,
                         view->strides, view->itemsize, fort);
    }
    else {
        if (_indirect_copy_nd(dest, view, fort) < 0) {
            Py_DECREF(bytes);
            Py_DECREF(mem);
            return NULL;
        }
    }
    if (buffertype == PyBUF_SHADOW) {
        /* return a shadowed memory-view object */
        view->buf = dest;
        mem->base = PyTuple_Pack(2, obj, bytes);
        Py_DECREF(bytes);
        if (mem->base == NULL) {
            Py_DECREF(mem);
            return NULL;
        }
    }
    else {
        PyBuffer_Release(view);  /* XXX ? */
        /* steal the reference */
        mem->base = bytes;
    }
    _PyObject_GC_TRACK(mem);
    return (PyObject *)mem;
}


static PyObject *
memory_format_get(PyMemoryViewObject *self)
{
    return PyString_FromString(self->view.format);
}

static PyObject *
memory_itemsize_get(PyMemoryViewObject *self)
{
    return PyLong_FromSsize_t(self->view.itemsize);
}

static PyObject *
_IntTupleFromSsizet(int len, Py_ssize_t *vals)
{
    int i;
    PyObject *o;
    PyObject *intTuple;

    if (vals == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    intTuple = PyTuple_New(len);
    if (!intTuple) return NULL;
    for(i=0; i<len; i++) {
        o = PyLong_FromSsize_t(vals[i]);
        if (!o) {
            Py_DECREF(intTuple);
            return NULL;
        }
        PyTuple_SET_ITEM(intTuple, i, o);
    }
    return intTuple;
}

static PyObject *
memory_shape_get(PyMemoryViewObject *self)
{
    return _IntTupleFromSsizet(self->view.ndim, self->view.shape);
}

static PyObject *
memory_strides_get(PyMemoryViewObject *self)
{
    return _IntTupleFromSsizet(self->view.ndim, self->view.strides);
}

static PyObject *
memory_suboffsets_get(PyMemoryViewObject *self)
{
    return _IntTupleFromSsizet(self->view.ndim, self->view.suboffsets);
}

static PyObject *
memory_readonly_get(PyMemoryViewObject *self)
{
    return PyBool_FromLong(self->view.readonly);
}

static PyObject *
memory_ndim_get(PyMemoryViewObject *self)
{
    return PyLong_FromLong(self->view.ndim);
}

static PyGetSetDef memory_getsetlist[] ={
    {"format",          (getter)memory_format_get,      NULL, NULL},
    {"itemsize",        (getter)memory_itemsize_get,    NULL, NULL},
    {"shape",           (getter)memory_shape_get,       NULL, NULL},
    {"strides",         (getter)memory_strides_get,     NULL, NULL},
    {"suboffsets",      (getter)memory_suboffsets_get,  NULL, NULL},
    {"readonly",        (getter)memory_readonly_get,    NULL, NULL},
    {"ndim",            (getter)memory_ndim_get,        NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


static PyObject *
memory_tobytes(PyMemoryViewObject *self, PyObject *noargs)
{
    Py_buffer view;
    PyObject *res;

    if (PyObject_GetBuffer((PyObject *)self, &view, PyBUF_SIMPLE) < 0)
        return NULL;

    res = PyBytes_FromStringAndSize(NULL, view.len);
    PyBuffer_ToContiguous(PyBytes_AS_STRING(res), &view, view.len, 'C');
    PyBuffer_Release(&view);
    return res;
}

/* TODO: rewrite this function using the struct module to unpack
   each buffer item */

static PyObject *
memory_tolist(PyMemoryViewObject *mem, PyObject *noargs)
{
    Py_buffer *view = &(mem->view);
    Py_ssize_t i;
    PyObject *res, *item;
    char *buf;

    if (strcmp(view->format, "B") || view->itemsize != 1) {
        PyErr_SetString(PyExc_NotImplementedError, 
                "tolist() only supports byte views");
        return NULL;
    }
    if (view->ndim != 1) {
        PyErr_SetString(PyExc_NotImplementedError, 
                "tolist() only supports one-dimensional objects");
        return NULL;
    }
    res = PyList_New(view->len);
    if (res == NULL)
        return NULL;
    buf = view->buf;
    for (i = 0; i < view->len; i++) {
        item = PyInt_FromLong((unsigned char) *buf);
        if (item == NULL) {
            Py_DECREF(res);
            return NULL;
        }
        PyList_SET_ITEM(res, i, item);
        buf++;
    }
    return res;
}

static PyMethodDef memory_methods[] = {
    {"tobytes", (PyCFunction)memory_tobytes, METH_NOARGS, NULL},
    {"tolist", (PyCFunction)memory_tolist, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};


static void
memory_dealloc(PyMemoryViewObject *self)
{
    _PyObject_GC_UNTRACK(self);
    if (self->view.obj != NULL) {
        if (self->base && PyTuple_Check(self->base)) {
            /* Special case when first element is generic object
               with buffer interface and the second element is a
               contiguous "shadow" that must be copied back into
               the data areay of the first tuple element before
               releasing the buffer on the first element.
            */

            PyObject_CopyData(PyTuple_GET_ITEM(self->base,0),
                              PyTuple_GET_ITEM(self->base,1));

            /* The view member should have readonly == -1 in
               this instance indicating that the memory can
               be "locked" and was locked and will be unlocked
               again after this call.
            */
            PyBuffer_Release(&(self->view));
        }
        else {
            PyBuffer_Release(&(self->view));
        }
        Py_CLEAR(self->base);
    }
    PyObject_GC_Del(self);
}

static PyObject *
memory_repr(PyMemoryViewObject *self)
{
    return PyString_FromFormat("<memory at %p>", self);
}

/* Sequence methods */
static Py_ssize_t
memory_length(PyMemoryViewObject *self)
{
    return get_shape0(&self->view);
}

/* Alternate version of memory_subcript that only accepts indices.
   Used by PySeqIter_New().
*/
static PyObject *
memory_item(PyMemoryViewObject *self, Py_ssize_t result)
{
    Py_buffer *view = &(self->view);

    if (view->ndim == 0) {
        PyErr_SetString(PyExc_IndexError,
                        "invalid indexing of 0-dim memory");
        return NULL;
    }
    if (view->ndim == 1) {
        /* Return a bytes object */
        char *ptr;
        ptr = (char *)view->buf;
        if (result < 0) {
            result += get_shape0(view);
        }
        if ((result < 0) || (result >= get_shape0(view))) {
            PyErr_SetString(PyExc_IndexError,
                                "index out of bounds");
            return NULL;
        }
        if (view->strides == NULL)
            ptr += view->itemsize * result;
        else
            ptr += view->strides[0] * result;
        if (view->suboffsets != NULL &&
            view->suboffsets[0] >= 0) {
            ptr = *((char **)ptr) + view->suboffsets[0];
        }
        return PyBytes_FromStringAndSize(ptr, view->itemsize);
    } else {
        /* Return a new memory-view object */
        Py_buffer newview;
        memset(&newview, 0, sizeof(newview));
        /* XXX:  This needs to be fixed so it actually returns a sub-view */
        return PyMemoryView_FromBuffer(&newview);
    }
}

/*
  mem[obj] returns a bytes object holding the data for one element if
           obj fully indexes the memory view or another memory-view object
           if it does not.

           0-d memory-view objects can be referenced using ... or () but
           not with anything else.
 */
static PyObject *
memory_subscript(PyMemoryViewObject *self, PyObject *key)
{
    Py_buffer *view;
    view = &(self->view);
    
    if (view->ndim == 0) {
        if (key == Py_Ellipsis ||
            (PyTuple_Check(key) && PyTuple_GET_SIZE(key)==0)) {
            Py_INCREF(self);
            return (PyObject *)self;
        }
        else {
            PyErr_SetString(PyExc_IndexError,
                                "invalid indexing of 0-dim memory");
            return NULL;
        }
    }
    if (PyIndex_Check(key)) {
        Py_ssize_t result;
        result = PyNumber_AsSsize_t(key, NULL);
        if (result == -1 && PyErr_Occurred())
                return NULL;
        return memory_item(self, result);
    }
    else if (PySlice_Check(key)) {
        Py_ssize_t start, stop, step, slicelength;

        if (_PySlice_Unpack(key, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = _PySlice_AdjustIndices(get_shape0(view), &start, &stop,
                                            step);
    
        if (step == 1 && view->ndim == 1) {
            Py_buffer newview;
            void *newbuf = (char *) view->buf
                                    + start * view->itemsize;
            int newflags = view->readonly
                    ? PyBUF_CONTIG_RO : PyBUF_CONTIG;
    
            /* XXX There should be an API to create a subbuffer */
            if (view->obj != NULL) {
                if (PyObject_GetBuffer(view->obj, &newview, newflags) == -1)
                    return NULL;
            }
            else {
                newview = *view;
            }
            newview.buf = newbuf;
            newview.len = slicelength * newview.itemsize;
            newview.format = view->format;
            newview.shape = &(newview.smalltable[0]);
            newview.shape[0] = slicelength;
            newview.strides = &(newview.itemsize);
            return PyMemoryView_FromBuffer(&newview);
        }
        PyErr_SetNone(PyExc_NotImplementedError);
        return NULL;
    }
    PyErr_Format(PyExc_TypeError,
        "cannot index memory using \"%.200s\"", 
        key->ob_type->tp_name);
    return NULL;
}


/* Need to support assigning memory if we can */
static int
memory_ass_sub(PyMemoryViewObject *self, PyObject *key, PyObject *value)
{
    Py_ssize_t start, len, bytelen;
    Py_buffer srcview;
    Py_buffer *view = &(self->view);
    char *srcbuf, *destbuf;

    if (view->readonly) {
        PyErr_SetString(PyExc_TypeError,
            "cannot modify read-only memory");
        return -1;
    }
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "cannot delete memory");
        return -1;
    }
    if (view->ndim != 1) {
        PyErr_SetNone(PyExc_NotImplementedError);
        return -1;
    }
    if (PyIndex_Check(key)) {
        start = PyNumber_AsSsize_t(key, NULL);
        if (start == -1 && PyErr_Occurred())
            return -1;
        if (start < 0) {
            start += get_shape0(view);
        }
        if ((start < 0) || (start >= get_shape0(view))) {
            PyErr_SetString(PyExc_IndexError,
                            "index out of bounds");
            return -1;
        }
        len = 1;
    }
    else if (PySlice_Check(key)) {
        Py_ssize_t stop, step;

        if (_PySlice_Unpack(key, &start, &stop, &step) < 0) {
            return -1;
        }
        len = _PySlice_AdjustIndices(get_shape0(view), &start, &stop, step);
        if (step != 1) {
            PyErr_SetNone(PyExc_NotImplementedError);
            return -1;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "cannot index memory using \"%.200s\"", 
            key->ob_type->tp_name);
        return -1;
    }
    if (PyObject_GetBuffer(value, &srcview, PyBUF_CONTIG_RO) == -1) {
        return -1;
    }
    /* XXX should we allow assignment of different item sizes
       as long as the byte length is the same?
       (e.g. assign 2 shorts to a 4-byte slice) */
    if (srcview.itemsize != view->itemsize) {
        PyErr_Format(PyExc_TypeError,
            "mismatching item sizes for \"%.200s\" and \"%.200s\"", 
            view->obj->ob_type->tp_name, srcview.obj->ob_type->tp_name);
        goto _error;
    }
    bytelen = len * view->itemsize;
    if (bytelen != srcview.len) {
        PyErr_SetString(PyExc_ValueError,
            "cannot modify size of memoryview object");
        goto _error;
    }
    /* Do the actual copy */
    destbuf = (char *) view->buf + start * view->itemsize;
    srcbuf = (char *) srcview.buf;
    if (destbuf + bytelen < srcbuf || srcbuf + bytelen < destbuf)
        /* No overlapping */
        memcpy(destbuf, srcbuf, bytelen);
    else
        memmove(destbuf, srcbuf, bytelen);

    PyBuffer_Release(&srcview);
    return 0;

_error:
    PyBuffer_Release(&srcview);
    return -1;
}

static PyObject *
memory_richcompare(PyObject *v, PyObject *w, int op)
{
    Py_buffer vv, ww;
    int equal = 0;
    PyObject *res;

    vv.obj = NULL;
    ww.obj = NULL;
    if (op != Py_EQ && op != Py_NE)
        goto _notimpl;
    if (PyObject_GetBuffer(v, &vv, PyBUF_CONTIG_RO) == -1) {
        PyErr_Clear();
        goto _notimpl;
    }
    if (PyObject_GetBuffer(w, &ww, PyBUF_CONTIG_RO) == -1) {
        PyErr_Clear();
        goto _notimpl;
    }

    if (vv.itemsize != ww.itemsize || vv.len != ww.len)
        goto _end;

    equal = !memcmp(vv.buf, ww.buf, vv.len);

_end:
    PyBuffer_Release(&vv);
    PyBuffer_Release(&ww);
    if ((equal && op == Py_EQ) || (!equal && op == Py_NE))
        res = Py_True;
    else
        res = Py_False;
    Py_INCREF(res);
    return res;

_notimpl:
    PyBuffer_Release(&vv);
    PyBuffer_Release(&ww);
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
}


static int
memory_traverse(PyMemoryViewObject *self, visitproc visit, void *arg)
{
    if (self->base != NULL)
        Py_VISIT(self->base);
    if (self->view.obj != NULL)
        Py_VISIT(self->view.obj);
    return 0;
}

static int
memory_clear(PyMemoryViewObject *self)
{
    Py_CLEAR(self->base);
    PyBuffer_Release(&self->view);
    return 0;
}


/* As mapping */
static PyMappingMethods memory_as_mapping = {
    (lenfunc)memory_length,               /* mp_length */
    (binaryfunc)memory_subscript,         /* mp_subscript */
    (objobjargproc)memory_ass_sub,        /* mp_ass_subscript */
};

static PySequenceMethods memory_as_sequence = {
	0,                                  /* sq_length */
	0,                                  /* sq_concat */
	0,                                  /* sq_repeat */
	(ssizeargfunc)memory_item,          /* sq_item */
};

/* Buffer methods */
static PyBufferProcs memory_as_buffer = {
    0,                                    /* bf_getreadbuffer */
    0,                                    /* bf_getwritebuffer */
    0,                                    /* bf_getsegcount */
    0,                                    /* bf_getcharbuffer */
    (getbufferproc)memory_getbuf,         /* bf_getbuffer */
    (releasebufferproc)memory_releasebuf, /* bf_releasebuffer */
};


PyTypeObject PyMemoryView_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "memoryview",
    sizeof(PyMemoryViewObject),
    0,
    (destructor)memory_dealloc,               /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    (reprfunc)memory_repr,                    /* tp_repr */
    0,                                        /* tp_as_number */
    &memory_as_sequence,                      /* tp_as_sequence */
    &memory_as_mapping,                       /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    PyObject_GenericGetAttr,                  /* tp_getattro */
    0,                                        /* tp_setattro */
    &memory_as_buffer,                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_HAVE_NEWBUFFER,            /* tp_flags */
    memory_doc,                               /* tp_doc */
    (traverseproc)memory_traverse,            /* tp_traverse */
    (inquiry)memory_clear,                    /* tp_clear */
    memory_richcompare,                       /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    memory_methods,                           /* tp_methods */
    0,                                        /* tp_members */
    memory_getsetlist,                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    memory_new,                               /* tp_new */
};

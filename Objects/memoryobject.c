
/* Memoryview object implementation */

#include "Python.h"

static int
memory_getbuf(PyMemoryViewObject *self, PyBuffer *view, int flags)
{
        if (view != NULL) 
                memcpy(view, &(self->view), sizeof(PyBuffer));
        return self->base->ob_type->tp_as_buffer->bf_getbuffer(self->base, 
                                                               NULL, PyBUF_FULL);
}

static void
memory_releasebuf(PyMemoryViewObject *self, PyBuffer *view) 
{
        PyObject_ReleaseBuffer(self->base, NULL);
}

PyDoc_STRVAR(memory_doc,
"memoryview(object)\n\
\n\
Create a new memoryview object which references the given object.");

PyObject *
PyMemoryView_FromMemory(PyBuffer *info)
{
	/* XXX(nnorwitz): need to implement something here? */
        PyErr_SetString(PyExc_NotImplementedError, "need to implement");
        return NULL;
}

PyObject *
PyMemoryView_FromObject(PyObject *base)
{
        PyMemoryViewObject *mview;

        if (!PyObject_CheckBuffer(base)) {
                PyErr_SetString(PyExc_TypeError, 
                                "cannot make memory view because object does "\
                                "not have the buffer interface");
                return NULL;                               
        }
        
        mview = (PyMemoryViewObject *)PyObject_New(PyMemoryViewObject, 
                                                   &PyMemoryView_Type);
        if (mview == NULL) return NULL;
        
        mview->base = NULL;
        if (PyObject_GetBuffer(base, &(mview->view), PyBUF_FULL) < 0) {
                Py_DECREF(mview);
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
        if (!PyArg_UnpackTuple(args, "memoryview", 1, 1, &obj)) return NULL;

        return PyMemoryView_FromObject(obj);        
}


static void
_strided_copy_nd(char *dest, char *src, int nd, Py_ssize_t *shape, 
                 Py_ssize_t *strides, int itemsize, char fort)
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

void _add_one_to_index_F(int nd, Py_ssize_t *index, Py_ssize_t *shape);
void _add_one_to_index_C(int nd, Py_ssize_t *index, Py_ssize_t *shape);

static int
_indirect_copy_nd(char *dest, PyBuffer *view, char fort)
{
        Py_ssize_t *indices;
        int k;
        Py_ssize_t elements;
        char *ptr;
        void (*func)(int, Py_ssize_t *, Py_ssize_t *);
        
        
        /* XXX(nnorwitz): need to check for overflow! */
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
                func = _add_one_to_index_F;
        }
        else {
                func = _add_one_to_index_C;
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
   PyBUF_WRITE buffer needs to be writeable (give error if not contiguous)
   PyBUF_SHADOW buffer needs to be writeable so shadow it with 
                a contiguous buffer if it is not. The view will point to
                the shadow buffer which can be written to and then
                will be copied back into the other buffer when the memory
                view is de-allocated.
 */

PyObject *
PyMemoryView_GetContiguous(PyObject *obj, int buffertype, char fort)
{
        PyMemoryViewObject *mem;
        PyObject *bytes;
        PyBuffer *view;
        int flags;
        char *dest;

        if (!PyObject_CheckBuffer(obj)) {
                PyErr_SetString(PyExc_TypeError,
                                "object does not have the buffer interface");
                return NULL;
        }
        
        mem = PyObject_New(PyMemoryViewObject, &PyMemoryView_Type);
        if (mem == NULL) return NULL;

        view = &PyMemoryView(mem);
        flags = PyBUF_FULL_RO;
        switch(buffertype) {
        case PyBUF_WRITE:
                flags = PyBUF_FULL;
                break;
        case PyBUF_SHADOW:
                flags = PyBUF_FULL_LCK;
                break;
        }

        if (PyObject_GetBuffer(obj, view, flags) != 0) {
                PyObject_DEL(mem);
                return NULL;
        }

        if (PyBuffer_IsContiguous(view, fort)) {
                /* no copy needed */
                Py_INCREF(obj);
                mem->base = obj;
                return (PyObject *)mem;
        }
        /* otherwise a copy is needed */
        if (buffertype == PyBUF_WRITE) {
                PyObject_DEL(mem);
                PyErr_SetString(PyExc_BufferError,
                                "writeable contiguous buffer requested for a non-contiguous" \
                                "object.");
                return NULL;
        }
        bytes = PyBytes_FromStringAndSize(NULL, view->len);
        if (bytes == NULL) {
                PyObject_ReleaseBuffer(obj, view);
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
                        PyObject_ReleaseBuffer(obj, view);
                        return NULL;
                }                 
        }
        if (buffertype == PyBUF_SHADOW) {
                /* return a shadowed memory-view object */
                view->buf = dest;
                mem->base = PyTuple_Pack(2, obj, bytes);
		/* XXX(nnorwitz): need to verify alloc was successful. */
                Py_DECREF(bytes);
        }
        else {
                PyObject_ReleaseBuffer(obj, view);
                /* steal the reference */
                mem->base = bytes;
        }
        return (PyObject *)mem;
}


static PyObject *
memory_format_get(PyMemoryViewObject *self)
{
        return PyUnicode_FromString(self->view.format);
}

static PyObject *
memory_itemsize_get(PyMemoryViewObject *self)
{
        return PyInt_FromLong(self->view.itemsize);
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
                o = PyInt_FromSsize_t(vals[i]);
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
memory_size_get(PyMemoryViewObject *self)
{
        return PyInt_FromSsize_t(self->view.len);
}

static PyObject *
memory_readonly_get(PyMemoryViewObject *self)
{
        return PyBool_FromLong(self->view.readonly);
}

static PyObject *
memory_ndim_get(PyMemoryViewObject *self)
{
        return PyInt_FromLong(self->view.ndim);
}

static PyGetSetDef memory_getsetlist[] ={ 
        {"format",	(getter)memory_format_get,	NULL, NULL},
        {"itemsize",	(getter)memory_itemsize_get,	NULL, NULL},
        {"shape",	(getter)memory_shape_get,	NULL, NULL},
        {"strides",	(getter)memory_strides_get,	NULL, NULL},
        {"suboffsets",	(getter)memory_suboffsets_get,	NULL, NULL},
        {"size",	(getter)memory_size_get,	NULL, NULL},
        {"readonly",	(getter)memory_readonly_get,	NULL, NULL},
        {"ndim",	(getter)memory_ndim_get,	NULL, NULL},
        {NULL, NULL, NULL, NULL},
};


static PyObject *
memory_tobytes(PyMemoryViewObject *mem, PyObject *noargs)
{
        /* Create new Bytes object for data */
        return PyBytes_FromObject((PyObject *)mem);
}

static PyObject *
memory_tolist(PyMemoryViewObject *mem, PyObject *noargs)
{
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
}



static PyMethodDef memory_methods[] = {
        {"tobytes", (PyCFunction)memory_tobytes, METH_NOARGS, NULL},
        {"tolist", (PyCFunction)memory_tolist, METH_NOARGS, NULL},
        {NULL,          NULL}           /* sentinel */
};


static void
memory_dealloc(PyMemoryViewObject *self)
{
        if (self->base != NULL) {
            if (PyTuple_Check(self->base)) {
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
                PyObject_ReleaseBuffer(PyTuple_GET_ITEM(self->base,0),
                                       &(self->view));
            }
            else {
                PyObject_ReleaseBuffer(self->base, &(self->view));
            }
            Py_CLEAR(self->base);
        }
        PyObject_DEL(self);
}

static PyObject *
memory_repr(PyMemoryViewObject *self)
{
	/* XXX(nnorwitz): the code should be different or remove condition. */
	if (self->base == NULL)
		return PyUnicode_FromFormat("<memory at %p>", self);
	else
		return PyUnicode_FromFormat("<memory at %p>", self);
}


static PyObject *
memory_str(PyMemoryViewObject *self)
{
        PyBuffer view;
        PyObject *res;

        if (PyObject_GetBuffer((PyObject *)self, &view, PyBUF_FULL) < 0)
                return NULL;
        
	res = PyBytes_FromStringAndSize(NULL, view.len);
        PyBuffer_ToContiguous(PyBytes_AS_STRING(res), &view, view.len, 'C');
        PyObject_ReleaseBuffer((PyObject *)self, &view);
        return res;
}

/* Sequence methods */

static Py_ssize_t
memory_length(PyMemoryViewObject *self)
{
        PyBuffer view;

        if (PyObject_GetBuffer((PyObject *)self, &view, PyBUF_FULL) < 0)
                return -1;
        PyObject_ReleaseBuffer((PyObject *)self, &view);
	return view.len;
}

static PyObject *
memory_subscript(PyMemoryViewObject *self, PyObject *key)
{
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
}

static int
memory_ass_sub(PyMemoryViewObject *self, PyObject *key, PyObject *value)
{
        return 0;
}

/* As mapping */
static PyMappingMethods memory_as_mapping = {
	(lenfunc)memory_length, /*mp_length*/
	(binaryfunc)memory_subscript, /*mp_subscript*/
	(objobjargproc)memory_ass_sub, /*mp_ass_subscript*/
};


/* Buffer methods */

static PyBufferProcs memory_as_buffer = {
	(getbufferproc)memory_getbuf,         /* bf_getbuffer */
        (releasebufferproc)memory_releasebuf, /* bf_releasebuffer */
};


PyTypeObject PyMemoryView_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"memoryview",
	sizeof(PyMemoryViewObject),
	0,
	(destructor)memory_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)memory_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,			                /* tp_as_sequence */
	&memory_as_mapping,	    	        /* tp_as_mapping */
	0,		                        /* tp_hash */
	0,					/* tp_call */
	(reprfunc)memory_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	&memory_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	memory_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,		                	/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	memory_methods,	   		        /* tp_methods */	
	0,	      		                /* tp_members */
	memory_getsetlist,  		        /* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	memory_new,				/* tp_new */
};

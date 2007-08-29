
/* Buffer object implementation */

#include "Python.h"


typedef struct {
	PyObject_HEAD
	PyObject *b_base;
	void *b_ptr;
	Py_ssize_t b_size;
	Py_ssize_t b_offset;
	int b_readonly;
	long b_hash;
} PyBufferObject;


static int
get_buf(PyBufferObject *self, PyBuffer *view, int flags)
{
	if (self->b_base == NULL) {
		view->buf = self->b_ptr;
		view->len = self->b_size;
	}
	else {
		Py_ssize_t count, offset;
		PyBufferProcs *bp = self->b_base->ob_type->tp_as_buffer;
                if ((*bp->bf_getbuffer)(self->b_base, view, flags) < 0)
                        return 0;
                count = view->len;
		/* apply constraints to the start/end */
		if (self->b_offset > count)
			offset = count;
		else
			offset = self->b_offset;
                view->buf = (char*)view->buf + offset;
		if (self->b_size == Py_END_OF_BUFFER)
			view->len = count;
		else
			view->len = self->b_size;
		if (offset + view->len > count)
			view->len = count - offset;
	}
	return 1;
}


static int
buffer_getbuf(PyBufferObject *self, PyBuffer *view, int flags)
{
        if (view == NULL) return 0;
        if (!get_buf(self, view, flags))
                return -1;
        return PyBuffer_FillInfo(view, view->buf, view->len, self->b_readonly,
                                 flags);
}


static void
buffer_releasebuf(PyBufferObject *self, PyBuffer *view) 
{
        /* No-op if there is no self->b_base */
	if (self->b_base != NULL) {
		PyBufferProcs *bp = self->b_base->ob_type->tp_as_buffer;
                if (bp->bf_releasebuffer != NULL) {
                        (*bp->bf_releasebuffer)(self->b_base, view);
                }
        }
}

static PyObject *
buffer_from_memory(PyObject *base, Py_ssize_t size, Py_ssize_t offset,
                   void *ptr, int readonly)
{
	PyBufferObject * b;

	if (size < 0 && size != Py_END_OF_BUFFER) {
		PyErr_SetString(PyExc_ValueError,
				"size must be zero or positive");
		return NULL;
	}
	if (offset < 0) {
		PyErr_SetString(PyExc_ValueError,
				"offset must be zero or positive");
		return NULL;
	}

	b = PyObject_NEW(PyBufferObject, &PyBuffer_Type);
	if (b == NULL)
		return NULL;

	Py_XINCREF(base);
	b->b_base = base;
	b->b_ptr = ptr;
	b->b_size = size;
	b->b_offset = offset;
	b->b_readonly = readonly;
	b->b_hash = -1;

	return (PyObject *) b;
}

static PyObject *
buffer_from_object(PyObject *base, Py_ssize_t size, Py_ssize_t offset,
                   int readonly)
{
	if (offset < 0) {
		PyErr_SetString(PyExc_ValueError,
				"offset must be zero or positive");
		return NULL;
	}
	if (PyBuffer_Check(base) && (((PyBufferObject *)base)->b_base)) {
		/* another buffer, refer to the base object */
		PyBufferObject *b = (PyBufferObject *)base;
		if (b->b_size != Py_END_OF_BUFFER) {
			Py_ssize_t base_size = b->b_size - offset;
			if (base_size < 0)
				base_size = 0;
			if (size == Py_END_OF_BUFFER || size > base_size)
				size = base_size;
		}
		offset += b->b_offset;
		base = b->b_base;
	}
	return buffer_from_memory(base, size, offset, NULL, readonly);
}


PyObject *
PyBuffer_FromObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if (pb == NULL ||
            pb->bf_getbuffer == NULL) {
                PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return buffer_from_object(base, size, offset, 1);
}

PyObject *
PyBuffer_FromReadWriteObject(PyObject *base, Py_ssize_t offset,
                             Py_ssize_t size)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if (pb == NULL ||
            pb->bf_getbuffer == NULL) {
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return buffer_from_object(base, size,  offset, 0);
}

PyObject *
PyBuffer_FromMemory(void *ptr, Py_ssize_t size)
{
	return buffer_from_memory(NULL, size, 0, ptr, 1);
}

PyObject *
PyBuffer_FromReadWriteMemory(void *ptr, Py_ssize_t size)
{
	return buffer_from_memory(NULL, size, 0, ptr, 0);
}

PyObject *
PyBuffer_New(Py_ssize_t size)
{
	PyObject *o;
	PyBufferObject * b;

	if (size < 0) {
		PyErr_SetString(PyExc_ValueError,
				"size must be zero or positive");
		return NULL;
	}
	/* XXX: check for overflow in multiply */
	/* Inline PyObject_New */
	o = (PyObject *)PyObject_MALLOC(sizeof(*b) + size);
	if (o == NULL)
		return PyErr_NoMemory();
	b = (PyBufferObject *) PyObject_INIT(o, &PyBuffer_Type);

	b->b_base = NULL;
	b->b_ptr = (void *)(b + 1);
	b->b_size = size;
	b->b_offset = 0;
	b->b_readonly = 0;
	b->b_hash = -1;

	return o;
}

/* Methods */

static PyObject *
buffer_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *ob;
	Py_ssize_t offset = 0;
	Py_ssize_t size = Py_END_OF_BUFFER;

	if (!_PyArg_NoKeywords("buffer()", kw))
		return NULL;

	if (!PyArg_ParseTuple(args, "O|nn:buffer", &ob, &offset, &size))
	    return NULL;
	return PyBuffer_FromObject(ob, offset, size);
}

PyDoc_STRVAR(buffer_doc,
"buffer(object [, offset[, size]])\n\
\n\
Create a new buffer object which references the given object.\n\
The buffer will reference a slice of the target object from the\n\
start of the object (or at the specified offset). The slice will\n\
extend to the end of the target object (or with the specified size).");


static void
buffer_dealloc(PyBufferObject *self)
{
	Py_XDECREF(self->b_base);
	PyObject_DEL(self);
}

static int
get_bufx(PyObject *obj, PyBuffer *view, int flags)
{
	PyBufferProcs *bp;

	if (PyBuffer_Check(obj)) {
		if (!get_buf((PyBufferObject *)obj, view, flags)) {
			PyErr_Clear();
			return 0;
		}
		else
			return 1;
	}
	bp = obj->ob_type->tp_as_buffer;
	if (bp == NULL ||
	    bp->bf_getbuffer == NULL)
		return 0;
	if ((*bp->bf_getbuffer)(obj, view, PyBUF_SIMPLE) < 0)
		return 0;
        return 1;
}

static PyObject *
buffer_richcompare(PyObject *self, PyObject *other, int op)
{
	void *p1, *p2;
	Py_ssize_t len1, len2, min_len;
	int cmp, ok;
        PyBuffer v1, v2;

	ok = 1;
	if (!get_bufx(self, &v1, PyBUF_SIMPLE))
		ok = 0;
	if (!get_bufx(other, &v2, PyBUF_SIMPLE)) {
                if (ok) PyObject_ReleaseBuffer((PyObject *)self, &v1);
		ok = 0;
        }
	if (!ok) {
		/* If we can't get the buffers,
		   == and != are still defined
		   (and the objects are unequal) */
		PyObject *result;
		if (op == Py_EQ)
			result = Py_False;
		else if (op == Py_NE)
			result = Py_True;
		else
			result = Py_NotImplemented;
		Py_INCREF(result);
		return result;
	}
        len1 = v1.len;
        len2 = v2.len;
        p1 = v1.buf;
        p2 = v2.buf;
	min_len = (len1 < len2) ? len1 : len2;
	cmp = memcmp(p1, p2, min_len);
	if (cmp == 0)
		cmp = (len1 < len2) ? -1 :
		      (len1 > len2) ? 1 : 0;
        PyObject_ReleaseBuffer((PyObject *)self, &v1);
        PyObject_ReleaseBuffer(other, &v2);
	return Py_CmpToRich(op, cmp);
}

static PyObject *
buffer_repr(PyBufferObject *self)
{
	const char *status = self->b_readonly ? "read-only" : "read-write";

	if (self->b_base == NULL)
		return PyUnicode_FromFormat(
                                "<%s buffer ptr %p, size %zd at %p>",
                                status,
                                self->b_ptr,
                                self->b_size,
                                self);
	else
		return PyUnicode_FromFormat(
			"<%s buffer for %p, size %zd, offset %zd at %p>",
			status,
			self->b_base,
			self->b_size,
			self->b_offset,
			self);
}

static long
buffer_hash(PyBufferObject *self)
{
        PyBuffer view;
	register Py_ssize_t len;
	register unsigned char *p;
	register long x;

	if (self->b_hash != -1)
		return self->b_hash;

        if (!get_buf(self, &view, PyBUF_SIMPLE))
		return -1;
        if (!(self->b_readonly)) {
                PyErr_SetString(PyExc_TypeError,
                                "writable buffers are not hashable");
                PyObject_ReleaseBuffer((PyObject *)self, &view);
		return -1;
	}
                
	p = (unsigned char *) view.buf;
	len = view.len;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= view.len;
	if (x == -1)
		x = -2;
	self->b_hash = x;
        PyObject_ReleaseBuffer((PyObject *)self, &view);
	return x;
}

static PyObject *
buffer_str(PyBufferObject *self)
{
        PyBuffer view;
        PyObject *res;

	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return NULL;
	res = PyString_FromStringAndSize((const char *)view.buf, view.len);
        PyObject_ReleaseBuffer((PyObject *)self, &view);
        return res;
}

/* Sequence methods */

static Py_ssize_t
buffer_length(PyBufferObject *self)
{
        PyBuffer view;

	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return -1;
        PyObject_ReleaseBuffer((PyObject *)self, &view);
	return view.len;
}

static PyObject *
buffer_concat(PyBufferObject *self, PyObject *other)
{
	PyBufferProcs *pb = other->ob_type->tp_as_buffer;
	char *p;
	PyObject *ob;
        PyBuffer view, view2;

	if (pb == NULL ||
            pb->bf_getbuffer == NULL)
	{
		PyErr_BadArgument();
		return NULL;
	}

 	if (!get_buf(self, &view, PyBUF_SIMPLE))
 		return NULL;
 
	/* optimize special case */
        /* XXX bad idea type-wise */
	if (view.len == 0) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
                Py_INCREF(other);
                return other;
	}

        if (PyObject_GetBuffer((PyObject *)other, &view2, PyBUF_SIMPLE) < 0) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
                return NULL;
        }

	/* XXX(nnorwitz): need to check for overflow! */
 	ob = PyBytes_FromStringAndSize(NULL, view.len+view2.len);
	if (ob == NULL) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
                PyObject_ReleaseBuffer(other, &view2);
		return NULL;
        }
 	p = PyBytes_AS_STRING(ob);
 	memcpy(p, view.buf, view.len);
 	memcpy(p + view.len, view2.buf, view2.len);

        PyObject_ReleaseBuffer((PyObject *)self, &view);
        PyObject_ReleaseBuffer(other, &view2);
        return ob;
}

static PyObject *
buffer_repeat(PyBufferObject *self, Py_ssize_t count)
{
	PyObject *ob;
	register char *p;
        PyBuffer view;

	if (count < 0)
		count = 0;
	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return NULL;
	/* XXX(nnorwitz): need to check for overflow! */
	ob = PyBytes_FromStringAndSize(NULL, view.len * count);
	if (ob == NULL)
		return NULL;

	p = PyBytes_AS_STRING(ob);
	while (count--) {
	    memcpy(p, view.buf, view.len);
	    p += view.len;
	}

        PyObject_ReleaseBuffer((PyObject *)self, &view);
	return ob;
}

static PyObject *
buffer_item(PyBufferObject *self, Py_ssize_t idx)
{
        PyBuffer view;
        PyObject *ob;

	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return NULL;
	if (idx < 0 || idx >= view.len) {
		PyErr_SetString(PyExc_IndexError, "buffer index out of range");
		return NULL;
	}
	ob = PyBytes_FromStringAndSize((char *)view.buf + idx, 1);
        PyObject_ReleaseBuffer((PyObject *)self, &view);
        return ob;
}

static PyObject *
buffer_slice(PyBufferObject *self, Py_ssize_t left, Py_ssize_t right)
{
        PyObject *ob;
        PyBuffer view;
	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return NULL;
	if (left < 0)
		left = 0;
	if (right < 0)
		right = 0;
	if (right > view.len)
		right = view.len;
	if (right < left)
		right = left;
	/* XXX(nnorwitz): is it possible to access unitialized memory? */
	ob = PyBytes_FromStringAndSize((char *)view.buf + left,
                                       right - left);
        PyObject_ReleaseBuffer((PyObject *)self, &view);
        return ob;
}

static PyObject *
buffer_subscript(PyBufferObject *self, PyObject *item)
{
	PyBuffer view;
	PyObject *ob;
	
	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return NULL;
	if (PyIndex_Check(item)) {
		Py_ssize_t idx = PyNumber_AsSsize_t(item, PyExc_IndexError);

		if (idx == -1 && PyErr_Occurred())
			return NULL;
		if (idx < 0)
			idx += view.len;
		if (idx < 0 || idx >= view.len) {
			PyErr_SetString(PyExc_IndexError,
					"buffer index out of range");
			return NULL;
		}
		ob = PyBytes_FromStringAndSize((char *)view.buf + idx, 1);
		PyObject_ReleaseBuffer((PyObject *)self, &view);
		return ob;
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength, cur, i;

		if (PySlice_GetIndicesEx((PySliceObject*)item, view.len,
				 &start, &stop, &step, &slicelength) < 0) {
			PyObject_ReleaseBuffer((PyObject *)self, &view);
			return NULL;
		}

		if (slicelength <= 0) {
			PyObject_ReleaseBuffer((PyObject *)self, &view);
			return PyBytes_FromStringAndSize("", 0);
		}
		else if (step == 1) {
			ob = PyBytes_FromStringAndSize((char *)view.buf +
							start, stop - start);
			PyObject_ReleaseBuffer((PyObject *)self, &view);
			return ob;
		}
		else {
			char *source_buf = (char *)view.buf;
			char *result_buf = (char *)PyMem_Malloc(slicelength);

			if (result_buf == NULL)
				return PyErr_NoMemory();

			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				result_buf[i] = source_buf[cur];
			}

			ob = PyBytes_FromStringAndSize(result_buf,
						       slicelength);
			PyMem_Free(result_buf);
			PyObject_ReleaseBuffer((PyObject *)self, &view);
			return ob;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"sequence index must be integer");
		return NULL;
	}
}

static int
buffer_ass_item(PyBufferObject *self, Py_ssize_t idx, PyObject *other)
{
	PyBufferProcs *pb;
        PyBuffer view, view2;

	if (!get_buf(self, &view, PyBUF_SIMPLE))
		return -1;
        
	if (self->b_readonly || view.readonly) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
                PyObject_ReleaseBuffer((PyObject *)self, &view);
		return -1;
	}

	if (idx < 0 || idx >= view.len) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
		PyErr_SetString(PyExc_IndexError,
				"buffer assignment index out of range");
		return -1;
	}

	pb = other ? other->ob_type->tp_as_buffer : NULL;
	if (pb == NULL ||
            pb->bf_getbuffer == NULL) {
		PyErr_BadArgument();
                PyObject_ReleaseBuffer((PyObject *)self, &view);
		return -1;
	}
        
        if (PyObject_GetBuffer(other, &view2, PyBUF_SIMPLE) < 0) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
                return -1;
        }
	if (view.len != 1) {
                PyObject_ReleaseBuffer((PyObject *)self, &view);
                PyObject_ReleaseBuffer(other, &view2);
		PyErr_SetString(PyExc_TypeError,
				"right operand must be a single byte");
		return -1;
	}

	((char *)(view.buf))[idx] = *((char *)(view2.buf));
        PyObject_ReleaseBuffer((PyObject *)self, &view);
        PyObject_ReleaseBuffer(other, &view2);
	return 0;
}

static int
buffer_ass_slice(PyBufferObject *self, Py_ssize_t left, Py_ssize_t right,
                 PyObject *other)
{
	PyBufferProcs *pb;
        PyBuffer v1, v2;
	Py_ssize_t slice_len;

	pb = other ? other->ob_type->tp_as_buffer : NULL;
	if (pb == NULL ||
            pb->bf_getbuffer == NULL) {
		PyErr_BadArgument();
		return -1;
	}
	if (!get_buf(self, &v1, PyBUF_SIMPLE))
                return -1;

	if (self->b_readonly || v1.readonly) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
                PyObject_ReleaseBuffer((PyObject *)self, &v1);
		return -1;
	}

        if ((*pb->bf_getbuffer)(other, &v2, PyBUF_SIMPLE) < 0) {
                PyObject_ReleaseBuffer((PyObject *)self, &v1);
                return -1;
        }

	if (left < 0)
		left = 0;
	else if (left > v1.len)
		left = v1.len;
	if (right < left)
		right = left;
	else if (right > v1.len)
		right = v1.len;
	slice_len = right - left;

	if (v2.len != slice_len) {
		PyErr_SetString(
			PyExc_TypeError,
			"right operand length must match slice length");
                PyObject_ReleaseBuffer((PyObject *)self, &v1);
                PyObject_ReleaseBuffer(other, &v2);
		return -1;
	}

	if (slice_len)
	    memcpy((char *)v1.buf + left, v2.buf, slice_len);

        PyObject_ReleaseBuffer((PyObject *)self, &v1);
        PyObject_ReleaseBuffer(other, &v2);        
	return 0;
}

static int
buffer_ass_subscript(PyBufferObject *self, PyObject *item, PyObject *value)
{
	PyBuffer v1;

	if (!get_buf(self, &v1, PyBUF_SIMPLE))
		return -1;
	if (self->b_readonly || v1.readonly) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
		PyObject_ReleaseBuffer((PyObject *)self, &v1);
		return -1;
	}
	if (PyIndex_Check(item)) {
		Py_ssize_t idx = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (idx == -1 && PyErr_Occurred())
			return -1;
		if (idx < 0)
			idx += v1.len;
		PyObject_ReleaseBuffer((PyObject *)self, &v1);
		return buffer_ass_item(self, idx, value);
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;
		PyBuffer v2;
		PyBufferProcs *pb;
		
		if (PySlice_GetIndicesEx((PySliceObject *)item, v1.len,
				&start, &stop, &step, &slicelength) < 0) {
			PyObject_ReleaseBuffer((PyObject *)self, &v1);
			return -1;
		}

		pb = value ? value->ob_type->tp_as_buffer : NULL;
		if (pb == NULL ||
		    pb->bf_getbuffer == NULL) {
		    	PyObject_ReleaseBuffer((PyObject *)self, &v1);
			PyErr_BadArgument();
			return -1;
		}
		if ((*pb->bf_getbuffer)(value, &v2, PyBUF_SIMPLE) < 0) {
			PyObject_ReleaseBuffer((PyObject *)self, &v1);
			return -1;
		}

		if (v2.len != slicelength) {
			PyObject_ReleaseBuffer((PyObject *)self, &v1);
			PyObject_ReleaseBuffer(value, &v2);
			PyErr_SetString(PyExc_TypeError, "right operand"
					" length must match slice length");
			return -1;
		}

		if (slicelength == 0)
			/* nothing to do */;
		else if (step == 1)
			memcpy((char *)v1.buf + start, v2.buf, slicelength);
		else {
			Py_ssize_t cur, i;
			
			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				((char *)v1.buf)[cur] = ((char *)v2.buf)[i];
			}
		}
		PyObject_ReleaseBuffer((PyObject *)self, &v1);
		PyObject_ReleaseBuffer(value, &v2);
		return 0;
	} else {
		PyErr_SetString(PyExc_TypeError,
				"buffer indices must be integers");
		PyObject_ReleaseBuffer((PyObject *)self, &v1);
		return -1;
	}
}

/* Buffer methods */

static PySequenceMethods buffer_as_sequence = {
	(lenfunc)buffer_length, /*sq_length*/
	(binaryfunc)buffer_concat, /*sq_concat*/
	(ssizeargfunc)buffer_repeat, /*sq_repeat*/
	(ssizeargfunc)buffer_item, /*sq_item*/
	(ssizessizeargfunc)buffer_slice, /*sq_slice*/
	(ssizeobjargproc)buffer_ass_item, /*sq_ass_item*/
	(ssizessizeobjargproc)buffer_ass_slice, /*sq_ass_slice*/
};

static PyMappingMethods buffer_as_mapping = {
	(lenfunc)buffer_length,
	(binaryfunc)buffer_subscript,
	(objobjargproc)buffer_ass_subscript,
};

static PyBufferProcs buffer_as_buffer = {
	(getbufferproc)buffer_getbuf,
        (releasebufferproc)buffer_releasebuf,
};

PyTypeObject PyBuffer_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"buffer",
	sizeof(PyBufferObject),
	0,
	(destructor)buffer_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)buffer_repr,			/* tp_repr */
	0,					/* tp_as_number */
	&buffer_as_sequence,			/* tp_as_sequence */
	&buffer_as_mapping,			/* tp_as_mapping */
	(hashfunc)buffer_hash,			/* tp_hash */
	0,					/* tp_call */
	(reprfunc)buffer_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	&buffer_as_buffer,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	buffer_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	buffer_richcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */	
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	buffer_new,				/* tp_new */
};

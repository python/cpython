
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


enum buffer_t {
    READ_BUFFER,
    WRITE_BUFFER,
    CHAR_BUFFER,
    ANY_BUFFER
};

static int
get_buf(PyBufferObject *self, void **ptr, Py_ssize_t *size,
	enum buffer_t buffer_type)
{
	if (self->b_base == NULL) {
		assert (ptr != NULL);
		*ptr = self->b_ptr;
		*size = self->b_size;
	}
	else {
		Py_ssize_t count, offset;
		readbufferproc proc = 0;
		PyBufferProcs *bp = self->b_base->ob_type->tp_as_buffer;
		if ((*bp->bf_getsegcount)(self->b_base, NULL) != 1) {
			PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
			return 0;
		}
		if ((buffer_type == READ_BUFFER) ||
			((buffer_type == ANY_BUFFER) && self->b_readonly))
		    proc = bp->bf_getreadbuffer;
		else if ((buffer_type == WRITE_BUFFER) ||
			(buffer_type == ANY_BUFFER))
    		    proc = (readbufferproc)bp->bf_getwritebuffer;
		else if (buffer_type == CHAR_BUFFER) {
		    if (!PyType_HasFeature(self->ob_type,
				Py_TPFLAGS_HAVE_GETCHARBUFFER)) {
			PyErr_SetString(PyExc_TypeError,
				"Py_TPFLAGS_HAVE_GETCHARBUFFER needed");
			return 0;
		    }
		    proc = (readbufferproc)bp->bf_getcharbuffer;
		}
		if (!proc) {
		    char *buffer_type_name;
		    switch (buffer_type) {
			case READ_BUFFER:
			    buffer_type_name = "read";
			    break;
			case WRITE_BUFFER:
			    buffer_type_name = "write";
			    break;
			case CHAR_BUFFER:
			    buffer_type_name = "char";
			    break;
			default:
			    buffer_type_name = "no";
			    break;
		    }
		    PyErr_Format(PyExc_TypeError,
			    "%s buffer type not available",
			    buffer_type_name);
		    return 0;
		}
		if ((count = (*proc)(self->b_base, 0, ptr)) < 0)
			return 0;
		/* apply constraints to the start/end */
		if (self->b_offset > count)
			offset = count;
		else
			offset = self->b_offset;
		*(char **)ptr = *(char **)ptr + offset;
		if (self->b_size == Py_END_OF_BUFFER)
			*size = count;
		else
			*size = self->b_size;
		if (offset + *size > count)
			*size = count - offset;
	}
	return 1;
}


static PyObject *
buffer_from_memory(PyObject *base, Py_ssize_t size, Py_ssize_t offset, void *ptr,
		   int readonly)
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
	if ( b == NULL )
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
buffer_from_object(PyObject *base, Py_ssize_t size, Py_ssize_t offset, int readonly)
{
	if (offset < 0) {
		PyErr_SetString(PyExc_ValueError,
				"offset must be zero or positive");
		return NULL;
	}
	if ( PyBuffer_Check(base) && (((PyBufferObject *)base)->b_base) ) {
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

	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return buffer_from_object(base, size, offset, 1);
}

PyObject *
PyBuffer_FromReadWriteObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if ( pb == NULL ||
	     pb->bf_getwritebuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
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
	if ( o == NULL )
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
	if (Py_Py3kWarningFlag &&
	    PyErr_WarnEx(PyExc_DeprecationWarning,
			 "buffer() not supported in 3.x; "
			 "use memoryview()", 1) < 0)
		return NULL;
	
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
buffer_compare(PyBufferObject *self, PyBufferObject *other)
{
	void *p1, *p2;
	Py_ssize_t len_self, len_other, min_len;
	int cmp;

	if (!get_buf(self, &p1, &len_self, ANY_BUFFER))
		return -1;
	if (!get_buf(other, &p2, &len_other, ANY_BUFFER))
		return -1;
	min_len = (len_self < len_other) ? len_self : len_other;
	if (min_len > 0) {
		cmp = memcmp(p1, p2, min_len);
		if (cmp != 0)
			return cmp < 0 ? -1 : 1;
	}
	return (len_self < len_other) ? -1 : (len_self > len_other) ? 1 : 0;
}

static PyObject *
buffer_repr(PyBufferObject *self)
{
	const char *status = self->b_readonly ? "read-only" : "read-write";

	if ( self->b_base == NULL )
		return PyString_FromFormat("<%s buffer ptr %p, size %zd at %p>",
					   status,
					   self->b_ptr,
					   self->b_size,
					   self);
	else
		return PyString_FromFormat(
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
	void *ptr;
	Py_ssize_t size;
	register Py_ssize_t len;
	register unsigned char *p;
	register long x;

	if ( self->b_hash != -1 )
		return self->b_hash;

	/* XXX potential bugs here, a readonly buffer does not imply that the
	 * underlying memory is immutable.  b_readonly is a necessary but not
	 * sufficient condition for a buffer to be hashable.  Perhaps it would
	 * be better to only allow hashing if the underlying object is known to
	 * be immutable (e.g. PyString_Check() is true).  Another idea would
	 * be to call tp_hash on the underlying object and see if it raises
	 * an error. */
	if ( !self->b_readonly )
	{
		PyErr_SetString(PyExc_TypeError,
				"writable buffers are not hashable");
		return -1;
	}

	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return -1;
	p = (unsigned char *) ptr;
	len = size;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= size;
	if (x == -1)
		x = -2;
	self->b_hash = x;
	return x;
}

static PyObject *
buffer_str(PyBufferObject *self)
{
	void *ptr;
	Py_ssize_t size;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return NULL;
	return PyString_FromStringAndSize((const char *)ptr, size);
}

/* Sequence methods */

static Py_ssize_t
buffer_length(PyBufferObject *self)
{
	void *ptr;
	Py_ssize_t size;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return -1;
	return size;
}

static PyObject *
buffer_concat(PyBufferObject *self, PyObject *other)
{
	PyBufferProcs *pb = other->ob_type->tp_as_buffer;
	void *ptr1, *ptr2;
	char *p;
	PyObject *ob;
	Py_ssize_t size, count;

	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_BadArgument();
		return NULL;
	}
	if ( (*pb->bf_getsegcount)(other, NULL) != 1 )
	{
		/* ### use a different exception type/message? */
		PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
		return NULL;
	}

 	if (!get_buf(self, &ptr1, &size, ANY_BUFFER))
 		return NULL;
 
	/* optimize special case */
	if ( size == 0 )
	{
	    Py_INCREF(other);
	    return other;
	}

	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &ptr2)) < 0 )
		return NULL;

 	ob = PyString_FromStringAndSize(NULL, size + count);
	if ( ob == NULL )
		return NULL;
 	p = PyString_AS_STRING(ob);
 	memcpy(p, ptr1, size);
 	memcpy(p + size, ptr2, count);

	/* there is an extra byte in the string object, so this is safe */
	p[size + count] = '\0';

	return ob;
}

static PyObject *
buffer_repeat(PyBufferObject *self, Py_ssize_t count)
{
	PyObject *ob;
	register char *p;
	void *ptr;
	Py_ssize_t size;

	if ( count < 0 )
		count = 0;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return NULL;
	ob = PyString_FromStringAndSize(NULL, size * count);
	if ( ob == NULL )
		return NULL;

	p = PyString_AS_STRING(ob);
	while ( count-- )
	{
	    memcpy(p, ptr, size);
	    p += size;
	}

	/* there is an extra byte in the string object, so this is safe */
	*p = '\0';

	return ob;
}

static PyObject *
buffer_item(PyBufferObject *self, Py_ssize_t idx)
{
	void *ptr;
	Py_ssize_t size;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return NULL;
	if ( idx < 0 || idx >= size ) {
		PyErr_SetString(PyExc_IndexError, "buffer index out of range");
		return NULL;
	}
	return PyString_FromStringAndSize((char *)ptr + idx, 1);
}

static PyObject *
buffer_slice(PyBufferObject *self, Py_ssize_t left, Py_ssize_t right)
{
	void *ptr;
	Py_ssize_t size;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return NULL;
	if ( left < 0 )
		left = 0;
	if ( right < 0 )
		right = 0;
	if ( right > size )
		right = size;
	if ( right < left )
		right = left;
	return PyString_FromStringAndSize((char *)ptr + left,
					  right - left);
}

static PyObject *
buffer_subscript(PyBufferObject *self, PyObject *item)
{
	void *p;
	Py_ssize_t size;
	
	if (!get_buf(self, &p, &size, ANY_BUFFER))
		return NULL;
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += size;
		return buffer_item(self, i);
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength, cur, i;

		if (PySlice_GetIndicesEx((PySliceObject*)item, size,
				 &start, &stop, &step, &slicelength) < 0) {
			return NULL;
		}

		if (slicelength <= 0)
			return PyString_FromStringAndSize("", 0);
		else if (step == 1)
			return PyString_FromStringAndSize((char *)p + start,
							  stop - start);
		else {
			PyObject *result;
			char *source_buf = (char *)p;
			char *result_buf = (char *)PyMem_Malloc(slicelength);

			if (result_buf == NULL)
				return PyErr_NoMemory();

			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				result_buf[i] = source_buf[cur];
			}

			result = PyString_FromStringAndSize(result_buf,
							    slicelength);
			PyMem_Free(result_buf);
			return result;
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
	void *ptr1, *ptr2;
	Py_ssize_t size;
	Py_ssize_t count;

	if ( self->b_readonly ) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
		return -1;
	}

	if (!get_buf(self, &ptr1, &size, ANY_BUFFER))
		return -1;

	if (idx < 0 || idx >= size) {
		PyErr_SetString(PyExc_IndexError,
				"buffer assignment index out of range");
		return -1;
	}

	pb = other ? other->ob_type->tp_as_buffer : NULL;
	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_BadArgument();
		return -1;
	}
	if ( (*pb->bf_getsegcount)(other, NULL) != 1 )
	{
		/* ### use a different exception type/message? */
		PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
		return -1;
	}

	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &ptr2)) < 0 )
		return -1;
	if ( count != 1 ) {
		PyErr_SetString(PyExc_TypeError,
				"right operand must be a single byte");
		return -1;
	}

	((char *)ptr1)[idx] = *(char *)ptr2;
	return 0;
}

static int
buffer_ass_slice(PyBufferObject *self, Py_ssize_t left, Py_ssize_t right, PyObject *other)
{
	PyBufferProcs *pb;
	void *ptr1, *ptr2;
	Py_ssize_t size;
	Py_ssize_t slice_len;
	Py_ssize_t count;

	if ( self->b_readonly ) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
		return -1;
	}

	pb = other ? other->ob_type->tp_as_buffer : NULL;
	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_BadArgument();
		return -1;
	}
	if ( (*pb->bf_getsegcount)(other, NULL) != 1 )
	{
		/* ### use a different exception type/message? */
		PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
		return -1;
	}
	if (!get_buf(self, &ptr1, &size, ANY_BUFFER))
		return -1;
	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &ptr2)) < 0 )
		return -1;

	if ( left < 0 )
		left = 0;
	else if ( left > size )
		left = size;
	if ( right < left )
		right = left;
	else if ( right > size )
		right = size;
	slice_len = right - left;

	if ( count != slice_len ) {
		PyErr_SetString(
			PyExc_TypeError,
			"right operand length must match slice length");
		return -1;
	}

	if ( slice_len )
	    memcpy((char *)ptr1 + left, ptr2, slice_len);

	return 0;
}

static int
buffer_ass_subscript(PyBufferObject *self, PyObject *item, PyObject *value)
{
	PyBufferProcs *pb;
	void *ptr1, *ptr2;
	Py_ssize_t selfsize;
	Py_ssize_t othersize;

	if ( self->b_readonly ) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
		return -1;
	}

	pb = value ? value->ob_type->tp_as_buffer : NULL;
	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_BadArgument();
		return -1;
	}
	if ( (*pb->bf_getsegcount)(value, NULL) != 1 )
	{
		/* ### use a different exception type/message? */
		PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
		return -1;
	}
	if (!get_buf(self, &ptr1, &selfsize, ANY_BUFFER))
		return -1;
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return -1;
		if (i < 0)
			i += selfsize;
		return buffer_ass_item(self, i, value);
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;
		
		if (PySlice_GetIndicesEx((PySliceObject *)item, selfsize,
				&start, &stop, &step, &slicelength) < 0)
			return -1;

		if ((othersize = (*pb->bf_getreadbuffer)(value, 0, &ptr2)) < 0)
			return -1;

		if (othersize != slicelength) {
			PyErr_SetString(
				PyExc_TypeError,
				"right operand length must match slice length");
			return -1;
		}

		if (slicelength == 0)
			return 0;
		else if (step == 1) {
			memcpy((char *)ptr1 + start, ptr2, slicelength);
			return 0;
		}
		else {
			Py_ssize_t cur, i;
			
			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				((char *)ptr1)[cur] = ((char *)ptr2)[i];
			}

			return 0;
		}
	} else {
		PyErr_SetString(PyExc_TypeError,
				"buffer indices must be integers");
		return -1;
	}
}

/* Buffer methods */

static Py_ssize_t
buffer_getreadbuf(PyBufferObject *self, Py_ssize_t idx, void **pp)
{
	Py_ssize_t size;
	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	if (!get_buf(self, pp, &size, READ_BUFFER))
		return -1;
	return size;
}

static Py_ssize_t
buffer_getwritebuf(PyBufferObject *self, Py_ssize_t idx, void **pp)
{
	Py_ssize_t size;

	if ( self->b_readonly )
	{
		PyErr_SetString(PyExc_TypeError, "buffer is read-only");
		return -1;
	}

	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	if (!get_buf(self, pp, &size, WRITE_BUFFER))
		return -1;
	return size;
}

static Py_ssize_t
buffer_getsegcount(PyBufferObject *self, Py_ssize_t *lenp)
{
	void *ptr;
	Py_ssize_t size;
	if (!get_buf(self, &ptr, &size, ANY_BUFFER))
		return -1;
	if (lenp)
		*lenp = size;
	return 1;
}

static Py_ssize_t
buffer_getcharbuf(PyBufferObject *self, Py_ssize_t idx, const char **pp)
{
	void *ptr;
	Py_ssize_t size;
	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	if (!get_buf(self, &ptr, &size, CHAR_BUFFER))
		return -1;
	*pp = (const char *)ptr;
	return size;
}

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
	(readbufferproc)buffer_getreadbuf,
	(writebufferproc)buffer_getwritebuf,
	(segcountproc)buffer_getsegcount,
	(charbufferproc)buffer_getcharbuf,
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
	(cmpfunc)buffer_compare,		/* tp_compare */
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GETCHARBUFFER, /* tp_flags */
	buffer_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
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

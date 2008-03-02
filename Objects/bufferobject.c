
/* Buffer object implementation */

#include "Python.h"


typedef struct {
	PyObject_HEAD
	PyObject *b_base;
	void *b_ptr;
	int b_size;
	int b_readonly;
	long b_hash;
} PyBufferObject;


static PyObject *
_PyBuffer_FromMemory(PyObject *base, void *ptr, int size, int readonly)
{
	PyBufferObject * b;

	if ( size < 0 ) {
		PyErr_SetString(PyExc_ValueError,
				"size must be zero or positive");
		return NULL;
	}

	b = PyObject_NEW(PyBufferObject, &PyBuffer_Type);
	if ( b == NULL )
		return NULL;

	Py_XINCREF(base);
	b->b_base = base;
	b->b_ptr = ptr;
	b->b_size = size;
	b->b_readonly = readonly;
	b->b_hash = -1;

	return (PyObject *) b;
}

static PyObject *
_PyBuffer_FromObject(PyObject *base, int offset, int size,
                     getreadbufferproc proc, int readonly)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;
	void *p;
	int count;

	if ( offset < 0 ) {
		PyErr_SetString(PyExc_ValueError,
				"offset must be zero or positive");
		return NULL;
	}

	if ( (*pb->bf_getsegcount)(base, NULL) != 1 )
	{
		PyErr_SetString(PyExc_TypeError,
				"single-segment buffer object expected");
		return NULL;
	}
	if ( (count = (*proc)(base, 0, &p)) < 0 )
		return NULL;

	/* apply constraints to the start/end */
	if ( size == Py_END_OF_BUFFER || size < 0 )
		size = count;
	if ( offset > count )
		offset = count;
	if ( offset + size > count )
		size = count - offset;
	
	/* if the base object is another buffer, then "deref" it,
	 * except if the base of the other buffer is NULL
	 */
	if ( PyBuffer_Check(base) && (((PyBufferObject *)base)->b_base) )
		base = ((PyBufferObject *)base)->b_base;
	
	return _PyBuffer_FromMemory(base, (char *)p + offset, size, readonly);
}


PyObject *
PyBuffer_FromObject(PyObject *base, int offset, int size)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return _PyBuffer_FromObject(base, offset, size,
				    pb->bf_getreadbuffer, 1);
}

PyObject *
PyBuffer_FromReadWriteObject(PyObject *base, int offset, int size)
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if ( pb == NULL ||
	     pb->bf_getwritebuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return _PyBuffer_FromObject(base, offset, size,
				    (getreadbufferproc)pb->bf_getwritebuffer,
				    0);
}

PyObject *
PyBuffer_FromMemory(void *ptr, int size)
{
	return _PyBuffer_FromMemory(NULL, ptr, size, 1);
}

PyObject *
PyBuffer_FromReadWriteMemory(void *ptr, int size)
{
	return _PyBuffer_FromMemory(NULL, ptr, size, 0);
}

PyObject *
PyBuffer_New(int size)
{
	PyObject *o;
	PyBufferObject * b;

	if (size < 0) {
		PyErr_SetString(PyExc_ValueError,
				"size must be zero or positive");
		return NULL;
	}
	if (sizeof(*b) > INT_MAX - size) {
		/* unlikely */
		return PyErr_NoMemory();
	}
	/* Inline PyObject_New */
	o = PyObject_MALLOC(sizeof(*b) + size);
	if ( o == NULL )
		return PyErr_NoMemory();
	b = (PyBufferObject *) PyObject_INIT(o, &PyBuffer_Type);

	b->b_base = NULL;
	b->b_ptr = (void *)(b + 1);
	b->b_size = size;
	b->b_readonly = 0;
	b->b_hash = -1;

	return o;
}

/* Methods */

static PyObject *
buffer_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *ob;
	int offset = 0;
	int size = Py_END_OF_BUFFER;

	if ( !PyArg_ParseTuple(args, "O|ii:buffer", &ob, &offset, &size) )
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
	int len_self = self->b_size;
	int len_other = other->b_size;
	int min_len = (len_self < len_other) ? len_self : len_other;
	int cmp;
	if (min_len > 0) {
		cmp = memcmp(self->b_ptr, other->b_ptr, min_len);
		if (cmp != 0)
			return cmp;
	}
	return (len_self < len_other) ? -1 : (len_self > len_other) ? 1 : 0;
}

static PyObject *
buffer_repr(PyBufferObject *self)
{
	char *status = self->b_readonly ? "read-only" : "read-write";

	if ( self->b_base == NULL )
		return PyString_FromFormat("<%s buffer ptr %p, size %d at %p>",
					   status,
					   self->b_ptr,
					   self->b_size,
					   self);
	else
		return PyString_FromFormat(
			"<%s buffer for %p, ptr %p, size %d at %p>",
			status,
			self->b_base,
			self->b_ptr,
			self->b_size,
			self);
}

static long
buffer_hash(PyBufferObject *self)
{
	register int len;
	register unsigned char *p;
	register long x;

	if ( self->b_hash != -1 )
		return self->b_hash;

	if ( !self->b_readonly )
	{
		/* ### use different wording, since this is conditional? */
		PyErr_SetString(PyExc_TypeError, "unhashable type");
		return -1;
	}

	len = self->b_size;
	p = (unsigned char *) self->b_ptr;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= self->b_size;
	if (x == -1)
		x = -2;
	self->b_hash = x;
	return x;
}

static PyObject *
buffer_str(PyBufferObject *self)
{
	return PyString_FromStringAndSize(self->b_ptr, self->b_size);
}

/* Sequence methods */

static int
buffer_length(PyBufferObject *self)
{
	return self->b_size;
}

static PyObject *
buffer_concat(PyBufferObject *self, PyObject *other)
{
	PyBufferProcs *pb = other->ob_type->tp_as_buffer;
	char *p1;
	void *p2;
	PyObject *ob;
	int count;

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

	/* optimize special case */
	if ( self->b_size == 0 )
	{
	    Py_INCREF(other);
	    return other;
	}

	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &p2)) < 0 )
		return NULL;

	assert(count <= PY_SIZE_MAX - self->b_size);

	ob = PyString_FromStringAndSize(NULL, self->b_size + count);
	p1 = PyString_AS_STRING(ob);
	memcpy(p1, self->b_ptr, self->b_size);
	memcpy(p1 + self->b_size, p2, count);

	/* there is an extra byte in the string object, so this is safe */
	p1[self->b_size + count] = '\0';

	return ob;
}

static PyObject *
buffer_repeat(PyBufferObject *self, int count)
{
	PyObject *ob;
	register char *p;
	void *ptr = self->b_ptr;
	int size = self->b_size;

	if ( count < 0 )
		count = 0;
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
buffer_item(PyBufferObject *self, int idx)
{
	if ( idx < 0 || idx >= self->b_size )
	{
		PyErr_SetString(PyExc_IndexError, "buffer index out of range");
		return NULL;
	}
	return PyString_FromStringAndSize((char *)self->b_ptr + idx, 1);
}

static PyObject *
buffer_slice(PyBufferObject *self, int left, int right)
{
	if ( left < 0 )
		left = 0;
	if ( right < 0 )
		right = 0;
	if ( right > self->b_size )
		right = self->b_size;
	if ( right < left )
		right = left;
	return PyString_FromStringAndSize((char *)self->b_ptr + left,
					  right - left);
}

static int
buffer_ass_item(PyBufferObject *self, int idx, PyObject *other)
{
	PyBufferProcs *pb;
	void *p;
	int count;

	if ( self->b_readonly ) {
		PyErr_SetString(PyExc_TypeError,
				"buffer is read-only");
		return -1;
	}

	if (idx < 0 || idx >= self->b_size) {
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

	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &p)) < 0 )
		return -1;
	if ( count != 1 ) {
		PyErr_SetString(PyExc_TypeError,
				"right operand must be a single byte");
		return -1;
	}

	((char *)self->b_ptr)[idx] = *(char *)p;
	return 0;
}

static int
buffer_ass_slice(PyBufferObject *self, int left, int right, PyObject *other)
{
	PyBufferProcs *pb;
	void *p;
	int slice_len;
	int count;

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
	if ( (count = (*pb->bf_getreadbuffer)(other, 0, &p)) < 0 )
		return -1;

	if ( left < 0 )
		left = 0;
	else if ( left > self->b_size )
		left = self->b_size;
	if ( right < left )
		right = left;
	else if ( right > self->b_size )
		right = self->b_size;
	slice_len = right - left;

	if ( count != slice_len ) {
		PyErr_SetString(
			PyExc_TypeError,
			"right operand length must match slice length");
		return -1;
	}

	if ( slice_len )
	    memcpy((char *)self->b_ptr + left, p, slice_len);

	return 0;
}

/* Buffer methods */

static int
buffer_getreadbuf(PyBufferObject *self, int idx, void **pp)
{
	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	*pp = self->b_ptr;
	return self->b_size;
}

static int
buffer_getwritebuf(PyBufferObject *self, int idx, void **pp)
{
	if ( self->b_readonly )
	{
		PyErr_SetString(PyExc_TypeError, "buffer is read-only");
		return -1;
	}
	return buffer_getreadbuf(self, idx, pp);
}

static int
buffer_getsegcount(PyBufferObject *self, int *lenp)
{
	if ( lenp )
		*lenp = self->b_size;
	return 1;
}

static int
buffer_getcharbuf(PyBufferObject *self, int idx, const char **pp)
{
	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent buffer segment");
		return -1;
	}
	*pp = (const char *)self->b_ptr;
	return self->b_size;
}


static PySequenceMethods buffer_as_sequence = {
	(inquiry)buffer_length, /*sq_length*/
	(binaryfunc)buffer_concat, /*sq_concat*/
	(intargfunc)buffer_repeat, /*sq_repeat*/
	(intargfunc)buffer_item, /*sq_item*/
	(intintargfunc)buffer_slice, /*sq_slice*/
	(intobjargproc)buffer_ass_item, /*sq_ass_item*/
	(intintobjargproc)buffer_ass_slice, /*sq_ass_slice*/
};

static PyBufferProcs buffer_as_buffer = {
	(getreadbufferproc)buffer_getreadbuf,
	(getwritebufferproc)buffer_getwritebuf,
	(getsegcountproc)buffer_getsegcount,
	(getcharbufferproc)buffer_getcharbuf,
};

PyTypeObject PyBuffer_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
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
	0,					/* tp_as_mapping */
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

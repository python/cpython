/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Buffer object implementation */

#include "Python.h"


typedef struct {
	PyObject_HEAD
	PyObject *b_base;
	void *b_ptr;
	int b_size;
	int b_readonly;
#ifdef CACHE_HASH
	long b_hash;
#endif
} PyBufferObject;


static PyObject *
_PyBuffer_FromMemory(base, ptr, size, readonly)
	PyObject *base;
	void *ptr;
	int size;
	int readonly;
{
	PyBufferObject * b;

	b = PyObject_NEW(PyBufferObject, &PyBuffer_Type);
	if ( b == NULL )
		return NULL;

	Py_XINCREF(base);
	b->b_base = base;
	b->b_ptr = ptr;
	b->b_size = size;
	b->b_readonly = readonly;
#ifdef CACHE_HASH
	b->b_hash = -1;
#endif

	return (PyObject *) b;
}

static PyObject *
_PyBuffer_FromObject(base, offset, size, proc, readonly)
	PyObject *base;
	int offset;
	int size;
	getreadbufferproc proc;
	int readonly;
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;
	void *p;
	int count;

	if ( (*pb->bf_getsegcount)(base, NULL) != 1 )
	{
		PyErr_SetString(PyExc_TypeError, "single-segment buffer object expected");
		return NULL;
	}
	if ( (count = (*proc)(base, 0, &p)) < 0 )
		return NULL;

	/* apply constraints to the start/end */
	if ( size == Py_END_OF_BUFFER )
		size = count;
	if ( offset > count )
		offset = count;
	if ( offset + size > count )
		size = count - offset;

	/* if the base object is another buffer, then "deref" it */
	if ( PyBuffer_Check(base) )
		base = ((PyBufferObject *)base)->b_base;

	return _PyBuffer_FromMemory(base, (char *)p + offset, size, readonly);
}


PyObject *
PyBuffer_FromObject(base, offset, size)
	PyObject *base;
	int offset;
	int size;
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if ( pb == NULL ||
	     pb->bf_getreadbuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return _PyBuffer_FromObject(base, offset, size, pb->bf_getreadbuffer, 1);
}

PyObject *
PyBuffer_FromReadWriteObject(base, offset, size)
	PyObject *base;
	int offset;
	int size;
{
	PyBufferProcs *pb = base->ob_type->tp_as_buffer;

	if ( pb == NULL ||
	     pb->bf_getwritebuffer == NULL ||
	     pb->bf_getsegcount == NULL )
	{
		PyErr_SetString(PyExc_TypeError, "buffer object expected");
		return NULL;
	}

	return _PyBuffer_FromObject(base, offset, size, (getreadbufferproc)pb->bf_getwritebuffer, 0);
}

PyObject *
PyBuffer_FromMemory(ptr, size)
	void *ptr;
	int size;
{
	return _PyBuffer_FromMemory(NULL, ptr, size, 1);
}

PyObject *
PyBuffer_FromReadWriteMemory(ptr, size)
	void *ptr;
	int size;
{
	return _PyBuffer_FromMemory(NULL, ptr, size, 0);
}

PyObject *
PyBuffer_New(size)
	int size;
{
	PyBufferObject * b;

	b = (PyBufferObject *)malloc(sizeof(*b) + size);
	if ( b == NULL )
		return NULL;
	b->ob_type = &PyBuffer_Type;
	_Py_NewReference((PyObject *)b);

	b->b_base = NULL;
	b->b_ptr = (void *)(b + 1);
	b->b_size = size;
	b->b_readonly = 0;
#ifdef CACHE_HASH
	b->b_hash = -1;
#endif

	return (PyObject *) b;
}

/* Methods */

static void
buffer_dealloc(self)
	PyBufferObject *self;
{
	Py_XDECREF(self->b_base);
	free((void *)self);
}

static int
buffer_compare(self, other)
	PyBufferObject *self;
	PyBufferObject *other;
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
buffer_repr(self)
	PyBufferObject *self;
{
	char buf[300];
	char *status = self->b_readonly ? "read-only" : "read-write";

	if ( self->b_base == NULL )
	{
		sprintf(buf, "<%s buffer ptr %lx, size %d at %lx>",
			status,
			(long)self->b_ptr,
			self->b_size,
			(long)self);
	}
	else
	{
		sprintf(buf, "<%s buffer for %lx, ptr %lx, size %d at %lx>",
			status,
			(long)self->b_base,
			(long)self->b_ptr,
			self->b_size,
			(long)self);
	}

	return PyString_FromString(buf);
}

static long
buffer_hash(self)
	PyBufferObject *self;
{
	register int len;
	register unsigned char *p;
	register long x;

#ifdef CACHE_HASH
	if ( self->b_hash != -1 )
		return self->b_hash;
#endif

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
#ifdef CACHE_HASH
	self->b_hash = x;
#endif
	return x;
}

static PyObject *
buffer_str(self)
	PyBufferObject *self;
{
	return PyString_FromStringAndSize(self->b_ptr, self->b_size);
}

/* Sequence methods */

static int
buffer_length(self)
	PyBufferObject *self;
{
	return self->b_size;
}

static PyObject *
buffer_concat(self, other)
	PyBufferObject *self;
	PyObject *other;
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
		PyErr_SetString(PyExc_TypeError, "single-segment buffer object expected");
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

	/* optimize special case */
	if ( count == 0 )
	{
	    Py_INCREF(self);
	    return (PyObject *)self;
	}

	ob = PyString_FromStringAndSize(NULL, self->b_size + count);
	p1 = PyString_AS_STRING(ob);
	memcpy(p1, self->b_ptr, self->b_size);
	memcpy(p1 + self->b_size, p2, count);

	/* there is an extra byte in the string object, so this is safe */
	p1[self->b_size + count] = '\0';

	return ob;
}

static PyObject *
buffer_repeat(self, count)
	PyBufferObject *self;
	int count;
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
buffer_item(self, idx)
	PyBufferObject *self;
	int idx;
{
	if ( idx < 0 || idx >= self->b_size )
	{
		PyErr_SetString(PyExc_IndexError, "buffer index out of range");
		return NULL;
	}
	return PyString_FromStringAndSize((char *)self->b_ptr + idx, 1);
}

static PyObject *
buffer_slice(self, left, right)
	PyBufferObject *self;
	int left;
	int right;
{
	if ( left < 0 )
		left = 0;
	if ( right < 0 )
		right = 0;
	if ( right > self->b_size )
		right = self->b_size;
	if ( left == 0 && right == self->b_size )
	{
		/* same as self */
		Py_INCREF(self);
		return (PyObject *)self;
	}
	if ( right < left )
		right = left;
	return PyString_FromStringAndSize((char *)self->b_ptr + left, right - left);
}

static int
buffer_ass_item(self, idx, other)
	PyBufferObject *self;
	int idx;
	PyObject *other;
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
		PyErr_SetString(PyExc_TypeError, "single-segment buffer object expected");
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
buffer_ass_slice(self, left, right, other)
	PyBufferObject *self;
	int left;
	int right;
	PyObject *other;
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
		PyErr_SetString(PyExc_TypeError, "single-segment buffer object expected");
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
		PyErr_SetString(PyExc_TypeError,
				"right operand length must match slice length");
		return -1;
	}

	if ( slice_len )
	    memcpy((char *)self->b_ptr + left, p, slice_len);

	return 0;
}

/* Buffer methods */

static int
buffer_getreadbuf(self, idx, pp)
	PyBufferObject *self;
	int idx;
	void ** pp;
{
	if ( idx != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent buffer segment");
		return -1;
	}
	*pp = self->b_ptr;
	return self->b_size;
}

static int
buffer_getwritebuf(self, idx, pp)
	PyBufferObject *self;
	int idx;
	void ** pp;
{
	if ( self->b_readonly )
	{
		PyErr_SetString(PyExc_TypeError, "buffer is read-only");
		return -1;
	}
	return buffer_getreadbuf(self, idx, pp);
}

static int
buffer_getsegcount(self, lenp)
	PyBufferObject *self;
	int *lenp;
{
	if ( lenp )
		*lenp = self->b_size;
	return 1;
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
};

PyTypeObject PyBuffer_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"buffer",
	sizeof(PyBufferObject),
	0,
	(destructor)buffer_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)buffer_compare, /*tp_compare*/
	(reprfunc)buffer_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&buffer_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)buffer_hash,	/*tp_hash*/
	0,		/*tp_call*/
	(reprfunc)buffer_str,		/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	&buffer_as_buffer,	/*tp_as_buffer*/
	0,		/*tp_xxx4*/
	0,		/*tp_doc*/
};


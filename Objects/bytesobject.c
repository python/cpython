/* Bytes object implementation */

/* XXX TO DO: optimizations */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

/* Direct API functions */

PyObject *
PyBytes_FromStringAndSize(const char *sval, Py_ssize_t size)
{
    PyBytesObject *new;

    if (size != 0) {
	assert(sval != NULL);
	assert(size > 0);
    }

    new = PyObject_New(PyBytesObject, &PyBytes_Type);
    if (new == NULL)
	return NULL;

    if (size > 0) {
	new->ob_sval = PyMem_Malloc(size);
	if (new->ob_sval == NULL) {
	    Py_DECREF(new);
	    return NULL;
	}
	memcpy(new->ob_sval, sval, size);
	new->ob_size = size;
    }
    
    return (PyObject *)new;
}

Py_ssize_t
PyBytes_Size(PyObject *self)
{
    assert(self != NULL);
    assert(PyBytes_Check(self));

    return ((PyBytesObject *)self)->ob_size;
}

char  *
PyBytes_AsString(PyObject *self)
{
    assert(self != NULL);
    assert(PyBytes_Check(self));

    return ((PyBytesObject *)self)->ob_sval;
}

int
PyBytes_Resize(PyObject *self, Py_ssize_t size)
{
    void *sval;

    assert(self != NULL);
    assert(PyBytes_Check(self));
    assert(size >= 0);

    sval = PyMem_Realloc(((PyBytesObject *)self)->ob_sval, size);
    if (sval == NULL) {
	PyErr_NoMemory();
	return -1;
    }

    ((PyBytesObject *)self)->ob_sval = sval;
    ((PyBytesObject *)self)->ob_size = size;

    return 0;
}

/* Functions stuffed into the type object */

static Py_ssize_t
bytes_length(PyBytesObject *self)
{
    return self->ob_size;
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
    return PyInt_FromLong((unsigned char)(self->ob_sval[i]));
}

static long
bytes_nohash(PyObject *self)
{
    PyErr_SetString(PyExc_TypeError, "bytes objects are unhashable");
    return -1;
}

static int
bytes_init(PyBytesObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sequence", 0};
    PyObject *arg = NULL;
    PyObject *it; /* iter(arg) */
    PyObject *(*iternext)(PyObject *);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:bytes", kwlist, &arg))
	return -1;

    /* Verify list invariants established by PyType_GenericAlloc() */
    if (self->ob_size != 0) {
	assert(self->ob_sval != NULL);
	assert(self->ob_size > 0);
    }

    /* Empty previous contents */
    if (PyBytes_Resize((PyObject *)self, 0) < 0)
	return -1;

    /* Quick check if we're done */
    if (arg == 0)
	return 0;

    /* XXX Optimize this if the arguments is a list, tuple, or bytes */

    /* Get the iterator */
    it = PyObject_GetIter(arg);
    if (it == NULL)
	return 0;
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
	value = PyNumber_Index(item);
	if (value == -1 && PyErr_Occurred())
	    goto error;

	/* Range check */
	if (value < 0 || value >= 256) {
	    PyErr_SetString(PyExc_ValueError, "bytes must be in range(0, 256)");
	    goto error;
	}

	/* Append the byte */
	/* XXX Speed this up */
	if (PyBytes_Resize((PyObject *)self, self->ob_size+1) < 0)
	    goto error;
	self->ob_sval[self->ob_size-1] = value;
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
	sprintf(buffer, ", 0x%02x", (unsigned char) (self->ob_sval[i]));
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

	cmp = memcmp(self->ob_sval, other->ob_sval, minsize);
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
    if (self->ob_sval != 0) {
	PyMem_Free(self->ob_sval);
    }
    self->ob_type->tp_free((PyObject *)self);
}

static PySequenceMethods bytes_as_sequence = {
    (lenfunc)bytes_length,		/*sq_length*/
    (binaryfunc)0,			/*sq_concat*/
    (ssizeargfunc)0,			/*sq_repeat*/
    (ssizeargfunc)bytes_getitem,	/*sq_item*/
    (ssizessizeargfunc)0,		/*sq_slice*/
    0,					/*sq_ass_item*/
    0,					/*sq_ass_slice*/
    (objobjproc)0,			/*sq_contains*/
};

static PyMappingMethods bytes_as_mapping = {
	(lenfunc)bytes_length,
	(binaryfunc)0,
	0,
};

static PyBufferProcs bytes_as_buffer = {
/*
	(readbufferproc)bytes_buffer_getreadbuf,
	(writebufferproc)bytes_buffer_getwritebuf,
	(segcountproc)bytes_buffer_getsegcount,
	(charbufferproc)bytes_buffer_getcharbuf,
*/
};

static PyMethodDef
bytes_methods[] = {
	{NULL,     NULL}
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
    (destructor)bytes_dealloc,		/* tp_dealloc */
    0,			 		/* tp_print */
    0,					/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    (reprfunc)bytes_repr,		/* tp_repr */
    0,					/* tp_as_number */
    &bytes_as_sequence,			/* tp_as_sequence */
    &bytes_as_mapping,			/* tp_as_mapping */
    bytes_nohash, 			/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    PyObject_GenericGetAttr,		/* tp_getattro */
    0,					/* tp_setattro */
    &bytes_as_buffer,			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES, /* tp_flags */ /* bytes is 'final' or 'sealed' */
    bytes_doc,				/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    (richcmpfunc)bytes_richcompare,	/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    bytes_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    0,					/* tp_base */
    0,					/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)bytes_init,		/* tp_init */
    PyType_GenericAlloc,		/* tp_alloc */
    PyType_GenericNew,			/* tp_new */
    PyObject_Del,	                /* tp_free */
};

/* enumerate object */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long      en_index;        /* current index of enumeration */
	PyObject* en_sit;          /* secondary iterator of enumeration */
	PyObject* en_result;	   /* result tuple  */
} enumobject;

PyTypeObject PyEnum_Type;

static PyObject *
enum_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	enumobject *en;
	PyObject *seq = NULL;
	static char *kwlist[] = {"sequence", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:enumerate", kwlist,
					 &seq))
		return NULL;

	en = (enumobject *)type->tp_alloc(type, 0);
	if (en == NULL)
		return NULL;
	en->en_index = 0;
	en->en_sit = PyObject_GetIter(seq);
	if (en->en_sit == NULL) {
		Py_DECREF(en);
		return NULL;
	}
	en->en_result = PyTuple_Pack(2, Py_None, Py_None);
	if (en->en_result == NULL) {
		Py_DECREF(en->en_sit);
		Py_DECREF(en);
		return NULL;
	}
	return (PyObject *)en;
}

static void
enum_dealloc(enumobject *en)
{
	PyObject_GC_UnTrack(en);
	Py_XDECREF(en->en_sit);
	Py_XDECREF(en->en_result);
	en->ob_type->tp_free(en);
}

static int
enum_traverse(enumobject *en, visitproc visit, void *arg)
{
	int err;

	if (en->en_sit) {
		err = visit(en->en_sit, arg);
		if (err)
			return err;
	}
	if (en->en_result) {
		err = visit(en->en_result, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
enum_next(enumobject *en)
{
	PyObject *next_index;
	PyObject *next_item;
	PyObject *result = en->en_result;
	PyObject *it = en->en_sit;

	next_item = (*it->ob_type->tp_iternext)(it);
	if (next_item == NULL)
		return NULL;

	next_index = PyInt_FromLong(en->en_index);
	if (next_index == NULL) {
		Py_DECREF(next_item);
		return NULL;
	}
	en->en_index++;

	if (result->ob_refcnt == 1) {
		Py_INCREF(result);
		Py_DECREF(PyTuple_GET_ITEM(result, 0));
		Py_DECREF(PyTuple_GET_ITEM(result, 1));
	} else {
		result = PyTuple_New(2);
		if (result == NULL) {
			Py_DECREF(next_index);
			Py_DECREF(next_item);
			return NULL;
		}
	}
	PyTuple_SET_ITEM(result, 0, next_index);
	PyTuple_SET_ITEM(result, 1, next_item);
	return result;
}

PyDoc_STRVAR(enum_doc,
"enumerate(iterable) -> iterator for index, value of iterable\n"
"\n"
"Return an enumerate object.  iterable must be an other object that supports\n"
"iteration.  The enumerate object yields pairs containing a count (from\n"
"zero) and a value yielded by the iterable argument.  enumerate is useful\n"
"for obtaining an indexed list: (0, seq[0]), (1, seq[1]), (2, seq[2]), ...");

PyTypeObject PyEnum_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /* ob_size */
	"enumerate",                    /* tp_name */
	sizeof(enumobject),             /* tp_basicsize */
	0,                              /* tp_itemsize */
	/* methods */
	(destructor)enum_dealloc,       /* tp_dealloc */
	0,                              /* tp_print */
	0,                              /* tp_getattr */
	0,                              /* tp_setattr */
	0,                              /* tp_compare */
	0,                              /* tp_repr */
	0,                              /* tp_as_number */
	0,                              /* tp_as_sequence */
	0,                              /* tp_as_mapping */
	0,                              /* tp_hash */
	0,                              /* tp_call */
	0,                              /* tp_str */
	PyObject_GenericGetAttr,        /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,    /* tp_flags */
	enum_doc,                       /* tp_doc */
	(traverseproc)enum_traverse,    /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	PyObject_SelfIter,		/* tp_iter */
	(iternextfunc)enum_next,        /* tp_iternext */
	0,                              /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	0,                              /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	0,                              /* tp_init */
	PyType_GenericAlloc,            /* tp_alloc */
	enum_new,                       /* tp_new */
	PyObject_GC_Del,                /* tp_free */
};

/* Reversed Object ***************************************************************/

typedef struct {
	PyObject_HEAD
	long      index;
	PyObject* seq;
} reversedobject;

static PyObject *
reversed_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	long n;
	PyObject *seq;
	reversedobject *ro;

	if (!PyArg_UnpackTuple(args, "reversed", 1, 1, &seq))
		return NULL;

	if (PyObject_HasAttrString(seq, "__reversed__"))
		return PyObject_CallMethod(seq, "__reversed__", NULL);

	if (!PySequence_Check(seq)) {
		PyErr_SetString(PyExc_TypeError,
				"argument to reversed() must be a sequence");
		return NULL;
	}

	n = PySequence_Size(seq);
	if (n == -1)
		return NULL;

	ro = (reversedobject *)type->tp_alloc(type, 0);
	if (ro == NULL)
		return NULL;

	ro->index = n-1;
	Py_INCREF(seq);
	ro->seq = seq;
	return (PyObject *)ro;
}

static void
reversed_dealloc(reversedobject *ro)
{
	PyObject_GC_UnTrack(ro);
	Py_XDECREF(ro->seq);
	ro->ob_type->tp_free(ro);
}

static int
reversed_traverse(reversedobject *ro, visitproc visit, void *arg)
{
	if (ro->seq)
		return visit((PyObject *)(ro->seq), arg);
	return 0;
}

static PyObject *
reversed_next(reversedobject *ro)
{
	PyObject *item;
	long index = ro->index;

	if (index >= 0) {
		item = PySequence_GetItem(ro->seq, index);
		if (item != NULL) {
			ro->index--;
			return item;
		}
		if (PyErr_ExceptionMatches(PyExc_IndexError) ||
		    PyErr_ExceptionMatches(PyExc_StopIteration))
			PyErr_Clear();
	}
	ro->index = -1;
	Py_CLEAR(ro->seq);
	return NULL;
}

PyDoc_STRVAR(reversed_doc,
"reversed(sequence) -> reverse iterator over values of the sequence\n"
"\n"
"Return a reverse iterator");

static int
reversed_len(reversedobject *ro)
{
	int position, seqsize;

	if (ro->seq == NULL)
		return 0;
	seqsize = PySequence_Size(ro->seq);
	if (seqsize == -1)
		return -1;
	position = ro->index + 1;
	return (seqsize < position)  ?  0  :  position;
}

static PySequenceMethods reversed_as_sequence = {
	(inquiry)reversed_len,		/* sq_length */
	0,				/* sq_concat */
};

PyTypeObject PyReversed_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /* ob_size */
	"reversed",                     /* tp_name */
	sizeof(reversedobject),         /* tp_basicsize */
	0,                              /* tp_itemsize */
	/* methods */
	(destructor)reversed_dealloc,   /* tp_dealloc */
	0,                              /* tp_print */
	0,                              /* tp_getattr */
	0,                              /* tp_setattr */
	0,                              /* tp_compare */
	0,                              /* tp_repr */
	0,                              /* tp_as_number */
	&reversed_as_sequence,          /* tp_as_sequence */
	0,                              /* tp_as_mapping */
	0,                              /* tp_hash */
	0,                              /* tp_call */
	0,                              /* tp_str */
	PyObject_GenericGetAttr,        /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,    /* tp_flags */
	reversed_doc,                   /* tp_doc */
	(traverseproc)reversed_traverse,/* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	PyObject_SelfIter,		/* tp_iter */
	(iternextfunc)reversed_next,    /* tp_iternext */
	0,				/* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	0,                              /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	0,                              /* tp_init */
	PyType_GenericAlloc,            /* tp_alloc */
	reversed_new,                   /* tp_new */
	PyObject_GC_Del,                /* tp_free */
};

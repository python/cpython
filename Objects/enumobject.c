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
	en->en_result = PyTuple_New(2);
	if (en->en_result == NULL) {
		Py_DECREF(en->en_sit);
		Py_DECREF(en);
		return NULL;
	}
	Py_INCREF(Py_None);
	PyTuple_SET_ITEM(en->en_result, 0, Py_None);
	Py_INCREF(Py_None);
	PyTuple_SET_ITEM(en->en_result, 1, Py_None);
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

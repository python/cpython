/* enumerate object */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long      en_index;        /* current index of enumeration */
	PyObject* en_sit;          /* secondary iterator of enumeration */
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
	return (PyObject *)en;
}

static void
enum_dealloc(enumobject *en)
{
	PyObject_GC_UnTrack(en);
	Py_XDECREF(en->en_sit);
	en->ob_type->tp_free(en);
}

static int
enum_traverse(enumobject *en, visitproc visit, void *arg)
{
	if (en->en_sit)
		return visit(en->en_sit, arg);
	return 0;
}

static PyObject *
enum_next(enumobject *en)
{
	PyObject *result;
	PyObject *next_index;

	PyObject *next_item = PyIter_Next(en->en_sit);
	if (next_item == NULL)
		return NULL;

	result = PyTuple_New(2);
	if (result == NULL) {
		Py_DECREF(next_item);
		return NULL;
	}

	next_index = PyInt_FromLong(en->en_index++);
	if (next_index == NULL) {
		Py_DECREF(next_item);
		Py_DECREF(result);
		return NULL;
	}

	PyTuple_SET_ITEM(result, 0, next_index);
	PyTuple_SET_ITEM(result, 1, next_item);
	return result;
}

static PyObject *
enum_getiter(PyObject *en)
{
	Py_INCREF(en);
	return en;
}

static PyMethodDef enum_methods[] = {
	{"next",    (PyCFunction)enum_next, METH_NOARGS,
	 "return the next (index, value) pair, or raise StopIteration"},
	{NULL,      NULL}       /* sentinel */
};

static char enum_doc[] =
	"enumerate(iterable) -> create an enumerating-iterator";

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
	(getiterfunc)enum_getiter,      /* tp_iter */
	(iternextfunc)enum_next,        /* tp_iternext */
	enum_methods,                   /* tp_methods */
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

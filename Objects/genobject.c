/* Generator object implementation */

#include "Python.h"
#include "frameobject.h"
#include "genobject.h"
#include "ceval.h"
#include "structmember.h"

static int
gen_traverse(PyGenObject *gen, visitproc visit, void *arg)
{
	return visit((PyObject *)gen->gi_frame, arg);
}

static void
gen_dealloc(PyGenObject *gen)
{
	_PyObject_GC_UNTRACK(gen);
	if (gen->gi_weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) gen);
	Py_DECREF(gen->gi_frame);
	PyObject_GC_Del(gen);
}

static PyObject *
gen_iternext(PyGenObject *gen)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyFrameObject *f = gen->gi_frame;
	PyObject *result;

	if (gen->gi_running) {
		PyErr_SetString(PyExc_ValueError,
				"generator already executing");
		return NULL;
	}
	if (f->f_stacktop == NULL)
		return NULL;

	/* Generators always return to their most recent caller, not
	 * necessarily their creator. */
	Py_XINCREF(tstate->frame);
	assert(f->f_back == NULL);
	f->f_back = tstate->frame;

	gen->gi_running = 1;
	result = PyEval_EvalFrame(f);
	gen->gi_running = 0;

	/* Don't keep the reference to f_back any longer than necessary.  It
	 * may keep a chain of frames alive or it could create a reference
	 * cycle. */
	assert(f->f_back != NULL);
	Py_CLEAR(f->f_back);

	/* If the generator just returned (as opposed to yielding), signal
	 * that the generator is exhausted. */
	if (result == Py_None && f->f_stacktop == NULL) {
		Py_DECREF(result);
		result = NULL;
	}

	return result;
}

static PyMemberDef gen_memberlist[] = {
	{"gi_frame",	T_OBJECT, offsetof(PyGenObject, gi_frame),	RO},
	{"gi_running",	T_INT,    offsetof(PyGenObject, gi_running),	RO},
	{NULL}	/* Sentinel */
};

PyTypeObject PyGen_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"generator",				/* tp_name */
	sizeof(PyGenObject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)gen_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0, 					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)gen_traverse,		/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PyGenObject, gi_weakreflist),	/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)gen_iternext,		/* tp_iternext */
	0,					/* tp_methods */
	gen_memberlist,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
};

PyObject *
PyGen_New(PyFrameObject *f)
{
	PyGenObject *gen = PyObject_GC_New(PyGenObject, &PyGen_Type);
	if (gen == NULL) {
		Py_DECREF(f);
		return NULL;
	}
	gen->gi_frame = f;
	gen->gi_running = 0;
	gen->gi_weakreflist = NULL;
	_PyObject_GC_TRACK(gen);
	return (PyObject *)gen;
}

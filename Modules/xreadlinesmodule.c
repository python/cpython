#include "Python.h"

static char xreadlines_doc [] =
"xreadlines(f)\n\
\n\
Return an xreadlines object for the file f.";

typedef struct {
	PyObject_HEAD
	PyObject *file;
	PyObject *lines;
	int lineslen;
	int lineno;
	int abslineno;
} PyXReadlinesObject;

staticforward PyTypeObject XReadlinesObject_Type;

static void
xreadlines_dealloc(PyXReadlinesObject *op)
{
	Py_XDECREF(op->file);
	Py_XDECREF(op->lines);
	PyObject_DEL(op);
}

/* A larger chunk size doesn't seem to make a difference */
#define CHUNKSIZE  8192

static PyXReadlinesObject *
newreadlinesobject(PyObject *file)
{
	PyXReadlinesObject *op;
	op = PyObject_NEW(PyXReadlinesObject, &XReadlinesObject_Type);
	if (op == NULL)
		return NULL;
	Py_XINCREF(file);
	op->file = file;
	op->lines = NULL;
	op->abslineno = op->lineno = op->lineslen = 0;
	return op;
}

static PyObject *
xreadlines(PyObject *self, PyObject *args)
{
	PyObject *file;
	PyXReadlinesObject *ret;

	if (!PyArg_ParseTuple(args, "O:xreadlines", &file))
		return NULL;
	ret = newreadlinesobject(file);
	return (PyObject*)ret;
}

static PyObject *
xreadlines_item(PyXReadlinesObject *a, int i)
{
	if (i != a->abslineno) {
		PyErr_SetString(PyExc_RuntimeError,
			"xreadlines object accessed out of order");
		return NULL;
	}
	if (a->lineno >= a->lineslen) {
		Py_XDECREF(a->lines);
		a->lines = PyObject_CallMethod(a->file, "readlines", "(i)",
					       CHUNKSIZE);
		if (a->lines == NULL)
			return NULL;
		a->lineno = 0;
		if ((a->lineslen = PySequence_Size(a->lines)) < 0)
			return NULL;
	}
	a->abslineno++;
	return PySequence_GetItem(a->lines, a->lineno++);
}

static PySequenceMethods xreadlines_as_sequence = {
	0, /*sq_length*/
	0, /*sq_concat*/
	0, /*sq_repeat*/
	(intargfunc)xreadlines_item, /*sq_item*/
};

static PyTypeObject XReadlinesObject_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"xreadlines",
	sizeof(PyXReadlinesObject) + PyGC_HEAD_SIZE,
	0,
	(destructor)xreadlines_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_compare*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	&xreadlines_as_sequence, /*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	0,		/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/*tp_flags*/
 	0,		/* tp_doc */
};

static PyMethodDef xreadlines_methods[] = {
	{"xreadlines", xreadlines, METH_VARARGS, xreadlines_doc},
	{NULL, NULL}
};

DL_EXPORT(void)
initxreadlines(void)
{
	PyObject *m;

	XReadlinesObject_Type.ob_type = &PyType_Type;
	m = Py_InitModule("xreadlines", xreadlines_methods);
}

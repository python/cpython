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
xreadlines_common(PyXReadlinesObject *a)
{
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

static PyObject *
xreadlines_item(PyXReadlinesObject *a, int i)
{
	if (i != a->abslineno) {
		PyErr_SetString(PyExc_RuntimeError,
			"xreadlines object accessed out of order");
		return NULL;
	}
	return xreadlines_common(a);
}

static PyObject *
xreadlines_getiter(PyXReadlinesObject *a)
{
	Py_INCREF(a);
	return (PyObject *)a;
}

static PyObject *
xreadlines_iternext(PyXReadlinesObject *a)
{
	PyObject *res;

	res = xreadlines_common(a);
	if (res == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
		PyErr_Clear();
	return res;
}

static PyObject *
xreadlines_next(PyXReadlinesObject *a, PyObject *args)
{
	PyObject *res;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	res = xreadlines_common(a);
	if (res == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
		PyErr_SetObject(PyExc_StopIteration, Py_None);
	return res;
}

static char next_doc[] = "x.next() -> the next line or raise StopIteration";

static PyMethodDef xreadlines_methods[] = {
	{"next", (PyCFunction)xreadlines_next, METH_VARARGS, next_doc},
	{NULL, NULL}
};

static PyObject *
xreadlines_getattr(PyObject *a, char *name)
{
	return Py_FindMethod(xreadlines_methods, a, name);
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
	"xreadlines.xreadlines",
	sizeof(PyXReadlinesObject),
	0,
	(destructor)xreadlines_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	xreadlines_getattr,			/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	&xreadlines_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)xreadlines_getiter,	/* tp_iter */
	(iternextfunc)xreadlines_iternext,	/* tp_iternext */
};

static PyMethodDef xreadlines_functions[] = {
	{"xreadlines", xreadlines, METH_VARARGS, xreadlines_doc},
	{NULL, NULL}
};

DL_EXPORT(void)
initxreadlines(void)
{
	XReadlinesObject_Type.ob_type = &PyType_Type;
	Py_InitModule("xreadlines", xreadlines_functions);
}

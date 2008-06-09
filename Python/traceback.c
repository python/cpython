
/* Traceback implementation */

#include "Python.h"

#include "code.h"
#include "frameobject.h"
#include "structmember.h"
#include "osdefs.h"
#include "traceback.h"

#define OFF(x) offsetof(PyTracebackObject, x)

static struct memberlist tb_memberlist[] = {
	{"tb_next",	T_OBJECT,	OFF(tb_next)},
	{"tb_frame",	T_OBJECT,	OFF(tb_frame)},
	{"tb_lasti",	T_INT,		OFF(tb_lasti)},
	{"tb_lineno",	T_INT,		OFF(tb_lineno)},
	{NULL}	/* Sentinel */
};

static PyObject *
tb_getattr(PyTracebackObject *tb, char *name)
{
	return PyMember_Get((char *)tb, tb_memberlist, name);
}

static void
tb_dealloc(PyTracebackObject *tb)
{
	PyObject_GC_UnTrack(tb);
	Py_TRASHCAN_SAFE_BEGIN(tb)
	Py_XDECREF(tb->tb_next);
	Py_XDECREF(tb->tb_frame);
	PyObject_GC_Del(tb);
	Py_TRASHCAN_SAFE_END(tb)
}

static int
tb_traverse(PyTracebackObject *tb, visitproc visit, void *arg)
{
	Py_VISIT(tb->tb_next);
	Py_VISIT(tb->tb_frame);
	return 0;
}

static void
tb_clear(PyTracebackObject *tb)
{
	Py_CLEAR(tb->tb_next);
	Py_CLEAR(tb->tb_frame);
}

PyTypeObject PyTraceBack_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"traceback",
	sizeof(PyTracebackObject),
	0,
	(destructor)tb_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)tb_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/* tp_hash */
	0,		/* tp_call */
	0,		/* tp_str */
	0,		/* tp_getattro */
	0,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
	0,             				/* tp_doc */
 	(traverseproc)tb_traverse,		/* tp_traverse */
	(inquiry)tb_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,			/* tp_members */
	0,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
};

static PyTracebackObject *
newtracebackobject(PyTracebackObject *next, PyFrameObject *frame)
{
	PyTracebackObject *tb;
	if ((next != NULL && !PyTraceBack_Check(next)) ||
			frame == NULL || !PyFrame_Check(frame)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	tb = PyObject_GC_New(PyTracebackObject, &PyTraceBack_Type);
	if (tb != NULL) {
		Py_XINCREF(next);
		tb->tb_next = next;
		Py_XINCREF(frame);
		tb->tb_frame = frame;
		tb->tb_lasti = frame->f_lasti;
		tb->tb_lineno = PyCode_Addr2Line(frame->f_code, 
						 frame->f_lasti);
		PyObject_GC_Track(tb);
	}
	return tb;
}

int
PyTraceBack_Here(PyFrameObject *frame)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyTracebackObject *oldtb = (PyTracebackObject *) tstate->curexc_traceback;
	PyTracebackObject *tb = newtracebackobject(oldtb, frame);
	if (tb == NULL)
		return -1;
	tstate->curexc_traceback = (PyObject *)tb;
	Py_XDECREF(oldtb);
	return 0;
}

int
Py_DisplaySourceLine(PyObject *f, const char *filename, int lineno)
{
	int err = 0;
	FILE *xfp = NULL;
	char linebuf[2000];
	int i;
	char namebuf[MAXPATHLEN+1];

	if (filename == NULL)
		return -1;
	/* This is needed by Emacs' compile command */
#define FMT "  File \"%.500s\", line %d, in %.500s\n"
	xfp = fopen(filename, "r" PY_STDIOTEXTMODE);
	if (xfp == NULL) {
		/* Search tail of filename in sys.path before giving up */
		PyObject *path;
		const char *tail = strrchr(filename, SEP);
		if (tail == NULL)
			tail = filename;
		else
			tail++;
		path = PySys_GetObject("path");
		if (path != NULL && PyList_Check(path)) {
			Py_ssize_t _npath = PyList_Size(path);
			int npath = Py_SAFE_DOWNCAST(_npath, Py_ssize_t, int);
			size_t taillen = strlen(tail);
			for (i = 0; i < npath; i++) {
				PyObject *v = PyList_GetItem(path, i);
				if (v == NULL) {
					PyErr_Clear();
					break;
				}
				if (PyString_Check(v)) {
					size_t len;
					len = PyString_GET_SIZE(v);
					if (len + 1 + taillen >= MAXPATHLEN)
						continue; /* Too long */
					strcpy(namebuf, PyString_AsString(v));
					if (strlen(namebuf) != len)
						continue; /* v contains '\0' */
					if (len > 0 && namebuf[len-1] != SEP)
						namebuf[len++] = SEP;
					strcpy(namebuf+len, tail);
					xfp = fopen(namebuf, "r" PY_STDIOTEXTMODE);
					if (xfp != NULL) {
						filename = namebuf;
						break;
					}
				}
			}
		}
	}

        if (xfp == NULL)
            return err;
        if (err != 0) {
            fclose(xfp);
            return err;
        }

	for (i = 0; i < lineno; i++) {
		char* pLastChar = &linebuf[sizeof(linebuf)-2];
		do {
			*pLastChar = '\0';
			if (Py_UniversalNewlineFgets(linebuf, sizeof linebuf, xfp, NULL) == NULL)
				break;
			/* fgets read *something*; if it didn't get as
			   far as pLastChar, it must have found a newline
			   or hit the end of the file;	if pLastChar is \n,
			   it obviously found a newline; else we haven't
			   yet seen a newline, so must continue */
		} while (*pLastChar != '\0' && *pLastChar != '\n');
	}
	if (i == lineno) {
		char *p = linebuf;
		while (*p == ' ' || *p == '\t' || *p == '\014')
			p++;
                    err = PyFile_WriteString(p, f);
                    if (err == 0 && strchr(p, '\n') == NULL)
                            err = PyFile_WriteString("\n", f);
	}
	fclose(xfp);
	return err;
}

static int
tb_displayline(PyObject *f, const char *filename, int lineno, const char *name)
{
	int err = 0;
        char linebuf[2000];

	if (filename == NULL || name == NULL)
		return -1;
	/* This is needed by Emacs' compile command */
#define FMT "  File \"%.500s\", line %d, in %.500s\n"
	PyOS_snprintf(linebuf, sizeof(linebuf), FMT, filename, lineno, name);
	err = PyFile_WriteString(linebuf, f);
	if (err != 0)
		return err;
        return Py_DisplaySourceLine(f, filename, lineno);
}

static int
tb_printinternal(PyTracebackObject *tb, PyObject *f, long limit)
{
	int err = 0;
	long depth = 0;
	PyTracebackObject *tb1 = tb;
	while (tb1 != NULL) {
		depth++;
		tb1 = tb1->tb_next;
	}
	while (tb != NULL && err == 0) {
		if (depth <= limit) {
			err = tb_displayline(f,
			    PyString_AsString(
				    tb->tb_frame->f_code->co_filename),
			    tb->tb_lineno,
			    PyString_AsString(tb->tb_frame->f_code->co_name));
		}
		depth--;
		tb = tb->tb_next;
		if (err == 0)
			err = PyErr_CheckSignals();
	}
	return err;
}

int
PyTraceBack_Print(PyObject *v, PyObject *f)
{
	int err;
	PyObject *limitv;
	long limit = 1000;
	if (v == NULL)
		return 0;
	if (!PyTraceBack_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	limitv = PySys_GetObject("tracebacklimit");
	if (limitv && PyInt_Check(limitv)) {
		limit = PyInt_AsLong(limitv);
		if (limit <= 0)
			return 0;
	}
	err = PyFile_WriteString("Traceback (most recent call last):\n", f);
	if (!err)
		err = tb_printinternal((PyTracebackObject *)v, f, limit);
	return err;
}

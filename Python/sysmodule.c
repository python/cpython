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

/* System module */

/*
Various bits of information used by the interpreter are collected in
module 'sys'.
Function member:
- exit(sts): raise SystemExit
Data members:
- stdin, stdout, stderr: standard file objects
- modules: the table of modules (dictionary)
- path: module search path (list of strings)
- argv: script arguments (list of strings)
- ps1, ps2: optional primary and secondary prompts (strings)
*/

#include "Python.h"

#include "osdefs.h"

PyObject *_PySys_TraceFunc, *_PySys_ProfileFunc;
int _PySys_CheckInterval = 10;

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

static PyObject *sysdict;

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
#endif

PyObject *
PySys_GetObject(name)
	char *name;
{
	return PyDict_GetItemString(sysdict, name);
}

FILE *
PySys_GetFile(name, def)
	char *name;
	FILE *def;
{
	FILE *fp = NULL;
	PyObject *v = PySys_GetObject(name);
	if (v != NULL && PyFile_Check(v))
		fp = PyFile_AsFile(v);
	if (fp == NULL)
		fp = def;
	return fp;
}

int
PySys_SetObject(name, v)
	char *name;
	PyObject *v;
{
	if (v == NULL) {
		if (PyDict_GetItemString(sysdict, name) == NULL)
			return 0;
		else
			return PyDict_DelItemString(sysdict, name);
	}
	else
		return PyDict_SetItemString(sysdict, name, v);
}

static PyObject *
sys_exit(self, args)
	PyObject *self;
	PyObject *args;
{
	/* Raise SystemExit so callers may catch it or clean up. */
	PyErr_SetObject(PyExc_SystemExit, args);
	return NULL;
}

static PyObject *
sys_settrace(self, args)
	PyObject *self;
	PyObject *args;
{
	if (args == Py_None)
		args = NULL;
	else
		Py_XINCREF(args);
	Py_XDECREF(_PySys_TraceFunc);
	_PySys_TraceFunc = args;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sys_setprofile(self, args)
	PyObject *self;
	PyObject *args;
{
	if (args == Py_None)
		args = NULL;
	else
		Py_XINCREF(args);
	Py_XDECREF(_PySys_ProfileFunc);
	_PySys_ProfileFunc = args;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sys_setcheckinterval(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, "i", &_PySys_CheckInterval))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef USE_MALLOPT
/* Link with -lmalloc (or -lmpc) on an SGI */
#include <malloc.h>

static PyObject *
sys_mdebug(self, args)
	PyObject *self;
	PyObject *args;
{
	int flag;
	if (!PyArg_Parse(args, "i", &flag))
		return NULL;
	mallopt(M_DEBUG, flag);
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* USE_MALLOPT */

static PyObject *
sys_getrefcount(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *arg;
	if (!PyArg_Parse(args, "O", &arg))
		return NULL;
	return PyInt_FromLong((long) arg->ob_refcnt);
}

#ifdef COUNT_ALLOCS
static PyObject *
sys_getcounts(self, args)
	PyObject *self, *args;
{
	extern PyObject *get_counts Py_PROTO((void));

	if (!PyArg_Parse(args, ""))
		return NULL;
	return get_counts();
}
#endif

#ifdef Py_TRACE_REFS
/* Defined in objects.c because it uses static globals if that file */
extern PyObject *_Py_GetObjects Py_PROTO((PyObject *, PyObject *));
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
/* Defined in ceval.c because it uses static globals if that file */
extern PyObject *_Py_GetDXProfile Py_PROTO((PyObject *,  PyObject *));
#endif

static PyMethodDef sys_methods[] = {
	/* Might as well keep this in alphabetic order */
	{"exit",	sys_exit, 0},
#ifdef COUNT_ALLOCS
	{"getcounts",	sys_getcounts, 0},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
	{"getdxp",	_Py_GetDXProfile, 1},
#endif
#ifdef Py_TRACE_REFS
	{"getobjects",	_Py_GetObjects, 1},
#endif
	{"getrefcount",	sys_getrefcount, 0},
#ifdef USE_MALLOPT
	{"mdebug",	sys_mdebug, 0},
#endif
	{"setcheckinterval",	sys_setcheckinterval, 1},
	{"setprofile",	sys_setprofile, 0},
	{"settrace",	sys_settrace, 0},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *sysin, *sysout, *syserr;

static PyObject *
list_builtin_module_names()
{
	PyObject *list = PyList_New(0);
	int i;
	if (list == NULL)
		return NULL;
	for (i = 0; _PyImport_Inittab[i].name != NULL; i++) {
		PyObject *name = PyString_FromString(_PyImport_Inittab[i].name);
		if (name == NULL)
			break;
		PyList_Append(list, name);
		Py_DECREF(name);
	}
	if (PyList_Sort(list) != 0) {
		Py_DECREF(list);
		list = NULL;
	}
	if (list) {
		PyObject *v = PyList_AsTuple(list);
		Py_DECREF(list);
		list = v;
	}
	return list;
}

void
PySys_Init()
{
	extern long PyInt_GetMax Py_PROTO((void));
	extern char *Py_GetVersion Py_PROTO((void));
	extern char *Py_GetCopyright Py_PROTO((void));
	extern char *Py_GetPlatform Py_PROTO((void));
	extern char *Py_GetPrefix Py_PROTO((void));
	extern char *Py_GetExecPrefix Py_PROTO((void));
	extern int fclose Py_PROTO((FILE *));
	PyObject *m = Py_InitModule("sys", sys_methods);
	PyObject *v;
	sysdict = PyModule_GetDict(m);
	Py_INCREF(sysdict);
	/* NB keep an extra ref to the std files to avoid closing them
	   when the user deletes them */
	sysin = PyFile_FromFile(stdin, "<stdin>", "r", fclose);
	sysout = PyFile_FromFile(stdout, "<stdout>", "w", fclose);
	syserr = PyFile_FromFile(stderr, "<stderr>", "w", fclose);
	if (PyErr_Occurred())
		Py_FatalError("can't initialize sys.std{in,out,err}");
	PyDict_SetItemString(sysdict, "stdin", sysin);
	PyDict_SetItemString(sysdict, "stdout", sysout);
	PyDict_SetItemString(sysdict, "stderr", syserr);
	PyDict_SetItemString(sysdict, "version",
			     v = PyString_FromString(Py_GetVersion()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "copyright",
			     v = PyString_FromString(Py_GetCopyright()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "platform",
			     v = PyString_FromString(Py_GetPlatform()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "prefix",
			     v = PyString_FromString(Py_GetPrefix()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "exec_prefix",
		   v = PyString_FromString(Py_GetExecPrefix()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "maxint",
			     v = PyInt_FromLong(PyInt_GetMax()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "modules", PyImport_GetModuleDict());
	PyDict_SetItemString(sysdict, "builtin_module_names",
		   v = list_builtin_module_names());
	Py_XDECREF(v);
#ifdef MS_COREDLL
	PyDict_SetItemString(sysdict, "dllhandle",
			     v = PyInt_FromLong((int)PyWin_DLLhModule));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "winver",
			     v = PyString_FromString(MS_DLL_ID));
	Py_XDECREF(v);
#endif
	if (PyErr_Occurred())
		Py_FatalError("can't insert sys.* objects in sys dict");
}

static PyObject *
makepathobject(path, delim)
	char *path;
	int delim;
{
	int i, n;
	char *p;
	PyObject *v, *w;
	
	n = 1;
	p = path;
	while ((p = strchr(p, delim)) != NULL) {
		n++;
		p++;
	}
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	for (i = 0; ; i++) {
		p = strchr(path, delim);
		if (p == NULL)
			p = strchr(path, '\0'); /* End of string */
		w = PyString_FromStringAndSize(path, (int) (p - path));
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i, w);
		if (*p == '\0')
			break;
		path = p+1;
	}
	return v;
}

void
PySys_SetPath(path)
	char *path;
{
	PyObject *v;
	if ((v = makepathobject(path, DELIM)) == NULL)
		Py_FatalError("can't create sys.path");
	if (PySys_SetObject("path", v) != 0)
		Py_FatalError("can't assign sys.path");
	Py_DECREF(v);
}

static PyObject *
makeargvobject(argc, argv)
	int argc;
	char **argv;
{
	PyObject *av;
	if (argc <= 0 || argv == NULL) {
		/* Ensure at least one (empty) argument is seen */
		static char *empty_argv[1] = {""};
		argv = empty_argv;
		argc = 1;
	}
	av = PyList_New(argc);
	if (av != NULL) {
		int i;
		for (i = 0; i < argc; i++) {
			PyObject *v = PyString_FromString(argv[i]);
			if (v == NULL) {
				Py_DECREF(av);
				av = NULL;
				break;
			}
			PyList_SetItem(av, i, v);
		}
	}
	return av;
}

void
PySys_SetArgv(argc, argv)
	int argc;
	char **argv;
{
	PyObject *av = makeargvobject(argc, argv);
	PyObject *path = PySys_GetObject("path");
	if (av == NULL)
		Py_FatalError("no mem for sys.argv");
	if (PySys_SetObject("argv", av) != 0)
		Py_FatalError("can't assign sys.argv");
	if (path != NULL) {
		char *argv0 = argv[0];
		char *p = NULL;
		int n = 0;
		PyObject *a;
#ifdef HAVE_READLINK
		char link[MAXPATHLEN+1];
		char argv0copy[2*MAXPATHLEN+1];
		int nr = 0;
		if (argc > 0 && argv0 != NULL)
			nr = readlink(argv0, link, MAXPATHLEN);
		if (nr > 0) {
			/* It's a symlink */
			link[nr] = '\0';
			if (link[0] == SEP)
				argv0 = link; /* Link to absolute path */
			else if (strchr(link, SEP) == NULL)
				; /* Link without path */
			else {
				/* Must join(dirname(argv0), link) */
				char *q = strrchr(argv0, SEP);
				if (q == NULL)
					argv0 = link; /* argv0 without path */
				else {
					/* Must make a copy */
					strcpy(argv0copy, argv0);
					q = strrchr(argv0copy, SEP);
					strcpy(q+1, link);
					argv0 = argv0copy;
				}
			}
		}
#endif /* HAVE_READLINK */
#if SEP == '\\' /* Special case for MS filename syntax */
		if (argc > 0 && argv0 != NULL) {
			char *q;
			p = strrchr(argv0, SEP);
			/* Test for alternate separator */
			q = strrchr(p ? p : argv0, '/');
			if (q != NULL)
				p = q;
			if (p != NULL) {
				n = p + 1 - argv0;
				if (n > 1 && p[-1] != ':')
					n--; /* Drop trailing separator */
			}
		}
#else /* All other filename syntaxes */
		if (argc > 0 && argv0 != NULL)
			p = strrchr(argv0, SEP);
		if (p != NULL) {
			n = p + 1 - argv0;
#if SEP == '/' /* Special case for Unix filename syntax */
			if (n > 1)
				n--; /* Drop trailing separator */
#endif /* Unix */
		}
#endif /* All others */
		a = PyString_FromStringAndSize(argv0, n);
		if (a == NULL)
			Py_FatalError("no mem for sys.path insertion");
		if (PyList_Insert(path, 0, a) < 0)
			Py_FatalError("sys.path.insert(0) failed");
		Py_DECREF(a);
	}
	Py_DECREF(av);
}

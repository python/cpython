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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
/* A string loaded from the DLL at startup: */
extern const char *PyWin_DLLVersionString;
#endif

PyObject *
PySys_GetObject(name)
	char *name;
{
	PyThreadState *tstate = PyThreadState_Get();
	PyObject *sd = tstate->interp->sysdict;
	return PyDict_GetItemString(sd, name);
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
	PyThreadState *tstate = PyThreadState_Get();
	PyObject *sd = tstate->interp->sysdict;
	if (v == NULL) {
		if (PyDict_GetItemString(sd, name) == NULL)
			return 0;
		else
			return PyDict_DelItemString(sd, name);
	}
	else
		return PyDict_SetItemString(sd, name, v);
}

static PyObject *
sys_exc_info(self, args)
	PyObject *self;
	PyObject *args;
{
	PyThreadState *tstate;
	if (!PyArg_Parse(args, ""))
		return NULL;
	tstate = PyThreadState_Get();
	return Py_BuildValue(
		"(OOO)",
		tstate->exc_type != NULL ? tstate->exc_type : Py_None,
		tstate->exc_value != NULL ? tstate->exc_value : Py_None,
		tstate->exc_traceback != NULL ?
			tstate->exc_traceback : Py_None);
}

static char exc_info_doc[] =
"exc_info() -> (type, value, traceback)\n\
\n\
Return information about the exception that is currently being handled.\n\
This should be called from inside an except clause only.";

static PyObject *
sys_exit(self, args)
	PyObject *self;
	PyObject *args;
{
	/* Raise SystemExit so callers may catch it or clean up. */
	PyErr_SetObject(PyExc_SystemExit, args);
	return NULL;
}

static char exit_doc[] =
"exit([status])\n\
\n\
Exit the interpreter by raising SystemExit(status).\n\
If the status is omitted or None, it defaults to zero (i.e., success).\n\
If the status numeric, it will be used as the system exit status.\n\
If it is another kind of object, it will be printed and the system\n\
exit status will be one (i.e., failure).";

static PyObject *
sys_settrace(self, args)
	PyObject *self;
	PyObject *args;
{
	PyThreadState *tstate = PyThreadState_Get();
	if (args == Py_None)
		args = NULL;
	else
		Py_XINCREF(args);
	Py_XDECREF(tstate->sys_tracefunc);
	tstate->sys_tracefunc = args;
	Py_INCREF(Py_None);
	return Py_None;
}

static char settrace_doc[] =
"settrace(function)\n\
\n\
Set the global debug tracing function.  It will be called on each\n\
function call.  See the debugger chapter in the library manual.";

static PyObject *
sys_setprofile(self, args)
	PyObject *self;
	PyObject *args;
{
	PyThreadState *tstate = PyThreadState_Get();
	if (args == Py_None)
		args = NULL;
	else
		Py_XINCREF(args);
	Py_XDECREF(tstate->sys_profilefunc);
	tstate->sys_profilefunc = args;
	Py_INCREF(Py_None);
	return Py_None;
}

static char setprofile_doc[] =
"setprofile(function)\n\
\n\
Set the profiling function.  It will be called on each function call\n\
and return.  See the profiler chapter in the library manual.";

static PyObject *
sys_setcheckinterval(self, args)
	PyObject *self;
	PyObject *args;
{
	PyThreadState *tstate = PyThreadState_Get();
	if (!PyArg_ParseTuple(args, "i", &tstate->interp->checkinterval))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char setcheckinterval_doc[] =
"setcheckinterval(n)\n\
\n\
Tell the Python interpreter to check for asynchronous events every\n\
n instructions.  This also affects how often thread switches occur.";

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

static char getrefcount_doc[] =
"getrefcount(object) -> integer\n\
\n\
Return the current reference count for the object.  This includes the\n\
temporary reference in the argument list, so it is at least 2.";

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
	{"exc_info",	sys_exc_info, 0, exc_info_doc},
	{"exit",	sys_exit, 0, exit_doc},
#ifdef COUNT_ALLOCS
	{"getcounts",	sys_getcounts, 0},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
	{"getdxp",	_Py_GetDXProfile, 1},
#endif
#ifdef Py_TRACE_REFS
	{"getobjects",	_Py_GetObjects, 1},
#endif
	{"getrefcount",	sys_getrefcount, 0, getrefcount_doc},
#ifdef USE_MALLOPT
	{"mdebug",	sys_mdebug, 0},
#endif
	{"setcheckinterval",	sys_setcheckinterval, 1, setcheckinterval_doc},
	{"setprofile",	sys_setprofile, 0, setprofile_doc},
	{"settrace",	sys_settrace, 0, settrace_doc},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
list_builtin_module_names()
{
	PyObject *list = PyList_New(0);
	int i;
	if (list == NULL)
		return NULL;
	for (i = 0; PyImport_Inittab[i].name != NULL; i++) {
		PyObject *name = PyString_FromString(
			PyImport_Inittab[i].name);
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

/* XXX This doc string is too long to be a single string literal in VC++ 5.0.
   Two literals concatenated works just fine.  If you have a K&R compiler
   or other abomination that however *does* understand longer strings,
   get rid of the !!! comment in the middle and the quotes that surround it. */
static char sys_doc[] =
"This module provides access to some objects used or maintained by the\n\
interpreter and to functions that interact strongly with the interpreter.\n\
\n\
Dynamic objects:\n\
\n\
argv -- command line arguments; argv[0] is the script pathname if known\n\
path -- module search path; path[0] is the script directory, else ''\n\
modules -- dictionary of loaded modules\n\
exitfunc -- you may set this to a function to be called when Python exits\n\
\n\
stdin -- standard input file object; used by raw_input() and input()\n\
stdout -- standard output file object; used by the print statement\n\
stderr -- standard error object; used for error messages\n\
  By assigning another file object (or an object that behaves like a file)\n\
  to one of these, it is possible to redirect all of the interpreter's I/O.\n\
\n\
last_type -- type of last uncaught exception\n\
last_value -- value of last uncaught exception\n\
last_traceback -- traceback of last uncaught exception\n\
  These three are only available in an interactive session after a\n\
  traceback has been printed.\n\
\n\
exc_type -- type of exception currently being handled\n\
exc_value -- value of exception currently being handled\n\
exc_traceback -- traceback of exception currently being handled\n\
  The function exc_info() should be used instead of these three,\n\
  because it is thread-safe.\n\
" /* !!! */ "\n\
Static objects:\n\
\n\
maxint -- the largest supported integer (the smallest is -maxint-1)\n\
builtin_module_names -- tuple of module names built into this intepreter\n\
version -- the version of this interpreter\n\
copyright -- copyright notice pertaining to this interpreter\n\
platform -- platform identifier\n\
executable -- pathname of this Python interpreter\n\
prefix -- prefix used to find the Python library\n\
exec_prefix -- prefix used to find the machine-specific Python library\n\
dllhandle -- [Windows only] integer handle of the Python DLL\n\
winver -- [Windows only] version number of the Python DLL\n\
__stdin__ -- the original stdin; don't use!\n\
__stdout__ -- the original stdout; don't use!\n\
__stderr__ -- the original stderr; don't use!\n\
\n\
Functions:\n\
\n\
exc_info() -- return thread-safe information about the current exception\n\
exit() -- exit the interpreter by raising SystemExit\n\
getrefcount() -- return the reference count for an object (plus one :-)\n\
setcheckinterval() -- control how often the interpreter checks for events\n\
setprofile() -- set the global profiling function\n\
settrace() -- set the global debug tracing function\n\
";

PyObject *
_PySys_Init()
{
	extern int fclose Py_PROTO((FILE *));
	PyObject *m, *v, *sysdict;
	PyObject *sysin, *sysout, *syserr;

	m = Py_InitModule3("sys", sys_methods, sys_doc);
	sysdict = PyModule_GetDict(m);

	sysin = PyFile_FromFile(stdin, "<stdin>", "r", NULL);
	sysout = PyFile_FromFile(stdout, "<stdout>", "w", NULL);
	syserr = PyFile_FromFile(stderr, "<stderr>", "w", NULL);
	if (PyErr_Occurred())
		return NULL;
	PyDict_SetItemString(sysdict, "stdin", sysin);
	PyDict_SetItemString(sysdict, "stdout", sysout);
	PyDict_SetItemString(sysdict, "stderr", syserr);
	/* Make backup copies for cleanup */
	PyDict_SetItemString(sysdict, "__stdin__", sysin);
	PyDict_SetItemString(sysdict, "__stdout__", sysout);
	PyDict_SetItemString(sysdict, "__stderr__", syserr);
	Py_XDECREF(sysin);
	Py_XDECREF(sysout);
	Py_XDECREF(syserr);
	PyDict_SetItemString(sysdict, "version",
			     v = PyString_FromString(Py_GetVersion()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "copyright",
			     v = PyString_FromString(Py_GetCopyright()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "platform",
			     v = PyString_FromString(Py_GetPlatform()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "executable",
			     v = PyString_FromString(Py_GetProgramFullPath()));
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
	PyDict_SetItemString(sysdict, "builtin_module_names",
		   v = list_builtin_module_names());
	Py_XDECREF(v);
#ifdef MS_COREDLL
	PyDict_SetItemString(sysdict, "dllhandle",
			     v = PyInt_FromLong((int)PyWin_DLLhModule));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "winver",
			     v = PyString_FromString(PyWin_DLLVersionString));
	Py_XDECREF(v);
#endif
	if (PyErr_Occurred())
		return NULL;
	return m;
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


/* APIs to write to sys.stdout or sys.stderr using a printf-like interface.
   Adapted from code submitted by Just van Rossum.

   PySys_WriteStdout(format, ...)
   PySys_WriteStderr(format, ...)

      The first function writes to sys.stdout; the second to sys.stderr.  When
      there is a problem, they write to the real (C level) stdout or stderr;
      no exceptions are raised (but a pending exception may be cleared when a
      new exception is caught).

      Both take a printf-style format string as their first argument followed
      by a variable length argument list determined by the format string.

      *** WARNING ***

      The format should limit the total size of the formatted output string to
      1000 bytes.  In particular, this means that no unrestricted "%s" formats
      should occur; these should be limited using "%.<N>s where <N> is a
      decimal number calculated so that <N> plus the maximum size of other
      formatted text does not exceed 1000 bytes.  Also watch out for "%f",
      which can print hundreds of digits for very large numbers.

 */

static void
mywrite(name, fp, format, va)
	char *name;
	FILE *fp;
	const char *format;
	va_list va;
{
	PyObject *file;

	file = PySys_GetObject(name);
	if (file == NULL || PyFile_AsFile(file) == fp)
		vfprintf(fp, format, va);
	else {
		char buffer[1001];
		vsprintf(buffer, format, va);
		if (PyFile_WriteString(buffer, file) != 0) {
			PyErr_Clear();
			fputs(buffer, fp);
		}
	}
}

void
#ifdef HAVE_STDARG_PROTOTYPES
PySys_WriteStdout(const char *format, ...)
#else
PySys_WriteStdout(va_alist)
	va_dcl
#endif
{
	va_list va;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(va, format);
#else
	char *format;
	va_start(va);
	format = va_arg(va, char *);
#endif
	mywrite("stdout", stdout, format, va);
	va_end(va);
}

void
#ifdef HAVE_STDARG_PROTOTYPES
PySys_WriteStderr(const char *format, ...)
#else
PySys_WriteStderr(va_alist)
	va_dcl
#endif
{
	va_list va;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(va, format);
#else
	char *format;
	va_start(va);
	format = va_arg(va, char *);
#endif
	mywrite("stderr", stderr, format, va);
	va_end(va);
}

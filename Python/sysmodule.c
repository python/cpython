
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
#include "compile.h"
#include "frameobject.h"

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
PySys_GetObject(char *name)
{
	PyThreadState *tstate = PyThreadState_Get();
	PyObject *sd = tstate->interp->sysdict;
	if (sd == NULL)
		return NULL;
	return PyDict_GetItemString(sd, name);
}

FILE *
PySys_GetFile(char *name, FILE *def)
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
PySys_SetObject(char *name, PyObject *v)
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
sys_displayhook(PyObject *self, PyObject *args)
{
	PyObject *o, *outf;
	PyInterpreterState *interp = PyThreadState_Get()->interp;
	PyObject *modules = interp->modules;
	PyObject *builtins = PyDict_GetItemString(modules, "__builtin__");

	/* parse arguments */
	if (!PyArg_ParseTuple(args, "O:displayhook", &o))
		return NULL;

	/* Print value except if None */
	/* After printing, also assign to '_' */
	/* Before, set '_' to None to avoid recursion */
	if (o == Py_None) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (PyObject_SetAttrString(builtins, "_", Py_None) != 0)
		return NULL;
	if (Py_FlushLine() != 0)
		return NULL;
	outf = PySys_GetObject("stdout");
	if (outf == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
		return NULL;
	}
	if (PyFile_WriteObject(o, outf, 0) != 0)
		return NULL;
	PyFile_SoftSpace(outf, 1);
	if (Py_FlushLine() != 0)
		return NULL;
	if (PyObject_SetAttrString(builtins, "_", o) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char displayhook_doc[] =
"displayhook(object) -> None\n"
"\n"
"Print an object to sys.stdout and also save it in __builtin__._\n";

static PyObject *
sys_excepthook(PyObject* self, PyObject* args)
{
	PyObject *exc, *value, *tb;
	if (!PyArg_ParseTuple(args, "OOO:excepthook", &exc, &value, &tb))
		return NULL;
	PyErr_Display(exc, value, tb);
	Py_INCREF(Py_None);
	return Py_None;
}

static char excepthook_doc[] =
"excepthook(exctype, value, traceback) -> None\n"
"\n"
"Handle an exception by displaying it with a traceback on sys.stderr.\n";

static PyObject *
sys_exc_info(PyObject *self, PyObject *args)
{
	PyThreadState *tstate;
	if (!PyArg_ParseTuple(args, ":exc_info"))
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
sys_exit(PyObject *self, PyObject *args)
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
sys_getdefaultencoding(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":getdefaultencoding"))
		return NULL;
	return PyString_FromString(PyUnicode_GetDefaultEncoding());
}

static char getdefaultencoding_doc[] =
"getdefaultencoding() -> string\n\
\n\
Return the current default string encoding used by the Unicode \n\
implementation.";

static PyObject *
sys_setdefaultencoding(PyObject *self, PyObject *args)
{
	char *encoding;
	if (!PyArg_ParseTuple(args, "s:setdefaultencoding", &encoding))
		return NULL;
	if (PyUnicode_SetDefaultEncoding(encoding))
	    	return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char setdefaultencoding_doc[] =
"setdefaultencoding(encoding)\n\
\n\
Set the current default string encoding used by the Unicode implementation.";

extern int _PyTrace_Init(void);

static PyObject *
sys_settrace(PyObject *self, PyObject *args)
{
	PyThreadState *tstate = PyThreadState_Get();
	if (_PyTrace_Init() == -1)
		return NULL;
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
sys_setprofile(PyObject *self, PyObject *args)
{
	PyThreadState *tstate = PyThreadState_Get();
	if (_PyTrace_Init() == -1)
		return NULL;
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
sys_setcheckinterval(PyObject *self, PyObject *args)
{
	PyThreadState *tstate = PyThreadState_Get();
	if (!PyArg_ParseTuple(args, "i:setcheckinterval", &tstate->interp->checkinterval))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char setcheckinterval_doc[] =
"setcheckinterval(n)\n\
\n\
Tell the Python interpreter to check for asynchronous events every\n\
n instructions.  This also affects how often thread switches occur.";

static PyObject *
sys_setrecursionlimit(PyObject *self, PyObject *args)
{
	int new_limit;
	if (!PyArg_ParseTuple(args, "i:setrecursionlimit", &new_limit))
		return NULL;
	if (new_limit <= 0) {
		PyErr_SetString(PyExc_ValueError, 
				"recursion limit must be positive");  
		return NULL;
	}
	Py_SetRecursionLimit(new_limit);
	Py_INCREF(Py_None);
	return Py_None;
}

static char setrecursionlimit_doc[] =
"setrecursionlimit(n)\n\
\n\
Set the maximum depth of the Python interpreter stack to n.  This\n\
limit prevents infinite recursion from causing an overflow of the C\n\
stack and crashing Python.  The highest possible limit is platform-\n\
dependent.";

static PyObject *
sys_getrecursionlimit(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":getrecursionlimit"))
		return NULL;
	return PyInt_FromLong(Py_GetRecursionLimit());
}

static char getrecursionlimit_doc[] =
"getrecursionlimit()\n\
\n\
Return the current value of the recursion limit, the maximum depth\n\
of the Python interpreter stack.  This limit prevents infinite\n\
recursion from causing an overflow of the C stack and crashing Python.";

#ifdef USE_MALLOPT
/* Link with -lmalloc (or -lmpc) on an SGI */
#include <malloc.h>

static PyObject *
sys_mdebug(PyObject *self, PyObject *args)
{
	int flag;
	if (!PyArg_ParseTuple(args, "i:mdebug", &flag))
		return NULL;
	mallopt(M_DEBUG, flag);
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* USE_MALLOPT */

static PyObject *
sys_getrefcount(PyObject *self, PyObject *args)
{
	PyObject *arg;
	if (!PyArg_ParseTuple(args, "O:getrefcount", &arg))
		return NULL;
	return PyInt_FromLong(arg->ob_refcnt);
}

#ifdef Py_TRACE_REFS
static PyObject *
sys_gettotalrefcount(PyObject *self, PyObject *args)
{
	extern long _Py_RefTotal;
	if (!PyArg_ParseTuple(args, ":gettotalrefcount"))
		return NULL;
	return PyInt_FromLong(_Py_RefTotal);
}

#endif /* Py_TRACE_REFS */

static char getrefcount_doc[] =
"getrefcount(object) -> integer\n\
\n\
Return the current reference count for the object.  This includes the\n\
temporary reference in the argument list, so it is at least 2.";

#ifdef COUNT_ALLOCS
static PyObject *
sys_getcounts(PyObject *self, PyObject *args)
{
	extern PyObject *get_counts(void);

	if (!PyArg_ParseTuple(args, ":getcounts"))
		return NULL;
	return get_counts();
}
#endif

static char getframe_doc[] =
"_getframe([depth]) -> frameobject\n\
\n\
Return a frame object from the call stack.  If optional integer depth is\n\
given, return the frame object that many calls below the top of the stack.\n\
If that is deeper than the call stack, ValueError is raised.  The default\n\
for depth is zero, returning the frame at the top of the call stack.\n\
\n\
This function should be used for internal and specialized\n\
purposes only.";

static PyObject *
sys_getframe(PyObject *self, PyObject *args)
{
	PyFrameObject *f = PyThreadState_Get()->frame;
	int depth = -1;

	if (!PyArg_ParseTuple(args, "|i:_getframe", &depth))
		return NULL;

	while (depth > 0 && f != NULL) {
		f = f->f_back;
		--depth;
	}
	if (f == NULL) {
		PyErr_SetString(PyExc_ValueError,
				"call stack is not deep enough");
		return NULL;
	}
	Py_INCREF(f);
	return (PyObject*)f;
}


#ifdef Py_TRACE_REFS
/* Defined in objects.c because it uses static globals if that file */
extern PyObject *_Py_GetObjects(PyObject *, PyObject *);
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
/* Defined in ceval.c because it uses static globals if that file */
extern PyObject *_Py_GetDXProfile(PyObject *,  PyObject *);
#endif

static PyMethodDef sys_methods[] = {
	/* Might as well keep this in alphabetic order */
	{"displayhook",	sys_displayhook, 1, displayhook_doc},
	{"exc_info",	sys_exc_info, 1, exc_info_doc},
	{"excepthook",	sys_excepthook, 1, excepthook_doc},
	{"exit",	sys_exit, 0, exit_doc},
	{"getdefaultencoding", sys_getdefaultencoding, 1,
	 getdefaultencoding_doc}, 
#ifdef COUNT_ALLOCS
	{"getcounts",	sys_getcounts, 1},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
	{"getdxp",	_Py_GetDXProfile, 1},
#endif
#ifdef Py_TRACE_REFS
	{"getobjects",	_Py_GetObjects, 1},
	{"gettotalrefcount", sys_gettotalrefcount, 1},
#endif
	{"getrefcount",	sys_getrefcount, 1, getrefcount_doc},
	{"getrecursionlimit", sys_getrecursionlimit, 1,
	 getrecursionlimit_doc},
	{"_getframe", sys_getframe, 1, getframe_doc},
#ifdef USE_MALLOPT
	{"mdebug",	sys_mdebug, 1},
#endif
	{"setdefaultencoding", sys_setdefaultencoding, 1,
	 setdefaultencoding_doc}, 
	{"setcheckinterval",	sys_setcheckinterval, 1,
	 setcheckinterval_doc}, 
	{"setprofile",	sys_setprofile, 0, setprofile_doc},
	{"setrecursionlimit", sys_setrecursionlimit, 1,
	 setrecursionlimit_doc},
	{"settrace",	sys_settrace, 0, settrace_doc},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
list_builtin_module_names(void)
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

static PyObject *warnoptions = NULL;

void
PySys_ResetWarnOptions(void)
{
	if (warnoptions == NULL || !PyList_Check(warnoptions))
		return;
	PyList_SetSlice(warnoptions, 0, PyList_GET_SIZE(warnoptions), NULL);
}

void
PySys_AddWarnOption(char *s)
{
	PyObject *str;

	if (warnoptions == NULL || !PyList_Check(warnoptions)) {
		Py_XDECREF(warnoptions);
		warnoptions = PyList_New(0);
		if (warnoptions == NULL)
			return;
	}
	str = PyString_FromString(s);
	if (str != NULL) {
		PyList_Append(warnoptions, str);
		Py_DECREF(str);
	}
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
\n\
displayhook -- called to show results in an interactive session\n\
excepthook -- called to handle any uncaught exception other than SystemExit\n\
  To customize printing in an interactive session or to install a custom\n\
  top-level exception handler, assign other functions to replace these.\n\
\n\
exitfunc -- if sys.exitfunc exists, this routine is called when Python exits\n\
  Assigning to sys.exitfunc is deprecated; use the atexit module instead.\n\
\n\
stdin -- standard input file object; used by raw_input() and input()\n\
stdout -- standard output file object; used by the print statement\n\
stderr -- standard error object; used for error messages\n\
  By assigning other file objects (or objects that behave like files)\n\
  to these, it is possible to redirect all of the interpreter's I/O.\n\
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
"
#ifndef MS_WIN16
/* concatenating string here */
"\n\
Static objects:\n\
\n\
maxint -- the largest supported integer (the smallest is -maxint-1)\n\
maxunicode -- the largest supported character\n\
builtin_module_names -- tuple of module names built into this intepreter\n\
version -- the version of this interpreter as a string\n\
version_info -- version information as a tuple\n\
hexversion -- version information encoded as a single integer\n\
copyright -- copyright notice pertaining to this interpreter\n\
platform -- platform identifier\n\
executable -- pathname of this Python interpreter\n\
prefix -- prefix used to find the Python library\n\
exec_prefix -- prefix used to find the machine-specific Python library\n\
"
#ifdef MS_WINDOWS
/* concatenating string here */
"dllhandle -- [Windows only] integer handle of the Python DLL\n\
winver -- [Windows only] version number of the Python DLL\n\
"
#endif /* MS_WINDOWS */
"__stdin__ -- the original stdin; don't touch!\n\
__stdout__ -- the original stdout; don't touch!\n\
__stderr__ -- the original stderr; don't touch!\n\
__displayhook__ -- the original displayhook; don't touch!\n\
__excepthook__ -- the original excepthook; don't touch!\n\
\n\
Functions:\n\
\n\
displayhook() -- print an object to the screen, and save it in __builtin__._\n\
excepthook() -- print an exception and its traceback to sys.stderr\n\
exc_info() -- return thread-safe information about the current exception\n\
exit() -- exit the interpreter by raising SystemExit\n\
getrefcount() -- return the reference count for an object (plus one :-)\n\
getrecursionlimit() -- return the max recursion depth for the interpreter\n\
setcheckinterval() -- control how often the interpreter checks for events\n\
setprofile() -- set the global profiling function\n\
setrecursionlimit() -- set the max recursion depth for the interpreter\n\
settrace() -- set the global debug tracing function\n\
"
#endif /* MS_WIN16 */
/* end of sys_doc */ ;

PyObject *
_PySys_Init(void)
{
	PyObject *m, *v, *sysdict;
	PyObject *sysin, *sysout, *syserr;
	char *s;

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
	PyDict_SetItemString(sysdict, "__displayhook__",
                             PyDict_GetItemString(sysdict, "displayhook"));
	PyDict_SetItemString(sysdict, "__excepthook__",
                             PyDict_GetItemString(sysdict, "excepthook"));
	Py_XDECREF(sysin);
	Py_XDECREF(sysout);
	Py_XDECREF(syserr);
	PyDict_SetItemString(sysdict, "version",
			     v = PyString_FromString(Py_GetVersion()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "hexversion",
			     v = PyInt_FromLong(PY_VERSION_HEX));
	Py_XDECREF(v);
	/*
	 * These release level checks are mutually exclusive and cover
	 * the field, so don't get too fancy with the pre-processor!
	 */
#if PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_ALPHA
	s = "alpha";
#elif PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_BETA
	s = "beta";
#elif PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_GAMMA
	s = "candidate";
#elif PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_FINAL
	s = "final";
#endif
	PyDict_SetItemString(sysdict, "version_info",
			     v = Py_BuildValue("iiisi", PY_MAJOR_VERSION,
					       PY_MINOR_VERSION,
					       PY_MICRO_VERSION, s,
					       PY_RELEASE_SERIAL));
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
	PyDict_SetItemString(sysdict, "maxunicode",
			     v = PyInt_FromLong(PyUnicode_GetMax()));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "builtin_module_names",
		   v = list_builtin_module_names());
	Py_XDECREF(v);
	{
		/* Assumes that longs are at least 2 bytes long.
		   Should be safe! */
		unsigned long number = 1;
		char *value;

		s = (char *) &number;
		if (s[0] == 0)
			value = "big";
		else
			value = "little";
		PyDict_SetItemString(sysdict, "byteorder",
				     v = PyString_FromString(value));
		Py_XDECREF(v);
	}
#ifdef MS_COREDLL
	PyDict_SetItemString(sysdict, "dllhandle",
			     v = PyLong_FromVoidPtr(PyWin_DLLhModule));
	Py_XDECREF(v);
	PyDict_SetItemString(sysdict, "winver",
			     v = PyString_FromString(PyWin_DLLVersionString));
	Py_XDECREF(v);
#endif
	if (warnoptions == NULL) {
		warnoptions = PyList_New(0);
	}
	else {
		Py_INCREF(warnoptions);
	}
	if (warnoptions != NULL) {
		PyDict_SetItemString(sysdict, "warnoptions", warnoptions);
	}
	
	if (PyErr_Occurred())
		return NULL;
	return m;
}

static PyObject *
makepathobject(char *path, int delim)
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
PySys_SetPath(char *path)
{
	PyObject *v;
	if ((v = makepathobject(path, DELIM)) == NULL)
		Py_FatalError("can't create sys.path");
	if (PySys_SetObject("path", v) != 0)
		Py_FatalError("can't assign sys.path");
	Py_DECREF(v);
}

static PyObject *
makeargvobject(int argc, char **argv)
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
PySys_SetArgv(int argc, char **argv)
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
#ifndef RISCOS
			n = p + 1 - argv0;
#else /* don't include trailing separator */
			n = p - argv0;
#endif /* RISCOS */
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
      no exceptions are raised.

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
mywrite(char *name, FILE *fp, const char *format, va_list va)
{
	PyObject *file;
	PyObject *error_type, *error_value, *error_traceback;

	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	file = PySys_GetObject(name);
	if (file == NULL || PyFile_AsFile(file) == fp)
		vfprintf(fp, format, va);
	else {
		char buffer[1001];
		if (vsprintf(buffer, format, va) >= sizeof(buffer))
		    Py_FatalError("PySys_WriteStdout/err: buffer overrun");
		if (PyFile_WriteString(buffer, file) != 0) {
			PyErr_Clear();
			fputs(buffer, fp);
		}
	}
	PyErr_Restore(error_type, error_value, error_traceback);
}

void
PySys_WriteStdout(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	mywrite("stdout", stdout, format, va);
	va_end(va);
}

void
PySys_WriteStderr(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	mywrite("stderr", stderr, format, va);
	va_end(va);
}

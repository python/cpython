
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
#include "structseq.h"
#include "code.h"
#include "frameobject.h"
#include "eval.h"

#include "osdefs.h"

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* MS_WINDOWS */

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
/* A string loaded from the DLL at startup: */
extern const char *PyWin_DLLVersionString;
#endif

#ifdef __VMS
#include <unixlib.h>
#endif

#ifdef HAVE_LANGINFO_H
#include <locale.h>
#include <langinfo.h>
#endif

PyObject *
PySys_GetObject(const char *name)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyObject *sd = tstate->interp->sysdict;
	if (sd == NULL)
		return NULL;
	return PyDict_GetItemString(sd, name);
}

int
PySys_SetObject(const char *name, PyObject *v)
{
	PyThreadState *tstate = PyThreadState_GET();
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
sys_displayhook(PyObject *self, PyObject *o)
{
	PyObject *outf;
	PyInterpreterState *interp = PyThreadState_GET()->interp;
	PyObject *modules = interp->modules;
	PyObject *builtins = PyDict_GetItemString(modules, "builtins");

	if (builtins == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "lost builtins module");
		return NULL;
	}

	/* Print value except if None */
	/* After printing, also assign to '_' */
	/* Before, set '_' to None to avoid recursion */
	if (o == Py_None) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (PyObject_SetAttrString(builtins, "_", Py_None) != 0)
		return NULL;
	outf = PySys_GetObject("stdout");
	if (outf == NULL || outf == Py_None) {
		PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
		return NULL;
	}
	if (PyFile_WriteObject(o, outf, 0) != 0)
		return NULL;
	if (PyFile_WriteString("\n", outf) != 0)
		return NULL;
	if (PyObject_SetAttrString(builtins, "_", o) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(displayhook_doc,
"displayhook(object) -> None\n"
"\n"
"Print an object to sys.stdout and also save it in builtins.\n"
);

static PyObject *
sys_excepthook(PyObject* self, PyObject* args)
{
	PyObject *exc, *value, *tb;
	if (!PyArg_UnpackTuple(args, "excepthook", 3, 3, &exc, &value, &tb))
		return NULL;
	PyErr_Display(exc, value, tb);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(excepthook_doc,
"excepthook(exctype, value, traceback) -> None\n"
"\n"
"Handle an exception by displaying it with a traceback on sys.stderr.\n"
);

static PyObject *
sys_exc_info(PyObject *self, PyObject *noargs)
{
	PyThreadState *tstate;
	tstate = PyThreadState_GET();
	return Py_BuildValue(
		"(OOO)",
		tstate->exc_type != NULL ? tstate->exc_type : Py_None,
		tstate->exc_value != NULL ? tstate->exc_value : Py_None,
		tstate->exc_traceback != NULL ?
			tstate->exc_traceback : Py_None);
}

PyDoc_STRVAR(exc_info_doc,
"exc_info() -> (type, value, traceback)\n\
\n\
Return information about the most recent exception caught by an except\n\
clause in the current stack frame or in an older stack frame."
);

static PyObject *
sys_exit(PyObject *self, PyObject *args)
{
	PyObject *exit_code = 0;
	if (!PyArg_UnpackTuple(args, "exit", 0, 1, &exit_code))
		return NULL;
	/* Raise SystemExit so callers may catch it or clean up. */
	PyErr_SetObject(PyExc_SystemExit, exit_code);
	return NULL;
}

PyDoc_STRVAR(exit_doc,
"exit([status])\n\
\n\
Exit the interpreter by raising SystemExit(status).\n\
If the status is omitted or None, it defaults to zero (i.e., success).\n\
If the status is numeric, it will be used as the system exit status.\n\
If it is another kind of object, it will be printed and the system\n\
exit status will be one (i.e., failure)."
);


static PyObject *
sys_getdefaultencoding(PyObject *self)
{
	return PyUnicode_FromString(PyUnicode_GetDefaultEncoding());
}

PyDoc_STRVAR(getdefaultencoding_doc,
"getdefaultencoding() -> string\n\
\n\
Return the current default string encoding used by the Unicode \n\
implementation."
);

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

PyDoc_STRVAR(setdefaultencoding_doc,
"setdefaultencoding(encoding)\n\
\n\
Set the current default string encoding used by the Unicode implementation."
);

static PyObject *
sys_getfilesystemencoding(PyObject *self)
{
	if (Py_FileSystemDefaultEncoding)
		return PyUnicode_FromString(Py_FileSystemDefaultEncoding);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(getfilesystemencoding_doc,
"getfilesystemencoding() -> string\n\
\n\
Return the encoding used to convert Unicode filenames in\n\
operating system filenames."
);



static PyObject *
sys_intern(PyObject *self, PyObject *args)
{
	PyObject *s;
	if (!PyArg_ParseTuple(args, "U:intern", &s))
		return NULL;
	if (PyUnicode_CheckExact(s)) {
		Py_INCREF(s);
		PyUnicode_InternInPlace(&s);
		return s;
	}
	else {
		PyErr_Format(PyExc_TypeError,
				"can't intern %.400s", s->ob_type->tp_name);
		return NULL;
	}
}

PyDoc_STRVAR(intern_doc,
"intern(string) -> string\n\
\n\
``Intern'' the given string.  This enters the string in the (global)\n\
table of interned strings whose purpose is to speed up dictionary lookups.\n\
Return the string itself or the previously interned string object with the\n\
same value.");


/*
 * Cached interned string objects used for calling the profile and
 * trace functions.  Initialized by trace_init().
 */
static PyObject *whatstrings[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int
trace_init(void)
{
	static char *whatnames[7] = {"call", "exception", "line", "return",
					"c_call", "c_exception", "c_return"};
	PyObject *name;
	int i;
	for (i = 0; i < 7; ++i) {
		if (whatstrings[i] == NULL) {
			name = PyUnicode_InternFromString(whatnames[i]);
			if (name == NULL)
				return -1;
			whatstrings[i] = name;
                }
	}
	return 0;
}


static PyObject *
call_trampoline(PyThreadState *tstate, PyObject* callback,
		PyFrameObject *frame, int what, PyObject *arg)
{
	PyObject *args = PyTuple_New(3);
	PyObject *whatstr;
	PyObject *result;

	if (args == NULL)
		return NULL;
	Py_INCREF(frame);
	whatstr = whatstrings[what];
	Py_INCREF(whatstr);
	if (arg == NULL)
		arg = Py_None;
	Py_INCREF(arg);
	PyTuple_SET_ITEM(args, 0, (PyObject *)frame);
	PyTuple_SET_ITEM(args, 1, whatstr);
	PyTuple_SET_ITEM(args, 2, arg);

	/* call the Python-level function */
	PyFrame_FastToLocals(frame);
	result = PyEval_CallObject(callback, args);
	PyFrame_LocalsToFast(frame, 1);
	if (result == NULL)
		PyTraceBack_Here(frame);

	/* cleanup */
	Py_DECREF(args);
	return result;
}

static int
profile_trampoline(PyObject *self, PyFrameObject *frame,
		   int what, PyObject *arg)
{
	PyThreadState *tstate = frame->f_tstate;
	PyObject *result;

	if (arg == NULL)
		arg = Py_None;
	result = call_trampoline(tstate, self, frame, what, arg);
	if (result == NULL) {
		PyEval_SetProfile(NULL, NULL);
		return -1;
	}
	Py_DECREF(result);
	return 0;
}

static int
trace_trampoline(PyObject *self, PyFrameObject *frame,
		 int what, PyObject *arg)
{
	PyThreadState *tstate = frame->f_tstate;
	PyObject *callback;
	PyObject *result;

	if (what == PyTrace_CALL)
		callback = self;
	else
		callback = frame->f_trace;
	if (callback == NULL)
		return 0;
	result = call_trampoline(tstate, callback, frame, what, arg);
	if (result == NULL) {
		PyEval_SetTrace(NULL, NULL);
		Py_XDECREF(frame->f_trace);
		frame->f_trace = NULL;
		return -1;
	}
	if (result != Py_None) {
		PyObject *temp = frame->f_trace;
		frame->f_trace = NULL;
		Py_XDECREF(temp);
		frame->f_trace = result;
	}
	else {
		Py_DECREF(result);
	}
	return 0;
}

static PyObject *
sys_settrace(PyObject *self, PyObject *args)
{
	if (trace_init() == -1)
		return NULL;
	if (args == Py_None)
		PyEval_SetTrace(NULL, NULL);
	else
		PyEval_SetTrace(trace_trampoline, args);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(settrace_doc,
"settrace(function)\n\
\n\
Set the global debug tracing function.  It will be called on each\n\
function call.  See the debugger chapter in the library manual."
);

static PyObject *
sys_gettrace(PyObject *self, PyObject *args)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyObject *temp = tstate->c_traceobj;

	if (temp == NULL)
		temp = Py_None;
	Py_INCREF(temp);
	return temp;
}

PyDoc_STRVAR(gettrace_doc,
"gettrace()\n\
\n\
Return the global debug tracing function set with sys.settrace.\n\
See the debugger chapter in the library manual."
);

static PyObject *
sys_setprofile(PyObject *self, PyObject *args)
{
	if (trace_init() == -1)
		return NULL;
	if (args == Py_None)
		PyEval_SetProfile(NULL, NULL);
	else
		PyEval_SetProfile(profile_trampoline, args);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setprofile_doc,
"setprofile(function)\n\
\n\
Set the profiling function.  It will be called on each function call\n\
and return.  See the profiler chapter in the library manual."
);

static PyObject *
sys_getprofile(PyObject *self, PyObject *args)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyObject *temp = tstate->c_profileobj;

	if (temp == NULL)
		temp = Py_None;
	Py_INCREF(temp);
	return temp;
}

PyDoc_STRVAR(getprofile_doc,
"getprofile()\n\
\n\
Return the profiling function set with sys.setprofile.\n\
See the profiler chapter in the library manual."
);

static PyObject *
sys_setcheckinterval(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i:setcheckinterval", &_Py_CheckInterval))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setcheckinterval_doc,
"setcheckinterval(n)\n\
\n\
Tell the Python interpreter to check for asynchronous events every\n\
n instructions.  This also affects how often thread switches occur."
);

static PyObject *
sys_getcheckinterval(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(_Py_CheckInterval);
}

PyDoc_STRVAR(getcheckinterval_doc,
"getcheckinterval() -> current check interval; see setcheckinterval()."
);

#ifdef WITH_TSC
static PyObject *
sys_settscdump(PyObject *self, PyObject *args)
{
	int bool;
	PyThreadState *tstate = PyThreadState_Get();

	if (!PyArg_ParseTuple(args, "i:settscdump", &bool))
		return NULL;
	if (bool)
		tstate->interp->tscdump = 1;
	else
		tstate->interp->tscdump = 0;
	Py_INCREF(Py_None);
	return Py_None;

}

PyDoc_STRVAR(settscdump_doc,
"settscdump(bool)\n\
\n\
If true, tell the Python interpreter to dump VM measurements to\n\
stderr.  If false, turn off dump.  The measurements are based on the\n\
processor's time-stamp counter."
);
#endif /* TSC */

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

PyDoc_STRVAR(setrecursionlimit_doc,
"setrecursionlimit(n)\n\
\n\
Set the maximum depth of the Python interpreter stack to n.  This\n\
limit prevents infinite recursion from causing an overflow of the C\n\
stack and crashing Python.  The highest possible limit is platform-\n\
dependent."
);

static PyObject *
sys_getrecursionlimit(PyObject *self)
{
	return PyLong_FromLong(Py_GetRecursionLimit());
}

PyDoc_STRVAR(getrecursionlimit_doc,
"getrecursionlimit()\n\
\n\
Return the current value of the recursion limit, the maximum depth\n\
of the Python interpreter stack.  This limit prevents infinite\n\
recursion from causing an overflow of the C stack and crashing Python."
);

#ifdef MS_WINDOWS
PyDoc_STRVAR(getwindowsversion_doc,
"getwindowsversion()\n\
\n\
Return information about the running version of Windows.\n\
The result is a tuple of (major, minor, build, platform, text)\n\
All elements are numbers, except text which is a string.\n\
Platform may be 0 for win32s, 1 for Windows 9x/ME, 2 for Windows NT/2000/XP\n\
"
);

static PyObject *
sys_getwindowsversion(PyObject *self)
{
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(ver);
	if (!GetVersionEx(&ver))
		return PyErr_SetFromWindowsErr(0);
	return Py_BuildValue("HHHHs",
	                     ver.dwMajorVersion,
	                     ver.dwMinorVersion,
	                     ver.dwBuildNumber,
	                     ver.dwPlatformId,
	                     ver.szCSDVersion);
}

#endif /* MS_WINDOWS */

#ifdef HAVE_DLOPEN
static PyObject *
sys_setdlopenflags(PyObject *self, PyObject *args)
{
	int new_val;
        PyThreadState *tstate = PyThreadState_GET();
	if (!PyArg_ParseTuple(args, "i:setdlopenflags", &new_val))
		return NULL;
        if (!tstate)
		return NULL;
        tstate->interp->dlopenflags = new_val;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setdlopenflags_doc,
"setdlopenflags(n) -> None\n\
\n\
Set the flags that will be used for dlopen() calls. Among other\n\
things, this will enable a lazy resolving of symbols when importing\n\
a module, if called as sys.setdlopenflags(0)\n\
To share symbols across extension modules, call as\n\
sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)"
);

static PyObject *
sys_getdlopenflags(PyObject *self, PyObject *args)
{
        PyThreadState *tstate = PyThreadState_GET();
        if (!tstate)
		return NULL;
        return PyLong_FromLong(tstate->interp->dlopenflags);
}

PyDoc_STRVAR(getdlopenflags_doc,
"getdlopenflags() -> int\n\
\n\
Return the current value of the flags that are used for dlopen()\n\
calls. The flag constants are defined in the dl module."
);
#endif

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
sys_getrefcount(PyObject *self, PyObject *arg)
{
	return PyLong_FromSsize_t(arg->ob_refcnt);
}

#ifdef Py_REF_DEBUG
static PyObject *
sys_gettotalrefcount(PyObject *self)
{
	return PyLong_FromSsize_t(_Py_GetRefTotal());
}
#endif /* Py_REF_DEBUG */

PyDoc_STRVAR(getrefcount_doc,
"getrefcount(object) -> integer\n\
\n\
Return the reference count of object.  The count returned is generally\n\
one higher than you might expect, because it includes the (temporary)\n\
reference as an argument to getrefcount()."
);

#ifdef COUNT_ALLOCS
static PyObject *
sys_getcounts(PyObject *self)
{
	extern PyObject *get_counts(void);

	return get_counts();
}
#endif

PyDoc_STRVAR(getframe_doc,
"_getframe([depth]) -> frameobject\n\
\n\
Return a frame object from the call stack.  If optional integer depth is\n\
given, return the frame object that many calls below the top of the stack.\n\
If that is deeper than the call stack, ValueError is raised.  The default\n\
for depth is zero, returning the frame at the top of the call stack.\n\
\n\
This function should be used for internal and specialized\n\
purposes only."
);

static PyObject *
sys_getframe(PyObject *self, PyObject *args)
{
	PyFrameObject *f = PyThreadState_GET()->frame;
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

PyDoc_STRVAR(current_frames_doc,
"_current_frames() -> dictionary\n\
\n\
Return a dictionary mapping each current thread T's thread id to T's\n\
current stack frame.\n\
\n\
This function should be used for specialized purposes only."
);

static PyObject *
sys_current_frames(PyObject *self, PyObject *noargs)
{
	return _PyThread_CurrentFrames();
}

PyDoc_STRVAR(call_tracing_doc,
"call_tracing(func, args) -> object\n\
\n\
Call func(*args), while tracing is enabled.  The tracing state is\n\
saved, and restored afterwards.  This is intended to be called from\n\
a debugger from a checkpoint, to recursively debug some other code."
);

static PyObject *
sys_call_tracing(PyObject *self, PyObject *args)
{
	PyObject *func, *funcargs;
	if (!PyArg_UnpackTuple(args, "call_tracing", 2, 2, &func, &funcargs))
		return NULL;
	return _PyEval_CallTracing(func, funcargs);
}

PyDoc_STRVAR(callstats_doc,
"callstats() -> tuple of integers\n\
\n\
Return a tuple of function call statistics, if CALL_PROFILE was defined\n\
when Python was built.  Otherwise, return None.\n\
\n\
When enabled, this function returns detailed, implementation-specific\n\
details about the number of function calls executed. The return value is\n\
a 11-tuple where the entries in the tuple are counts of:\n\
0. all function calls\n\
1. calls to PyFunction_Type objects\n\
2. PyFunction calls that do not create an argument tuple\n\
3. PyFunction calls that do not create an argument tuple\n\
   and bypass PyEval_EvalCodeEx()\n\
4. PyMethod calls\n\
5. PyMethod calls on bound methods\n\
6. PyType calls\n\
7. PyCFunction calls\n\
8. generator calls\n\
9. All other calls\n\
10. Number of stack pops performed by call_function()"
);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Py_TRACE_REFS
/* Defined in objects.c because it uses static globals if that file */
extern PyObject *_Py_GetObjects(PyObject *, PyObject *);
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
/* Defined in ceval.c because it uses static globals if that file */
extern PyObject *_Py_GetDXProfile(PyObject *,  PyObject *);
#endif

#ifdef __cplusplus
}
#endif

static PyObject *
sys_clear_type_cache(PyObject* self, PyObject* args)
{
	PyType_ClearCache();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(sys_clear_type_cache__doc__,
"_clear_type_cache() -> None\n\
Clear the internal type lookup cache.");


static PyObject *
sys_compact_freelists(PyObject* self, PyObject* args)
{
	size_t fsum, fbc, fbf;

	PyFloat_CompactFreeList(&fbc, &fbf, &fsum);

	return Py_BuildValue("((kkk))", fsum, fbc, fbf);

}

PyDoc_STRVAR(sys_compact_freelists__doc__,
"_compact_freelists() -> ((remaing_objects, total_blocks, freed_blocks),)\n\
Compact the free lists of floats.");

static PyMethodDef sys_methods[] = {
	/* Might as well keep this in alphabetic order */
	{"callstats", (PyCFunction)PyEval_GetCallStats, METH_NOARGS,
	 callstats_doc},
	{"_clear_type_cache",	sys_clear_type_cache,	  METH_NOARGS,
	 sys_clear_type_cache__doc__},
	{"_compact_freelists",	sys_compact_freelists,	  METH_NOARGS,
	 sys_compact_freelists__doc__},
	{"_current_frames", sys_current_frames, METH_NOARGS,
	 current_frames_doc},
	{"displayhook",	sys_displayhook, METH_O, displayhook_doc},
	{"exc_info",	sys_exc_info, METH_NOARGS, exc_info_doc},
	{"excepthook",	sys_excepthook, METH_VARARGS, excepthook_doc},
	{"exit",	sys_exit, METH_VARARGS, exit_doc},
	{"getdefaultencoding", (PyCFunction)sys_getdefaultencoding,
	 METH_NOARGS, getdefaultencoding_doc},
#ifdef HAVE_DLOPEN
	{"getdlopenflags", (PyCFunction)sys_getdlopenflags, METH_NOARGS,
	 getdlopenflags_doc},
#endif
#ifdef COUNT_ALLOCS
	{"getcounts",	(PyCFunction)sys_getcounts, METH_NOARGS},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
	{"getdxp",	_Py_GetDXProfile, METH_VARARGS},
#endif
	{"getfilesystemencoding", (PyCFunction)sys_getfilesystemencoding,
	 METH_NOARGS, getfilesystemencoding_doc},
#ifdef Py_TRACE_REFS
	{"getobjects",	_Py_GetObjects, METH_VARARGS},
#endif
#ifdef Py_REF_DEBUG
	{"gettotalrefcount", (PyCFunction)sys_gettotalrefcount, METH_NOARGS},
#endif
	{"getrefcount",	(PyCFunction)sys_getrefcount, METH_O, getrefcount_doc},
	{"getrecursionlimit", (PyCFunction)sys_getrecursionlimit, METH_NOARGS,
	 getrecursionlimit_doc},
	{"_getframe", sys_getframe, METH_VARARGS, getframe_doc},
#ifdef MS_WINDOWS
	{"getwindowsversion", (PyCFunction)sys_getwindowsversion, METH_NOARGS,
	 getwindowsversion_doc},
#endif /* MS_WINDOWS */
 	{"intern",	sys_intern,     METH_VARARGS, intern_doc},
#ifdef USE_MALLOPT
	{"mdebug",	sys_mdebug, METH_VARARGS},
#endif
	{"setdefaultencoding", sys_setdefaultencoding, METH_VARARGS,
	 setdefaultencoding_doc},
	{"setcheckinterval",	sys_setcheckinterval, METH_VARARGS,
	 setcheckinterval_doc},
	{"getcheckinterval",	sys_getcheckinterval, METH_NOARGS,
	 getcheckinterval_doc},
#ifdef HAVE_DLOPEN
	{"setdlopenflags", sys_setdlopenflags, METH_VARARGS,
	 setdlopenflags_doc},
#endif
	{"setprofile",	sys_setprofile, METH_O, setprofile_doc},
	{"getprofile",	sys_getprofile, METH_NOARGS, getprofile_doc},
	{"setrecursionlimit", sys_setrecursionlimit, METH_VARARGS,
	 setrecursionlimit_doc},
#ifdef WITH_TSC
	{"settscdump", sys_settscdump, METH_VARARGS, settscdump_doc},
#endif
	{"settrace",	sys_settrace, METH_O, settrace_doc},
	{"gettrace",	sys_gettrace, METH_NOARGS, gettrace_doc},
	{"call_tracing", sys_call_tracing, METH_VARARGS, call_tracing_doc},
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
		PyObject *name = PyUnicode_FromString(
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
PySys_AddWarnOption(const char *s)
{
	PyObject *str;

	if (warnoptions == NULL || !PyList_Check(warnoptions)) {
		Py_XDECREF(warnoptions);
		warnoptions = PyList_New(0);
		if (warnoptions == NULL)
			return;
	}
	str = PyUnicode_FromString(s);
	if (str != NULL) {
		PyList_Append(warnoptions, str);
		Py_DECREF(str);
	}
}

/* XXX This doc string is too long to be a single string literal in VC++ 5.0.
   Two literals concatenated works just fine.  If you have a K&R compiler
   or other abomination that however *does* understand longer strings,
   get rid of the !!! comment in the middle and the quotes that surround it. */
PyDoc_VAR(sys_doc) =
PyDoc_STR(
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
stdin -- standard input file object; used by raw_input() and input()\n\
stdout -- standard output file object; used by print()\n\
stderr -- standard error object; used for error messages\n\
  By assigning other file objects (or objects that behave like files)\n\
  to these, it is possible to redirect all of the interpreter's I/O.\n\
\n\
last_type -- type of last uncaught exception\n\
last_value -- value of last uncaught exception\n\
last_traceback -- traceback of last uncaught exception\n\
  These three are only available in an interactive session after a\n\
  traceback has been printed.\n\
"
)
/* concatenating string here */
PyDoc_STR(
"\n\
Static objects:\n\
\n\
float_info -- a dict with information about the float implementation.\n\
maxsize -- the largest supported length of containers.\n\
maxunicode -- the largest supported character\n\
builtin_module_names -- tuple of module names built into this interpreter\n\
subversion -- subversion information of the build as tuple\n\
version -- the version of this interpreter as a string\n\
version_info -- version information as a tuple\n\
hexversion -- version information encoded as a single integer\n\
copyright -- copyright notice pertaining to this interpreter\n\
platform -- platform identifier\n\
executable -- pathname of this Python interpreter\n\
prefix -- prefix used to find the Python library\n\
exec_prefix -- prefix used to find the machine-specific Python library\n\
"
)
#ifdef MS_WINDOWS
/* concatenating string here */
PyDoc_STR(
"dllhandle -- [Windows only] integer handle of the Python DLL\n\
winver -- [Windows only] version number of the Python DLL\n\
"
)
#endif /* MS_WINDOWS */
PyDoc_STR(
"__stdin__ -- the original stdin; don't touch!\n\
__stdout__ -- the original stdout; don't touch!\n\
__stderr__ -- the original stderr; don't touch!\n\
__displayhook__ -- the original displayhook; don't touch!\n\
__excepthook__ -- the original excepthook; don't touch!\n\
\n\
Functions:\n\
\n\
displayhook() -- print an object to the screen, and save it in builtins._\n\
excepthook() -- print an exception and its traceback to sys.stderr\n\
exc_info() -- return thread-safe information about the current exception\n\
exit() -- exit the interpreter by raising SystemExit\n\
getdlopenflags() -- returns flags to be used for dlopen() calls\n\
getprofile() -- get the global profiling function\n\
getrefcount() -- return the reference count for an object (plus one :-)\n\
getrecursionlimit() -- return the max recursion depth for the interpreter\n\
gettrace() -- get the global debug tracing function\n\
setcheckinterval() -- control how often the interpreter checks for events\n\
setdlopenflags() -- set the flags to be used for dlopen() calls\n\
setprofile() -- set the global profiling function\n\
setrecursionlimit() -- set the max recursion depth for the interpreter\n\
settrace() -- set the global debug tracing function\n\
"
)
/* end of sys_doc */ ;

/* Subversion branch and revision management */
static const char _patchlevel_revision[] = PY_PATCHLEVEL_REVISION;
static const char headurl[] = "$HeadURL$";
static int svn_initialized;
static char patchlevel_revision[50]; /* Just the number */
static char branch[50];
static char shortbranch[50];
static const char *svn_revision;

static void
svnversion_init(void)
{
	const char *python, *br_start, *br_end, *br_end2, *svnversion;
	Py_ssize_t len;
	int istag = 0;

	if (svn_initialized)
		return;

	python = strstr(headurl, "/python/");
	if (!python) {
		strcpy(branch, "unknown branch");
		strcpy(shortbranch, "unknown");
	}
	else {
		br_start = python + 8;
		br_end = strchr(br_start, '/');
		assert(br_end);

		/* Works even for trunk,
		   as we are in trunk/Python/sysmodule.c */
		br_end2 = strchr(br_end+1, '/');

		istag = strncmp(br_start, "tags", 4) == 0;
		if (strncmp(br_start, "trunk", 5) == 0) {
			strcpy(branch, "trunk");
			strcpy(shortbranch, "trunk");
		}
		else if (istag || strncmp(br_start, "branches", 8) == 0) {
			len = br_end2 - br_start;
			strncpy(branch, br_start, len);
			branch[len] = '\0';

			len = br_end2 - (br_end + 1);
			strncpy(shortbranch, br_end + 1, len);
			shortbranch[len] = '\0';
		}
		else {
			Py_FatalError("bad HeadURL");
			return;
		}
	}


	svnversion = _Py_svnversion();
	if (strcmp(svnversion, "exported") != 0)
		svn_revision = svnversion;
	else if (istag) {
		len = strlen(_patchlevel_revision);
		assert(len >= 13);
		assert(len < (sizeof(patchlevel_revision) + 13));
		strncpy(patchlevel_revision, _patchlevel_revision + 11,
			len - 13);
		patchlevel_revision[len - 13] = '\0';
		svn_revision = patchlevel_revision;
	}
	else
		svn_revision = "";

	svn_initialized = 1;
}

/* Return svnversion output if available.
   Else return Revision of patchlevel.h if on branch.
   Else return empty string */
const char*
Py_SubversionRevision()
{
	svnversion_init();
	return svn_revision;
}

const char*
Py_SubversionShortBranch()
{
	svnversion_init();
	return shortbranch;
}


PyDoc_STRVAR(flags__doc__,
"sys.flags\n\
\n\
Flags provided through command line arguments or environment vars.");

static PyTypeObject FlagsType;

static PyStructSequence_Field flags_fields[] = {
	{"debug",		"-d"},
	{"division_warning",	"-Q"},
	{"inspect",		"-i"},
	{"interactive",		"-i"},
	{"optimize",		"-O or -OO"},
	{"dont_write_bytecode",	"-B"},
	/* {"no_user_site",	"-s"}, */
	{"no_site",		"-S"},
	{"ignore_environment",	"-E"},
	{"tabcheck",		"-t or -tt"},
	{"verbose",		"-v"},
#ifdef RISCOS
	{"riscos_wimp",		"???"},
#endif
	/* {"unbuffered",		"-u"}, */
	/* {"skip_first",		"-x"}, */
	{0}
};

static PyStructSequence_Desc flags_desc = {
	"sys.flags",	/* name */
	flags__doc__,	/* doc */
	flags_fields,	/* fields */
#ifdef RISCOS
	11
#else
	10
#endif
};

static PyObject*
make_flags(void)
{
	int pos = 0;
	PyObject *seq;

	seq = PyStructSequence_New(&FlagsType);
	if (seq == NULL)
		return NULL;

#define SetFlag(flag) \
	PyStructSequence_SET_ITEM(seq, pos++, PyLong_FromLong(flag))

	SetFlag(Py_DebugFlag);
	SetFlag(Py_DivisionWarningFlag);
	SetFlag(Py_InspectFlag);
	SetFlag(Py_InteractiveFlag);
	SetFlag(Py_OptimizeFlag);
	SetFlag(Py_DontWriteBytecodeFlag);
	/* SetFlag(Py_NoUserSiteDirectory); */
	SetFlag(Py_NoSiteFlag);
	SetFlag(Py_IgnoreEnvironmentFlag);
	SetFlag(Py_TabcheckFlag);
	SetFlag(Py_VerboseFlag);
#ifdef RISCOS
	SetFlag(Py_RISCOSWimpFlag);
#endif
	/* SetFlag(saw_unbuffered_flag); */
	/* SetFlag(skipfirstline); */
#undef SetFlag

	if (PyErr_Occurred()) {
		return NULL;
	}
	return seq;
}

PyObject *
_PySys_Init(void)
{
	PyObject *m, *v, *sysdict;
	char *s;

	m = Py_InitModule3("sys", sys_methods, sys_doc);
	if (m == NULL)
		return NULL;
	sysdict = PyModule_GetDict(m);
#define SET_SYS_FROM_STRING(key, value)			\
	v = value;					\
	if (v != NULL)					\
		PyDict_SetItemString(sysdict, key, v);	\
	Py_XDECREF(v)

	{
		/* XXX: does this work on Win/Win64? (see posix_fstat) */
		struct stat sb;
		if (fstat(fileno(stdin), &sb) == 0 &&
		    S_ISDIR(sb.st_mode)) {
			/* There's nothing more we can do. */
			/* Py_FatalError() will core dump, so just exit. */
			PySys_WriteStderr("Python error: <stdin> is a directory, cannot continue\n");
			exit(EXIT_FAILURE);
		}
	}

        /* stdin/stdout/stderr are now set by pythonrun.c */

	PyDict_SetItemString(sysdict, "__displayhook__",
                             PyDict_GetItemString(sysdict, "displayhook"));
	PyDict_SetItemString(sysdict, "__excepthook__",
                             PyDict_GetItemString(sysdict, "excepthook"));
	SET_SYS_FROM_STRING("version",
			     PyUnicode_FromString(Py_GetVersion()));
	SET_SYS_FROM_STRING("hexversion",
			     PyLong_FromLong(PY_VERSION_HEX));
	svnversion_init();
	SET_SYS_FROM_STRING("subversion",
			    Py_BuildValue("(UUU)", "CPython", branch,
					  svn_revision));
	SET_SYS_FROM_STRING("dont_write_bytecode",
			    PyBool_FromLong(Py_DontWriteBytecodeFlag));
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

	SET_SYS_FROM_STRING("version_info",
			    Py_BuildValue("iiiUi", PY_MAJOR_VERSION,
					       PY_MINOR_VERSION,
					       PY_MICRO_VERSION, s,
					       PY_RELEASE_SERIAL));
	SET_SYS_FROM_STRING("api_version",
			    PyLong_FromLong(PYTHON_API_VERSION));
	SET_SYS_FROM_STRING("copyright",
			    PyUnicode_FromString(Py_GetCopyright()));
	SET_SYS_FROM_STRING("platform",
			    PyUnicode_FromString(Py_GetPlatform()));
	SET_SYS_FROM_STRING("executable",
			    PyUnicode_DecodeFSDefault(
				Py_GetProgramFullPath()));
	SET_SYS_FROM_STRING("prefix",
			    PyUnicode_DecodeFSDefault(Py_GetPrefix()));
	SET_SYS_FROM_STRING("exec_prefix",
		   	    PyUnicode_DecodeFSDefault(Py_GetExecPrefix()));
	SET_SYS_FROM_STRING("maxsize",
			    PyLong_FromSsize_t(PY_SSIZE_T_MAX));
	SET_SYS_FROM_STRING("float_info",
			    PyFloat_GetInfo());
	SET_SYS_FROM_STRING("maxunicode",
			    PyLong_FromLong(PyUnicode_GetMax()));
	SET_SYS_FROM_STRING("builtin_module_names",
			    list_builtin_module_names());
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
		SET_SYS_FROM_STRING("byteorder",
				    PyUnicode_FromString(value));
	}
#ifdef MS_COREDLL
	SET_SYS_FROM_STRING("dllhandle",
			    PyLong_FromVoidPtr(PyWin_DLLhModule));
	SET_SYS_FROM_STRING("winver",
			    PyUnicode_FromString(PyWin_DLLVersionString));
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

	if (FlagsType.tp_name == 0)
		PyStructSequence_InitType(&FlagsType, &flags_desc);
	SET_SYS_FROM_STRING("flags", make_flags());
	/* prevent user from creating new instances */
	FlagsType.tp_init = NULL;
	FlagsType.tp_new = NULL;

#undef SET_SYS_FROM_STRING
	if (PyErr_Occurred())
		return NULL;
	return m;
}

static PyObject *
makepathobject(const char *path, int delim)
{
	int i, n;
	const char *p;
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
		w = PyUnicode_DecodeFSDefaultAndSize(path, (Py_ssize_t) (p - path));
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
PySys_SetPath(const char *path)
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
#ifdef __VMS
			PyObject *v;

			/* argv[0] is the script pathname if known */
			if (i == 0) {
				char* fn = decc$translate_vms(argv[0]);
				if ((fn == (char *)0) || fn == (char *)-1)
					v = PyUnicode_FromString(argv[0]);
				else
					v = PyUnicode_FromString(
						decc$translate_vms(argv[0]));
			} else
				v = PyUnicode_FromString(argv[i]);
#else
			PyObject *v = PyUnicode_FromString(argv[i]);
#endif
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
#if defined(HAVE_REALPATH)
	char fullpath[MAXPATHLEN];
#elif defined(MS_WINDOWS)
	char fullpath[MAX_PATH];
#endif
	PyObject *av = makeargvobject(argc, argv);
	PyObject *path = PySys_GetObject("path");
	if (av == NULL)
		Py_FatalError("no mem for sys.argv");
	if (PySys_SetObject("argv", av) != 0)
		Py_FatalError("can't assign sys.argv");
	if (path != NULL) {
		char *argv0 = argv[0];
		char *p = NULL;
		Py_ssize_t n = 0;
		PyObject *a;
#ifdef HAVE_READLINK
		char link[MAXPATHLEN+1];
		char argv0copy[2*MAXPATHLEN+1];
		int nr = 0;
		if (argc > 0 && argv0 != NULL && strcmp(argv0, "-c") != 0)
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
		if (argc > 0 && argv0 != NULL && strcmp(argv0, "-c") != 0) {
			char *q;
#ifdef MS_WINDOWS
			char *ptemp;
			if (GetFullPathName(argv0,
					   sizeof(fullpath),
					   fullpath,
					   &ptemp)) {
				argv0 = fullpath;
			}
#endif
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
		if (argc > 0 && argv0 != NULL && strcmp(argv0, "-c") != 0) {
#if defined(HAVE_REALPATH)
			if (realpath(argv0, fullpath)) {
				argv0 = fullpath;
			}
#endif
			p = strrchr(argv0, SEP);
		}
		if (p != NULL) {
			n = p + 1 - argv0;
#if SEP == '/' /* Special case for Unix filename syntax */
			if (n > 1)
				n--; /* Drop trailing separator */
#endif /* Unix */
		}
#endif /* All others */
		a = PyUnicode_FromStringAndSize(argv0, n);
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
	char buffer[1001];
	int written;

	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	file = PySys_GetObject(name);
	written = PyOS_vsnprintf(buffer, sizeof(buffer), format, va);
	if (PyFile_WriteString(buffer, file) != 0) {
		PyErr_Clear();
		fputs(buffer, fp);
	}
	if (written < 0 || (size_t)written >= sizeof(buffer)) {
		const char *truncated = "... truncated";
		if (PyFile_WriteString(truncated, file) != 0) {
			PyErr_Clear();
			fputs(truncated, fp);
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

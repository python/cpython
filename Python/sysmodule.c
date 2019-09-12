
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
#include "internal/pystate.h"
#include "code.h"
#include "frameobject.h"
#include "pythread.h"

#include "osdefs.h"
#include <locale.h>

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* MS_WINDOWS */

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
/* A string loaded from the DLL at startup: */
extern const char *PyWin_DLLVersionString;
#endif

/*[clinic input]
module sys
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3726b388feee8cea]*/

#include "clinic/sysmodule.c.h"

_Py_IDENTIFIER(_);
_Py_IDENTIFIER(__sizeof__);
_Py_IDENTIFIER(_xoptions);
_Py_IDENTIFIER(buffer);
_Py_IDENTIFIER(builtins);
_Py_IDENTIFIER(encoding);
_Py_IDENTIFIER(path);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);
_Py_IDENTIFIER(warnoptions);
_Py_IDENTIFIER(write);

PyObject *
_PySys_GetObjectId(_Py_Identifier *key)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *sd = tstate->interp->sysdict;
    if (sd == NULL)
        return NULL;
    return _PyDict_GetItemId(sd, key);
}

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
_PySys_SetObjectId(_Py_Identifier *key, PyObject *v)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *sd = tstate->interp->sysdict;
    if (v == NULL) {
        if (_PyDict_GetItemId(sd, key) == NULL)
            return 0;
        else
            return _PyDict_DelItemId(sd, key);
    }
    else
        return _PyDict_SetItemId(sd, key, v);
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
sys_breakpointhook(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *keywords)
{
    assert(!PyErr_Occurred());
    char *envar = Py_GETENV("PYTHONBREAKPOINT");

    if (envar == NULL || strlen(envar) == 0) {
        envar = "pdb.set_trace";
    }
    else if (!strcmp(envar, "0")) {
        /* The breakpoint is explicitly no-op'd. */
        Py_RETURN_NONE;
    }
    /* According to POSIX the string returned by getenv() might be invalidated
     * or the string content might be overwritten by a subsequent call to
     * getenv().  Since importing a module can performs the getenv() calls,
     * we need to save a copy of envar. */
    envar = _PyMem_RawStrdup(envar);
    if (envar == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    const char *last_dot = strrchr(envar, '.');
    const char *attrname = NULL;
    PyObject *modulepath = NULL;

    if (last_dot == NULL) {
        /* The breakpoint is a built-in, e.g. PYTHONBREAKPOINT=int */
        modulepath = PyUnicode_FromString("builtins");
        attrname = envar;
    }
    else if (last_dot != envar) {
        /* Split on the last dot; */
        modulepath = PyUnicode_FromStringAndSize(envar, last_dot - envar);
        attrname = last_dot + 1;
    }
    else {
        goto warn;
    }
    if (modulepath == NULL) {
        PyMem_RawFree(envar);
        return NULL;
    }

    PyObject *fromlist = Py_BuildValue("(s)", attrname);
    if (fromlist == NULL) {
        Py_DECREF(modulepath);
        PyMem_RawFree(envar);
        return NULL;
    }
    PyObject *module = PyImport_ImportModuleLevelObject(
        modulepath, NULL, NULL, fromlist, 0);
    Py_DECREF(modulepath);
    Py_DECREF(fromlist);

    if (module == NULL) {
        if (PyErr_ExceptionMatches(PyExc_ImportError)) {
            goto warn;
        }
        PyMem_RawFree(envar);
        return NULL;
    }

    PyObject *hook = PyObject_GetAttrString(module, attrname);
    Py_DECREF(module);

    if (hook == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            goto warn;
        }
        PyMem_RawFree(envar);
        return NULL;
    }
    PyMem_RawFree(envar);
    PyObject *retval = _PyObject_FastCallKeywords(hook, args, nargs, keywords);
    Py_DECREF(hook);
    return retval;

  warn:
    /* If any of the imports went wrong, then warn and ignore. */
    PyErr_Clear();
    int status = PyErr_WarnFormat(
        PyExc_RuntimeWarning, 0,
        "Ignoring unimportable $PYTHONBREAKPOINT: \"%s\"", envar);
    PyMem_RawFree(envar);
    if (status < 0) {
        /* Printing the warning raised an exception. */
        return NULL;
    }
    /* The warning was (probably) issued. */
    Py_RETURN_NONE;
}

PyDoc_STRVAR(breakpointhook_doc,
"breakpointhook(*args, **kws)\n"
"\n"
"This hook function is called by built-in breakpoint().\n"
);

/* Write repr(o) to sys.stdout using sys.stdout.encoding and 'backslashreplace'
   error handler. If sys.stdout has a buffer attribute, use
   sys.stdout.buffer.write(encoded), otherwise redecode the string and use
   sys.stdout.write(redecoded).

   Helper function for sys_displayhook(). */
static int
sys_displayhook_unencodable(PyObject *outf, PyObject *o)
{
    PyObject *stdout_encoding = NULL;
    PyObject *encoded, *escaped_str, *repr_str, *buffer, *result;
    const char *stdout_encoding_str;
    int ret;

    stdout_encoding = _PyObject_GetAttrId(outf, &PyId_encoding);
    if (stdout_encoding == NULL)
        goto error;
    stdout_encoding_str = PyUnicode_AsUTF8(stdout_encoding);
    if (stdout_encoding_str == NULL)
        goto error;

    repr_str = PyObject_Repr(o);
    if (repr_str == NULL)
        goto error;
    encoded = PyUnicode_AsEncodedString(repr_str,
                                        stdout_encoding_str,
                                        "backslashreplace");
    Py_DECREF(repr_str);
    if (encoded == NULL)
        goto error;

    buffer = _PyObject_GetAttrId(outf, &PyId_buffer);
    if (buffer) {
        result = _PyObject_CallMethodIdObjArgs(buffer, &PyId_write, encoded, NULL);
        Py_DECREF(buffer);
        Py_DECREF(encoded);
        if (result == NULL)
            goto error;
        Py_DECREF(result);
    }
    else {
        PyErr_Clear();
        escaped_str = PyUnicode_FromEncodedObject(encoded,
                                                  stdout_encoding_str,
                                                  "strict");
        Py_DECREF(encoded);
        if (PyFile_WriteObject(escaped_str, outf, Py_PRINT_RAW) != 0) {
            Py_DECREF(escaped_str);
            goto error;
        }
        Py_DECREF(escaped_str);
    }
    ret = 0;
    goto finally;

error:
    ret = -1;
finally:
    Py_XDECREF(stdout_encoding);
    return ret;
}

static PyObject *
sys_displayhook(PyObject *self, PyObject *o)
{
    PyObject *outf;
    PyObject *builtins;
    static PyObject *newline = NULL;
    int err;

    builtins = _PyImport_GetModuleId(&PyId_builtins);
    if (builtins == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, "lost builtins module");
        }
        return NULL;
    }
    Py_DECREF(builtins);

    /* Print value except if None */
    /* After printing, also assign to '_' */
    /* Before, set '_' to None to avoid recursion */
    if (o == Py_None) {
        Py_RETURN_NONE;
    }
    if (_PyObject_SetAttrId(builtins, &PyId__, Py_None) != 0)
        return NULL;
    outf = _PySys_GetObjectId(&PyId_stdout);
    if (outf == NULL || outf == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
        return NULL;
    }
    if (PyFile_WriteObject(o, outf, 0) != 0) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
            /* repr(o) is not encodable to sys.stdout.encoding with
             * sys.stdout.errors error handler (which is probably 'strict') */
            PyErr_Clear();
            err = sys_displayhook_unencodable(outf, o);
            if (err)
                return NULL;
        }
        else {
            return NULL;
        }
    }
    if (newline == NULL) {
        newline = PyUnicode_FromString("\n");
        if (newline == NULL)
            return NULL;
    }
    if (PyFile_WriteObject(newline, outf, Py_PRINT_RAW) != 0)
        return NULL;
    if (_PyObject_SetAttrId(builtins, &PyId__, o) != 0)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(displayhook_doc,
"displayhook(object) -> None\n"
"\n"
"Print an object to sys.stdout and also save it in builtins._\n"
);

static PyObject *
sys_excepthook(PyObject* self, PyObject* args)
{
    PyObject *exc, *value, *tb;
    if (!PyArg_UnpackTuple(args, "excepthook", 3, 3, &exc, &value, &tb))
        return NULL;
    PyErr_Display(exc, value, tb);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(excepthook_doc,
"excepthook(exctype, value, traceback) -> None\n"
"\n"
"Handle an exception by displaying it with a traceback on sys.stderr.\n"
);

static PyObject *
sys_exc_info(PyObject *self, PyObject *noargs)
{
    _PyErr_StackItem *err_info = _PyErr_GetTopmostException(PyThreadState_GET());
    return Py_BuildValue(
        "(OOO)",
        err_info->exc_type != NULL ? err_info->exc_type : Py_None,
        err_info->exc_value != NULL ? err_info->exc_value : Py_None,
        err_info->exc_traceback != NULL ?
            err_info->exc_traceback : Py_None);
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
If the status is an integer, it will be used as the system exit status.\n\
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
sys_getfilesystemencoding(PyObject *self)
{
    if (Py_FileSystemDefaultEncoding)
        return PyUnicode_FromString(Py_FileSystemDefaultEncoding);
    PyErr_SetString(PyExc_RuntimeError,
                    "filesystem encoding is not initialized");
    return NULL;
}

PyDoc_STRVAR(getfilesystemencoding_doc,
"getfilesystemencoding() -> string\n\
\n\
Return the encoding used to convert Unicode filenames in\n\
operating system filenames."
);

static PyObject *
sys_getfilesystemencodeerrors(PyObject *self)
{
    if (Py_FileSystemDefaultEncodeErrors)
        return PyUnicode_FromString(Py_FileSystemDefaultEncodeErrors);
    PyErr_SetString(PyExc_RuntimeError,
        "filesystem encoding is not initialized");
    return NULL;
}

PyDoc_STRVAR(getfilesystemencodeerrors_doc,
    "getfilesystemencodeerrors() -> string\n\
\n\
Return the error mode used to convert Unicode filenames in\n\
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
static PyObject *whatstrings[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int
trace_init(void)
{
    static const char * const whatnames[8] = {
        "call", "exception", "line", "return",
        "c_call", "c_exception", "c_return",
        "opcode"
    };
    PyObject *name;
    int i;
    for (i = 0; i < 8; ++i) {
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
call_trampoline(PyObject* callback,
                PyFrameObject *frame, int what, PyObject *arg)
{
    PyObject *result;
    PyObject *stack[3];

    if (PyFrame_FastToLocalsWithError(frame) < 0) {
        return NULL;
    }

    stack[0] = (PyObject *)frame;
    stack[1] = whatstrings[what];
    stack[2] = (arg != NULL) ? arg : Py_None;

    /* call the Python-level function */
    result = _PyObject_FastCall(callback, stack, 3);

    PyFrame_LocalsToFast(frame, 1);
    if (result == NULL) {
        PyTraceBack_Here(frame);
    }

    return result;
}

static int
profile_trampoline(PyObject *self, PyFrameObject *frame,
                   int what, PyObject *arg)
{
    PyObject *result;

    if (arg == NULL)
        arg = Py_None;
    result = call_trampoline(self, frame, what, arg);
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
    PyObject *callback;
    PyObject *result;

    if (what == PyTrace_CALL)
        callback = self;
    else
        callback = frame->f_trace;
    if (callback == NULL)
        return 0;
    result = call_trampoline(callback, frame, what, arg);
    if (result == NULL) {
        PyEval_SetTrace(NULL, NULL);
        Py_CLEAR(frame->f_trace);
        return -1;
    }
    if (result != Py_None) {
        Py_XSETREF(frame->f_trace, result);
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
    Py_RETURN_NONE;
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
    Py_RETURN_NONE;
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
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "sys.getcheckinterval() and sys.setcheckinterval() "
                     "are deprecated.  Use sys.setswitchinterval() "
                     "instead.", 1) < 0)
        return NULL;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    if (!PyArg_ParseTuple(args, "i:setcheckinterval", &interp->check_interval))
        return NULL;
    Py_RETURN_NONE;
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
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "sys.getcheckinterval() and sys.setcheckinterval() "
                     "are deprecated.  Use sys.getswitchinterval() "
                     "instead.", 1) < 0)
        return NULL;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    return PyLong_FromLong(interp->check_interval);
}

PyDoc_STRVAR(getcheckinterval_doc,
"getcheckinterval() -> current check interval; see setcheckinterval()."
);

static PyObject *
sys_setswitchinterval(PyObject *self, PyObject *args)
{
    double d;
    if (!PyArg_ParseTuple(args, "d:setswitchinterval", &d))
        return NULL;
    if (d <= 0.0) {
        PyErr_SetString(PyExc_ValueError,
                        "switch interval must be strictly positive");
        return NULL;
    }
    _PyEval_SetSwitchInterval((unsigned long) (1e6 * d));
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setswitchinterval_doc,
"setswitchinterval(n)\n\
\n\
Set the ideal thread switching delay inside the Python interpreter\n\
The actual frequency of switching threads can be lower if the\n\
interpreter executes long sequences of uninterruptible code\n\
(this is implementation-specific and workload-dependent).\n\
\n\
The parameter must represent the desired switching delay in seconds\n\
A typical value is 0.005 (5 milliseconds)."
);

static PyObject *
sys_getswitchinterval(PyObject *self, PyObject *args)
{
    return PyFloat_FromDouble(1e-6 * _PyEval_GetSwitchInterval());
}

PyDoc_STRVAR(getswitchinterval_doc,
"getswitchinterval() -> current thread switch interval; see setswitchinterval()."
);

static PyObject *
sys_setrecursionlimit(PyObject *self, PyObject *args)
{
    int new_limit, mark;
    PyThreadState *tstate;

    if (!PyArg_ParseTuple(args, "i:setrecursionlimit", &new_limit))
        return NULL;

    if (new_limit < 1) {
        PyErr_SetString(PyExc_ValueError,
                        "recursion limit must be greater or equal than 1");
        return NULL;
    }

    /* Issue #25274: When the recursion depth hits the recursion limit in
       _Py_CheckRecursiveCall(), the overflowed flag of the thread state is
       set to 1 and a RecursionError is raised. The overflowed flag is reset
       to 0 when the recursion depth goes below the low-water mark: see
       Py_LeaveRecursiveCall().

       Reject too low new limit if the current recursion depth is higher than
       the new low-water mark. Otherwise it may not be possible anymore to
       reset the overflowed flag to 0. */
    mark = _Py_RecursionLimitLowerWaterMark(new_limit);
    tstate = PyThreadState_GET();
    if (tstate->recursion_depth >= mark) {
        PyErr_Format(PyExc_RecursionError,
                     "cannot set the recursion limit to %i at "
                     "the recursion depth %i: the limit is too low",
                     new_limit, tstate->recursion_depth);
        return NULL;
    }

    Py_SetRecursionLimit(new_limit);
    Py_RETURN_NONE;
}

/*[clinic input]
sys.set_coroutine_origin_tracking_depth

  depth: int

Enable or disable origin tracking for coroutine objects in this thread.

Coroutine objects will track 'depth' frames of traceback information about
where they came from, available in their cr_origin attribute. Set depth of 0
to disable.
[clinic start generated code]*/

static PyObject *
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth)
/*[clinic end generated code: output=0a2123c1cc6759c5 input=9083112cccc1bdcb]*/
{
    if (depth < 0) {
        PyErr_SetString(PyExc_ValueError, "depth must be >= 0");
        return NULL;
    }
    _PyEval_SetCoroutineOriginTrackingDepth(depth);
    Py_RETURN_NONE;
}

/*[clinic input]
sys.get_coroutine_origin_tracking_depth -> int

Check status of origin tracking for coroutine objects in this thread.
[clinic start generated code]*/

static int
sys_get_coroutine_origin_tracking_depth_impl(PyObject *module)
/*[clinic end generated code: output=3699f7be95a3afb8 input=335266a71205b61a]*/
{
    return _PyEval_GetCoroutineOriginTrackingDepth();
}

static PyObject *
sys_set_coroutine_wrapper(PyObject *self, PyObject *wrapper)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "set_coroutine_wrapper is deprecated", 1) < 0) {
        return NULL;
    }

    if (wrapper != Py_None) {
        if (!PyCallable_Check(wrapper)) {
            PyErr_Format(PyExc_TypeError,
                         "callable expected, got %.50s",
                         Py_TYPE(wrapper)->tp_name);
            return NULL;
        }
        _PyEval_SetCoroutineWrapper(wrapper);
    }
    else {
        _PyEval_SetCoroutineWrapper(NULL);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(set_coroutine_wrapper_doc,
"set_coroutine_wrapper(wrapper)\n\
\n\
Set a wrapper for coroutine objects."
);

static PyObject *
sys_get_coroutine_wrapper(PyObject *self, PyObject *args)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "get_coroutine_wrapper is deprecated", 1) < 0) {
        return NULL;
    }
    PyObject *wrapper = _PyEval_GetCoroutineWrapper();
    if (wrapper == NULL) {
        wrapper = Py_None;
    }
    Py_INCREF(wrapper);
    return wrapper;
}

PyDoc_STRVAR(get_coroutine_wrapper_doc,
"get_coroutine_wrapper()\n\
\n\
Return the wrapper for coroutine objects set by sys.set_coroutine_wrapper."
);


static PyTypeObject AsyncGenHooksType;

PyDoc_STRVAR(asyncgen_hooks_doc,
"asyncgen_hooks\n\
\n\
A named tuple providing information about asynchronous\n\
generators hooks.  The attributes are read only.");

static PyStructSequence_Field asyncgen_hooks_fields[] = {
    {"firstiter", "Hook to intercept first iteration"},
    {"finalizer", "Hook to intercept finalization"},
    {0}
};

static PyStructSequence_Desc asyncgen_hooks_desc = {
    "asyncgen_hooks",          /* name */
    asyncgen_hooks_doc,        /* doc */
    asyncgen_hooks_fields ,    /* fields */
    2
};


static PyObject *
sys_set_asyncgen_hooks(PyObject *self, PyObject *args, PyObject *kw)
{
    static char *keywords[] = {"firstiter", "finalizer", NULL};
    PyObject *firstiter = NULL;
    PyObject *finalizer = NULL;

    if (!PyArg_ParseTupleAndKeywords(
            args, kw, "|OO", keywords,
            &firstiter, &finalizer)) {
        return NULL;
    }

    if (finalizer && finalizer != Py_None) {
        if (!PyCallable_Check(finalizer)) {
            PyErr_Format(PyExc_TypeError,
                         "callable finalizer expected, got %.50s",
                         Py_TYPE(finalizer)->tp_name);
            return NULL;
        }
        _PyEval_SetAsyncGenFinalizer(finalizer);
    }
    else if (finalizer == Py_None) {
        _PyEval_SetAsyncGenFinalizer(NULL);
    }

    if (firstiter && firstiter != Py_None) {
        if (!PyCallable_Check(firstiter)) {
            PyErr_Format(PyExc_TypeError,
                         "callable firstiter expected, got %.50s",
                         Py_TYPE(firstiter)->tp_name);
            return NULL;
        }
        _PyEval_SetAsyncGenFirstiter(firstiter);
    }
    else if (firstiter == Py_None) {
        _PyEval_SetAsyncGenFirstiter(NULL);
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(set_asyncgen_hooks_doc,
"set_asyncgen_hooks(*, firstiter=None, finalizer=None)\n\
\n\
Set a finalizer for async generators objects."
);

static PyObject *
sys_get_asyncgen_hooks(PyObject *self, PyObject *args)
{
    PyObject *res;
    PyObject *firstiter = _PyEval_GetAsyncGenFirstiter();
    PyObject *finalizer = _PyEval_GetAsyncGenFinalizer();

    res = PyStructSequence_New(&AsyncGenHooksType);
    if (res == NULL) {
        return NULL;
    }

    if (firstiter == NULL) {
        firstiter = Py_None;
    }

    if (finalizer == NULL) {
        finalizer = Py_None;
    }

    Py_INCREF(firstiter);
    PyStructSequence_SET_ITEM(res, 0, firstiter);

    Py_INCREF(finalizer);
    PyStructSequence_SET_ITEM(res, 1, finalizer);

    return res;
}

PyDoc_STRVAR(get_asyncgen_hooks_doc,
"get_asyncgen_hooks()\n\
\n\
Return a namedtuple of installed asynchronous generators hooks \
(firstiter, finalizer)."
);


static PyTypeObject Hash_InfoType;

PyDoc_STRVAR(hash_info_doc,
"hash_info\n\
\n\
A named tuple providing parameters used for computing\n\
hashes. The attributes are read only.");

static PyStructSequence_Field hash_info_fields[] = {
    {"width", "width of the type used for hashing, in bits"},
    {"modulus", "prime number giving the modulus on which the hash "
                "function is based"},
    {"inf", "value to be used for hash of a positive infinity"},
    {"nan", "value to be used for hash of a nan"},
    {"imag", "multiplier used for the imaginary part of a complex number"},
    {"algorithm", "name of the algorithm for hashing of str, bytes and "
                  "memoryviews"},
    {"hash_bits", "internal output size of hash algorithm"},
    {"seed_bits", "seed size of hash algorithm"},
    {"cutoff", "small string optimization cutoff"},
    {NULL, NULL}
};

static PyStructSequence_Desc hash_info_desc = {
    "sys.hash_info",
    hash_info_doc,
    hash_info_fields,
    9,
};

static PyObject *
get_hash_info(void)
{
    PyObject *hash_info;
    int field = 0;
    PyHash_FuncDef *hashfunc;
    hash_info = PyStructSequence_New(&Hash_InfoType);
    if (hash_info == NULL)
        return NULL;
    hashfunc = PyHash_GetFuncDef();
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(8*sizeof(Py_hash_t)));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromSsize_t(_PyHASH_MODULUS));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(_PyHASH_INF));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(_PyHASH_NAN));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(_PyHASH_IMAG));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyUnicode_FromString(hashfunc->name));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(hashfunc->hash_bits));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(hashfunc->seed_bits));
    PyStructSequence_SET_ITEM(hash_info, field++,
                              PyLong_FromLong(Py_HASH_CUTOFF));
    if (PyErr_Occurred()) {
        Py_CLEAR(hash_info);
        return NULL;
    }
    return hash_info;
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
Return information about the running version of Windows as a named tuple.\n\
The members are named: major, minor, build, platform, service_pack,\n\
service_pack_major, service_pack_minor, suite_mask, and product_type. For\n\
backward compatibility, only the first 5 items are available by indexing.\n\
All elements are numbers, except service_pack and platform_type which are\n\
strings, and platform_version which is a 3-tuple. Platform is always 2.\n\
Product_type may be 1 for a workstation, 2 for a domain controller, 3 for a\n\
server. Platform_version is a 3-tuple containing a version number that is\n\
intended for identifying the OS rather than feature detection."
);

static PyTypeObject WindowsVersionType = {0, 0, 0, 0, 0, 0};

static PyStructSequence_Field windows_version_fields[] = {
    {"major", "Major version number"},
    {"minor", "Minor version number"},
    {"build", "Build number"},
    {"platform", "Operating system platform"},
    {"service_pack", "Latest Service Pack installed on the system"},
    {"service_pack_major", "Service Pack major version number"},
    {"service_pack_minor", "Service Pack minor version number"},
    {"suite_mask", "Bit mask identifying available product suites"},
    {"product_type", "System product type"},
    {"platform_version", "Diagnostic version number"},
    {0}
};

static PyStructSequence_Desc windows_version_desc = {
    "sys.getwindowsversion",  /* name */
    getwindowsversion_doc,    /* doc */
    windows_version_fields,   /* fields */
    5                         /* For backward compatibility,
                                 only the first 5 items are accessible
                                 via indexing, the rest are name only */
};

/* Disable deprecation warnings about GetVersionEx as the result is
   being passed straight through to the caller, who is responsible for
   using it correctly. */
#pragma warning(push)
#pragma warning(disable:4996)

static PyObject *
sys_getwindowsversion(PyObject *self)
{
    PyObject *version;
    int pos = 0;
    OSVERSIONINFOEX ver;
    DWORD realMajor, realMinor, realBuild;
    HANDLE hKernel32;
    wchar_t kernel32_path[MAX_PATH];
    LPVOID verblock;
    DWORD verblock_size;

    ver.dwOSVersionInfoSize = sizeof(ver);
    if (!GetVersionEx((OSVERSIONINFO*) &ver))
        return PyErr_SetFromWindowsErr(0);

    version = PyStructSequence_New(&WindowsVersionType);
    if (version == NULL)
        return NULL;

    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.dwMajorVersion));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.dwMinorVersion));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.dwBuildNumber));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.dwPlatformId));
    PyStructSequence_SET_ITEM(version, pos++, PyUnicode_FromString(ver.szCSDVersion));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.wServicePackMajor));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.wServicePackMinor));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.wSuiteMask));
    PyStructSequence_SET_ITEM(version, pos++, PyLong_FromLong(ver.wProductType));

    realMajor = ver.dwMajorVersion;
    realMinor = ver.dwMinorVersion;
    realBuild = ver.dwBuildNumber;

    // GetVersion will lie if we are running in a compatibility mode.
    // We need to read the version info from a system file resource
    // to accurately identify the OS version. If we fail for any reason,
    // just return whatever GetVersion said.
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32 && GetModuleFileNameW(hKernel32, kernel32_path, MAX_PATH) &&
        (verblock_size = GetFileVersionInfoSizeW(kernel32_path, NULL)) &&
        (verblock = PyMem_RawMalloc(verblock_size))) {
        VS_FIXEDFILEINFO *ffi;
        UINT ffi_len;

        if (GetFileVersionInfoW(kernel32_path, 0, verblock_size, verblock) &&
            VerQueryValueW(verblock, L"", (LPVOID)&ffi, &ffi_len)) {
            realMajor = HIWORD(ffi->dwProductVersionMS);
            realMinor = LOWORD(ffi->dwProductVersionMS);
            realBuild = HIWORD(ffi->dwProductVersionLS);
        }
        PyMem_RawFree(verblock);
    }
    PyStructSequence_SET_ITEM(version, pos++, Py_BuildValue("(kkk)",
        realMajor,
        realMinor,
        realBuild
    ));

    if (PyErr_Occurred()) {
        Py_DECREF(version);
        return NULL;
    }

    return version;
}

#pragma warning(pop)

PyDoc_STRVAR(enablelegacywindowsfsencoding_doc,
"_enablelegacywindowsfsencoding()\n\
\n\
Changes the default filesystem encoding to mbcs:replace for consistency\n\
with earlier versions of Python. See PEP 529 for more information.\n\
\n\
This is equivalent to defining the PYTHONLEGACYWINDOWSFSENCODING \n\
environment variable before launching Python."
);

static PyObject *
sys_enablelegacywindowsfsencoding(PyObject *self)
{
    Py_FileSystemDefaultEncoding = "mbcs";
    Py_FileSystemDefaultEncodeErrors = "replace";
    Py_RETURN_NONE;
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
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setdlopenflags_doc,
"setdlopenflags(n) -> None\n\
\n\
Set the flags used by the interpreter for dlopen calls, such as when the\n\
interpreter loads extension modules.  Among other things, this will enable\n\
a lazy resolving of symbols when importing a module, if called as\n\
sys.setdlopenflags(0).  To share symbols across extension modules, call as\n\
sys.setdlopenflags(os.RTLD_GLOBAL).  Symbolic names for the flag modules\n\
can be found in the os module (RTLD_xxx constants, e.g. os.RTLD_LAZY).");

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
Return the current value of the flags that are used for dlopen calls.\n\
The flag constants are defined in the os module.");

#endif  /* HAVE_DLOPEN */

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
    Py_RETURN_NONE;
}
#endif /* USE_MALLOPT */

size_t
_PySys_GetSizeOf(PyObject *o)
{
    PyObject *res = NULL;
    PyObject *method;
    Py_ssize_t size;

    /* Make sure the type is initialized. float gets initialized late */
    if (PyType_Ready(Py_TYPE(o)) < 0)
        return (size_t)-1;

    method = _PyObject_LookupSpecial(o, &PyId___sizeof__);
    if (method == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_TypeError,
                         "Type %.100s doesn't define __sizeof__",
                         Py_TYPE(o)->tp_name);
    }
    else {
        res = _PyObject_CallNoArg(method);
        Py_DECREF(method);
    }

    if (res == NULL)
        return (size_t)-1;

    size = PyLong_AsSsize_t(res);
    Py_DECREF(res);
    if (size == -1 && PyErr_Occurred())
        return (size_t)-1;

    if (size < 0) {
        PyErr_SetString(PyExc_ValueError, "__sizeof__() should return >= 0");
        return (size_t)-1;
    }

    /* add gc_head size */
    if (PyObject_IS_GC(o))
        return ((size_t)size) + sizeof(PyGC_Head);
    return (size_t)size;
}

static PyObject *
sys_getsizeof(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"object", "default", 0};
    size_t size;
    PyObject *o, *dflt = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:getsizeof",
                                     kwlist, &o, &dflt))
        return NULL;

    size = _PySys_GetSizeOf(o);

    if (size == (size_t)-1 && PyErr_Occurred()) {
        /* Has a default value been given */
        if (dflt != NULL && PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            Py_INCREF(dflt);
            return dflt;
        }
        else
            return NULL;
    }

    return PyLong_FromSize_t(size);
}

PyDoc_STRVAR(getsizeof_doc,
"getsizeof(object, default) -> int\n\
\n\
Return the size of object in bytes.");

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

static PyObject *
sys_getallocatedblocks(PyObject *self)
{
    return PyLong_FromSsize_t(_Py_GetAllocatedBlocks());
}

PyDoc_STRVAR(getallocatedblocks_doc,
"getallocatedblocks() -> integer\n\
\n\
Return the number of memory blocks currently allocated, regardless of their\n\
size."
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
    if (!PyArg_ParseTuple(args, "OO!:call_tracing", &func, &PyTuple_Type, &funcargs))
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

static PyObject *
sys_callstats(PyObject *self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                      "sys.callstats() has been deprecated in Python 3.7 "
                      "and will be removed in the future", 1) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


#ifdef __cplusplus
extern "C" {
#endif

static PyObject *
sys_debugmallocstats(PyObject *self, PyObject *args)
{
#ifdef WITH_PYMALLOC
    if (_PyObject_DebugMallocStats(stderr)) {
        fputc('\n', stderr);
    }
#endif
    _PyObject_DebugTypeStats(stderr);

    Py_RETURN_NONE;
}
PyDoc_STRVAR(debugmallocstats_doc,
"_debugmallocstats()\n\
\n\
Print summary info to stderr about the state of\n\
pymalloc's structures.\n\
\n\
In Py_DEBUG mode, also perform some expensive internal consistency\n\
checks.\n\
");

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
sys_is_finalizing(PyObject* self, PyObject* args)
{
    return PyBool_FromLong(_Py_IsFinalizing());
}

PyDoc_STRVAR(is_finalizing_doc,
"is_finalizing()\n\
Return True if Python is exiting.");


#ifdef ANDROID_API_LEVEL
PyDoc_STRVAR(getandroidapilevel_doc,
"getandroidapilevel()\n\
\n\
Return the build time API version of Android as an integer.");

static PyObject *
sys_getandroidapilevel(PyObject *self)
{
    return PyLong_FromLong(ANDROID_API_LEVEL);
}
#endif   /* ANDROID_API_LEVEL */


static PyMethodDef sys_methods[] = {
    /* Might as well keep this in alphabetic order */
    {"breakpointhook",  (PyCFunction)sys_breakpointhook,
     METH_FASTCALL | METH_KEYWORDS, breakpointhook_doc},
    {"callstats", (PyCFunction)sys_callstats, METH_NOARGS,
     callstats_doc},
    {"_clear_type_cache",       sys_clear_type_cache,     METH_NOARGS,
     sys_clear_type_cache__doc__},
    {"_current_frames", sys_current_frames, METH_NOARGS,
     current_frames_doc},
    {"displayhook",     sys_displayhook, METH_O, displayhook_doc},
    {"exc_info",        sys_exc_info, METH_NOARGS, exc_info_doc},
    {"excepthook",      sys_excepthook, METH_VARARGS, excepthook_doc},
    {"exit",            sys_exit, METH_VARARGS, exit_doc},
    {"getdefaultencoding", (PyCFunction)sys_getdefaultencoding,
     METH_NOARGS, getdefaultencoding_doc},
#ifdef HAVE_DLOPEN
    {"getdlopenflags", (PyCFunction)sys_getdlopenflags, METH_NOARGS,
     getdlopenflags_doc},
#endif
    {"getallocatedblocks", (PyCFunction)sys_getallocatedblocks, METH_NOARGS,
      getallocatedblocks_doc},
#ifdef COUNT_ALLOCS
    {"getcounts",       (PyCFunction)sys_getcounts, METH_NOARGS},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
    {"getdxp",          _Py_GetDXProfile, METH_VARARGS},
#endif
    {"getfilesystemencoding", (PyCFunction)sys_getfilesystemencoding,
     METH_NOARGS, getfilesystemencoding_doc},
    { "getfilesystemencodeerrors", (PyCFunction)sys_getfilesystemencodeerrors,
     METH_NOARGS, getfilesystemencodeerrors_doc },
#ifdef Py_TRACE_REFS
    {"getobjects",      _Py_GetObjects, METH_VARARGS},
#endif
#ifdef Py_REF_DEBUG
    {"gettotalrefcount", (PyCFunction)sys_gettotalrefcount, METH_NOARGS},
#endif
    {"getrefcount",     (PyCFunction)sys_getrefcount, METH_O, getrefcount_doc},
    {"getrecursionlimit", (PyCFunction)sys_getrecursionlimit, METH_NOARGS,
     getrecursionlimit_doc},
    {"getsizeof",   (PyCFunction)sys_getsizeof,
     METH_VARARGS | METH_KEYWORDS, getsizeof_doc},
    {"_getframe", sys_getframe, METH_VARARGS, getframe_doc},
#ifdef MS_WINDOWS
    {"getwindowsversion", (PyCFunction)sys_getwindowsversion, METH_NOARGS,
     getwindowsversion_doc},
    {"_enablelegacywindowsfsencoding", (PyCFunction)sys_enablelegacywindowsfsencoding,
     METH_NOARGS, enablelegacywindowsfsencoding_doc },
#endif /* MS_WINDOWS */
    {"intern",          sys_intern,     METH_VARARGS, intern_doc},
    {"is_finalizing",   sys_is_finalizing, METH_NOARGS, is_finalizing_doc},
#ifdef USE_MALLOPT
    {"mdebug",          sys_mdebug, METH_VARARGS},
#endif
    {"setcheckinterval",        sys_setcheckinterval, METH_VARARGS,
     setcheckinterval_doc},
    {"getcheckinterval",        sys_getcheckinterval, METH_NOARGS,
     getcheckinterval_doc},
    {"setswitchinterval",       sys_setswitchinterval, METH_VARARGS,
     setswitchinterval_doc},
    {"getswitchinterval",       sys_getswitchinterval, METH_NOARGS,
     getswitchinterval_doc},
#ifdef HAVE_DLOPEN
    {"setdlopenflags", sys_setdlopenflags, METH_VARARGS,
     setdlopenflags_doc},
#endif
    {"setprofile",      sys_setprofile, METH_O, setprofile_doc},
    {"getprofile",      sys_getprofile, METH_NOARGS, getprofile_doc},
    {"setrecursionlimit", sys_setrecursionlimit, METH_VARARGS,
     setrecursionlimit_doc},
    {"settrace",        sys_settrace, METH_O, settrace_doc},
    {"gettrace",        sys_gettrace, METH_NOARGS, gettrace_doc},
    {"call_tracing", sys_call_tracing, METH_VARARGS, call_tracing_doc},
    {"_debugmallocstats", sys_debugmallocstats, METH_NOARGS,
     debugmallocstats_doc},
    SYS_SET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF
    SYS_GET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF
    {"set_coroutine_wrapper", sys_set_coroutine_wrapper, METH_O,
     set_coroutine_wrapper_doc},
    {"get_coroutine_wrapper", sys_get_coroutine_wrapper, METH_NOARGS,
     get_coroutine_wrapper_doc},
    {"set_asyncgen_hooks", (PyCFunction)sys_set_asyncgen_hooks,
     METH_VARARGS | METH_KEYWORDS, set_asyncgen_hooks_doc},
    {"get_asyncgen_hooks", sys_get_asyncgen_hooks, METH_NOARGS,
     get_asyncgen_hooks_doc},
#ifdef ANDROID_API_LEVEL
    {"getandroidapilevel", (PyCFunction)sys_getandroidapilevel, METH_NOARGS,
     getandroidapilevel_doc},
#endif
    {NULL,              NULL}           /* sentinel */
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

/* Pre-initialization support for sys.warnoptions and sys._xoptions
 *
 * Modern internal code paths:
 *   These APIs get called after _Py_InitializeCore and get to use the
 *   regular CPython list, dict, and unicode APIs.
 *
 * Legacy embedding code paths:
 *   The multi-phase initialization API isn't public yet, so embedding
 *   apps still need to be able configure sys.warnoptions and sys._xoptions
 *   before they call Py_Initialize. To support this, we stash copies of
 *   the supplied wchar * sequences in linked lists, and then migrate the
 *   contents of those lists to the sys module in _PyInitializeCore.
 *
 */

struct _preinit_entry {
    wchar_t *value;
    struct _preinit_entry *next;
};

typedef struct _preinit_entry *_Py_PreInitEntry;

static _Py_PreInitEntry _preinit_warnoptions = NULL;
static _Py_PreInitEntry _preinit_xoptions = NULL;

static _Py_PreInitEntry
_alloc_preinit_entry(const wchar_t *value)
{
    /* To get this to work, we have to initialize the runtime implicitly */
    _PyRuntime_Initialize();

    /* Force default allocator, so we can ensure that it also gets used to
     * destroy the linked list in _clear_preinit_entries.
     */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    _Py_PreInitEntry node = PyMem_RawCalloc(1, sizeof(*node));
    if (node != NULL) {
        node->value = _PyMem_RawWcsdup(value);
        if (node->value == NULL) {
            PyMem_RawFree(node);
            node = NULL;
        };
    };

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return node;
};

static int
_append_preinit_entry(_Py_PreInitEntry *optionlist, const wchar_t *value)
{
    _Py_PreInitEntry new_entry = _alloc_preinit_entry(value);
    if (new_entry == NULL) {
        return -1;
    }
    /* We maintain the linked list in this order so it's easy to play back
     * the add commands in the same order later on in _Py_InitializeCore
     */
    _Py_PreInitEntry last_entry = *optionlist;
    if (last_entry == NULL) {
        *optionlist = new_entry;
    } else {
        while (last_entry->next != NULL) {
            last_entry = last_entry->next;
        }
        last_entry->next = new_entry;
    }
    return 0;
};

static void
_clear_preinit_entries(_Py_PreInitEntry *optionlist)
{
    _Py_PreInitEntry current = *optionlist;
    *optionlist = NULL;
    /* Deallocate the nodes and their contents using the default allocator */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    while (current != NULL) {
        _Py_PreInitEntry next = current->next;
        PyMem_RawFree(current->value);
        PyMem_RawFree(current);
        current = next;
    }
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
};

static void
_clear_all_preinit_options(void)
{
    _clear_preinit_entries(&_preinit_warnoptions);
    _clear_preinit_entries(&_preinit_xoptions);
}

static int
_PySys_ReadPreInitOptions(void)
{
    /* Rerun the add commands with the actual sys module available */
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL) {
        /* Still don't have a thread state, so something is wrong! */
        return -1;
    }
    _Py_PreInitEntry entry = _preinit_warnoptions;
    while (entry != NULL) {
        PySys_AddWarnOption(entry->value);
        entry = entry->next;
    }
    entry = _preinit_xoptions;
    while (entry != NULL) {
        PySys_AddXOption(entry->value);
        entry = entry->next;
    }

    _clear_all_preinit_options();
    return 0;
};

static PyObject *
get_warnoptions(void)
{
    PyObject *warnoptions = _PySys_GetObjectId(&PyId_warnoptions);
    if (warnoptions == NULL || !PyList_Check(warnoptions)) {
        /* PEP432 TODO: we can reach this if warnoptions is NULL in the main
        *  interpreter config. When that happens, we need to properly set
         * the `warnoptions` reference in the main interpreter config as well.
         *
         * For Python 3.7, we shouldn't be able to get here due to the
         * combination of how _PyMainInterpreter_ReadConfig and _PySys_EndInit
         * work, but we expect 3.8+ to make the _PyMainInterpreter_ReadConfig
         * call optional for embedding applications, thus making this
         * reachable again.
         */
        warnoptions = PyList_New(0);
        if (warnoptions == NULL)
            return NULL;
        if (_PySys_SetObjectId(&PyId_warnoptions, warnoptions)) {
            Py_DECREF(warnoptions);
            return NULL;
        }
        Py_DECREF(warnoptions);
    }
    return warnoptions;
}

void
PySys_ResetWarnOptions(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL) {
        _clear_preinit_entries(&_preinit_warnoptions);
        return;
    }

    PyObject *warnoptions = _PySys_GetObjectId(&PyId_warnoptions);
    if (warnoptions == NULL || !PyList_Check(warnoptions))
        return;
    PyList_SetSlice(warnoptions, 0, PyList_GET_SIZE(warnoptions), NULL);
}

int
_PySys_AddWarnOptionWithError(PyObject *option)
{
    PyObject *warnoptions = get_warnoptions();
    if (warnoptions == NULL) {
        return -1;
    }
    if (PyList_Append(warnoptions, option)) {
        return -1;
    }
    return 0;
}

void
PySys_AddWarnOptionUnicode(PyObject *option)
{
    (void)_PySys_AddWarnOptionWithError(option);
}

void
PySys_AddWarnOption(const wchar_t *s)
{
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL) {
        _append_preinit_entry(&_preinit_warnoptions, s);
        return;
    }
    PyObject *unicode;
    unicode = PyUnicode_FromWideChar(s, -1);
    if (unicode == NULL)
        return;
    PySys_AddWarnOptionUnicode(unicode);
    Py_DECREF(unicode);
}

int
PySys_HasWarnOptions(void)
{
    PyObject *warnoptions = _PySys_GetObjectId(&PyId_warnoptions);
    return (warnoptions != NULL && PyList_Check(warnoptions)
            && PyList_GET_SIZE(warnoptions) > 0);
}

static PyObject *
get_xoptions(void)
{
    PyObject *xoptions = _PySys_GetObjectId(&PyId__xoptions);
    if (xoptions == NULL || !PyDict_Check(xoptions)) {
        /* PEP432 TODO: we can reach this if xoptions is NULL in the main
        *  interpreter config. When that happens, we need to properly set
         * the `xoptions` reference in the main interpreter config as well.
         *
         * For Python 3.7, we shouldn't be able to get here due to the
         * combination of how _PyMainInterpreter_ReadConfig and _PySys_EndInit
         * work, but we expect 3.8+ to make the _PyMainInterpreter_ReadConfig
         * call optional for embedding applications, thus making this
         * reachable again.
         */
        xoptions = PyDict_New();
        if (xoptions == NULL)
            return NULL;
        if (_PySys_SetObjectId(&PyId__xoptions, xoptions)) {
            Py_DECREF(xoptions);
            return NULL;
        }
        Py_DECREF(xoptions);
    }
    return xoptions;
}

int
_PySys_AddXOptionWithError(const wchar_t *s)
{
    PyObject *name = NULL, *value = NULL;

    PyObject *opts = get_xoptions();
    if (opts == NULL) {
        goto error;
    }

    const wchar_t *name_end = wcschr(s, L'=');
    if (!name_end) {
        name = PyUnicode_FromWideChar(s, -1);
        value = Py_True;
        Py_INCREF(value);
    }
    else {
        name = PyUnicode_FromWideChar(s, name_end - s);
        value = PyUnicode_FromWideChar(name_end + 1, -1);
    }
    if (name == NULL || value == NULL) {
        goto error;
    }
    if (PyDict_SetItem(opts, name, value) < 0) {
        goto error;
    }
    Py_DECREF(name);
    Py_DECREF(value);
    return 0;

error:
    Py_XDECREF(name);
    Py_XDECREF(value);
    return -1;
}

void
PySys_AddXOption(const wchar_t *s)
{
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL) {
        _append_preinit_entry(&_preinit_xoptions, s);
        return;
    }
    if (_PySys_AddXOptionWithError(s) < 0) {
        /* No return value, therefore clear error state if possible */
        if (_PyThreadState_UncheckedGet()) {
            PyErr_Clear();
        }
    }
}

PyObject *
PySys_GetXOptions(void)
{
    return get_xoptions();
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
stdin -- standard input file object; used by input()\n\
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
builtin_module_names -- tuple of module names built into this interpreter\n\
copyright -- copyright notice pertaining to this interpreter\n\
exec_prefix -- prefix used to find the machine-specific Python library\n\
executable -- absolute path of the executable binary of the Python interpreter\n\
float_info -- a named tuple with information about the float implementation.\n\
float_repr_style -- string indicating the style of repr() output for floats\n\
hash_info -- a named tuple with information about the hash algorithm.\n\
hexversion -- version information encoded as a single integer\n\
implementation -- Python implementation information.\n\
int_info -- a named tuple with information about the int implementation.\n\
maxsize -- the largest supported length of containers.\n\
maxunicode -- the value of the largest Unicode code point\n\
platform -- platform identifier\n\
prefix -- prefix used to find the Python library\n\
thread_info -- a named tuple with information about the thread implementation.\n\
version -- the version of this interpreter as a string\n\
version_info -- version information as a named tuple\n\
"
)
#ifdef MS_COREDLL
/* concatenating string here */
PyDoc_STR(
"dllhandle -- [Windows only] integer handle of the Python DLL\n\
winver -- [Windows only] version number of the Python DLL\n\
"
)
#endif /* MS_COREDLL */
#ifdef MS_WINDOWS
/* concatenating string here */
PyDoc_STR(
"_enablelegacywindowsfsencoding -- [Windows only] \n\
"
)
#endif
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
getsizeof() -- return the size of an object in bytes\n\
gettrace() -- get the global debug tracing function\n\
setcheckinterval() -- control how often the interpreter checks for events\n\
setdlopenflags() -- set the flags to be used for dlopen() calls\n\
setprofile() -- set the global profiling function\n\
setrecursionlimit() -- set the max recursion depth for the interpreter\n\
settrace() -- set the global debug tracing function\n\
"
)
/* end of sys_doc */ ;


PyDoc_STRVAR(flags__doc__,
"sys.flags\n\
\n\
Flags provided through command line arguments or environment vars.");

static PyTypeObject FlagsType;

static PyStructSequence_Field flags_fields[] = {
    {"debug",                   "-d"},
    {"inspect",                 "-i"},
    {"interactive",             "-i"},
    {"optimize",                "-O or -OO"},
    {"dont_write_bytecode",     "-B"},
    {"no_user_site",            "-s"},
    {"no_site",                 "-S"},
    {"ignore_environment",      "-E"},
    {"verbose",                 "-v"},
    /* {"unbuffered",                   "-u"}, */
    /* {"skip_first",                   "-x"}, */
    {"bytes_warning",           "-b"},
    {"quiet",                   "-q"},
    {"hash_randomization",      "-R"},
    {"isolated",                "-I"},
    {"dev_mode",                "-X dev"},
    {"utf8_mode",               "-X utf8"},
    {0}
};

static PyStructSequence_Desc flags_desc = {
    "sys.flags",        /* name */
    flags__doc__,       /* doc */
    flags_fields,       /* fields */
    15
};

static PyObject*
make_flags(void)
{
    int pos = 0;
    PyObject *seq;
    _PyCoreConfig *core_config = &_PyGILState_GetInterpreterStateUnsafe()->core_config;

    seq = PyStructSequence_New(&FlagsType);
    if (seq == NULL)
        return NULL;

#define SetFlag(flag) \
    PyStructSequence_SET_ITEM(seq, pos++, PyLong_FromLong(flag))

    SetFlag(Py_DebugFlag);
    SetFlag(Py_InspectFlag);
    SetFlag(Py_InteractiveFlag);
    SetFlag(Py_OptimizeFlag);
    SetFlag(Py_DontWriteBytecodeFlag);
    SetFlag(Py_NoUserSiteDirectory);
    SetFlag(Py_NoSiteFlag);
    SetFlag(Py_IgnoreEnvironmentFlag);
    SetFlag(Py_VerboseFlag);
    /* SetFlag(saw_unbuffered_flag); */
    /* SetFlag(skipfirstline); */
    SetFlag(Py_BytesWarningFlag);
    SetFlag(Py_QuietFlag);
    SetFlag(Py_HashRandomizationFlag);
    SetFlag(Py_IsolatedFlag);
    PyStructSequence_SET_ITEM(seq, pos++, PyBool_FromLong(core_config->dev_mode));
    SetFlag(Py_UTF8Mode);
#undef SetFlag

    if (PyErr_Occurred()) {
        Py_DECREF(seq);
        return NULL;
    }
    return seq;
}

PyDoc_STRVAR(version_info__doc__,
"sys.version_info\n\
\n\
Version information as a named tuple.");

static PyTypeObject VersionInfoType;

static PyStructSequence_Field version_info_fields[] = {
    {"major", "Major release number"},
    {"minor", "Minor release number"},
    {"micro", "Patch release number"},
    {"releaselevel", "'alpha', 'beta', 'candidate', or 'final'"},
    {"serial", "Serial release number"},
    {0}
};

static PyStructSequence_Desc version_info_desc = {
    "sys.version_info",     /* name */
    version_info__doc__,    /* doc */
    version_info_fields,    /* fields */
    5
};

static PyObject *
make_version_info(void)
{
    PyObject *version_info;
    char *s;
    int pos = 0;

    version_info = PyStructSequence_New(&VersionInfoType);
    if (version_info == NULL) {
        return NULL;
    }

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

#define SetIntItem(flag) \
    PyStructSequence_SET_ITEM(version_info, pos++, PyLong_FromLong(flag))
#define SetStrItem(flag) \
    PyStructSequence_SET_ITEM(version_info, pos++, PyUnicode_FromString(flag))

    SetIntItem(PY_MAJOR_VERSION);
    SetIntItem(PY_MINOR_VERSION);
    SetIntItem(PY_MICRO_VERSION);
    SetStrItem(s);
    SetIntItem(PY_RELEASE_SERIAL);
#undef SetIntItem
#undef SetStrItem

    if (PyErr_Occurred()) {
        Py_CLEAR(version_info);
        return NULL;
    }
    return version_info;
}

/* sys.implementation values */
#define NAME "cpython"
const char *_PySys_ImplName = NAME;
#define MAJOR Py_STRINGIFY(PY_MAJOR_VERSION)
#define MINOR Py_STRINGIFY(PY_MINOR_VERSION)
#define TAG NAME "-" MAJOR MINOR
const char *_PySys_ImplCacheTag = TAG;
#undef NAME
#undef MAJOR
#undef MINOR
#undef TAG

static PyObject *
make_impl_info(PyObject *version_info)
{
    int res;
    PyObject *impl_info, *value, *ns;

    impl_info = PyDict_New();
    if (impl_info == NULL)
        return NULL;

    /* populate the dict */

    value = PyUnicode_FromString(_PySys_ImplName);
    if (value == NULL)
        goto error;
    res = PyDict_SetItemString(impl_info, "name", value);
    Py_DECREF(value);
    if (res < 0)
        goto error;

    value = PyUnicode_FromString(_PySys_ImplCacheTag);
    if (value == NULL)
        goto error;
    res = PyDict_SetItemString(impl_info, "cache_tag", value);
    Py_DECREF(value);
    if (res < 0)
        goto error;

    res = PyDict_SetItemString(impl_info, "version", version_info);
    if (res < 0)
        goto error;

    value = PyLong_FromLong(PY_VERSION_HEX);
    if (value == NULL)
        goto error;
    res = PyDict_SetItemString(impl_info, "hexversion", value);
    Py_DECREF(value);
    if (res < 0)
        goto error;

#ifdef MULTIARCH
    value = PyUnicode_FromString(MULTIARCH);
    if (value == NULL)
        goto error;
    res = PyDict_SetItemString(impl_info, "_multiarch", value);
    Py_DECREF(value);
    if (res < 0)
        goto error;
#endif

    /* dict ready */

    ns = _PyNamespace_New(impl_info);
    Py_DECREF(impl_info);
    return ns;

error:
    Py_CLEAR(impl_info);
    return NULL;
}

static struct PyModuleDef sysmodule = {
    PyModuleDef_HEAD_INIT,
    "sys",
    sys_doc,
    -1, /* multiple "initialization" just copies the module dict. */
    sys_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

/* Updating the sys namespace, returning NULL pointer on error */
#define SET_SYS_FROM_STRING_BORROW(key, value)             \
    do {                                                   \
        PyObject *v = (value);                             \
        if (v == NULL) {                                   \
            goto err_occurred;                             \
        }                                                  \
        res = PyDict_SetItemString(sysdict, key, v);       \
        if (res < 0) {                                     \
            goto err_occurred;                             \
        }                                                  \
    } while (0)
#define SET_SYS_FROM_STRING(key, value)                    \
    do {                                                   \
        PyObject *v = (value);                             \
        if (v == NULL) {                                   \
            goto err_occurred;                             \
        }                                                  \
        res = PyDict_SetItemString(sysdict, key, v);       \
        Py_DECREF(v);                                      \
        if (res < 0) {                                     \
            goto err_occurred;                             \
        }                                                  \
    } while (0)


_PyInitError
_PySys_BeginInit(PyObject **sysmod)
{
    PyObject *m, *sysdict, *version_info;
    int res;

    m = _PyModule_CreateInitialized(&sysmodule, PYTHON_API_VERSION);
    if (m == NULL) {
        return _Py_INIT_ERR("failed to create a module object");
    }
    sysdict = PyModule_GetDict(m);

    /* Check that stdin is not a directory
       Using shell redirection, you can redirect stdin to a directory,
       crashing the Python interpreter. Catch this common mistake here
       and output a useful error message. Note that under MS Windows,
       the shell already prevents that. */
#ifndef MS_WINDOWS
    {
        struct _Py_stat_struct sb;
        if (_Py_fstat_noraise(fileno(stdin), &sb) == 0 &&
            S_ISDIR(sb.st_mode)) {
            return _Py_INIT_USER_ERR("<stdin> is a directory, "
                                     "cannot continue");
        }
    }
#endif

    /* stdin/stdout/stderr are set in pylifecycle.c */

    SET_SYS_FROM_STRING_BORROW("__displayhook__",
                               PyDict_GetItemString(sysdict, "displayhook"));
    SET_SYS_FROM_STRING_BORROW("__excepthook__",
                               PyDict_GetItemString(sysdict, "excepthook"));
    SET_SYS_FROM_STRING_BORROW(
        "__breakpointhook__",
        PyDict_GetItemString(sysdict, "breakpointhook"));
    SET_SYS_FROM_STRING("version",
                         PyUnicode_FromString(Py_GetVersion()));
    SET_SYS_FROM_STRING("hexversion",
                         PyLong_FromLong(PY_VERSION_HEX));
    SET_SYS_FROM_STRING("_git",
                        Py_BuildValue("(szz)", "CPython", _Py_gitidentifier(),
                                      _Py_gitversion()));
    SET_SYS_FROM_STRING("_framework", PyUnicode_FromString(_PYTHONFRAMEWORK));
    SET_SYS_FROM_STRING("api_version",
                        PyLong_FromLong(PYTHON_API_VERSION));
    SET_SYS_FROM_STRING("copyright",
                        PyUnicode_FromString(Py_GetCopyright()));
    SET_SYS_FROM_STRING("platform",
                        PyUnicode_FromString(Py_GetPlatform()));
    SET_SYS_FROM_STRING("maxsize",
                        PyLong_FromSsize_t(PY_SSIZE_T_MAX));
    SET_SYS_FROM_STRING("float_info",
                        PyFloat_GetInfo());
    SET_SYS_FROM_STRING("int_info",
                        PyLong_GetInfo());
    /* initialize hash_info */
    if (Hash_InfoType.tp_name == NULL) {
        if (PyStructSequence_InitType2(&Hash_InfoType, &hash_info_desc) < 0) {
            goto type_init_failed;
        }
    }
    SET_SYS_FROM_STRING("hash_info",
                        get_hash_info());
    SET_SYS_FROM_STRING("maxunicode",
                        PyLong_FromLong(0x10FFFF));
    SET_SYS_FROM_STRING("builtin_module_names",
                        list_builtin_module_names());
#if PY_BIG_ENDIAN
    SET_SYS_FROM_STRING("byteorder",
                        PyUnicode_FromString("big"));
#else
    SET_SYS_FROM_STRING("byteorder",
                        PyUnicode_FromString("little"));
#endif

#ifdef MS_COREDLL
    SET_SYS_FROM_STRING("dllhandle",
                        PyLong_FromVoidPtr(PyWin_DLLhModule));
    SET_SYS_FROM_STRING("winver",
                        PyUnicode_FromString(PyWin_DLLVersionString));
#endif
#ifdef ABIFLAGS
    SET_SYS_FROM_STRING("abiflags",
                        PyUnicode_FromString(ABIFLAGS));
#endif

    /* version_info */
    if (VersionInfoType.tp_name == NULL) {
        if (PyStructSequence_InitType2(&VersionInfoType,
                                       &version_info_desc) < 0) {
            goto type_init_failed;
        }
    }
    version_info = make_version_info();
    SET_SYS_FROM_STRING("version_info", version_info);
    /* prevent user from creating new instances */
    VersionInfoType.tp_init = NULL;
    VersionInfoType.tp_new = NULL;
    res = PyDict_DelItemString(VersionInfoType.tp_dict, "__new__");
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
        PyErr_Clear();

    /* implementation */
    SET_SYS_FROM_STRING("implementation", make_impl_info(version_info));

    /* flags */
    if (FlagsType.tp_name == 0) {
        if (PyStructSequence_InitType2(&FlagsType, &flags_desc) < 0) {
            goto type_init_failed;
        }
    }
    /* Set flags to their default values */
    SET_SYS_FROM_STRING("flags", make_flags());

#if defined(MS_WINDOWS)
    /* getwindowsversion */
    if (WindowsVersionType.tp_name == 0)
        if (PyStructSequence_InitType2(&WindowsVersionType,
                                       &windows_version_desc) < 0) {
            goto type_init_failed;
        }
    /* prevent user from creating new instances */
    WindowsVersionType.tp_init = NULL;
    WindowsVersionType.tp_new = NULL;
    assert(!PyErr_Occurred());
    res = PyDict_DelItemString(WindowsVersionType.tp_dict, "__new__");
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Clear();
    }
#endif

    /* float repr style: 0.03 (short) vs 0.029999999999999999 (legacy) */
#ifndef PY_NO_SHORT_FLOAT_REPR
    SET_SYS_FROM_STRING("float_repr_style",
                        PyUnicode_FromString("short"));
#else
    SET_SYS_FROM_STRING("float_repr_style",
                        PyUnicode_FromString("legacy"));
#endif

    SET_SYS_FROM_STRING("thread_info", PyThread_GetInfo());

    /* initialize asyncgen_hooks */
    if (AsyncGenHooksType.tp_name == NULL) {
        if (PyStructSequence_InitType2(
                &AsyncGenHooksType, &asyncgen_hooks_desc) < 0) {
            goto type_init_failed;
        }
    }

    if (PyErr_Occurred()) {
        goto err_occurred;
    }

    *sysmod = m;

    return _Py_INIT_OK();

type_init_failed:
    return _Py_INIT_ERR("failed to initialize a type");

err_occurred:
    return _Py_INIT_ERR("can't initialize sys module");
}

/* Updating the sys namespace, returning integer error codes */
#define SET_SYS_FROM_STRING_INT_RESULT(key, value)         \
    do {                                                   \
        PyObject *v = (value);                             \
        if (v == NULL)                                     \
            return -1;                                     \
        res = PyDict_SetItemString(sysdict, key, v);       \
        Py_DECREF(v);                                      \
        if (res < 0) {                                     \
            return res;                                    \
        }                                                  \
    } while (0)

int
_PySys_EndInit(PyObject *sysdict, _PyMainInterpreterConfig *config)
{
    int res;

    /* _PyMainInterpreterConfig_Read() must set all these variables */
    assert(config->module_search_path != NULL);
    assert(config->executable != NULL);
    assert(config->prefix != NULL);
    assert(config->base_prefix != NULL);
    assert(config->exec_prefix != NULL);
    assert(config->base_exec_prefix != NULL);

    SET_SYS_FROM_STRING_BORROW("path", config->module_search_path);
    SET_SYS_FROM_STRING_BORROW("executable", config->executable);
    SET_SYS_FROM_STRING_BORROW("prefix", config->prefix);
    SET_SYS_FROM_STRING_BORROW("base_prefix", config->base_prefix);
    SET_SYS_FROM_STRING_BORROW("exec_prefix", config->exec_prefix);
    SET_SYS_FROM_STRING_BORROW("base_exec_prefix", config->base_exec_prefix);

#ifdef MS_WINDOWS
    const wchar_t *baseExecutable = _wgetenv(L"__PYVENV_BASE_EXECUTABLE__");
    if (baseExecutable) {
        SET_SYS_FROM_STRING("_base_executable",
                            PyUnicode_FromWideChar(baseExecutable, -1));
        _wputenv_s(L"__PYVENV_BASE_EXECUTABLE__", L"");
    } else {
        SET_SYS_FROM_STRING_BORROW("_base_executable", config->executable);
    }
#endif

    if (config->argv != NULL) {
        SET_SYS_FROM_STRING_BORROW("argv", config->argv);
    }
    if (config->warnoptions != NULL) {
        SET_SYS_FROM_STRING_BORROW("warnoptions", config->warnoptions);
    }
    if (config->xoptions != NULL) {
        SET_SYS_FROM_STRING_BORROW("_xoptions", config->xoptions);
    }

    /* Set flags to their final values */
    SET_SYS_FROM_STRING_INT_RESULT("flags", make_flags());
    /* prevent user from creating new instances */
    FlagsType.tp_init = NULL;
    FlagsType.tp_new = NULL;
    res = PyDict_DelItemString(FlagsType.tp_dict, "__new__");
    if (res < 0) {
        if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
            return res;
        }
        PyErr_Clear();
    }

    SET_SYS_FROM_STRING_INT_RESULT("dont_write_bytecode",
                         PyBool_FromLong(Py_DontWriteBytecodeFlag));

    if (get_warnoptions() == NULL)
        return -1;

    if (get_xoptions() == NULL)
        return -1;

    /* Transfer any sys.warnoptions and sys._xoptions set directly
     * by an embedding application from the linked list to the module. */
    if (_PySys_ReadPreInitOptions() != 0)
        return -1;

    if (PyErr_Occurred())
        return -1;
    return 0;

err_occurred:
    return -1;
}

#undef SET_SYS_FROM_STRING
#undef SET_SYS_FROM_STRING_BORROW
#undef SET_SYS_FROM_STRING_INT_RESULT

static PyObject *
makepathobject(const wchar_t *path, wchar_t delim)
{
    int i, n;
    const wchar_t *p;
    PyObject *v, *w;

    n = 1;
    p = path;
    while ((p = wcschr(p, delim)) != NULL) {
        n++;
        p++;
    }
    v = PyList_New(n);
    if (v == NULL)
        return NULL;
    for (i = 0; ; i++) {
        p = wcschr(path, delim);
        if (p == NULL)
            p = path + wcslen(path); /* End of string */
        w = PyUnicode_FromWideChar(path, (Py_ssize_t)(p - path));
        if (w == NULL) {
            Py_DECREF(v);
            return NULL;
        }
        PyList_SET_ITEM(v, i, w);
        if (*p == '\0')
            break;
        path = p+1;
    }
    return v;
}

void
PySys_SetPath(const wchar_t *path)
{
    PyObject *v;
    if ((v = makepathobject(path, DELIM)) == NULL)
        Py_FatalError("can't create sys.path");
    if (_PySys_SetObjectId(&PyId_path, v) != 0)
        Py_FatalError("can't assign sys.path");
    Py_DECREF(v);
}

static PyObject *
makeargvobject(int argc, wchar_t **argv)
{
    PyObject *av;
    if (argc <= 0 || argv == NULL) {
        /* Ensure at least one (empty) argument is seen */
        static wchar_t *empty_argv[1] = {L""};
        argv = empty_argv;
        argc = 1;
    }
    av = PyList_New(argc);
    if (av != NULL) {
        int i;
        for (i = 0; i < argc; i++) {
            PyObject *v = PyUnicode_FromWideChar(argv[i], -1);
            if (v == NULL) {
                Py_DECREF(av);
                av = NULL;
                break;
            }
            PyList_SET_ITEM(av, i, v);
        }
    }
    return av;
}

void
PySys_SetArgvEx(int argc, wchar_t **argv, int updatepath)
{
    PyObject *av = makeargvobject(argc, argv);
    if (av == NULL) {
        Py_FatalError("no mem for sys.argv");
    }
    if (PySys_SetObject("argv", av) != 0) {
        Py_DECREF(av);
        Py_FatalError("can't assign sys.argv");
    }
    Py_DECREF(av);

    if (updatepath) {
        /* If argv[0] is not '-c' nor '-m', prepend argv[0] to sys.path.
           If argv[0] is a symlink, use the real path. */
        PyObject *argv0 = NULL;
        if (!_PyPathConfig_ComputeArgv0(argc, argv, &argv0)) {
            return;
        }
        if (argv0 == NULL) {
            Py_FatalError("can't compute path0 from argv");
        }

        PyObject *sys_path = _PySys_GetObjectId(&PyId_path);
        if (sys_path != NULL) {
            if (PyList_Insert(sys_path, 0, argv0) < 0) {
                Py_DECREF(argv0);
                Py_FatalError("can't prepend path0 to sys.path");
            }
        }
        Py_DECREF(argv0);
    }
}

void
PySys_SetArgv(int argc, wchar_t **argv)
{
    PySys_SetArgvEx(argc, argv, Py_IsolatedFlag == 0);
}

/* Reimplementation of PyFile_WriteString() no calling indirectly
   PyErr_CheckSignals(): avoid the call to PyObject_Str(). */

static int
sys_pyfile_write_unicode(PyObject *unicode, PyObject *file)
{
    PyObject *writer = NULL, *result = NULL;
    int err;

    if (file == NULL)
        return -1;

    writer = _PyObject_GetAttrId(file, &PyId_write);
    if (writer == NULL)
        goto error;

    result = PyObject_CallFunctionObjArgs(writer, unicode, NULL);
    if (result == NULL) {
        goto error;
    } else {
        err = 0;
        goto finally;
    }

error:
    err = -1;
finally:
    Py_XDECREF(writer);
    Py_XDECREF(result);
    return err;
}

static int
sys_pyfile_write(const char *text, PyObject *file)
{
    PyObject *unicode = NULL;
    int err;

    if (file == NULL)
        return -1;

    unicode = PyUnicode_FromString(text);
    if (unicode == NULL)
        return -1;

    err = sys_pyfile_write_unicode(unicode, file);
    Py_DECREF(unicode);
    return err;
}

/* APIs to write to sys.stdout or sys.stderr using a printf-like interface.
   Adapted from code submitted by Just van Rossum.

   PySys_WriteStdout(format, ...)
   PySys_WriteStderr(format, ...)

      The first function writes to sys.stdout; the second to sys.stderr.  When
      there is a problem, they write to the real (C level) stdout or stderr;
      no exceptions are raised.

      PyErr_CheckSignals() is not called to avoid the execution of the Python
      signal handlers: they may raise a new exception whereas sys_write()
      ignores all exceptions.

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
sys_write(_Py_Identifier *key, FILE *fp, const char *format, va_list va)
{
    PyObject *file;
    PyObject *error_type, *error_value, *error_traceback;
    char buffer[1001];
    int written;

    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    file = _PySys_GetObjectId(key);
    written = PyOS_vsnprintf(buffer, sizeof(buffer), format, va);
    if (sys_pyfile_write(buffer, file) != 0) {
        PyErr_Clear();
        fputs(buffer, fp);
    }
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        const char *truncated = "... truncated";
        if (sys_pyfile_write(truncated, file) != 0)
            fputs(truncated, fp);
    }
    PyErr_Restore(error_type, error_value, error_traceback);
}

void
PySys_WriteStdout(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_write(&PyId_stdout, stdout, format, va);
    va_end(va);
}

void
PySys_WriteStderr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_write(&PyId_stderr, stderr, format, va);
    va_end(va);
}

static void
sys_format(_Py_Identifier *key, FILE *fp, const char *format, va_list va)
{
    PyObject *file, *message;
    PyObject *error_type, *error_value, *error_traceback;
    const char *utf8;

    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    file = _PySys_GetObjectId(key);
    message = PyUnicode_FromFormatV(format, va);
    if (message != NULL) {
        if (sys_pyfile_write_unicode(message, file) != 0) {
            PyErr_Clear();
            utf8 = PyUnicode_AsUTF8(message);
            if (utf8 != NULL)
                fputs(utf8, fp);
        }
        Py_DECREF(message);
    }
    PyErr_Restore(error_type, error_value, error_traceback);
}

void
PySys_FormatStdout(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_format(&PyId_stdout, stdout, format, va);
    va_end(va);
}

void
PySys_FormatStderr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_format(&PyId_stderr, stderr, format, va);
    va_end(va);
}

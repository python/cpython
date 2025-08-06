
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
#include "pycore_audit.h"         // _Py_AuditHookEntry
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_SetAsyncGenFinalizer()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_import.h"        // _PyImport_SetDLOpenFlags()
#include "pycore_initconfig.h"    // _PyStatus_EXCEPTION()
#include "pycore_interpframe.h"   // _PyFrame_GetFirstComplete()
#include "pycore_long.h"          // _PY_LONG_MAX_STR_DIGITS_THRESHOLD
#include "pycore_modsupport.h"    // _PyModule_CreateInitialized()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_object.h"        // _PyObject_DebugTypeStats()
#include "pycore_optimizer.h"     // _PyDumpExecutors()
#include "pycore_pathconfig.h"    // _PyPathConfig_ComputeSysPath0()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pylifecycle.h"   // _PyErr_WriteUnraisableDefaultHook()
#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR
#include "pycore_pymem.h"         // _PyMem_DefaultRawFree()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_pystats.h"       // _Py_PrintSpecializationStats()
#include "pycore_structseq.h"     // _PyStructSequence_InitBuiltinWithFlags()
#include "pycore_sysmodule.h"     // export _PySys_GetSizeOf()
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal()

#include "pydtrace.h"             // PyDTrace_AUDIT()
#include "osdefs.h"               // DELIM
#include "stdlib_module_names.h"  // _Py_stdlib_module_names

#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // getpid()
#endif

#ifdef MS_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif /* MS_WINDOWS */

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
/* A string loaded from the DLL at startup: */
extern const char *PyWin_DLLVersionString;
#endif

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

/*[clinic input]
module sys
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3726b388feee8cea]*/

#include "clinic/sysmodule.c.h"


PyObject *
PySys_GetAttr(PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%T'",
                     name);
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *sysdict = tstate->interp->sysdict;
    if (sysdict == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no sys module");
        return NULL;
    }
    PyObject *value;
    if (PyDict_GetItemRef(sysdict, name, &value) == 0) {
        PyErr_Format(PyExc_RuntimeError, "lost sys.%U", name);
    }
    return value;
}

PyObject *
PySys_GetAttrString(const char *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *sysdict = tstate->interp->sysdict;
    if (sysdict == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no sys module");
        return NULL;
    }
    PyObject *value;
    if (PyDict_GetItemStringRef(sysdict, name, &value) == 0) {
        PyErr_Format(PyExc_RuntimeError, "lost sys.%s", name);
    }
    return value;
}

int
PySys_GetOptionalAttr(PyObject *name, PyObject **value)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%T'",
                     name);
        *value = NULL;
        return -1;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *sysdict = tstate->interp->sysdict;
    if (sysdict == NULL) {
        *value = NULL;
        return 0;
    }
    return PyDict_GetItemRef(sysdict, name, value);
}

int
PySys_GetOptionalAttrString(const char *name, PyObject **value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *sysdict = tstate->interp->sysdict;
    if (sysdict == NULL) {
        *value = NULL;
        return 0;
    }
    return PyDict_GetItemStringRef(sysdict, name, value);
}

PyObject *
PySys_GetObject(const char *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *sysdict = tstate->interp->sysdict;
    if (sysdict == NULL) {
        return NULL;
    }
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    PyObject *value;
    (void) PyDict_GetItemStringRef(sysdict, name, &value);
    /* XXX Suppress a new exception if it was raised and restore
     * the old one. */
    if (_PyErr_Occurred(tstate)) {
        PyErr_FormatUnraisable("Exception ignored in PySys_GetObject()");
    }
    _PyErr_SetRaisedException(tstate, exc);
    Py_XDECREF(value);  // return a borrowed reference
    return value;
}

static int
sys_set_object(PyInterpreterState *interp, PyObject *key, PyObject *v)
{
    if (key == NULL) {
        return -1;
    }
    PyObject *sd = interp->sysdict;
    if (sd == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no sys module");
        return -1;
    }
    if (v == NULL) {
        if (PyDict_Pop(sd, key, NULL) < 0) {
            return -1;
        }
        return 0;
    }
    else {
        return PyDict_SetItem(sd, key, v);
    }
}

int
_PySys_SetAttr(PyObject *key, PyObject *v)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return sys_set_object(interp, key, v);
}

static int
sys_set_object_str(PyInterpreterState *interp, const char *name, PyObject *v)
{
    PyObject *key = v ? PyUnicode_InternFromString(name)
                      : PyUnicode_FromString(name);
    int r = sys_set_object(interp, key, v);
    Py_XDECREF(key);
    return r;
}

int
PySys_SetObject(const char *name, PyObject *v)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return sys_set_object_str(interp, name, v);
}

int
_PySys_ClearAttrString(PyInterpreterState *interp,
                       const char *name, int verbose)
{
    if (verbose) {
        PySys_WriteStderr("# clear sys.%s\n", name);
    }
    /* To play it safe, we set the attr to None instead of deleting it. */
    if (PyDict_SetItemString(interp->sysdict, name, Py_None) < 0) {
        return -1;
    }
    return 0;
}


static int
should_audit(PyInterpreterState *interp)
{
    /* interp must not be NULL, but test it just in case for extra safety */
    assert(interp != NULL);
    if (!interp) {
        return 0;
    }
    return (interp->runtime->audit_hooks.head
            || interp->audit_hooks
            || PyDTrace_AUDIT_ENABLED());
}


static int
sys_audit_tstate(PyThreadState *ts, const char *event,
                 const char *argFormat, va_list vargs)
{
    assert(event != NULL);
    assert(!argFormat || !strchr(argFormat, 'N'));

    if (!ts) {
        /* Audit hooks cannot be called with a NULL thread state */
        return 0;
    }

    /* The current implementation cannot be called if tstate is not
       the current Python thread state. */
    assert(ts == _PyThreadState_GET());

    /* Early exit when no hooks are registered */
    PyInterpreterState *is = ts->interp;
    if (!should_audit(is)) {
        return 0;
    }

    PyObject *eventName = NULL;
    PyObject *eventArgs = NULL;
    PyObject *hooks = NULL;
    PyObject *hook = NULL;
    int res = -1;

    int dtrace = PyDTrace_AUDIT_ENABLED();


    PyObject *exc = _PyErr_GetRaisedException(ts);

    /* Initialize event args now */
    if (argFormat && argFormat[0]) {
        eventArgs = Py_VaBuildValue(argFormat, vargs);
        if (eventArgs && !PyTuple_Check(eventArgs)) {
            PyObject *argTuple = PyTuple_Pack(1, eventArgs);
            Py_SETREF(eventArgs, argTuple);
        }
    }
    else {
        eventArgs = PyTuple_New(0);
    }
    if (!eventArgs) {
        goto exit;
    }

    /* Call global hooks
     *
     * We don't worry about any races on hooks getting added,
     * since that would not leave is in an inconsistent state. */
    _Py_AuditHookEntry *e = is->runtime->audit_hooks.head;
    for (; e; e = e->next) {
        if (e->hookCFunction(event, eventArgs, e->userData) < 0) {
            goto exit;
        }
    }

    /* Dtrace USDT point */
    if (dtrace) {
        PyDTrace_AUDIT(event, (void *)eventArgs);
    }

    /* Call interpreter hooks */
    if (is->audit_hooks) {
        eventName = PyUnicode_FromString(event);
        if (!eventName) {
            goto exit;
        }

        hooks = PyObject_GetIter(is->audit_hooks);
        if (!hooks) {
            goto exit;
        }

        /* Disallow tracing in hooks unless explicitly enabled */
        PyThreadState_EnterTracing(ts);
        while ((hook = PyIter_Next(hooks)) != NULL) {
            PyObject *o;
            int canTrace = PyObject_GetOptionalAttr(hook, &_Py_ID(__cantrace__), &o);
            if (o) {
                canTrace = PyObject_IsTrue(o);
                Py_DECREF(o);
            }
            if (canTrace < 0) {
                break;
            }
            if (canTrace) {
                PyThreadState_LeaveTracing(ts);
            }
            PyObject* args[2] = {eventName, eventArgs};
            o = _PyObject_VectorcallTstate(ts, hook, args, 2, NULL);
            if (canTrace) {
                PyThreadState_EnterTracing(ts);
            }
            if (!o) {
                break;
            }
            Py_DECREF(o);
            Py_CLEAR(hook);
        }
        PyThreadState_LeaveTracing(ts);
        if (_PyErr_Occurred(ts)) {
            goto exit;
        }
    }

    res = 0;

exit:
    Py_XDECREF(hook);
    Py_XDECREF(hooks);
    Py_XDECREF(eventName);
    Py_XDECREF(eventArgs);

    if (!res) {
        _PyErr_SetRaisedException(ts, exc);
    }
    else {
        assert(_PyErr_Occurred(ts));
        Py_XDECREF(exc);
    }

    return res;
}

int
_PySys_Audit(PyThreadState *tstate, const char *event,
             const char *argFormat, ...)
{
    va_list vargs;
    va_start(vargs, argFormat);
    int res = sys_audit_tstate(tstate, event, argFormat, vargs);
    va_end(vargs);
    return res;
}

int
PySys_Audit(const char *event, const char *argFormat, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    va_list vargs;
    va_start(vargs, argFormat);
    int res = sys_audit_tstate(tstate, event, argFormat, vargs);
    va_end(vargs);
    return res;
}

int
PySys_AuditTuple(const char *event, PyObject *args)
{
    if (args == NULL) {
        return PySys_Audit(event, NULL);
    }

    if (!PyTuple_Check(args)) {
        PyErr_Format(PyExc_TypeError, "args must be tuple, got %s",
                     Py_TYPE(args)->tp_name);
        return -1;
    }
    return PySys_Audit(event, "O", args);
}

/* We expose this function primarily for our own cleanup during
 * finalization. In general, it should not need to be called,
 * and as such the function is not exported.
 *
 * Must be finalizing to clear hooks */
void
_PySys_ClearAuditHooks(PyThreadState *ts)
{
    assert(ts != NULL);
    if (!ts) {
        return;
    }

    _PyRuntimeState *runtime = ts->interp->runtime;
    /* The hooks are global so we have to check for runtime finalization. */
    PyThreadState *finalizing = _PyRuntimeState_GetFinalizing(runtime);
    assert(finalizing == ts);
    if (finalizing != ts) {
        return;
    }

    const PyConfig *config = _PyInterpreterState_GetConfig(ts->interp);
    if (config->verbose) {
        PySys_WriteStderr("# clear sys.audit hooks\n");
    }

    /* Hooks can abort later hooks for this event, but cannot
       abort the clear operation itself. */
    _PySys_Audit(ts, "cpython._PySys_ClearAuditHooks", NULL);
    _PyErr_Clear(ts);

    /* We don't worry about the very unlikely race right here,
     * since it's entirely benign.  Nothing else removes entries
     * from the list and adding an entry right now would not cause
     * any trouble. */
    _Py_AuditHookEntry *e = runtime->audit_hooks.head, *n;
    runtime->audit_hooks.head = NULL;
    while (e) {
        n = e->next;
        PyMem_RawFree(e);
        e = n;
    }
}

static void
add_audit_hook_entry_unlocked(_PyRuntimeState *runtime,
                              _Py_AuditHookEntry *entry)
{
    if (runtime->audit_hooks.head == NULL) {
        runtime->audit_hooks.head = entry;
    }
    else {
        _Py_AuditHookEntry *last = runtime->audit_hooks.head;
        while (last->next) {
            last = last->next;
        }
        last->next = entry;
    }
}

int
PySys_AddAuditHook(Py_AuditHookFunction hook, void *userData)
{
    /* tstate can be NULL, so access directly _PyRuntime:
       PySys_AddAuditHook() can be called before Python is initialized. */
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate;
    if (runtime->initialized) {
        tstate = _PyThreadState_GET();
    }
    else {
        tstate = NULL;
    }

    /* Invoke existing audit hooks to allow them an opportunity to abort. */
    /* Cannot invoke hooks until we are initialized */
    if (tstate != NULL) {
        if (_PySys_Audit(tstate, "sys.addaudithook", NULL) < 0) {
            if (_PyErr_ExceptionMatches(tstate, PyExc_RuntimeError)) {
                /* We do not report errors derived from RuntimeError */
                _PyErr_Clear(tstate);
                return 0;
            }
            return -1;
        }
    }

    _Py_AuditHookEntry *e = (_Py_AuditHookEntry*)PyMem_RawMalloc(
            sizeof(_Py_AuditHookEntry));
    if (!e) {
        if (tstate != NULL) {
            _PyErr_NoMemory(tstate);
        }
        return -1;
    }
    e->next = NULL;
    e->hookCFunction = (Py_AuditHookFunction)hook;
    e->userData = userData;

    PyMutex_Lock(&runtime->audit_hooks.mutex);
    add_audit_hook_entry_unlocked(runtime, e);
    PyMutex_Unlock(&runtime->audit_hooks.mutex);

    return 0;
}

/*[clinic input]
sys.addaudithook

    hook: object

Adds a new audit hook callback.
[clinic start generated code]*/

static PyObject *
sys_addaudithook_impl(PyObject *module, PyObject *hook)
/*[clinic end generated code: output=4f9c17aaeb02f44e input=0f3e191217a45e34]*/
{
    PyThreadState *tstate = _PyThreadState_GET();

    /* Invoke existing audit hooks to allow them an opportunity to abort. */
    if (_PySys_Audit(tstate, "sys.addaudithook", NULL) < 0) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_Exception)) {
            /* We do not report errors derived from Exception */
            _PyErr_Clear(tstate);
            Py_RETURN_NONE;
        }
        return NULL;
    }

    PyInterpreterState *interp = tstate->interp;
    if (interp->audit_hooks == NULL) {
        interp->audit_hooks = PyList_New(0);
        if (interp->audit_hooks == NULL) {
            return NULL;
        }
        /* Avoid having our list of hooks show up in the GC module */
        PyObject_GC_UnTrack(interp->audit_hooks);
    }

    if (PyList_Append(interp->audit_hooks, hook) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
sys.audit

    event: str
    /
    *args: tuple

Passes the event to any audit hooks that are attached.
[clinic start generated code]*/

static PyObject *
sys_audit_impl(PyObject *module, const char *event, PyObject *args)
/*[clinic end generated code: output=1d0fc82da768f49d input=ec3b688527945109]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);

    if (!should_audit(tstate->interp)) {
        Py_RETURN_NONE;
    }

    int res = _PySys_Audit(tstate, event, "O", args);
    if (res < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
sys_breakpointhook(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *keywords)
{
    PyThreadState *tstate = _PyThreadState_GET();
    assert(!_PyErr_Occurred(tstate));
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
        _PyErr_NoMemory(tstate);
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

    PyObject *module = PyImport_Import(modulepath);
    Py_DECREF(modulepath);

    if (module == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_ImportError)) {
            goto warn;
        }
        PyMem_RawFree(envar);
        return NULL;
    }

    PyObject *hook = PyObject_GetAttrString(module, attrname);
    Py_DECREF(module);

    if (hook == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
            goto warn;
        }
        PyMem_RawFree(envar);
        return NULL;
    }
    PyMem_RawFree(envar);
    PyObject *retval = PyObject_Vectorcall(hook, args, nargs, keywords);
    Py_DECREF(hook);
    return retval;

  warn:
    /* If any of the imports went wrong, then warn and ignore. */
    _PyErr_Clear(tstate);
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
"breakpointhook($module, /, *args, **kwargs)\n"
"--\n"
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

    stdout_encoding = PyObject_GetAttr(outf, &_Py_ID(encoding));
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

    if (PyObject_GetOptionalAttr(outf, &_Py_ID(buffer), &buffer) < 0) {
        Py_DECREF(encoded);
        goto error;
    }
    if (buffer) {
        result = PyObject_CallMethodOneArg(buffer, &_Py_ID(write), encoded);
        Py_DECREF(buffer);
        Py_DECREF(encoded);
        if (result == NULL)
            goto error;
        Py_DECREF(result);
    }
    else {
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

/*[clinic input]
sys.displayhook

    object as o: object
    /

Print an object to sys.stdout and also save it in builtins._
[clinic start generated code]*/

static PyObject *
sys_displayhook(PyObject *module, PyObject *o)
/*[clinic end generated code: output=347477d006df92ed input=08ba730166d7ef72]*/
{
    PyObject *outf;
    PyObject *builtins;
    PyThreadState *tstate = _PyThreadState_GET();

    builtins = PyImport_GetModule(&_Py_ID(builtins));
    if (builtins == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "lost builtins module");
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
    if (PyObject_SetAttr(builtins, _Py_LATIN1_CHR('_'), Py_None) != 0)
        return NULL;
    outf = PySys_GetAttr(&_Py_ID(stdout));
    if (outf == NULL) {
        return NULL;
    }
    if (outf == Py_None) {
        _PyErr_SetString(tstate, PyExc_RuntimeError, "lost sys.stdout");
        Py_DECREF(outf);
        return NULL;
    }
    if (PyFile_WriteObject(o, outf, 0) != 0) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_UnicodeEncodeError)) {
            int err;
            /* repr(o) is not encodable to sys.stdout.encoding with
             * sys.stdout.errors error handler (which is probably 'strict') */
            _PyErr_Clear(tstate);
            err = sys_displayhook_unencodable(outf, o);
            if (err) {
                Py_DECREF(outf);
                return NULL;
            }
        }
        else {
            Py_DECREF(outf);
            return NULL;
        }
    }
    if (PyFile_WriteObject(_Py_LATIN1_CHR('\n'), outf, Py_PRINT_RAW) != 0) {
        Py_DECREF(outf);
        return NULL;
    }
    Py_DECREF(outf);
    if (PyObject_SetAttr(builtins, _Py_LATIN1_CHR('_'), o) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
sys.excepthook

    exctype:   object
    value:     object
    traceback: object
    /

Handle an exception by displaying it with a traceback on sys.stderr.
[clinic start generated code]*/

static PyObject *
sys_excepthook_impl(PyObject *module, PyObject *exctype, PyObject *value,
                    PyObject *traceback)
/*[clinic end generated code: output=18d99fdda21b6b5e input=ecf606fa826f19d9]*/
{
    PyErr_Display(NULL, value, traceback);
    Py_RETURN_NONE;
}


/*[clinic input]
sys.exception

Return the current exception.

Return the most recent exception caught by an except clause
in the current stack frame or in an older stack frame, or None
if no such exception exists.
[clinic start generated code]*/

static PyObject *
sys_exception_impl(PyObject *module)
/*[clinic end generated code: output=2381ee2f25953e40 input=c88fbb94b6287431]*/
{
    _PyErr_StackItem *err_info = _PyErr_GetTopmostException(_PyThreadState_GET());
    if (err_info->exc_value != NULL) {
        return Py_NewRef(err_info->exc_value);
    }
    Py_RETURN_NONE;
}


/*[clinic input]
sys.exc_info

Return current exception information: (type, value, traceback).

Return information about the most recent exception caught by an except
clause in the current stack frame or in an older stack frame.
[clinic start generated code]*/

static PyObject *
sys_exc_info_impl(PyObject *module)
/*[clinic end generated code: output=3afd0940cf3a4d30 input=b5c5bf077788a3e5]*/
{
    _PyErr_StackItem *err_info = _PyErr_GetTopmostException(_PyThreadState_GET());
    return _PyErr_StackItemToExcInfoTuple(err_info);
}


/*[clinic input]
sys.unraisablehook

    unraisable: object
    /

Handle an unraisable exception.

The unraisable argument has the following attributes:

* exc_type: Exception type.
* exc_value: Exception value, can be None.
* exc_traceback: Exception traceback, can be None.
* err_msg: Error message, can be None.
* object: Object causing the exception, can be None.
[clinic start generated code]*/

static PyObject *
sys_unraisablehook(PyObject *module, PyObject *unraisable)
/*[clinic end generated code: output=bb92838b32abaa14 input=ec3af148294af8d3]*/
{
    return _PyErr_WriteUnraisableDefaultHook(unraisable);
}


/*[clinic input]
sys.exit

    status: object = None
    /

Exit the interpreter by raising SystemExit(status).

If the status is omitted or None, it defaults to zero (i.e., success).
If the status is an integer, it will be used as the system exit status.
If it is another kind of object, it will be printed and the system
exit status will be one (i.e., failure).
[clinic start generated code]*/

static PyObject *
sys_exit_impl(PyObject *module, PyObject *status)
/*[clinic end generated code: output=13870986c1ab2ec0 input=b86ca9497baa94f2]*/
{
    /* Raise SystemExit so callers may catch it or clean up. */
    PyErr_SetObject(PyExc_SystemExit, status);
    return NULL;
}


static PyObject *
get_utf8_unicode(void)
{
    _Py_DECLARE_STR(utf_8, "utf-8");
    PyObject *ret = &_Py_STR(utf_8);
    return Py_NewRef(ret);
}

/*[clinic input]
sys.getdefaultencoding

Return the current default encoding used by the Unicode implementation.
[clinic start generated code]*/

static PyObject *
sys_getdefaultencoding_impl(PyObject *module)
/*[clinic end generated code: output=256d19dfcc0711e6 input=d416856ddbef6909]*/
{
    return get_utf8_unicode();
}

/*[clinic input]
sys.getfilesystemencoding

Return the encoding used to convert Unicode filenames to OS filenames.
[clinic start generated code]*/

static PyObject *
sys_getfilesystemencoding_impl(PyObject *module)
/*[clinic end generated code: output=1dc4bdbe9be44aa7 input=8475f8649b8c7d8c]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    if (wcscmp(config->filesystem_encoding, L"utf-8") == 0) {
        return get_utf8_unicode();
    }

    PyObject *u = PyUnicode_FromWideChar(config->filesystem_encoding, -1);
    if (u == NULL) {
        return NULL;
    }
    _PyUnicode_InternImmortal(interp, &u);
    return u;
}

/*[clinic input]
sys.getfilesystemencodeerrors

Return the error mode used Unicode to OS filename conversion.
[clinic start generated code]*/

static PyObject *
sys_getfilesystemencodeerrors_impl(PyObject *module)
/*[clinic end generated code: output=ba77b36bbf7c96f5 input=22a1e8365566f1e5]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    PyObject *u = PyUnicode_FromWideChar(config->filesystem_errors, -1);
    if (u == NULL) {
        return NULL;
    }
    _PyUnicode_InternImmortal(interp, &u);
    return u;
}

/*[clinic input]
sys.intern

    string as s: unicode
    /

``Intern'' the given string.

This enters the string in the (global) table of interned strings whose
purpose is to speed up dictionary lookups. Return the string itself or
the previously interned string object with the same value.
[clinic start generated code]*/

static PyObject *
sys_intern_impl(PyObject *module, PyObject *s)
/*[clinic end generated code: output=be680c24f5c9e5d6 input=849483c006924e2f]*/
{
    if (PyUnicode_CheckExact(s)) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        Py_INCREF(s);
        _PyUnicode_InternMortal(interp, &s);
        return s;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "can't intern %.400s", Py_TYPE(s)->tp_name);
        return NULL;
    }
}


/*[clinic input]
sys._is_interned -> bool

  string: unicode
  /

Return True if the given string is "interned".
[clinic start generated code]*/

static int
sys__is_interned_impl(PyObject *module, PyObject *string)
/*[clinic end generated code: output=c3678267b4e9d7ed input=039843e17883b606]*/
{
    return PyUnicode_CHECK_INTERNED(string);
}

/*[clinic input]
sys._is_immortal -> bool

  op: object
  /

Return True if the given object is "immortal" per PEP 683.

This function should be used for specialized purposes only.
[clinic start generated code]*/

static int
sys__is_immortal_impl(PyObject *module, PyObject *op)
/*[clinic end generated code: output=c2f5d6a80efb8d1a input=4609c9bf5481db76]*/
{
    return PyUnstable_IsImmortal(op);
}

/*
 * Cached interned string objects used for calling the profile and
 * trace functions.
 */
static PyObject *whatstrings[8] = {
   &_Py_ID(call),
   &_Py_ID(exception),
   &_Py_ID(line),
   &_Py_ID(return),
   &_Py_ID(c_call),
   &_Py_ID(c_exception),
   &_Py_ID(c_return),
   &_Py_ID(opcode),
};


static PyObject *
call_trampoline(PyThreadState *tstate, PyObject* callback,
                PyFrameObject *frame, int what, PyObject *arg)
{
    /* call the Python-level function */
    if (arg == NULL) {
        arg = Py_None;
    }
    PyObject *args[3] = {(PyObject *)frame, whatstrings[what], arg};
    PyObject *result = _PyObject_VectorcallTstate(tstate, callback, args, 3, NULL);

    return result;
}

static int
profile_trampoline(PyObject *self, PyFrameObject *frame,
                   int what, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *result = call_trampoline(tstate, self, frame, what, arg);
    if (result == NULL) {
        _PyEval_SetProfile(tstate, NULL, NULL);
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
    if (what == PyTrace_CALL) {
        callback = self;
    }
    else {
        callback = frame->f_trace;
    }
    if (callback == NULL) {
        return 0;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *result = call_trampoline(tstate, callback, frame, what, arg);
    if (result == NULL) {
        _PyEval_SetTrace(tstate, NULL, NULL);
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

/*[clinic input]
sys.settrace

    function: object
    /

Set the global debug tracing function.

It will be called on each function call.  See the debugger chapter
in the library manual.
[clinic start generated code]*/

static PyObject *
sys_settrace(PyObject *module, PyObject *function)
/*[clinic end generated code: output=999d12e9d6ec4678 input=8107feb01c5f1c4e]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (function == Py_None) {
        if (_PyEval_SetTrace(tstate, NULL, NULL) < 0) {
            return NULL;
        }
    }
    else {
        if (_PyEval_SetTrace(tstate, trace_trampoline, function) < 0) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

/*[clinic input]
sys._settraceallthreads

    function as arg: object
    /

Set the global debug tracing function in all running threads belonging to the current interpreter.

It will be called on each function call. See the debugger chapter
in the library manual.
[clinic start generated code]*/

static PyObject *
sys__settraceallthreads(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=161cca30207bf3ca input=d4bde1f810d73675]*/
{
    PyObject* argument = NULL;
    Py_tracefunc func = NULL;

    if (arg != Py_None) {
        func = trace_trampoline;
        argument = arg;
    }


    PyEval_SetTraceAllThreads(func, argument);

    Py_RETURN_NONE;
}

/*[clinic input]
sys.gettrace

Return the global debug tracing function set with sys.settrace.

See the debugger chapter in the library manual.
[clinic start generated code]*/

static PyObject *
sys_gettrace_impl(PyObject *module)
/*[clinic end generated code: output=e97e3a4d8c971b6e input=373b51bb2147f4d8]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *temp = tstate->c_traceobj;

    if (temp == NULL)
        temp = Py_None;
    return Py_NewRef(temp);
}

/*[clinic input]
sys.setprofile

    function: object
    /

Set the profiling function.

It will be called on each function call and return.  See the profiler
chapter in the library manual.
[clinic start generated code]*/

static PyObject *
sys_setprofile(PyObject *module, PyObject *function)
/*[clinic end generated code: output=1c3503105939db9c input=055d0d7961413a62]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (function == Py_None) {
        if (_PyEval_SetProfile(tstate, NULL, NULL) < 0) {
            return NULL;
        }
    }
    else {
        if (_PyEval_SetProfile(tstate, profile_trampoline, function) < 0) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

/*[clinic input]
sys._setprofileallthreads

    function as arg: object
    /

Set the profiling function in all running threads belonging to the current interpreter.

It will be called on each function call and return.  See the profiler
chapter in the library manual.
[clinic start generated code]*/

static PyObject *
sys__setprofileallthreads(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=2d61319e27b309fe input=a10589439ba20cee]*/
{
    PyObject* argument = NULL;
    Py_tracefunc func = NULL;

    if (arg != Py_None) {
        func = profile_trampoline;
        argument = arg;
    }

    PyEval_SetProfileAllThreads(func, argument);

    Py_RETURN_NONE;
}

/*[clinic input]
sys.getprofile

Return the profiling function set with sys.setprofile.

See the profiler chapter in the library manual.
[clinic start generated code]*/

static PyObject *
sys_getprofile_impl(PyObject *module)
/*[clinic end generated code: output=579b96b373448188 input=1b3209d89a32965d]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *temp = tstate->c_profileobj;

    if (temp == NULL)
        temp = Py_None;
    return Py_NewRef(temp);
}


/*[clinic input]
sys.setswitchinterval

    interval: double
    /

Set the ideal thread switching delay inside the Python interpreter.

The actual frequency of switching threads can be lower if the
interpreter executes long sequences of uninterruptible code
(this is implementation-specific and workload-dependent).

The parameter must represent the desired switching delay in seconds
A typical value is 0.005 (5 milliseconds).
[clinic start generated code]*/

static PyObject *
sys_setswitchinterval_impl(PyObject *module, double interval)
/*[clinic end generated code: output=65a19629e5153983 input=561b477134df91d9]*/
{
    if (interval <= 0.0) {
        PyErr_SetString(PyExc_ValueError,
                        "switch interval must be strictly positive");
        return NULL;
    }
    _PyEval_SetSwitchInterval((unsigned long) (1e6 * interval));
    Py_RETURN_NONE;
}


/*[clinic input]
sys.getswitchinterval -> double

Return the current thread switch interval; see sys.setswitchinterval().
[clinic start generated code]*/

static double
sys_getswitchinterval_impl(PyObject *module)
/*[clinic end generated code: output=a38c277c85b5096d input=bdf9d39c0ebbbb6f]*/
{
    return 1e-6 * _PyEval_GetSwitchInterval();
}

/*[clinic input]
sys.setrecursionlimit

    limit as new_limit: int
    /

Set the maximum depth of the Python interpreter stack to n.

This limit prevents infinite recursion from causing an overflow of the C
stack and crashing Python.  The highest possible limit is platform-
dependent.
[clinic start generated code]*/

static PyObject *
sys_setrecursionlimit_impl(PyObject *module, int new_limit)
/*[clinic end generated code: output=35e1c64754800ace input=b0f7a23393924af3]*/
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (new_limit < 1) {
        _PyErr_SetString(tstate, PyExc_ValueError,
                         "recursion limit must be greater or equal than 1");
        return NULL;
    }

    /* Reject too low new limit if the current recursion depth is higher than
       the new low-water mark. */
    int depth = tstate->py_recursion_limit - tstate->py_recursion_remaining;
    if (depth >= new_limit) {
        _PyErr_Format(tstate, PyExc_RecursionError,
                      "cannot set the recursion limit to %i at "
                      "the recursion depth %i: the limit is too low",
                      new_limit, depth);
        return NULL;
    }

    Py_SetRecursionLimit(new_limit);
    Py_RETURN_NONE;
}

/*[clinic input]
sys.set_coroutine_origin_tracking_depth

  depth: int

Enable or disable origin tracking for coroutine objects in this thread.

Coroutine objects will track 'depth' frames of traceback information
about where they came from, available in their cr_origin attribute.

Set a depth of 0 to disable.
[clinic start generated code]*/

static PyObject *
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth)
/*[clinic end generated code: output=0a2123c1cc6759c5 input=a1d0a05f89d2c426]*/
{
    if (_PyEval_SetCoroutineOriginTrackingDepth(depth) < 0) {
        return NULL;
    }
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
    }

    if (firstiter && firstiter != Py_None) {
        if (!PyCallable_Check(firstiter)) {
            PyErr_Format(PyExc_TypeError,
                         "callable firstiter expected, got %.50s",
                         Py_TYPE(firstiter)->tp_name);
            return NULL;
        }
    }

    PyObject *cur_finalizer = _PyEval_GetAsyncGenFinalizer();

    if (finalizer && finalizer != Py_None) {
        if (_PyEval_SetAsyncGenFinalizer(finalizer) < 0) {
            return NULL;
        }
    }
    else if (finalizer == Py_None && _PyEval_SetAsyncGenFinalizer(NULL) < 0) {
        return NULL;
    }

    if (firstiter && firstiter != Py_None) {
        if (_PyEval_SetAsyncGenFirstiter(firstiter) < 0) {
            goto error;
        }
    }
    else if (firstiter == Py_None && _PyEval_SetAsyncGenFirstiter(NULL) < 0) {
        goto error;
    }

    Py_RETURN_NONE;

error:
    _PyEval_SetAsyncGenFinalizer(cur_finalizer);
    return NULL;
}

PyDoc_STRVAR(set_asyncgen_hooks_doc,
"set_asyncgen_hooks([firstiter] [, finalizer])\n\
\n\
Set a finalizer for async generators objects."
);

/*[clinic input]
sys.get_asyncgen_hooks

Return the installed asynchronous generators hooks.

This returns a namedtuple of the form (firstiter, finalizer).
[clinic start generated code]*/

static PyObject *
sys_get_asyncgen_hooks_impl(PyObject *module)
/*[clinic end generated code: output=53a253707146f6cf input=3676b9ea62b14625]*/
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

    PyStructSequence_SET_ITEM(res, 0, Py_NewRef(firstiter));
    PyStructSequence_SET_ITEM(res, 1, Py_NewRef(finalizer));

    return res;
}


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
get_hash_info(PyThreadState *tstate)
{
    PyObject *hash_info;
    int field = 0;
    PyHash_FuncDef *hashfunc;
    hash_info = PyStructSequence_New(&Hash_InfoType);
    if (hash_info == NULL) {
        return NULL;
    }
    hashfunc = PyHash_GetFuncDef();

#define SET_HASH_INFO_ITEM(CALL)                             \
    do {                                                     \
        PyObject *item = (CALL);                             \
        if (item == NULL) {                                  \
            Py_CLEAR(hash_info);                             \
            return NULL;                                     \
        }                                                    \
        PyStructSequence_SET_ITEM(hash_info, field++, item); \
    } while(0)

    SET_HASH_INFO_ITEM(PyLong_FromLong(8 * sizeof(Py_hash_t)));
    SET_HASH_INFO_ITEM(PyLong_FromSsize_t(_PyHASH_MODULUS));
    SET_HASH_INFO_ITEM(PyLong_FromLong(_PyHASH_INF));
    SET_HASH_INFO_ITEM(PyLong_FromLong(0));  // This is no longer used
    SET_HASH_INFO_ITEM(PyLong_FromLong(_PyHASH_IMAG));
    SET_HASH_INFO_ITEM(PyUnicode_FromString(hashfunc->name));
    SET_HASH_INFO_ITEM(PyLong_FromLong(hashfunc->hash_bits));
    SET_HASH_INFO_ITEM(PyLong_FromLong(hashfunc->seed_bits));
    SET_HASH_INFO_ITEM(PyLong_FromLong(Py_HASH_CUTOFF));

#undef SET_HASH_INFO_ITEM

    return hash_info;
}
/*[clinic input]
sys.getrecursionlimit

Return the current value of the recursion limit.

The recursion limit is the maximum depth of the Python interpreter
stack.  This limit prevents infinite recursion from causing an overflow
of the C stack and crashing Python.
[clinic start generated code]*/

static PyObject *
sys_getrecursionlimit_impl(PyObject *module)
/*[clinic end generated code: output=d571fb6b4549ef2e input=1c6129fd2efaeea8]*/
{
    return PyLong_FromLong(Py_GetRecursionLimit());
}

#ifdef MS_WINDOWS

static PyTypeObject WindowsVersionType = { 0 };

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
    "sys.getwindowsversion",       /* name */
    sys_getwindowsversion__doc__,  /* doc */
    windows_version_fields,        /* fields */
    5                              /* For backward compatibility,
                                      only the first 5 items are accessible
                                      via indexing, the rest are name only */
};

static PyObject *
_sys_getwindowsversion_from_kernel32(void)
{
#ifndef MS_WINDOWS_DESKTOP
    PyErr_SetString(PyExc_OSError, "cannot read version info on this platform");
    return NULL;
#else
    HANDLE hKernel32;
    wchar_t kernel32_path[MAX_PATH];
    LPVOID verblock;
    DWORD verblock_size;
    VS_FIXEDFILEINFO *ffi;
    UINT ffi_len;
    DWORD realMajor, realMinor, realBuild;

    Py_BEGIN_ALLOW_THREADS
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    Py_END_ALLOW_THREADS
    if (!hKernel32 || !GetModuleFileNameW(hKernel32, kernel32_path, MAX_PATH)) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
    verblock_size = GetFileVersionInfoSizeW(kernel32_path, NULL);
    if (!verblock_size) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
    verblock = PyMem_RawMalloc(verblock_size);
    if (!verblock ||
        !GetFileVersionInfoW(kernel32_path, 0, verblock_size, verblock) ||
        !VerQueryValueW(verblock, L"", (LPVOID)&ffi, &ffi_len)) {
        PyErr_SetFromWindowsErr(0);
        if (verblock) {
            PyMem_RawFree(verblock);
        }
        return NULL;
    }

    realMajor = HIWORD(ffi->dwProductVersionMS);
    realMinor = LOWORD(ffi->dwProductVersionMS);
    realBuild = HIWORD(ffi->dwProductVersionLS);
    PyMem_RawFree(verblock);
    return Py_BuildValue("(kkk)", realMajor, realMinor, realBuild);
#endif /* !MS_WINDOWS_DESKTOP */
}

/* Disable deprecation warnings about GetVersionEx as the result is
   being passed straight through to the caller, who is responsible for
   using it correctly. */
#pragma warning(push)
#pragma warning(disable:4996)

/*[clinic input]
sys.getwindowsversion

Return info about the running version of Windows as a named tuple.

The members are named: major, minor, build, platform, service_pack,
service_pack_major, service_pack_minor, suite_mask, product_type and
platform_version. For backward compatibility, only the first 5 items
are available by indexing. All elements are numbers, except
service_pack and platform_type which are strings, and platform_version
which is a 3-tuple. Platform is always 2. Product_type may be 1 for a
workstation, 2 for a domain controller, 3 for a server.
Platform_version is a 3-tuple containing a version number that is
intended for identifying the OS rather than feature detection.
[clinic start generated code]*/

static PyObject *
sys_getwindowsversion_impl(PyObject *module)
/*[clinic end generated code: output=1ec063280b932857 input=73a228a328fee63a]*/
{
    PyObject *version;
    int pos = 0;
    OSVERSIONINFOEXW ver;

    if (PyObject_GetOptionalAttrString(module, "_cached_windows_version", &version) < 0) {
        return NULL;
    };
    if (version && PyObject_TypeCheck(version, &WindowsVersionType)) {
        return version;
    }
    Py_XDECREF(version);

    ver.dwOSVersionInfoSize = sizeof(ver);
    if (!GetVersionExW((OSVERSIONINFOW*) &ver))
        return PyErr_SetFromWindowsErr(0);

    version = PyStructSequence_New(&WindowsVersionType);
    if (version == NULL)
        return NULL;

#define SET_VERSION_INFO(CALL)                               \
    do {                                                     \
        PyObject *item = (CALL);                             \
        if (item == NULL) {                                  \
            goto error;                                      \
        }                                                    \
        PyStructSequence_SET_ITEM(version, pos++, item);     \
    } while(0)

    SET_VERSION_INFO(PyLong_FromLong(ver.dwMajorVersion));
    SET_VERSION_INFO(PyLong_FromLong(ver.dwMinorVersion));
    SET_VERSION_INFO(PyLong_FromLong(ver.dwBuildNumber));
    SET_VERSION_INFO(PyLong_FromLong(ver.dwPlatformId));
    SET_VERSION_INFO(PyUnicode_FromWideChar(ver.szCSDVersion, -1));
    SET_VERSION_INFO(PyLong_FromLong(ver.wServicePackMajor));
    SET_VERSION_INFO(PyLong_FromLong(ver.wServicePackMinor));
    SET_VERSION_INFO(PyLong_FromLong(ver.wSuiteMask));
    SET_VERSION_INFO(PyLong_FromLong(ver.wProductType));

    // GetVersion will lie if we are running in a compatibility mode.
    // We need to read the version info from a system file resource
    // to accurately identify the OS version. If we fail for any reason,
    // just return whatever GetVersion said.
    PyObject *realVersion = _sys_getwindowsversion_from_kernel32();
    if (!realVersion) {
        if (!PyErr_ExceptionMatches(PyExc_WindowsError)) {
            return NULL;
        }

        PyErr_Clear();
        realVersion = Py_BuildValue("(kkk)",
            ver.dwMajorVersion,
            ver.dwMinorVersion,
            ver.dwBuildNumber
        );
    }

    SET_VERSION_INFO(realVersion);

#undef SET_VERSION_INFO

    if (PyObject_SetAttrString(module, "_cached_windows_version", version) < 0) {
        goto error;
    }

    return version;

error:
    Py_DECREF(version);
    return NULL;
}

#pragma warning(pop)

/*[clinic input]
sys._enablelegacywindowsfsencoding

Changes the default filesystem encoding to mbcs:replace.

This is done for consistency with earlier versions of Python. See PEP
529 for more information.

This is equivalent to defining the PYTHONLEGACYWINDOWSFSENCODING
environment variable before launching Python.
[clinic start generated code]*/

static PyObject *
sys__enablelegacywindowsfsencoding_impl(PyObject *module)
/*[clinic end generated code: output=f5c3855b45e24fe9 input=2bfa931a20704492]*/
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
        "sys._enablelegacywindowsfsencoding() is deprecated and will be "
        "removed in Python 3.16. Use PYTHONLEGACYWINDOWSFSENCODING "
        "instead.", 1))
    {
        return NULL;
    }
    if (_PyUnicode_EnableLegacyWindowsFSEncoding() < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

#endif /* MS_WINDOWS */

#ifdef HAVE_DLOPEN

/*[clinic input]
sys.setdlopenflags

    flags as new_val: int
    /

Set the flags used by the interpreter for dlopen calls.

This is used, for example, when the interpreter loads extension
modules. Among other things, this will enable a lazy resolving of
symbols when importing a module, if called as sys.setdlopenflags(0).
To share symbols across extension modules, call as
sys.setdlopenflags(os.RTLD_GLOBAL).  Symbolic names for the flag
modules can be found in the os module (RTLD_xxx constants, e.g.
os.RTLD_LAZY).
[clinic start generated code]*/

static PyObject *
sys_setdlopenflags_impl(PyObject *module, int new_val)
/*[clinic end generated code: output=ec918b7fe0a37281 input=4c838211e857a77f]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyImport_SetDLOpenFlags(interp, new_val);
    Py_RETURN_NONE;
}


/*[clinic input]
sys.getdlopenflags

Return the current value of the flags that are used for dlopen calls.

The flag constants are defined in the os module.
[clinic start generated code]*/

static PyObject *
sys_getdlopenflags_impl(PyObject *module)
/*[clinic end generated code: output=e92cd1bc5005da6e input=dc4ea0899c53b4b6]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return PyLong_FromLong(
            _PyImport_GetDLOpenFlags(interp));
}

#endif  /* HAVE_DLOPEN */

#ifdef USE_MALLOPT
/* Link with -lmalloc (or -lmpc) on an SGI */
#include <malloc.h>

/*[clinic input]
sys.mdebug

    flag: int
    /
[clinic start generated code]*/

static PyObject *
sys_mdebug_impl(PyObject *module, int flag)
/*[clinic end generated code: output=5431d545847c3637 input=151d150ae1636f8a]*/
{
    int flag;
    mallopt(M_DEBUG, flag);
    Py_RETURN_NONE;
}
#endif /* USE_MALLOPT */


/*[clinic input]
sys.get_int_max_str_digits

Return the maximum string digits limit for non-binary int<->str conversions.
[clinic start generated code]*/

static PyObject *
sys_get_int_max_str_digits_impl(PyObject *module)
/*[clinic end generated code: output=0042f5e8ae0e8631 input=61bf9f99bc8b112d]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return PyLong_FromLong(interp->long_state.max_str_digits);
}


/*[clinic input]
sys.set_int_max_str_digits

    maxdigits: int

Set the maximum string digits limit for non-binary int<->str conversions.
[clinic start generated code]*/

static PyObject *
sys_set_int_max_str_digits_impl(PyObject *module, int maxdigits)
/*[clinic end generated code: output=734d4c2511f2a56d input=d7e3f325db6910c5]*/
{
    if (_PySys_SetIntMaxStrDigits(maxdigits) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

size_t
_PySys_GetSizeOf(PyObject *o)
{
    PyObject *res = NULL;
    PyObject *method;
    Py_ssize_t size;
    PyThreadState *tstate = _PyThreadState_GET();

    /* Make sure the type is initialized. float gets initialized late */
    if (PyType_Ready(Py_TYPE(o)) < 0) {
        return (size_t)-1;
    }

    method = _PyObject_LookupSpecial(o, &_Py_ID(__sizeof__));
    if (method == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "Type %.100s doesn't define __sizeof__",
                          Py_TYPE(o)->tp_name);
        }
    }
    else {
        res = _PyObject_CallNoArgs(method);
        Py_DECREF(method);
    }

    if (res == NULL)
        return (size_t)-1;

    size = PyLong_AsSsize_t(res);
    Py_DECREF(res);
    if (size == -1 && _PyErr_Occurred(tstate))
        return (size_t)-1;

    if (size < 0) {
        _PyErr_SetString(tstate, PyExc_ValueError,
                          "__sizeof__() should return >= 0");
        return (size_t)-1;
    }

    size_t presize = 0;
    if (!Py_IS_TYPE(o, &PyType_Type) ||
         PyType_HasFeature((PyTypeObject *)o, Py_TPFLAGS_HEAPTYPE))
    {
        /* Add the size of the pre-header if "o" is not a static type */
        presize = _PyType_PreHeaderSize(Py_TYPE(o));
    }

    return (size_t)size + presize;
}

static PyObject *
sys_getsizeof(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"object", "default", 0};
    size_t size;
    PyObject *o, *dflt = NULL;
    PyThreadState *tstate = _PyThreadState_GET();

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O:getsizeof",
                                     kwlist, &o, &dflt)) {
        return NULL;
    }

    size = _PySys_GetSizeOf(o);

    if (size == (size_t)-1 && _PyErr_Occurred(tstate)) {
        /* Has a default value been given */
        if (dflt != NULL && _PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_Clear(tstate);
            return Py_NewRef(dflt);
        }
        else
            return NULL;
    }

    return PyLong_FromSize_t(size);
}

PyDoc_STRVAR(getsizeof_doc,
"getsizeof(object [, default]) -> int\n\
\n\
Return the size of object in bytes.");

/*[clinic input]
sys.getrefcount -> Py_ssize_t

    object:  object
    /

Return the reference count of object.

The count returned is generally one higher than you might expect,
because it includes the (temporary) reference as an argument to
getrefcount().
[clinic start generated code]*/

static Py_ssize_t
sys_getrefcount_impl(PyObject *module, PyObject *object)
/*[clinic end generated code: output=5fd477f2264b85b2 input=bf474efd50a21535]*/
{
    return Py_REFCNT(object);
}

#ifdef Py_REF_DEBUG
/*[clinic input]
sys.gettotalrefcount -> Py_ssize_t
[clinic start generated code]*/

static Py_ssize_t
sys_gettotalrefcount_impl(PyObject *module)
/*[clinic end generated code: output=4103886cf17c25bc input=53b744faa5d2e4f6]*/
{
    /* It may make sense to return the total for the current interpreter
       or have a second function that does so. */
    return _Py_GetGlobalRefTotal();
}

#endif /* Py_REF_DEBUG */

/*[clinic input]
sys.getallocatedblocks -> Py_ssize_t

Return the number of memory blocks currently allocated.
[clinic start generated code]*/

static Py_ssize_t
sys_getallocatedblocks_impl(PyObject *module)
/*[clinic end generated code: output=f0c4e873f0b6dcf7 input=dab13ee346a0673e]*/
{
    // It might make sense to return the count
    // for just the current interpreter.
    return _Py_GetGlobalAllocatedBlocks();
}

/*[clinic input]
sys.getunicodeinternedsize -> Py_ssize_t

    *
    _only_immortal: bool = False

Return the number of elements of the unicode interned dictionary
[clinic start generated code]*/

static Py_ssize_t
sys_getunicodeinternedsize_impl(PyObject *module, int _only_immortal)
/*[clinic end generated code: output=29a6377a94a14f70 input=0330b3408dd5bcc6]*/
{
    if (_only_immortal) {
        return _PyUnicode_InternedSize_Immortal();
    }
    else {
        return _PyUnicode_InternedSize();
    }
}

/*[clinic input]
sys._getframe

    depth: int = 0
    /

Return a frame object from the call stack.

If optional integer depth is given, return the frame object that many
calls below the top of the stack.  If that is deeper than the call
stack, ValueError is raised.  The default for depth is zero, returning
the frame at the top of the call stack.

This function should be used for internal and specialized purposes
only.
[clinic start generated code]*/

static PyObject *
sys__getframe_impl(PyObject *module, int depth)
/*[clinic end generated code: output=d438776c04d59804 input=c1be8a6464b11ee5]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *frame = tstate->current_frame;

    if (frame != NULL) {
        while (depth > 0) {
            frame = _PyFrame_GetFirstComplete(frame->previous);
            if (frame == NULL) {
                break;
            }
            --depth;
        }
    }
    if (frame == NULL) {
        _PyErr_SetString(tstate, PyExc_ValueError,
                         "call stack is not deep enough");
        return NULL;
    }

    PyObject *pyFrame = Py_XNewRef((PyObject *)_PyFrame_GetFrameObject(frame));
    if (pyFrame && _PySys_Audit(tstate, "sys._getframe", "(O)", pyFrame) < 0) {
        Py_DECREF(pyFrame);
        return NULL;
    }
    return pyFrame;
}

/*[clinic input]
sys._current_frames

Return a dict mapping each thread's thread id to its current stack frame.

This function should be used for specialized purposes only.
[clinic start generated code]*/

static PyObject *
sys__current_frames_impl(PyObject *module)
/*[clinic end generated code: output=d2a41ac0a0a3809a input=2a9049c5f5033691]*/
{
    return _PyThread_CurrentFrames();
}

/*[clinic input]
sys._current_exceptions

Return a dict mapping each thread's identifier to its current raised exception.

This function should be used for specialized purposes only.
[clinic start generated code]*/

static PyObject *
sys__current_exceptions_impl(PyObject *module)
/*[clinic end generated code: output=2ccfd838c746f0ba input=0e91818fbf2edc1f]*/
{
    return _PyThread_CurrentExceptions();
}

/*[clinic input]
sys.call_tracing

    func: object
    args as funcargs: object(subclass_of='&PyTuple_Type')
    /

Call func(*args), while tracing is enabled.

The tracing state is saved, and restored afterwards.  This is intended
to be called from a debugger from a checkpoint, to recursively debug
some other code.
[clinic start generated code]*/

static PyObject *
sys_call_tracing_impl(PyObject *module, PyObject *func, PyObject *funcargs)
/*[clinic end generated code: output=7e4999853cd4e5a6 input=5102e8b11049f92f]*/
{
    return _PyEval_CallTracing(func, funcargs);
}

/*[clinic input]
sys._debugmallocstats

Print summary info to stderr about the state of pymalloc's structures.

In Py_DEBUG mode, also perform some expensive internal consistency
checks.
[clinic start generated code]*/

static PyObject *
sys__debugmallocstats_impl(PyObject *module)
/*[clinic end generated code: output=ec3565f8c7cee46a input=33c0c9c416f98424]*/
{
#ifdef WITH_PYMALLOC
    if (_PyObject_DebugMallocStats(stderr)) {
        fputc('\n', stderr);
    }
#endif
    _PyObject_DebugTypeStats(stderr);

    Py_RETURN_NONE;
}

#ifdef Py_TRACE_REFS
/* Defined in objects.c because it uses static globals in that file */
extern PyObject *_Py_GetObjects(PyObject *, PyObject *);
#endif


/*[clinic input]
sys._clear_type_cache

Clear the internal type lookup cache.
[clinic start generated code]*/

static PyObject *
sys__clear_type_cache_impl(PyObject *module)
/*[clinic end generated code: output=20e48ca54a6f6971 input=127f3e04a8d9b555]*/
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "sys._clear_type_cache() is deprecated and"
                     " scheduled for removal in a future version."
                     " Use sys._clear_internal_caches() instead.",
                     1) < 0)
    {
        return NULL;
    }
    PyType_ClearCache();
    Py_RETURN_NONE;
}

/*[clinic input]
sys._clear_internal_caches

Clear all internal performance-related caches.
[clinic start generated code]*/

static PyObject *
sys__clear_internal_caches_impl(PyObject *module)
/*[clinic end generated code: output=0ee128670a4966d6 input=253e741ca744f6e8]*/
{
#ifdef _Py_TIER2
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _Py_Executors_InvalidateAll(interp, 0);
#endif
#ifdef Py_GIL_DISABLED
    if (_Py_ClearUnusedTLBC(_PyInterpreterState_GET()) < 0) {
        return NULL;
    }
#endif
    PyType_ClearCache();
    Py_RETURN_NONE;
}

/* Note that, for now, we do not have a per-interpreter equivalent
  for sys.is_finalizing(). */

/*[clinic input]
sys.is_finalizing

Return True if Python is exiting.
[clinic start generated code]*/

static PyObject *
sys_is_finalizing_impl(PyObject *module)
/*[clinic end generated code: output=735b5ff7962ab281 input=f0df747a039948a5]*/
{
    return PyBool_FromLong(Py_IsFinalizing());
}


#ifdef Py_STATS
/*[clinic input]
sys._stats_on

Turns on stats gathering (stats gathering is off by default).
[clinic start generated code]*/

static PyObject *
sys__stats_on_impl(PyObject *module)
/*[clinic end generated code: output=aca53eafcbb4d9fe input=43b5bfe145299e55]*/
{
    _Py_StatsOn();
    Py_RETURN_NONE;
}

/*[clinic input]
sys._stats_off

Turns off stats gathering (stats gathering is off by default).
[clinic start generated code]*/

static PyObject *
sys__stats_off_impl(PyObject *module)
/*[clinic end generated code: output=1534c1ee63812214 input=d1a84c60c56cbce2]*/
{
    _Py_StatsOff();
    Py_RETURN_NONE;
}

/*[clinic input]
sys._stats_clear

Clears the stats.
[clinic start generated code]*/

static PyObject *
sys__stats_clear_impl(PyObject *module)
/*[clinic end generated code: output=fb65a2525ee50604 input=3e03f2654f44da96]*/
{
    _Py_StatsClear();
    Py_RETURN_NONE;
}

/*[clinic input]
sys._stats_dump -> bool

Dump stats to file, and clears the stats.

Return False if no statistics were not dumped because stats gathering was off.
[clinic start generated code]*/

static int
sys__stats_dump_impl(PyObject *module)
/*[clinic end generated code: output=6e346b4ba0de4489 input=31a489e39418b2a5]*/
{
    int res = _Py_PrintSpecializationStats(1);
    _Py_StatsClear();
    return res;
}
#endif   // Py_STATS


#ifdef ANDROID_API_LEVEL
/*[clinic input]
sys.getandroidapilevel

Return the build time API version of Android as an integer.
[clinic start generated code]*/

static PyObject *
sys_getandroidapilevel_impl(PyObject *module)
/*[clinic end generated code: output=214abf183a1c70c1 input=3e6d6c9fcdd24ac6]*/
{
    return PyLong_FromLong(ANDROID_API_LEVEL);
}
#endif   /* ANDROID_API_LEVEL */

/*[clinic input]
sys.activate_stack_trampoline

    backend: str
    /

Activate stack profiler trampoline *backend*.
[clinic start generated code]*/

static PyObject *
sys_activate_stack_trampoline_impl(PyObject *module, const char *backend)
/*[clinic end generated code: output=5783cdeb51874b43 input=a12df928758a82b4]*/
{
#ifdef PY_HAVE_PERF_TRAMPOLINE
#ifdef _Py_JIT
    if (_PyInterpreterState_GET()->jit) {
        PyErr_SetString(PyExc_ValueError, "Cannot activate the perf trampoline if the JIT is active");
        return NULL;
    }
#endif

    if (strcmp(backend, "perf") == 0) {
        _PyPerf_Callbacks cur_cb;
        _PyPerfTrampoline_GetCallbacks(&cur_cb);
        if (cur_cb.write_state != _Py_perfmap_callbacks.write_state) {
            if (_PyPerfTrampoline_SetCallbacks(&_Py_perfmap_callbacks) < 0 ) {
                PyErr_SetString(PyExc_ValueError, "can't activate perf trampoline");
                return NULL;
            }
        }
        else if (strcmp(backend, "perf_jit") == 0) {
            _PyPerf_Callbacks cur_cb;
            _PyPerfTrampoline_GetCallbacks(&cur_cb);
            if (cur_cb.write_state != _Py_perfmap_jit_callbacks.write_state) {
                if (_PyPerfTrampoline_SetCallbacks(&_Py_perfmap_jit_callbacks) < 0 ) {
                    PyErr_SetString(PyExc_ValueError, "can't activate perf jit trampoline");
                    return NULL;
                }
            }
        }
    }
    else {
        PyErr_Format(PyExc_ValueError, "invalid backend: %s", backend);
        return NULL;
    }
    if (_PyPerfTrampoline_Init(1) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_ValueError, "perf trampoline not available");
    return NULL;
#endif
}


/*[clinic input]
sys.deactivate_stack_trampoline

Deactivate the current stack profiler trampoline backend.

If no stack profiler is activated, this function has no effect.
[clinic start generated code]*/

static PyObject *
sys_deactivate_stack_trampoline_impl(PyObject *module)
/*[clinic end generated code: output=b50da25465df0ef1 input=9f629a6be9fe7fc8]*/
{
    if  (_PyPerfTrampoline_Init(0) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
sys.is_stack_trampoline_active

Return *True* if a stack profiler trampoline is active.
[clinic start generated code]*/

static PyObject *
sys_is_stack_trampoline_active_impl(PyObject *module)
/*[clinic end generated code: output=ab2746de0ad9d293 input=29616b7bf6a0b703]*/
{
#ifdef PY_HAVE_PERF_TRAMPOLINE
    if (_PyIsPerfTrampolineActive()) {
        Py_RETURN_TRUE;
    }
#endif
    Py_RETURN_FALSE;
}


/*[clinic input]
sys.is_remote_debug_enabled

Return True if remote debugging is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
sys_is_remote_debug_enabled_impl(PyObject *module)
/*[clinic end generated code: output=7ca3d38bdd5935eb input=7335c4a2fe8cf4f3]*/
{
#if !defined(Py_REMOTE_DEBUG) || !defined(Py_SUPPORTS_REMOTE_DEBUG)
    Py_RETURN_FALSE;
#else
    const PyConfig *config = _Py_GetConfig();
    return PyBool_FromLong(config->remote_debug);
#endif
}

/*[clinic input]
sys.remote_exec

    pid: int
    script: object

Executes a file containing Python code in a given remote Python process.

This function returns immediately, and the code will be executed by the
target process's main thread at the next available opportunity, similarly
to how signals are handled. There is no interface to determine when the
code has been executed. The caller is responsible for making sure that
the file still exists whenever the remote process tries to read it and that
it hasn't been overwritten.

The remote process must be running a CPython interpreter of the same major
and minor version as the local process. If either the local or remote
interpreter is pre-release (alpha, beta, or release candidate) then the
local and remote interpreters must be the same exact version.

Args:
     pid (int): The process ID of the target Python process.
     script (str|bytes): The path to a file containing
         the Python code to be executed.
[clinic start generated code]*/

static PyObject *
sys_remote_exec_impl(PyObject *module, int pid, PyObject *script)
/*[clinic end generated code: output=7d94c56afe4a52c0 input=39908ca2c5fe1eb0]*/
{
    PyObject *path;
    const char *debugger_script_path;

    if (PyUnicode_FSConverter(script, &path) == 0) {
        return NULL;
    }

    if (PySys_Audit("sys.remote_exec", "iO", pid, script) < 0) {
        return NULL;
    }

    debugger_script_path = PyBytes_AS_STRING(path);
#ifdef MS_WINDOWS
    PyObject *unicode_path;
    if (PyUnicode_FSDecoder(path, &unicode_path) < 0) {
        goto error;
    }
    // Use UTF-16 (wide char) version of the path for permission checks
    wchar_t *debugger_script_path_w = PyUnicode_AsWideCharString(unicode_path, NULL);
    Py_DECREF(unicode_path);
    if (debugger_script_path_w == NULL) {
        goto error;
    }
    DWORD attr = GetFileAttributesW(debugger_script_path_w);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        PyMem_Free(debugger_script_path_w);
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            PyErr_SetString(PyExc_FileNotFoundError, "Script file does not exist");
        }
        else if (err == ERROR_ACCESS_DENIED) {
            PyErr_SetString(PyExc_PermissionError, "Script file cannot be read");
        }
        else {
            PyErr_SetFromWindowsErr(err);
        }
        goto error;
    }
    PyMem_Free(debugger_script_path_w);
#else // MS_WINDOWS
    if (access(debugger_script_path, F_OK | R_OK) != 0) {
        switch (errno) {
            case ENOENT:
                PyErr_SetString(PyExc_FileNotFoundError, "Script file does not exist");
                break;
            case EACCES:
                PyErr_SetString(PyExc_PermissionError, "Script file cannot be read");
                break;
            default:
                PyErr_SetFromErrno(PyExc_OSError);
        }
        goto error;
    }
#endif // MS_WINDOWS
    if (_PySysRemoteDebug_SendExec(pid, 0, debugger_script_path) < 0) {
        goto error;
    }

    Py_DECREF(path);
    Py_RETURN_NONE;

error:
    Py_DECREF(path);
    return NULL;
}



/*[clinic input]
sys._dump_tracelets

    outpath: object

Dump the graph of tracelets in graphviz format
[clinic start generated code]*/

static PyObject *
sys__dump_tracelets_impl(PyObject *module, PyObject *outpath)
/*[clinic end generated code: output=a7fe265e2bc3b674 input=5bff6880cd28ffd1]*/
{
    FILE *out = Py_fopen(outpath, "wb");
    if (out == NULL) {
        return NULL;
    }
    int err = _PyDumpExecutors(out);
    fclose(out);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
sys._getframemodulename

    depth: int = 0

Return the name of the module for a calling frame.

The default depth returns the module containing the call to this API.
A more typical use in a library will pass a depth of 1 to get the user's
module rather than the library module.

If no frame, module, or name can be found, returns None.
[clinic start generated code]*/

static PyObject *
sys__getframemodulename_impl(PyObject *module, int depth)
/*[clinic end generated code: output=1d70ef691f09d2db input=d4f1a8ed43b8fb46]*/
{
    if (PySys_Audit("sys._getframemodulename", "i", depth) < 0) {
        return NULL;
    }
    _PyInterpreterFrame *f = _PyThreadState_GET()->current_frame;
    while (f && (_PyFrame_IsIncomplete(f) || depth-- > 0)) {
        f = f->previous;
    }
    if (f == NULL || PyStackRef_IsNull(f->f_funcobj)) {
        Py_RETURN_NONE;
    }
    PyObject *func = PyStackRef_AsPyObjectBorrow(f->f_funcobj);
    PyObject *r = PyFunction_GetModule(func);
    if (!r) {
        PyErr_Clear();
        r = Py_None;
    }
    return Py_NewRef(r);
}

/*[clinic input]
sys._get_cpu_count_config -> int

Private function for getting PyConfig.cpu_count
[clinic start generated code]*/

static int
sys__get_cpu_count_config_impl(PyObject *module)
/*[clinic end generated code: output=36611bb5efad16dc input=523e1ade2204084e]*/
{
    const PyConfig *config = _Py_GetConfig();
    return config->cpu_count;
}

/*[clinic input]
sys._baserepl

Private function for getting the base REPL
[clinic start generated code]*/

static PyObject *
sys__baserepl_impl(PyObject *module)
/*[clinic end generated code: output=f19a36375ebe0a45 input=ade0ebb9fab56f3c]*/
{
    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    PyRun_AnyFileExFlags(stdin, "<stdin>", 0, &cf);
    Py_RETURN_NONE;
}

/*[clinic input]
sys._is_gil_enabled -> bool

Return True if the GIL is currently enabled and False otherwise.
[clinic start generated code]*/

static int
sys__is_gil_enabled_impl(PyObject *module)
/*[clinic end generated code: output=57732cf53f5b9120 input=7e9c47f15a00e809]*/
{
#ifdef Py_GIL_DISABLED
    return _PyEval_IsGILEnabled(_PyThreadState_GET());
#else
    return 1;
#endif
}


#ifndef MS_WINDOWS
static PerfMapState perf_map_state;
#endif

PyAPI_FUNC(int) PyUnstable_PerfMapState_Init(void) {
#ifndef MS_WINDOWS
    char filename[100];
    pid_t pid = getpid();
    // Use nofollow flag to prevent symlink attacks.
    int flags = O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    snprintf(filename, sizeof(filename) - 1, "/tmp/perf-%jd.map",
                (intmax_t)pid);
    int fd = open(filename, flags, 0600);
    if (fd == -1) {
        return -1;
    }
    else{
        perf_map_state.perf_map = fdopen(fd, "a");
        if (perf_map_state.perf_map == NULL) {
            close(fd);
            return -1;
        }
    }
    perf_map_state.map_lock = PyThread_allocate_lock();
    if (perf_map_state.map_lock == NULL) {
        fclose(perf_map_state.perf_map);
        return -2;
    }
#endif
    return 0;
}

PyAPI_FUNC(int) PyUnstable_WritePerfMapEntry(
    const void *code_addr,
    unsigned int code_size,
    const char *entry_name
) {
#ifndef MS_WINDOWS
    if (perf_map_state.perf_map == NULL) {
        int ret = PyUnstable_PerfMapState_Init();
        if (ret != 0){
            return ret;
        }
    }
    PyThread_acquire_lock(perf_map_state.map_lock, 1);
    fprintf(perf_map_state.perf_map, "%" PRIxPTR " %x %s\n", (uintptr_t) code_addr, code_size, entry_name);
    fflush(perf_map_state.perf_map);
    PyThread_release_lock(perf_map_state.map_lock);
#endif
    return 0;
}

PyAPI_FUNC(void) PyUnstable_PerfMapState_Fini(void) {
#ifndef MS_WINDOWS
    if (perf_map_state.perf_map != NULL) {
        // close the file
        PyThread_acquire_lock(perf_map_state.map_lock, 1);
        fclose(perf_map_state.perf_map);
        PyThread_release_lock(perf_map_state.map_lock);

        // clean up the lock and state
        PyThread_free_lock(perf_map_state.map_lock);
        perf_map_state.perf_map = NULL;
    }
#endif
}

PyAPI_FUNC(int) PyUnstable_CopyPerfMapFile(const char* parent_filename) {
#ifndef MS_WINDOWS
    if (perf_map_state.perf_map == NULL) {
        int ret = PyUnstable_PerfMapState_Init();
        if (ret != 0) {
            return ret;
        }
    }
    FILE* from = fopen(parent_filename, "r");
    if (!from) {
        return -1;
    }
    char buf[4096];
    PyThread_acquire_lock(perf_map_state.map_lock, 1);
    int fflush_result = 0, result = 0;
    while (1) {
        size_t bytes_read = fread(buf, 1, sizeof(buf), from);
        size_t bytes_written = fwrite(buf, 1, bytes_read, perf_map_state.perf_map);
        fflush_result = fflush(perf_map_state.perf_map);
        if (fflush_result != 0 || bytes_read == 0 || bytes_written < bytes_read) {
            result = -1;
            goto close_and_release;
        }
        if (bytes_read < sizeof(buf) && feof(from)) {
            goto close_and_release;
        }
    }
close_and_release:
    fclose(from);
    PyThread_release_lock(perf_map_state.map_lock);
    return result;
#endif
    return 0;
}


static PyMethodDef sys_methods[] = {
    /* Might as well keep this in alphabetic order */
    SYS_ADDAUDITHOOK_METHODDEF
    SYS_AUDIT_METHODDEF
    {"breakpointhook", _PyCFunction_CAST(sys_breakpointhook),
     METH_FASTCALL | METH_KEYWORDS, breakpointhook_doc},
    SYS__CLEAR_INTERNAL_CACHES_METHODDEF
    SYS__CLEAR_TYPE_CACHE_METHODDEF
    SYS__CURRENT_FRAMES_METHODDEF
    SYS__CURRENT_EXCEPTIONS_METHODDEF
    SYS_DISPLAYHOOK_METHODDEF
    SYS_EXCEPTION_METHODDEF
    SYS_EXC_INFO_METHODDEF
    SYS_EXCEPTHOOK_METHODDEF
    SYS_EXIT_METHODDEF
    SYS_GETDEFAULTENCODING_METHODDEF
    SYS_GETDLOPENFLAGS_METHODDEF
    SYS_GETALLOCATEDBLOCKS_METHODDEF
    SYS_GETUNICODEINTERNEDSIZE_METHODDEF
    SYS_GETFILESYSTEMENCODING_METHODDEF
    SYS_GETFILESYSTEMENCODEERRORS_METHODDEF
#ifdef Py_TRACE_REFS
    {"getobjects", _Py_GetObjects, METH_VARARGS},
#endif
    SYS_GETTOTALREFCOUNT_METHODDEF
    SYS_GETREFCOUNT_METHODDEF
    SYS_GETRECURSIONLIMIT_METHODDEF
    {"getsizeof", _PyCFunction_CAST(sys_getsizeof),
     METH_VARARGS | METH_KEYWORDS, getsizeof_doc},
    SYS__GETFRAME_METHODDEF
    SYS__GETFRAMEMODULENAME_METHODDEF
    SYS_GETWINDOWSVERSION_METHODDEF
    SYS__ENABLELEGACYWINDOWSFSENCODING_METHODDEF
    SYS__IS_IMMORTAL_METHODDEF
    SYS_INTERN_METHODDEF
    SYS__IS_INTERNED_METHODDEF
    SYS_IS_FINALIZING_METHODDEF
    SYS_MDEBUG_METHODDEF
    SYS_SETSWITCHINTERVAL_METHODDEF
    SYS_GETSWITCHINTERVAL_METHODDEF
    SYS_SETDLOPENFLAGS_METHODDEF
    SYS_SETPROFILE_METHODDEF
    SYS__SETPROFILEALLTHREADS_METHODDEF
    SYS_GETPROFILE_METHODDEF
    SYS_SETRECURSIONLIMIT_METHODDEF
    SYS_SETTRACE_METHODDEF
    SYS__SETTRACEALLTHREADS_METHODDEF
    SYS_GETTRACE_METHODDEF
    SYS_CALL_TRACING_METHODDEF
    SYS__DEBUGMALLOCSTATS_METHODDEF
    SYS_SET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF
    SYS_GET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF
    {"set_asyncgen_hooks", _PyCFunction_CAST(sys_set_asyncgen_hooks),
     METH_VARARGS | METH_KEYWORDS, set_asyncgen_hooks_doc},
    SYS_GET_ASYNCGEN_HOOKS_METHODDEF
    SYS_GETANDROIDAPILEVEL_METHODDEF
    SYS_ACTIVATE_STACK_TRAMPOLINE_METHODDEF
    SYS_DEACTIVATE_STACK_TRAMPOLINE_METHODDEF
    SYS_IS_STACK_TRAMPOLINE_ACTIVE_METHODDEF
    SYS_IS_REMOTE_DEBUG_ENABLED_METHODDEF
    SYS_REMOTE_EXEC_METHODDEF
    SYS_UNRAISABLEHOOK_METHODDEF
    SYS_GET_INT_MAX_STR_DIGITS_METHODDEF
    SYS_SET_INT_MAX_STR_DIGITS_METHODDEF
    SYS__BASEREPL_METHODDEF
#ifdef Py_STATS
    SYS__STATS_ON_METHODDEF
    SYS__STATS_OFF_METHODDEF
    SYS__STATS_CLEAR_METHODDEF
    SYS__STATS_DUMP_METHODDEF
#endif
    SYS__GET_CPU_COUNT_CONFIG_METHODDEF
    SYS__IS_GIL_ENABLED_METHODDEF
    SYS__DUMP_TRACELETS_METHODDEF
    {NULL, NULL}  // sentinel
};


static PyObject *
list_builtin_module_names(void)
{
    PyObject *list = _PyImport_GetBuiltinModuleNames();
    if (list == NULL) {
        return NULL;
    }
    if (PyList_Sort(list) != 0) {
        goto error;
    }
    PyObject *tuple = PyList_AsTuple(list);
    Py_DECREF(list);
    return tuple;

error:
    Py_DECREF(list);
    return NULL;
}


static PyObject *
list_stdlib_module_names(void)
{
    Py_ssize_t len = Py_ARRAY_LENGTH(_Py_stdlib_module_names);
    PyObject *names = PyTuple_New(len);
    if (names == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *name = PyUnicode_FromString(_Py_stdlib_module_names[i]);
        if (name == NULL) {
            Py_DECREF(names);
            return NULL;
        }
        PyTuple_SET_ITEM(names, i, name);
    }

    PyObject *set = PyObject_CallFunction((PyObject *)&PyFrozenSet_Type,
                                          "(O)", names);
    Py_DECREF(names);
    return set;
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

    /* Use the default allocator, so we can ensure that it also gets used to
     * destroy the linked list in _clear_preinit_entries.
     */
    _Py_PreInitEntry node = _PyMem_DefaultRawCalloc(1, sizeof(*node));
    if (node != NULL) {
        node->value = _PyMem_DefaultRawWcsdup(value);
        if (node->value == NULL) {
            _PyMem_DefaultRawFree(node);
            node = NULL;
        };
    };
    return node;
}

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
}

static void
_clear_preinit_entries(_Py_PreInitEntry *optionlist)
{
    _Py_PreInitEntry current = *optionlist;
    *optionlist = NULL;
    /* Deallocate the nodes and their contents using the default allocator */
    while (current != NULL) {
        _Py_PreInitEntry next = current->next;
        _PyMem_DefaultRawFree(current->value);
        _PyMem_DefaultRawFree(current);
        current = next;
    }
}


PyStatus
_PySys_ReadPreinitWarnOptions(PyWideStringList *options)
{
    PyStatus status;
    _Py_PreInitEntry entry;

    for (entry = _preinit_warnoptions; entry != NULL; entry = entry->next) {
        status = PyWideStringList_Append(options, entry->value);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    _clear_preinit_entries(&_preinit_warnoptions);
    return _PyStatus_OK();
}


PyStatus
_PySys_ReadPreinitXOptions(PyConfig *config)
{
    PyStatus status;
    _Py_PreInitEntry entry;

    for (entry = _preinit_xoptions; entry != NULL; entry = entry->next) {
        status = PyWideStringList_Append(&config->xoptions, entry->value);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    _clear_preinit_entries(&_preinit_xoptions);
    return _PyStatus_OK();
}


static PyObject *
get_warnoptions(PyThreadState *tstate)
{
    PyObject *warnoptions;
    if (PySys_GetOptionalAttr(&_Py_ID(warnoptions), &warnoptions) < 0) {
        return NULL;
    }
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
        Py_XDECREF(warnoptions);
        warnoptions = PyList_New(0);
        if (warnoptions == NULL) {
            return NULL;
        }
        if (sys_set_object(tstate->interp, &_Py_ID(warnoptions), warnoptions)) {
            Py_DECREF(warnoptions);
            return NULL;
        }
    }
    return warnoptions;
}

void
PySys_ResetWarnOptions(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        _clear_preinit_entries(&_preinit_warnoptions);
        return;
    }

    PyObject *warnoptions;
    if (PySys_GetOptionalAttr(&_Py_ID(warnoptions), &warnoptions) < 0) {
        PyErr_Clear();
        return;
    }
    if (warnoptions != NULL && PyList_Check(warnoptions)) {
        PyList_SetSlice(warnoptions, 0, PyList_GET_SIZE(warnoptions), NULL);
    }
    Py_XDECREF(warnoptions);
}

static int
_PySys_AddWarnOptionWithError(PyThreadState *tstate, PyObject *option)
{
    assert(tstate != NULL);
    PyObject *warnoptions = get_warnoptions(tstate);
    if (warnoptions == NULL) {
        return -1;
    }
    if (PyList_Append(warnoptions, option)) {
        Py_DECREF(warnoptions);
        return -1;
    }
    Py_DECREF(warnoptions);
    return 0;
}

// Removed in Python 3.13 API, but kept for the stable ABI
PyAPI_FUNC(void)
PySys_AddWarnOptionUnicode(PyObject *option)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);
    assert(!_PyErr_Occurred(tstate));
    if (_PySys_AddWarnOptionWithError(tstate, option) < 0) {
        /* No return value, therefore clear error state if possible */
        _PyErr_Clear(tstate);
    }
}

// Removed in Python 3.13 API, but kept for the stable ABI
PyAPI_FUNC(void)
PySys_AddWarnOption(const wchar_t *s)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        _append_preinit_entry(&_preinit_warnoptions, s);
        return;
    }
    PyObject *unicode;
    unicode = PyUnicode_FromWideChar(s, -1);
    if (unicode == NULL)
        return;
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PySys_AddWarnOptionUnicode(unicode);
_Py_COMP_DIAG_POP
    Py_DECREF(unicode);
}

// Removed in Python 3.13 API, but kept for the stable ABI
PyAPI_FUNC(int)
PySys_HasWarnOptions(void)
{
    PyObject *warnoptions;
    if (PySys_GetOptionalAttr(&_Py_ID(warnoptions), &warnoptions) < 0) {
        PyErr_Clear();
        return 0;
    }
    int r = (warnoptions != NULL && PyList_Check(warnoptions) &&
             PyList_GET_SIZE(warnoptions) > 0);
    Py_XDECREF(warnoptions);
    return r;
}

static PyObject *
get_xoptions(PyThreadState *tstate)
{
    PyObject *xoptions;
    if (PySys_GetOptionalAttr(&_Py_ID(_xoptions), &xoptions) < 0) {
        return NULL;
    }
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
        Py_XDECREF(xoptions);
        xoptions = PyDict_New();
        if (xoptions == NULL) {
            return NULL;
        }
        if (sys_set_object(tstate->interp, &_Py_ID(_xoptions), xoptions)) {
            Py_DECREF(xoptions);
            return NULL;
        }
    }
    return xoptions;
}

static int
_PySys_AddXOptionWithError(const wchar_t *s)
{
    PyObject *name = NULL, *value = NULL;

    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *opts = get_xoptions(tstate);
    if (opts == NULL) {
        goto error;
    }

    const wchar_t *name_end = wcschr(s, L'=');
    if (!name_end) {
        name = PyUnicode_FromWideChar(s, -1);
        if (name == NULL) {
            goto error;
        }
        value = Py_NewRef(Py_True);
    }
    else {
        name = PyUnicode_FromWideChar(s, name_end - s);
        if (name == NULL) {
            goto error;
        }
        value = PyUnicode_FromWideChar(name_end + 1, -1);
        if (value == NULL) {
            goto error;
        }
    }
    if (PyDict_SetItem(opts, name, value) < 0) {
        goto error;
    }
    Py_DECREF(name);
    Py_DECREF(value);
    Py_DECREF(opts);
    return 0;

error:
    Py_XDECREF(name);
    Py_XDECREF(value);
    Py_XDECREF(opts);
    return -1;
}

// Removed in Python 3.13 API, but kept for the stable ABI
PyAPI_FUNC(void)
PySys_AddXOption(const wchar_t *s)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        _append_preinit_entry(&_preinit_xoptions, s);
        return;
    }
    if (_PySys_AddXOptionWithError(s) < 0) {
        /* No return value, therefore clear error state if possible */
        _PyErr_Clear(tstate);
    }
}

PyObject *
PySys_GetXOptions(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *opts = get_xoptions(tstate);
    Py_XDECREF(opts);
    return opts;
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
last_exc - the last uncaught exception\n\
  Only available in an interactive session after a\n\
  traceback has been printed.\n\
last_type -- type of last uncaught exception\n\
last_value -- value of last uncaught exception\n\
last_traceback -- traceback of last uncaught exception\n\
  These three are the (deprecated) legacy representation of last_exc.\n\
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
"_enablelegacywindowsfsencoding -- [Windows only]\n\
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
exception() -- return the current thread's active exception\n\
exc_info() -- return information about the current thread's active exception\n\
exit() -- exit the interpreter by raising SystemExit\n\
getdlopenflags() -- returns flags to be used for dlopen() calls\n\
getprofile() -- get the global profiling function\n\
getrefcount() -- return the reference count for an object (plus one :-)\n\
getrecursionlimit() -- return the max recursion depth for the interpreter\n\
getsizeof() -- return the size of an object in bytes\n\
gettrace() -- get the global debug tracing function\n\
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
    {"bytes_warning",           "-b"},
    {"quiet",                   "-q"},
    {"hash_randomization",      "-R"},
    {"isolated",                "-I"},
    {"dev_mode",                "-X dev"},
    {"utf8_mode",               "-X utf8"},
    {"warn_default_encoding",   "-X warn_default_encoding"},
    {"safe_path", "-P"},
    {"int_max_str_digits",      "-X int_max_str_digits"},
    {"gil",                     "-X gil"},
    {"thread_inherit_context",  "-X thread_inherit_context"},
    {"context_aware_warnings",    "-X context_aware_warnings"},
    {0}
};

#define SYS_FLAGS_INT_MAX_STR_DIGITS 17

static PyStructSequence_Desc flags_desc = {
    "sys.flags",        /* name */
    flags__doc__,       /* doc */
    flags_fields,       /* fields */
    18
};

static void
sys_set_flag(PyObject *flags, Py_ssize_t pos, PyObject *value)
{
    assert(pos >= 0 && pos < (Py_ssize_t)(Py_ARRAY_LENGTH(flags_fields) - 1));

    PyObject *old_value = PyStructSequence_GET_ITEM(flags, pos);
    PyStructSequence_SET_ITEM(flags, pos, Py_NewRef(value));
    Py_XDECREF(old_value);
}


int
_PySys_SetFlagObj(Py_ssize_t pos, PyObject *value)
{
    PyObject *flags = PySys_GetAttrString("flags");
    if (flags == NULL) {
        return -1;
    }

    sys_set_flag(flags, pos, value);
    Py_DECREF(flags);
    return 0;
}


static int
_PySys_SetFlagInt(Py_ssize_t pos, int value)
{
    PyObject *obj = PyLong_FromLong(value);
    if (obj == NULL) {
        return -1;
    }

    int res = _PySys_SetFlagObj(pos, obj);
    Py_DECREF(obj);
    return res;
}


static int
set_flags_from_config(PyInterpreterState *interp, PyObject *flags)
{
    const PyPreConfig *preconfig = &interp->runtime->preconfig;
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    // _PySys_UpdateConfig() modifies sys.flags in-place:
    // Py_XDECREF() is needed in this case.
    Py_ssize_t pos = 0;
#define SetFlagObj(expr) \
    do { \
        PyObject *value = (expr); \
        if (value == NULL) { \
            return -1; \
        } \
        sys_set_flag(flags, pos, value); \
        Py_DECREF(value); \
        pos++; \
    } while (0)
#define SetFlag(expr) SetFlagObj(PyLong_FromLong(expr))

    SetFlag(config->parser_debug);
    SetFlag(config->inspect);
    SetFlag(config->interactive);
    SetFlag(config->optimization_level);
    SetFlag(!config->write_bytecode);
    SetFlag(!config->user_site_directory);
    SetFlag(!config->site_import);
    SetFlag(!config->use_environment);
    SetFlag(config->verbose);
    SetFlag(config->bytes_warning);
    SetFlag(config->quiet);
    SetFlag(config->use_hash_seed == 0 || config->hash_seed != 0);
    SetFlag(config->isolated);
    SetFlagObj(PyBool_FromLong(config->dev_mode));
    SetFlag(preconfig->utf8_mode);
    SetFlag(config->warn_default_encoding);
    SetFlagObj(PyBool_FromLong(config->safe_path));
    SetFlag(config->int_max_str_digits);
#ifdef Py_GIL_DISABLED
    if (config->enable_gil == _PyConfig_GIL_DEFAULT) {
        SetFlagObj(Py_NewRef(Py_None));
    }
    else {
        SetFlag(config->enable_gil);
    }
#else
    SetFlagObj(PyLong_FromLong(1));
#endif
    SetFlag(config->thread_inherit_context);
    SetFlag(config->context_aware_warnings);
#undef SetFlagObj
#undef SetFlag
    return 0;
}


static PyObject*
make_flags(PyInterpreterState *interp)
{
    PyObject *flags = PyStructSequence_New(&FlagsType);
    if (flags == NULL) {
        return NULL;
    }

    if (set_flags_from_config(interp, flags) < 0) {
        Py_DECREF(flags);
        return NULL;
    }
    return flags;
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
make_version_info(PyThreadState *tstate)
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

    if (_PyErr_Occurred(tstate)) {
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

    // PEP-734
#if defined(__wasi__) || defined(__EMSCRIPTEN__)
    // It is not enabled on WASM builds just yet
    value = Py_False;
#else
    value = Py_True;
#endif
    res = PyDict_SetItemString(impl_info, "supports_isolated_interpreters", value);
    if (res < 0) {
        goto error;
    }

    /* dict ready */

    ns = _PyNamespace_New(impl_info);
    Py_DECREF(impl_info);
    return ns;

error:
    Py_CLEAR(impl_info);
    return NULL;
}

#ifdef __EMSCRIPTEN__

PyDoc_STRVAR(emscripten_info__doc__,
"sys._emscripten_info\n\
\n\
WebAssembly Emscripten platform information.");

static PyTypeObject *EmscriptenInfoType;

static PyStructSequence_Field emscripten_info_fields[] = {
    {"emscripten_version", "Emscripten version (major, minor, micro)"},
    {"runtime", "Runtime (Node.JS version, browser user agent)"},
    {"pthreads", "pthread support"},
    {"shared_memory", "shared memory support"},
    {0}
};

static PyStructSequence_Desc emscripten_info_desc = {
    "sys._emscripten_info",     /* name */
    emscripten_info__doc__ ,    /* doc */
    emscripten_info_fields,     /* fields */
    4
};

EM_JS(char *, _Py_emscripten_runtime, (void), {
    var info;
    if (typeof navigator == 'object') {
        info = navigator.userAgent;
    } else if (typeof process == 'object') {
        info = "Node.js ".concat(process.version);
    } else {
        info = "UNKNOWN";
    }
    var len = lengthBytesUTF8(info) + 1;
    var res = _malloc(len);
    if (res) stringToUTF8(info, res, len);
#if __wasm64__
    return BigInt(res);
#else
    return res;
#endif
});

static PyObject *
make_emscripten_info(void)
{
    PyObject *emscripten_info = NULL;
    PyObject *version = NULL;
    char *ua;
    int pos = 0;

    emscripten_info = PyStructSequence_New(EmscriptenInfoType);
    if (emscripten_info == NULL) {
        return NULL;
    }

    version = Py_BuildValue("(iii)",
        __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
    if (version == NULL) {
        goto error;
    }
    PyStructSequence_SET_ITEM(emscripten_info, pos++, version);

    ua = _Py_emscripten_runtime();
    if (ua != NULL) {
        PyObject *oua = PyUnicode_DecodeUTF8(ua, strlen(ua), "strict");
        free(ua);
        if (oua == NULL) {
            goto error;
        }
        PyStructSequence_SET_ITEM(emscripten_info, pos++, oua);
    } else {
        PyStructSequence_SET_ITEM(emscripten_info, pos++, Py_NewRef(Py_None));
    }

#define SetBoolItem(flag) \
    PyStructSequence_SET_ITEM(emscripten_info, pos++, PyBool_FromLong(flag))

#ifdef __EMSCRIPTEN_PTHREADS__
    SetBoolItem(1);
#else
    SetBoolItem(0);
#endif

#ifdef __EMSCRIPTEN_SHARED_MEMORY__
    SetBoolItem(1);
#else
    SetBoolItem(0);
#endif

#undef SetBoolItem

    if (PyErr_Occurred()) {
        goto error;
    }
    return emscripten_info;

  error:
    Py_CLEAR(emscripten_info);
    return NULL;
}

#endif // __EMSCRIPTEN__

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
#define SET_SYS(key, value)                                \
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

#define SET_SYS_FROM_STRING(key, value) \
        SET_SYS(key, PyUnicode_FromString(value))

static PyStatus
_PySys_InitCore(PyThreadState *tstate, PyObject *sysdict)
{
    PyObject *version_info;
    int res;
    PyInterpreterState *interp = tstate->interp;

    /* stdin/stdout/stderr are set in pylifecycle.c */

#define COPY_SYS_ATTR(tokey, fromkey) \
        SET_SYS(tokey, PyMapping_GetItemString(sysdict, fromkey))

    COPY_SYS_ATTR("__displayhook__", "displayhook");
    COPY_SYS_ATTR("__excepthook__", "excepthook");
    COPY_SYS_ATTR("__breakpointhook__", "breakpointhook");
    COPY_SYS_ATTR("__unraisablehook__", "unraisablehook");

#undef COPY_SYS_ATTR

    SET_SYS_FROM_STRING("version", Py_GetVersion());
    SET_SYS("hexversion", PyLong_FromLong(PY_VERSION_HEX));
    SET_SYS("_git", Py_BuildValue("(szz)", "CPython", _Py_gitidentifier(),
                                  _Py_gitversion()));
    SET_SYS_FROM_STRING("_framework", _PYTHONFRAMEWORK);
    SET_SYS("api_version", PyLong_FromLong(PYTHON_API_VERSION));
    SET_SYS_FROM_STRING("copyright", Py_GetCopyright());
    SET_SYS_FROM_STRING("platform", Py_GetPlatform());
    SET_SYS("maxsize", PyLong_FromSsize_t(PY_SSIZE_T_MAX));
    SET_SYS("float_info", PyFloat_GetInfo());
    SET_SYS("int_info", PyLong_GetInfo());
    /* initialize hash_info */
    if (_PyStructSequence_InitBuiltin(interp, &Hash_InfoType,
                                      &hash_info_desc) < 0)
    {
        goto type_init_failed;
    }
    SET_SYS("hash_info", get_hash_info(tstate));
    SET_SYS("maxunicode", PyLong_FromLong(0x10FFFF));
    SET_SYS("builtin_module_names", list_builtin_module_names());
    SET_SYS("stdlib_module_names", list_stdlib_module_names());
#if PY_BIG_ENDIAN
    SET_SYS_FROM_STRING("byteorder", "big");
#else
    SET_SYS_FROM_STRING("byteorder", "little");
#endif

#ifdef MS_COREDLL
    SET_SYS("dllhandle", PyLong_FromVoidPtr(PyWin_DLLhModule));
    SET_SYS_FROM_STRING("winver", PyWin_DLLVersionString);
#endif
#ifdef ABIFLAGS
    SET_SYS_FROM_STRING("abiflags", ABIFLAGS);
#endif

#define ENSURE_INFO_TYPE(TYPE, DESC) \
    do { \
        if (_PyStructSequence_InitBuiltinWithFlags( \
                interp, &TYPE, &DESC, Py_TPFLAGS_DISALLOW_INSTANTIATION) < 0) { \
            goto type_init_failed; \
        } \
    } while (0)

    /* version_info */
    ENSURE_INFO_TYPE(VersionInfoType, version_info_desc);
    version_info = make_version_info(tstate);
    SET_SYS("version_info", version_info);

    /* implementation */
    SET_SYS("implementation", make_impl_info(version_info));

    // sys.flags: updated in-place later by _PySys_UpdateConfig()
    ENSURE_INFO_TYPE(FlagsType, flags_desc);
    SET_SYS("flags", make_flags(tstate->interp));

#if defined(MS_WINDOWS)
    /* getwindowsversion */
    ENSURE_INFO_TYPE(WindowsVersionType, windows_version_desc);

    SET_SYS_FROM_STRING("_vpath", VPATH);
#endif

#undef ENSURE_INFO_TYPE

    /* float repr style: 0.03 (short) vs 0.029999999999999999 (legacy) */
#if _PY_SHORT_FLOAT_REPR == 1
    SET_SYS_FROM_STRING("float_repr_style", "short");
#else
    SET_SYS_FROM_STRING("float_repr_style", "legacy");
#endif

    SET_SYS("thread_info", PyThread_GetInfo());

    /* initialize asyncgen_hooks */
    if (_PyStructSequence_InitBuiltin(interp, &AsyncGenHooksType,
                                      &asyncgen_hooks_desc) < 0)
    {
        goto type_init_failed;
    }

#ifdef __EMSCRIPTEN__
    if (EmscriptenInfoType == NULL) {
        EmscriptenInfoType = PyStructSequence_NewType(&emscripten_info_desc);
        if (EmscriptenInfoType == NULL) {
            goto type_init_failed;
        }
    }
    SET_SYS("_emscripten_info", make_emscripten_info());
#endif

    /* adding sys.path_hooks and sys.path_importer_cache */
    SET_SYS("meta_path", PyList_New(0));
    SET_SYS("path_importer_cache", PyDict_New());
    SET_SYS("path_hooks", PyList_New(0));

    if (_PyErr_Occurred(tstate)) {
        goto err_occurred;
    }
    return _PyStatus_OK();

type_init_failed:
    return _PyStatus_ERR("failed to initialize a type");

err_occurred:
    return _PyStatus_ERR("can't initialize sys module");
}


// Update sys attributes for a new PyConfig configuration.
// This function also adds attributes that _PySys_InitCore() didn't add.
int
_PySys_UpdateConfig(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    PyObject *sysdict = interp->sysdict;
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    int res;

#define COPY_LIST(KEY, VALUE) \
        SET_SYS(KEY, _PyWideStringList_AsList(&(VALUE)));

#define SET_SYS_FROM_WSTR(KEY, VALUE) \
        SET_SYS(KEY, PyUnicode_FromWideChar(VALUE, -1));

#define COPY_WSTR(SYS_ATTR, WSTR) \
    if (WSTR != NULL) { \
        SET_SYS_FROM_WSTR(SYS_ATTR, WSTR); \
    }

    if (config->module_search_paths_set) {
        COPY_LIST("path", config->module_search_paths);
    }

    COPY_WSTR("executable", config->executable);
    COPY_WSTR("_base_executable", config->base_executable);
    COPY_WSTR("prefix", config->prefix);
    COPY_WSTR("base_prefix", config->base_prefix);
    COPY_WSTR("exec_prefix", config->exec_prefix);
    COPY_WSTR("base_exec_prefix", config->base_exec_prefix);
    COPY_WSTR("platlibdir", config->platlibdir);

    if (config->pycache_prefix != NULL) {
        SET_SYS_FROM_WSTR("pycache_prefix", config->pycache_prefix);
    } else {
        if (PyDict_SetItemString(sysdict, "pycache_prefix", Py_None) < 0) {
            return -1;
        }
    }

    COPY_LIST("argv", config->argv);
    COPY_LIST("orig_argv", config->orig_argv);
    COPY_LIST("warnoptions", config->warnoptions);

    SET_SYS("_xoptions", _PyConfig_CreateXOptionsDict(config));

    const wchar_t *stdlibdir = _Py_GetStdlibDir();
    if (stdlibdir != NULL) {
        SET_SYS_FROM_WSTR("_stdlib_dir", stdlibdir);
    }
    else {
        if (PyDict_SetItemString(sysdict, "_stdlib_dir", Py_None) < 0) {
            return -1;
        }
    }

#undef SET_SYS_FROM_WSTR
#undef COPY_LIST
#undef COPY_WSTR

    // sys.flags
    PyObject *flags = PySys_GetAttrString("flags");
    if (flags == NULL) {
        return -1;
    }
    if (set_flags_from_config(interp, flags) < 0) {
        Py_DECREF(flags);
        return -1;
    }
    Py_DECREF(flags);

    SET_SYS("dont_write_bytecode", PyBool_FromLong(!config->write_bytecode));

    if (_PyErr_Occurred(tstate)) {
        goto err_occurred;
    }

    return 0;

err_occurred:
    return -1;
}

#undef SET_SYS
#undef SET_SYS_FROM_STRING


/* Set up a preliminary stderr printer until we have enough
   infrastructure for the io module in place.

   Use UTF-8/backslashreplace and ignore EAGAIN errors. */
static PyStatus
_PySys_SetPreliminaryStderr(PyObject *sysdict)
{
    PyObject *pstderr = PyFile_NewStdPrinter(fileno(stderr));
    if (pstderr == NULL) {
        goto error;
    }
    if (PyDict_SetItem(sysdict, &_Py_ID(stderr), pstderr) < 0) {
        goto error;
    }
    if (PyDict_SetItemString(sysdict, "__stderr__", pstderr) < 0) {
        goto error;
    }
    Py_DECREF(pstderr);
    return _PyStatus_OK();

error:
    Py_XDECREF(pstderr);
    return _PyStatus_ERR("can't set preliminary stderr");
}

PyObject *_Py_CreateMonitoringObject(void);

/*[clinic input]
module _jit
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=10952f74d7bbd972]*/

PyDoc_STRVAR(_jit_doc, "Utilities for observing just-in-time compilation.");

/*[clinic input]
_jit.is_available -> bool
Return True if the current Python executable supports JIT compilation, and False otherwise.
[clinic start generated code]*/

static int
_jit_is_available_impl(PyObject *module)
/*[clinic end generated code: output=6849a9cd2ff4aac9 input=03add84aa8347cf1]*/
{
    (void)module;
#ifdef _Py_TIER2
    return true;
#else
    return false;
#endif
}

/*[clinic input]
_jit.is_enabled -> bool
Return True if JIT compilation is enabled for the current Python process (implies sys._jit.is_available()), and False otherwise.
[clinic start generated code]*/

static int
_jit_is_enabled_impl(PyObject *module)
/*[clinic end generated code: output=55865f8de993fe42 input=02439394da8e873f]*/
{
    (void)module;
    return _PyInterpreterState_GET()->jit;
}

/*[clinic input]
_jit.is_active -> bool
Return True if the topmost Python frame is currently executing JIT code (implies sys._jit.is_enabled()), and False otherwise.
[clinic start generated code]*/

static int
_jit_is_active_impl(PyObject *module)
/*[clinic end generated code: output=7facca06b10064d4 input=be2fcd8a269d9b72]*/
{
    (void)module;
    return _PyThreadState_GET()->current_executor != NULL;
}

static PyMethodDef _jit_methods[] = {
    _JIT_IS_AVAILABLE_METHODDEF
    _JIT_IS_ENABLED_METHODDEF
    _JIT_IS_ACTIVE_METHODDEF
    {NULL}
};

static struct PyModuleDef _jit_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "sys._jit",
    .m_doc = _jit_doc,
    .m_size = -1,
    .m_methods = _jit_methods,
};

/* Create sys module without all attributes.
   _PySys_UpdateConfig() should be called later to add remaining attributes. */
PyStatus
_PySys_Create(PyThreadState *tstate, PyObject **sysmod_p)
{
    assert(!_PyErr_Occurred(tstate));

    PyInterpreterState *interp = tstate->interp;

    PyObject *modules = _PyImport_InitModules(interp);
    if (modules == NULL) {
        goto error;
    }

    PyObject *sysmod = _PyModule_CreateInitialized(&sysmodule, PYTHON_API_VERSION);
    if (sysmod == NULL) {
        return _PyStatus_ERR("failed to create a module object");
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(sysmod, Py_MOD_GIL_NOT_USED);
#endif

    PyObject *sysdict = PyModule_GetDict(sysmod);
    if (sysdict == NULL) {
        goto error;
    }
    interp->sysdict = Py_NewRef(sysdict);

    interp->sysdict_copy = PyDict_Copy(sysdict);
    if (interp->sysdict_copy == NULL) {
        goto error;
    }

    if (PyDict_SetItemString(sysdict, "modules", modules) < 0) {
        goto error;
    }

    PyStatus status = _PySys_SetPreliminaryStderr(sysdict);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PySys_InitCore(tstate, sysdict);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (_PyImport_FixupBuiltin(tstate, sysmod, "sys", modules) < 0) {
        goto error;
    }

    PyObject *monitoring = _Py_CreateMonitoringObject();
    if (monitoring == NULL) {
        goto error;
    }
    int err = PyDict_SetItemString(sysdict, "monitoring", monitoring);
    Py_DECREF(monitoring);
    if (err < 0) {
        goto error;
    }

    PyObject *_jit = _PyModule_CreateInitialized(&_jit_module, PYTHON_API_VERSION);
    if (_jit == NULL) {
        goto error;
    }
    err = PyDict_SetItemString(sysdict, "_jit", _jit);
    Py_DECREF(_jit);
    if (err) {
        goto error;
    }

    assert(!_PyErr_Occurred(tstate));

    *sysmod_p = sysmod;
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("can't initialize sys module");
}


void
_PySys_FiniTypes(PyInterpreterState *interp)
{
    _PyStructSequence_FiniBuiltin(interp, &VersionInfoType);
    _PyStructSequence_FiniBuiltin(interp, &FlagsType);
#if defined(MS_WINDOWS)
    _PyStructSequence_FiniBuiltin(interp, &WindowsVersionType);
#endif
    _PyStructSequence_FiniBuiltin(interp, &Hash_InfoType);
    _PyStructSequence_FiniBuiltin(interp, &AsyncGenHooksType);
#ifdef __EMSCRIPTEN__
    if (_Py_IsMainInterpreter(interp)) {
        Py_CLEAR(EmscriptenInfoType);
    }
#endif
}


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

// Removed in Python 3.13 API, but kept for the stable ABI
PyAPI_FUNC(void)
PySys_SetPath(const wchar_t *path)
{
    PyObject *v;
    if ((v = makepathobject(path, DELIM)) == NULL)
        Py_FatalError("can't create sys.path");
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (sys_set_object(interp, &_Py_ID(path), v) != 0) {
        Py_FatalError("can't assign sys.path");
    }
    Py_DECREF(v);
}

static PyObject *
make_sys_argv(int argc, wchar_t * const * argv)
{
    PyObject *list = PyList_New(argc);
    if (list == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < argc; i++) {
        PyObject *v = PyUnicode_FromWideChar(argv[i], -1);
        if (v == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, v);
    }
    return list;
}

void
PySys_SetArgvEx(int argc, wchar_t **argv, int updatepath)
{
    wchar_t* empty_argv[1] = {L""};
    PyThreadState *tstate = _PyThreadState_GET();

    if (argc < 1 || argv == NULL) {
        /* Ensure at least one (empty) argument is seen */
        argv = empty_argv;
        argc = 1;
    }

    PyObject *av = make_sys_argv(argc, argv);
    if (av == NULL) {
        Py_FatalError("no mem for sys.argv");
    }
    if (sys_set_object_str(tstate->interp, "argv", av) != 0) {
        Py_DECREF(av);
        Py_FatalError("can't assign sys.argv");
    }
    Py_DECREF(av);

    if (updatepath) {
        /* If argv[0] is not '-c' nor '-m', prepend argv[0] to sys.path.
           If argv[0] is a symlink, use the real path. */
        const PyWideStringList argv_list = {.length = argc, .items = argv};
        PyObject *path0 = NULL;
        if (_PyPathConfig_ComputeSysPath0(&argv_list, &path0)) {
            if (path0 == NULL) {
                Py_FatalError("can't compute path0 from argv");
            }

            PyObject *sys_path;
            if (PySys_GetOptionalAttr(&_Py_ID(path), &sys_path) < 0) {
                Py_FatalError("can't get sys.path");
            }
            else if (sys_path != NULL) {
                if (PyList_Insert(sys_path, 0, path0) < 0) {
                    Py_FatalError("can't prepend path0 to sys.path");
                }
                Py_DECREF(sys_path);
            }
            Py_DECREF(path0);
        }
    }
}

void
PySys_SetArgv(int argc, wchar_t **argv)
{
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PySys_SetArgvEx(argc, argv, Py_IsolatedFlag == 0);
_Py_COMP_DIAG_POP
}

/* Reimplementation of PyFile_WriteString() no calling indirectly
   PyErr_CheckSignals(): avoid the call to PyObject_Str(). */

static int
sys_pyfile_write_unicode(PyObject *unicode, PyObject *file)
{
    if (file == NULL)
        return -1;
    assert(unicode != NULL);
    PyObject *result = PyObject_CallMethodOneArg(file, &_Py_ID(write), unicode);
    if (result == NULL) {
        return -1;
    }
    Py_DECREF(result);
    return 0;
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
sys_write(PyObject *key, FILE *fp, const char *format, va_list va)
{
    PyObject *file;
    char buffer[1001];
    int written;
    PyThreadState *tstate = _PyThreadState_GET();

    PyObject *exc = _PyErr_GetRaisedException(tstate);
    written = PyOS_vsnprintf(buffer, sizeof(buffer), format, va);
    file = PySys_GetAttr(key);
    if (sys_pyfile_write(buffer, file) != 0) {
        _PyErr_Clear(tstate);
        fputs(buffer, fp);
    }
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        const char *truncated = "... truncated";
        if (sys_pyfile_write(truncated, file) != 0)
            fputs(truncated, fp);
    }
    Py_XDECREF(file);
    _PyErr_SetRaisedException(tstate, exc);
}

void
PySys_WriteStdout(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_write(&_Py_ID(stdout), stdout, format, va);
    va_end(va);
}

void
PySys_WriteStderr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_write(&_Py_ID(stderr), stderr, format, va);
    va_end(va);
}

static void
sys_format(PyObject *key, FILE *fp, const char *format, va_list va)
{
    PyObject *file, *message;
    const char *utf8;
    PyThreadState *tstate = _PyThreadState_GET();

    PyObject *exc = _PyErr_GetRaisedException(tstate);
    message = PyUnicode_FromFormatV(format, va);
    if (message != NULL) {
        file = PySys_GetAttr(key);
        if (sys_pyfile_write_unicode(message, file) != 0) {
            _PyErr_Clear(tstate);
            utf8 = PyUnicode_AsUTF8(message);
            if (utf8 != NULL)
                fputs(utf8, fp);
        }
        Py_XDECREF(file);
        Py_DECREF(message);
    }
    _PyErr_SetRaisedException(tstate, exc);
}

void
PySys_FormatStdout(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_format(&_Py_ID(stdout), stdout, format, va);
    va_end(va);
}

void
PySys_FormatStderr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    sys_format(&_Py_ID(stderr), stderr, format, va);
    va_end(va);
}


int
_PySys_SetIntMaxStrDigits(int maxdigits)
{
    if (maxdigits != 0 && maxdigits < _PY_LONG_MAX_STR_DIGITS_THRESHOLD) {
        PyErr_Format(
            PyExc_ValueError, "maxdigits must be >= %d or 0 for unlimited",
            _PY_LONG_MAX_STR_DIGITS_THRESHOLD);
        return -1;
    }

    // Set sys.flags.int_max_str_digits
    const Py_ssize_t pos = SYS_FLAGS_INT_MAX_STR_DIGITS;
    if (_PySys_SetFlagInt(pos, maxdigits) < 0) {
        return -1;
    }

    // Set PyInterpreterState.long_state.max_str_digits
    // and PyInterpreterState.config.int_max_str_digits.
    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->long_state.max_str_digits = maxdigits;
    interp->config.int_max_str_digits = maxdigits;
    return 0;
}

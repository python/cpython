
/* Error handling */

#include "Python.h"
#include "internal/pystate.h"

#ifndef __STDC__
#ifndef MS_WINDOWS
extern char *strerror(int);
#endif
#endif

#ifdef MS_WINDOWS
#include <windows.h>
#include <winbase.h>
#endif

#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

_Py_IDENTIFIER(builtins);
_Py_IDENTIFIER(stderr);


void
PyErr_Restore(PyObject *type, PyObject *value, PyObject *traceback)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *oldtype, *oldvalue, *oldtraceback;

    if (traceback != NULL && !PyTraceBack_Check(traceback)) {
        /* XXX Should never happen -- fatal error instead? */
        /* Well, it could be None. */
        Py_DECREF(traceback);
        traceback = NULL;
    }

    /* Save these in locals to safeguard against recursive
       invocation through Py_XDECREF */
    oldtype = tstate->curexc_type;
    oldvalue = tstate->curexc_value;
    oldtraceback = tstate->curexc_traceback;

    tstate->curexc_type = type;
    tstate->curexc_value = value;
    tstate->curexc_traceback = traceback;

    Py_XDECREF(oldtype);
    Py_XDECREF(oldvalue);
    Py_XDECREF(oldtraceback);
}

_PyErr_StackItem *
_PyErr_GetTopmostException(PyThreadState *tstate)
{
    _PyErr_StackItem *exc_info = tstate->exc_info;
    while ((exc_info->exc_type == NULL || exc_info->exc_type == Py_None) &&
           exc_info->previous_item != NULL)
    {
        exc_info = exc_info->previous_item;
    }
    return exc_info;
}

static PyObject*
_PyErr_CreateException(PyObject *exception, PyObject *value)
{
    if (value == NULL || value == Py_None) {
        return _PyObject_CallNoArg(exception);
    }
    else if (PyTuple_Check(value)) {
        return PyObject_Call(exception, value, NULL);
    }
    else {
        return PyObject_CallFunctionObjArgs(exception, value, NULL);
    }
}

void
PyErr_SetObject(PyObject *exception, PyObject *value)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *exc_value;
    PyObject *tb = NULL;

    if (exception != NULL &&
        !PyExceptionClass_Check(exception)) {
        PyErr_Format(PyExc_SystemError,
                     "exception %R not a BaseException subclass",
                     exception);
        return;
    }

    Py_XINCREF(value);
    exc_value = _PyErr_GetTopmostException(tstate)->exc_value;
    if (exc_value != NULL && exc_value != Py_None) {
        /* Implicit exception chaining */
        Py_INCREF(exc_value);
        if (value == NULL || !PyExceptionInstance_Check(value)) {
            /* We must normalize the value right now */
            PyObject *fixed_value;

            /* Issue #23571: functions must not be called with an
               exception set */
            PyErr_Clear();

            fixed_value = _PyErr_CreateException(exception, value);
            Py_XDECREF(value);
            if (fixed_value == NULL) {
                Py_DECREF(exc_value);
                return;
            }

            value = fixed_value;
        }

        /* Avoid reference cycles through the context chain.
           This is O(chain length) but context chains are
           usually very short. Sensitive readers may try
           to inline the call to PyException_GetContext. */
        if (exc_value != value) {
            PyObject *o = exc_value, *context;
            while ((context = PyException_GetContext(o))) {
                Py_DECREF(context);
                if (context == value) {
                    PyException_SetContext(o, NULL);
                    break;
                }
                o = context;
            }
            PyException_SetContext(value, exc_value);
        }
        else {
            Py_DECREF(exc_value);
        }
    }
    if (value != NULL && PyExceptionInstance_Check(value))
        tb = PyException_GetTraceback(value);
    Py_XINCREF(exception);
    PyErr_Restore(exception, value, tb);
}

/* Set a key error with the specified argument, wrapping it in a
 * tuple automatically so that tuple keys are not unpacked as the
 * exception arguments. */
void
_PyErr_SetKeyError(PyObject *arg)
{
    PyObject *tup;
    tup = PyTuple_Pack(1, arg);
    if (!tup)
        return; /* caller will expect error to be set anyway */
    PyErr_SetObject(PyExc_KeyError, tup);
    Py_DECREF(tup);
}

void
PyErr_SetNone(PyObject *exception)
{
    PyErr_SetObject(exception, (PyObject *)NULL);
}

void
PyErr_SetString(PyObject *exception, const char *string)
{
    PyObject *value = PyUnicode_FromString(string);
    PyErr_SetObject(exception, value);
    Py_XDECREF(value);
}


PyObject* _Py_HOT_FUNCTION
PyErr_Occurred(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return tstate == NULL ? NULL : tstate->curexc_type;
}


int
PyErr_GivenExceptionMatches(PyObject *err, PyObject *exc)
{
    if (err == NULL || exc == NULL) {
        /* maybe caused by "import exceptions" that failed early on */
        return 0;
    }
    if (PyTuple_Check(exc)) {
        Py_ssize_t i, n;
        n = PyTuple_Size(exc);
        for (i = 0; i < n; i++) {
            /* Test recursively */
             if (PyErr_GivenExceptionMatches(
                 err, PyTuple_GET_ITEM(exc, i)))
             {
                 return 1;
             }
        }
        return 0;
    }
    /* err might be an instance, so check its class. */
    if (PyExceptionInstance_Check(err))
        err = PyExceptionInstance_Class(err);

    if (PyExceptionClass_Check(err) && PyExceptionClass_Check(exc)) {
        return PyType_IsSubtype((PyTypeObject *)err, (PyTypeObject *)exc);
    }

    return err == exc;
}


int
PyErr_ExceptionMatches(PyObject *exc)
{
    return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}


#ifndef Py_NORMALIZE_RECURSION_LIMIT
#define Py_NORMALIZE_RECURSION_LIMIT 32
#endif

/* Used in many places to normalize a raised exception, including in
   eval_code2(), do_raise(), and PyErr_Print()

   XXX: should PyErr_NormalizeException() also call
            PyException_SetTraceback() with the resulting value and tb?
*/
void
PyErr_NormalizeException(PyObject **exc, PyObject **val, PyObject **tb)
{
    int recursion_depth = 0;
    PyObject *type, *value, *initial_tb;

  restart:
    type = *exc;
    if (type == NULL) {
        /* There was no exception, so nothing to do. */
        return;
    }

    value = *val;
    /* If PyErr_SetNone() was used, the value will have been actually
       set to NULL.
    */
    if (!value) {
        value = Py_None;
        Py_INCREF(value);
    }

    /* Normalize the exception so that if the type is a class, the
       value will be an instance.
    */
    if (PyExceptionClass_Check(type)) {
        PyObject *inclass = NULL;
        int is_subclass = 0;

        if (PyExceptionInstance_Check(value)) {
            inclass = PyExceptionInstance_Class(value);
            is_subclass = PyObject_IsSubclass(inclass, type);
            if (is_subclass < 0) {
                goto error;
            }
        }

        /* If the value was not an instance, or is not an instance
           whose class is (or is derived from) type, then use the
           value as an argument to instantiation of the type
           class.
        */
        if (!is_subclass) {
            PyObject *fixed_value = _PyErr_CreateException(type, value);
            if (fixed_value == NULL) {
                goto error;
            }
            Py_DECREF(value);
            value = fixed_value;
        }
        /* If the class of the instance doesn't exactly match the
           class of the type, believe the instance.
        */
        else if (inclass != type) {
            Py_INCREF(inclass);
            Py_DECREF(type);
            type = inclass;
        }
    }
    *exc = type;
    *val = value;
    return;

  error:
    Py_DECREF(type);
    Py_DECREF(value);
    recursion_depth++;
    if (recursion_depth == Py_NORMALIZE_RECURSION_LIMIT) {
        PyErr_SetString(PyExc_RecursionError, "maximum recursion depth "
                        "exceeded while normalizing an exception");
    }
    /* If the new exception doesn't set a traceback and the old
       exception had a traceback, use the old traceback for the
       new exception.  It's better than nothing.
    */
    initial_tb = *tb;
    PyErr_Fetch(exc, val, tb);
    assert(*exc != NULL);
    if (initial_tb != NULL) {
        if (*tb == NULL)
            *tb = initial_tb;
        else
            Py_DECREF(initial_tb);
    }
    /* Abort when Py_NORMALIZE_RECURSION_LIMIT has been exceeded, and the
       corresponding RecursionError could not be normalized, and the
       MemoryError raised when normalize this RecursionError could not be
       normalized. */
    if (recursion_depth >= Py_NORMALIZE_RECURSION_LIMIT + 2) {
        if (PyErr_GivenExceptionMatches(*exc, PyExc_MemoryError)) {
            Py_FatalError("Cannot recover from MemoryErrors "
                          "while normalizing exceptions.");
        }
        else {
            Py_FatalError("Cannot recover from the recursive normalization "
                          "of an exception.");
        }
    }
    goto restart;
}


void
PyErr_Fetch(PyObject **p_type, PyObject **p_value, PyObject **p_traceback)
{
    PyThreadState *tstate = PyThreadState_GET();

    *p_type = tstate->curexc_type;
    *p_value = tstate->curexc_value;
    *p_traceback = tstate->curexc_traceback;

    tstate->curexc_type = NULL;
    tstate->curexc_value = NULL;
    tstate->curexc_traceback = NULL;
}

void
PyErr_Clear(void)
{
    PyErr_Restore(NULL, NULL, NULL);
}

void
PyErr_GetExcInfo(PyObject **p_type, PyObject **p_value, PyObject **p_traceback)
{
    PyThreadState *tstate = PyThreadState_GET();

    _PyErr_StackItem *exc_info = _PyErr_GetTopmostException(tstate);
    *p_type = exc_info->exc_type;
    *p_value = exc_info->exc_value;
    *p_traceback = exc_info->exc_traceback;


    Py_XINCREF(*p_type);
    Py_XINCREF(*p_value);
    Py_XINCREF(*p_traceback);
}

void
PyErr_SetExcInfo(PyObject *p_type, PyObject *p_value, PyObject *p_traceback)
{
    PyObject *oldtype, *oldvalue, *oldtraceback;
    PyThreadState *tstate = PyThreadState_GET();

    oldtype = tstate->exc_info->exc_type;
    oldvalue = tstate->exc_info->exc_value;
    oldtraceback = tstate->exc_info->exc_traceback;

    tstate->exc_info->exc_type = p_type;
    tstate->exc_info->exc_value = p_value;
    tstate->exc_info->exc_traceback = p_traceback;

    Py_XDECREF(oldtype);
    Py_XDECREF(oldvalue);
    Py_XDECREF(oldtraceback);
}

/* Like PyErr_Restore(), but if an exception is already set,
   set the context associated with it.
 */
void
_PyErr_ChainExceptions(PyObject *exc, PyObject *val, PyObject *tb)
{
    if (exc == NULL)
        return;

    if (PyErr_Occurred()) {
        PyObject *exc2, *val2, *tb2;
        PyErr_Fetch(&exc2, &val2, &tb2);
        PyErr_NormalizeException(&exc, &val, &tb);
        if (tb != NULL) {
            PyException_SetTraceback(val, tb);
            Py_DECREF(tb);
        }
        Py_DECREF(exc);
        PyErr_NormalizeException(&exc2, &val2, &tb2);
        PyException_SetContext(val2, val);
        PyErr_Restore(exc2, val2, tb2);
    }
    else {
        PyErr_Restore(exc, val, tb);
    }
}

static PyObject *
_PyErr_FormatVFromCause(PyObject *exception, const char *format, va_list vargs)
{
    PyObject *exc, *val, *val2, *tb;

    assert(PyErr_Occurred());
    PyErr_Fetch(&exc, &val, &tb);
    PyErr_NormalizeException(&exc, &val, &tb);
    if (tb != NULL) {
        PyException_SetTraceback(val, tb);
        Py_DECREF(tb);
    }
    Py_DECREF(exc);
    assert(!PyErr_Occurred());

    PyErr_FormatV(exception, format, vargs);

    PyErr_Fetch(&exc, &val2, &tb);
    PyErr_NormalizeException(&exc, &val2, &tb);
    Py_INCREF(val);
    PyException_SetCause(val2, val);
    PyException_SetContext(val2, val);
    PyErr_Restore(exc, val2, tb);

    return NULL;
}

PyObject *
_PyErr_FormatFromCause(PyObject *exception, const char *format, ...)
{
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    _PyErr_FormatVFromCause(exception, format, vargs);
    va_end(vargs);
    return NULL;
}

/* Convenience functions to set a type error exception and return 0 */

int
PyErr_BadArgument(void)
{
    PyErr_SetString(PyExc_TypeError,
                    "bad argument type for built-in operation");
    return 0;
}

PyObject *
PyErr_NoMemory(void)
{
    if (Py_TYPE(PyExc_MemoryError) == NULL) {
        /* PyErr_NoMemory() has been called before PyExc_MemoryError has been
           initialized by _PyExc_Init() */
        Py_FatalError("Out of memory and PyExc_MemoryError is not "
                      "initialized yet");
    }
    PyErr_SetNone(PyExc_MemoryError);
    return NULL;
}

PyObject *
PyErr_SetFromErrnoWithFilenameObject(PyObject *exc, PyObject *filenameObject)
{
    return PyErr_SetFromErrnoWithFilenameObjects(exc, filenameObject, NULL);
}

PyObject *
PyErr_SetFromErrnoWithFilenameObjects(PyObject *exc, PyObject *filenameObject, PyObject *filenameObject2)
{
    PyObject *message;
    PyObject *v, *args;
    int i = errno;
#ifdef MS_WINDOWS
    WCHAR *s_buf = NULL;
#endif /* Unix/Windows */

#ifdef EINTR
    if (i == EINTR && PyErr_CheckSignals())
        return NULL;
#endif

#ifndef MS_WINDOWS
    if (i != 0) {
        char *s = strerror(i);
        message = PyUnicode_DecodeLocale(s, "surrogateescape");
    }
    else {
        /* Sometimes errno didn't get set */
        message = PyUnicode_FromString("Error");
    }
#else
    if (i == 0)
        message = PyUnicode_FromString("Error"); /* Sometimes errno didn't get set */
    else
    {
        /* Note that the Win32 errors do not lineup with the
           errno error.  So if the error is in the MSVC error
           table, we use it, otherwise we assume it really _is_
           a Win32 error code
        */
        if (i > 0 && i < _sys_nerr) {
            message = PyUnicode_FromString(_sys_errlist[i]);
        }
        else {
            int len = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,                   /* no message source */
                i,
                MAKELANGID(LANG_NEUTRAL,
                           SUBLANG_DEFAULT),
                           /* Default language */
                (LPWSTR) &s_buf,
                0,                      /* size not used */
                NULL);                  /* no args */
            if (len==0) {
                /* Only ever seen this in out-of-mem
                   situations */
                s_buf = NULL;
                message = PyUnicode_FromFormat("Windows Error 0x%x", i);
            } else {
                /* remove trailing cr/lf and dots */
                while (len > 0 && (s_buf[len-1] <= L' ' || s_buf[len-1] == L'.'))
                    s_buf[--len] = L'\0';
                message = PyUnicode_FromWideChar(s_buf, len);
            }
        }
    }
#endif /* Unix/Windows */

    if (message == NULL)
    {
#ifdef MS_WINDOWS
        LocalFree(s_buf);
#endif
        return NULL;
    }

    if (filenameObject != NULL) {
        if (filenameObject2 != NULL)
            args = Py_BuildValue("(iOOiO)", i, message, filenameObject, 0, filenameObject2);
        else
            args = Py_BuildValue("(iOO)", i, message, filenameObject);
    } else {
        assert(filenameObject2 == NULL);
        args = Py_BuildValue("(iO)", i, message);
    }
    Py_DECREF(message);

    if (args != NULL) {
        v = PyObject_Call(exc, args, NULL);
        Py_DECREF(args);
        if (v != NULL) {
            PyErr_SetObject((PyObject *) Py_TYPE(v), v);
            Py_DECREF(v);
        }
    }
#ifdef MS_WINDOWS
    LocalFree(s_buf);
#endif
    return NULL;
}

PyObject *
PyErr_SetFromErrnoWithFilename(PyObject *exc, const char *filename)
{
    PyObject *name = filename ? PyUnicode_DecodeFSDefault(filename) : NULL;
    PyObject *result = PyErr_SetFromErrnoWithFilenameObjects(exc, name, NULL);
    Py_XDECREF(name);
    return result;
}

#ifdef MS_WINDOWS
PyObject *
PyErr_SetFromErrnoWithUnicodeFilename(PyObject *exc, const Py_UNICODE *filename)
{
    PyObject *name = filename ? PyUnicode_FromWideChar(filename, -1) : NULL;
    PyObject *result = PyErr_SetFromErrnoWithFilenameObjects(exc, name, NULL);
    Py_XDECREF(name);
    return result;
}
#endif /* MS_WINDOWS */

PyObject *
PyErr_SetFromErrno(PyObject *exc)
{
    return PyErr_SetFromErrnoWithFilenameObjects(exc, NULL, NULL);
}

#ifdef MS_WINDOWS
/* Windows specific error code handling */
PyObject *PyErr_SetExcFromWindowsErrWithFilenameObject(
    PyObject *exc,
    int ierr,
    PyObject *filenameObject)
{
    return PyErr_SetExcFromWindowsErrWithFilenameObjects(exc, ierr,
        filenameObject, NULL);
}

PyObject *PyErr_SetExcFromWindowsErrWithFilenameObjects(
    PyObject *exc,
    int ierr,
    PyObject *filenameObject,
    PyObject *filenameObject2)
{
    int len;
    WCHAR *s_buf = NULL; /* Free via LocalFree */
    PyObject *message;
    PyObject *args, *v;
    DWORD err = (DWORD)ierr;
    if (err==0) err = GetLastError();
    len = FormatMessageW(
        /* Error API error */
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,           /* no message source */
        err,
        MAKELANGID(LANG_NEUTRAL,
        SUBLANG_DEFAULT), /* Default language */
        (LPWSTR) &s_buf,
        0,              /* size not used */
        NULL);          /* no args */
    if (len==0) {
        /* Only seen this in out of mem situations */
        message = PyUnicode_FromFormat("Windows Error 0x%x", err);
        s_buf = NULL;
    } else {
        /* remove trailing cr/lf and dots */
        while (len > 0 && (s_buf[len-1] <= L' ' || s_buf[len-1] == L'.'))
            s_buf[--len] = L'\0';
        message = PyUnicode_FromWideChar(s_buf, len);
    }

    if (message == NULL)
    {
        LocalFree(s_buf);
        return NULL;
    }

    if (filenameObject == NULL) {
        assert(filenameObject2 == NULL);
        filenameObject = filenameObject2 = Py_None;
    }
    else if (filenameObject2 == NULL)
        filenameObject2 = Py_None;
    /* This is the constructor signature for OSError.
       The POSIX translation will be figured out by the constructor. */
    args = Py_BuildValue("(iOOiO)", 0, message, filenameObject, err, filenameObject2);
    Py_DECREF(message);

    if (args != NULL) {
        v = PyObject_Call(exc, args, NULL);
        Py_DECREF(args);
        if (v != NULL) {
            PyErr_SetObject((PyObject *) Py_TYPE(v), v);
            Py_DECREF(v);
        }
    }
    LocalFree(s_buf);
    return NULL;
}

PyObject *PyErr_SetExcFromWindowsErrWithFilename(
    PyObject *exc,
    int ierr,
    const char *filename)
{
    PyObject *name = filename ? PyUnicode_DecodeFSDefault(filename) : NULL;
    PyObject *ret = PyErr_SetExcFromWindowsErrWithFilenameObjects(exc,
                                                                 ierr,
                                                                 name,
                                                                 NULL);
    Py_XDECREF(name);
    return ret;
}

PyObject *PyErr_SetExcFromWindowsErrWithUnicodeFilename(
    PyObject *exc,
    int ierr,
    const Py_UNICODE *filename)
{
    PyObject *name = filename ? PyUnicode_FromWideChar(filename, -1) : NULL;
    PyObject *ret = PyErr_SetExcFromWindowsErrWithFilenameObjects(exc,
                                                                 ierr,
                                                                 name,
                                                                 NULL);
    Py_XDECREF(name);
    return ret;
}

PyObject *PyErr_SetExcFromWindowsErr(PyObject *exc, int ierr)
{
    return PyErr_SetExcFromWindowsErrWithFilename(exc, ierr, NULL);
}

PyObject *PyErr_SetFromWindowsErr(int ierr)
{
    return PyErr_SetExcFromWindowsErrWithFilename(PyExc_OSError,
                                                  ierr, NULL);
}

PyObject *PyErr_SetFromWindowsErrWithFilename(
    int ierr,
    const char *filename)
{
    PyObject *name = filename ? PyUnicode_DecodeFSDefault(filename) : NULL;
    PyObject *result = PyErr_SetExcFromWindowsErrWithFilenameObjects(
                                                  PyExc_OSError,
                                                  ierr, name, NULL);
    Py_XDECREF(name);
    return result;
}

PyObject *PyErr_SetFromWindowsErrWithUnicodeFilename(
    int ierr,
    const Py_UNICODE *filename)
{
    PyObject *name = filename ? PyUnicode_FromWideChar(filename, -1) : NULL;
    PyObject *result = PyErr_SetExcFromWindowsErrWithFilenameObjects(
                                                  PyExc_OSError,
                                                  ierr, name, NULL);
    Py_XDECREF(name);
    return result;
}
#endif /* MS_WINDOWS */

PyObject *
PyErr_SetImportErrorSubclass(PyObject *exception, PyObject *msg,
    PyObject *name, PyObject *path)
{
    int issubclass;
    PyObject *kwargs, *error;

    issubclass = PyObject_IsSubclass(exception, PyExc_ImportError);
    if (issubclass < 0) {
        return NULL;
    }
    else if (!issubclass) {
        PyErr_SetString(PyExc_TypeError, "expected a subclass of ImportError");
        return NULL;
    }

    if (msg == NULL) {
        PyErr_SetString(PyExc_TypeError, "expected a message argument");
        return NULL;
    }

    if (name == NULL) {
        name = Py_None;
    }
    if (path == NULL) {
        path = Py_None;
    }

    kwargs = PyDict_New();
    if (kwargs == NULL) {
        return NULL;
    }
    if (PyDict_SetItemString(kwargs, "name", name) < 0) {
        goto done;
    }
    if (PyDict_SetItemString(kwargs, "path", path) < 0) {
        goto done;
    }

    error = _PyObject_FastCallDict(exception, &msg, 1, kwargs);
    if (error != NULL) {
        PyErr_SetObject((PyObject *)Py_TYPE(error), error);
        Py_DECREF(error);
    }

done:
    Py_DECREF(kwargs);
    return NULL;
}

PyObject *
PyErr_SetImportError(PyObject *msg, PyObject *name, PyObject *path)
{
    return PyErr_SetImportErrorSubclass(PyExc_ImportError, msg, name, path);
}

void
_PyErr_BadInternalCall(const char *filename, int lineno)
{
    PyErr_Format(PyExc_SystemError,
                 "%s:%d: bad argument to internal function",
                 filename, lineno);
}

/* Remove the preprocessor macro for PyErr_BadInternalCall() so that we can
   export the entry point for existing object code: */
#undef PyErr_BadInternalCall
void
PyErr_BadInternalCall(void)
{
    assert(0 && "bad argument to internal function");
    PyErr_Format(PyExc_SystemError,
                 "bad argument to internal function");
}
#define PyErr_BadInternalCall() _PyErr_BadInternalCall(__FILE__, __LINE__)


PyObject *
PyErr_FormatV(PyObject *exception, const char *format, va_list vargs)
{
    PyObject* string;

    /* Issue #23571: PyUnicode_FromFormatV() must not be called with an
       exception set, it calls arbitrary Python code like PyObject_Repr() */
    PyErr_Clear();

    string = PyUnicode_FromFormatV(format, vargs);

    PyErr_SetObject(exception, string);
    Py_XDECREF(string);
    return NULL;
}


PyObject *
PyErr_Format(PyObject *exception, const char *format, ...)
{
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    PyErr_FormatV(exception, format, vargs);
    va_end(vargs);
    return NULL;
}


PyObject *
PyErr_NewException(const char *name, PyObject *base, PyObject *dict)
{
    const char *dot;
    PyObject *modulename = NULL;
    PyObject *classname = NULL;
    PyObject *mydict = NULL;
    PyObject *bases = NULL;
    PyObject *result = NULL;
    dot = strrchr(name, '.');
    if (dot == NULL) {
        PyErr_SetString(PyExc_SystemError,
            "PyErr_NewException: name must be module.class");
        return NULL;
    }
    if (base == NULL)
        base = PyExc_Exception;
    if (dict == NULL) {
        dict = mydict = PyDict_New();
        if (dict == NULL)
            goto failure;
    }
    if (PyDict_GetItemString(dict, "__module__") == NULL) {
        modulename = PyUnicode_FromStringAndSize(name,
                                             (Py_ssize_t)(dot-name));
        if (modulename == NULL)
            goto failure;
        if (PyDict_SetItemString(dict, "__module__", modulename) != 0)
            goto failure;
    }
    if (PyTuple_Check(base)) {
        bases = base;
        /* INCREF as we create a new ref in the else branch */
        Py_INCREF(bases);
    } else {
        bases = PyTuple_Pack(1, base);
        if (bases == NULL)
            goto failure;
    }
    /* Create a real class. */
    result = PyObject_CallFunction((PyObject *)&PyType_Type, "sOO",
                                   dot+1, bases, dict);
  failure:
    Py_XDECREF(bases);
    Py_XDECREF(mydict);
    Py_XDECREF(classname);
    Py_XDECREF(modulename);
    return result;
}


/* Create an exception with docstring */
PyObject *
PyErr_NewExceptionWithDoc(const char *name, const char *doc,
                          PyObject *base, PyObject *dict)
{
    int result;
    PyObject *ret = NULL;
    PyObject *mydict = NULL; /* points to the dict only if we create it */
    PyObject *docobj;

    if (dict == NULL) {
        dict = mydict = PyDict_New();
        if (dict == NULL) {
            return NULL;
        }
    }

    if (doc != NULL) {
        docobj = PyUnicode_FromString(doc);
        if (docobj == NULL)
            goto failure;
        result = PyDict_SetItemString(dict, "__doc__", docobj);
        Py_DECREF(docobj);
        if (result < 0)
            goto failure;
    }

    ret = PyErr_NewException(name, base, dict);
  failure:
    Py_XDECREF(mydict);
    return ret;
}


static void
write_unraisable_exc_file(PyObject *exc_type, PyObject *exc_value,
                          PyObject *exc_tb, PyObject *obj, PyObject *file)
{
    if (obj) {
        if (PyFile_WriteString("Exception ignored in: ", file) < 0) {
            return;
        }
        if (PyFile_WriteObject(obj, file, 0) < 0) {
            PyErr_Clear();
            if (PyFile_WriteString("<object repr() failed>", file) < 0) {
                return;
            }
        }
        if (PyFile_WriteString("\n", file) < 0) {
            return;
        }
    }

    if (exc_tb != NULL) {
        if (PyTraceBack_Print(exc_tb, file) < 0) {
            /* continue even if writing the traceback failed */
            PyErr_Clear();
        }
    }

    if (!exc_type) {
        return;
    }

    assert(PyExceptionClass_Check(exc_type));
    char* className = PyExceptionClass_Name(exc_type);
    if (className != NULL) {
        char *dot = strrchr(className, '.');
        if (dot != NULL) {
            className = dot+1;
        }
    }

    _Py_IDENTIFIER(__module__);
    PyObject *moduleName = _PyObject_GetAttrId(exc_type, &PyId___module__);
    if (moduleName == NULL || !PyUnicode_Check(moduleName)) {
        Py_XDECREF(moduleName);
        PyErr_Clear();
        if (PyFile_WriteString("<unknown>", file) < 0) {
            return;
        }
    }
    else {
        if (!_PyUnicode_EqualToASCIIId(moduleName, &PyId_builtins)) {
            if (PyFile_WriteObject(moduleName, file, Py_PRINT_RAW) < 0) {
                Py_DECREF(moduleName);
                return;
            }
            Py_DECREF(moduleName);
            if (PyFile_WriteString(".", file) < 0) {
                return;
            }
        }
        else {
            Py_DECREF(moduleName);
        }
    }

    if (className == NULL) {
        if (PyFile_WriteString("<unknown>", file) < 0) {
            return;
        }
    }
    else {
        if (PyFile_WriteString(className, file) < 0) {
            return;
        }
    }

    if (exc_value && exc_value != Py_None) {
        if (PyFile_WriteString(": ", file) < 0) {
            return;
        }
        if (PyFile_WriteObject(exc_value, file, Py_PRINT_RAW) < 0) {
            PyErr_Clear();
            if (PyFile_WriteString("<exception str() failed>", file) < 0) {
                return;
            }
        }
    }
    if (PyFile_WriteString("\n", file) < 0) {
        return;
    }
}


/* Display an unraisable exception into sys.stderr.

   Called when an exception has occurred but there is no way for Python to
   handle it. For example, when a destructor raises an exception or during
   garbage collection (gc.collect()).

   An exception must be set when calling this function. */
void
PyErr_WriteUnraisable(PyObject *obj)
{
    PyObject *f, *exc_type, *exc_value, *exc_tb;

    PyErr_Fetch(&exc_type, &exc_value, &exc_tb);

    f = _PySys_GetObjectId(&PyId_stderr);
    /* Do nothing if sys.stderr is not available or set to None */
    if (f != NULL && f != Py_None) {
        write_unraisable_exc_file(exc_type, exc_value, exc_tb, obj, f);
    }

    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value);
    Py_XDECREF(exc_tb);
    PyErr_Clear(); /* Just in case */
}

extern PyObject *PyModule_GetWarningsModule(void);


void
PyErr_SyntaxLocation(const char *filename, int lineno)
{
    PyErr_SyntaxLocationEx(filename, lineno, -1);
}


/* Set file and line information for the current exception.
   If the exception is not a SyntaxError, also sets additional attributes
   to make printing of exceptions believe it is a syntax error. */

void
PyErr_SyntaxLocationObject(PyObject *filename, int lineno, int col_offset)
{
    PyObject *exc, *v, *tb, *tmp;
    _Py_IDENTIFIER(filename);
    _Py_IDENTIFIER(lineno);
    _Py_IDENTIFIER(msg);
    _Py_IDENTIFIER(offset);
    _Py_IDENTIFIER(print_file_and_line);
    _Py_IDENTIFIER(text);

    /* add attributes for the line number and filename for the error */
    PyErr_Fetch(&exc, &v, &tb);
    PyErr_NormalizeException(&exc, &v, &tb);
    /* XXX check that it is, indeed, a syntax error. It might not
     * be, though. */
    tmp = PyLong_FromLong(lineno);
    if (tmp == NULL)
        PyErr_Clear();
    else {
        if (_PyObject_SetAttrId(v, &PyId_lineno, tmp))
            PyErr_Clear();
        Py_DECREF(tmp);
    }
    tmp = NULL;
    if (col_offset >= 0) {
        tmp = PyLong_FromLong(col_offset);
        if (tmp == NULL)
            PyErr_Clear();
    }
    if (_PyObject_SetAttrId(v, &PyId_offset, tmp ? tmp : Py_None))
        PyErr_Clear();
    Py_XDECREF(tmp);
    if (filename != NULL) {
        if (_PyObject_SetAttrId(v, &PyId_filename, filename))
            PyErr_Clear();

        tmp = PyErr_ProgramTextObject(filename, lineno);
        if (tmp) {
            if (_PyObject_SetAttrId(v, &PyId_text, tmp))
                PyErr_Clear();
            Py_DECREF(tmp);
        }
    }
    if (exc != PyExc_SyntaxError) {
        if (!_PyObject_HasAttrId(v, &PyId_msg)) {
            tmp = PyObject_Str(v);
            if (tmp) {
                if (_PyObject_SetAttrId(v, &PyId_msg, tmp))
                    PyErr_Clear();
                Py_DECREF(tmp);
            } else {
                PyErr_Clear();
            }
        }
        if (!_PyObject_HasAttrId(v, &PyId_print_file_and_line)) {
            if (_PyObject_SetAttrId(v, &PyId_print_file_and_line,
                                    Py_None))
                PyErr_Clear();
        }
    }
    PyErr_Restore(exc, v, tb);
}

void
PyErr_SyntaxLocationEx(const char *filename, int lineno, int col_offset)
{
    PyObject *fileobj;
    if (filename != NULL) {
        fileobj = PyUnicode_DecodeFSDefault(filename);
        if (fileobj == NULL)
            PyErr_Clear();
    }
    else
        fileobj = NULL;
    PyErr_SyntaxLocationObject(fileobj, lineno, col_offset);
    Py_XDECREF(fileobj);
}

/* Attempt to load the line of text that the exception refers to.  If it
   fails, it will return NULL but will not set an exception.

   XXX The functionality of this function is quite similar to the
   functionality in tb_displayline() in traceback.c. */

static PyObject *
err_programtext(FILE *fp, int lineno)
{
    int i;
    char linebuf[1000];

    if (fp == NULL)
        return NULL;
    for (i = 0; i < lineno; i++) {
        char *pLastChar = &linebuf[sizeof(linebuf) - 2];
        do {
            *pLastChar = '\0';
            if (Py_UniversalNewlineFgets(linebuf, sizeof linebuf,
                                         fp, NULL) == NULL)
                break;
            /* fgets read *something*; if it didn't get as
               far as pLastChar, it must have found a newline
               or hit the end of the file; if pLastChar is \n,
               it obviously found a newline; else we haven't
               yet seen a newline, so must continue */
        } while (*pLastChar != '\0' && *pLastChar != '\n');
    }
    fclose(fp);
    if (i == lineno) {
        PyObject *res;
        res = PyUnicode_FromString(linebuf);
        if (res == NULL)
            PyErr_Clear();
        return res;
    }
    return NULL;
}

PyObject *
PyErr_ProgramText(const char *filename, int lineno)
{
    FILE *fp;
    if (filename == NULL || *filename == '\0' || lineno <= 0)
        return NULL;
    fp = _Py_fopen(filename, "r" PY_STDIOTEXTMODE);
    return err_programtext(fp, lineno);
}

PyObject *
PyErr_ProgramTextObject(PyObject *filename, int lineno)
{
    FILE *fp;
    if (filename == NULL || lineno <= 0)
        return NULL;
    fp = _Py_fopen_obj(filename, "r" PY_STDIOTEXTMODE);
    if (fp == NULL) {
        PyErr_Clear();
        return NULL;
    }
    return err_programtext(fp, lineno);
}

#ifdef __cplusplus
}
#endif

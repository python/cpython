/***********************************************************
Copyright (C) 1994 Steen Lumholt.

                        All Rights Reserved

******************************************************************/

/* _tkinter.c -- Interface to libtk.a and libtcl.a. */

/* TCL/TK VERSION INFO:

    Only Tcl/Tk 8.5.12 and later are supported.  Older versions are not
    supported. Use Python 3.10 or older if you cannot upgrade your
    Tcl/Tk libraries.
*/

/* XXX Further speed-up ideas, involving Tcl 8.0 features:

   - Register a new Tcl type, "Python callable", which can be called more
   efficiently and passed to Tcl_EvalObj() directly (if this is possible).

*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#ifdef MS_WINDOWS
#  include "pycore_fileutils.h"   // _Py_stat()
#endif

#include "pycore_long.h"          // _PyLong_IsNegative()
#include "pycore_sysmodule.h"     // _PySys_GetOptionalAttrString()

#ifdef MS_WINDOWS
#  include <windows.h>
#endif

#define CHECK_SIZE(size, elemsize) \
    ((size_t)(size) <= Py_MIN((size_t)INT_MAX, UINT_MAX / (size_t)(elemsize)))

/* If Tcl is compiled for threads, we must also define TCL_THREAD. We define
   it always; if Tcl is not threaded, the thread functions in
   Tcl are empty.  */
#define TCL_THREADS

#ifdef TK_FRAMEWORK
#  include <Tcl/tcl.h>
#  include <Tk/tk.h>
#else
#  include <tcl.h>
#  include <tk.h>
#endif

#include "tkinter.h"

#if TK_HEX_VERSION < 0x0805020c
#error "Tk older than 8.5.12 not supported"
#endif

#ifndef TCL_WITH_EXTERNAL_TOMMATH
#define TCL_NO_TOMMATH_H
#endif
#include <tclTomMath.h>

#if defined(TCL_WITH_EXTERNAL_TOMMATH) || (TK_HEX_VERSION >= 0x08070000)
#define USE_DEPRECATED_TOMMATH_API 0
#else
#define USE_DEPRECATED_TOMMATH_API 1
#endif

// As suggested by https://core.tcl-lang.org/tcl/wiki?name=Migrating+C+extensions+to+Tcl+9
#ifndef TCL_SIZE_MAX
typedef int Tcl_Size;
#define TCL_SIZE_MAX INT_MAX
#endif

#if !(defined(MS_WINDOWS) || defined(__CYGWIN__))
#define HAVE_CREATEFILEHANDLER
#endif

#ifdef HAVE_CREATEFILEHANDLER

/* This bit is to ensure that TCL_UNIX_FD is defined and doesn't interfere
   with the proper calculation of FHANDLETYPE == TCL_UNIX_FD below. */
#ifndef TCL_UNIX_FD
#  ifdef TCL_WIN_SOCKET
#    define TCL_UNIX_FD (! TCL_WIN_SOCKET)
#  else
#    define TCL_UNIX_FD 1
#  endif
#endif

/* Tcl_CreateFileHandler() changed several times; these macros deal with the
   messiness.  In Tcl 8.0 and later, it is not available on Windows (and on
   Unix, only because Jack added it back); when available on Windows, it only
   applies to sockets. */

#ifdef MS_WINDOWS
#define FHANDLETYPE TCL_WIN_SOCKET
#else
#define FHANDLETYPE TCL_UNIX_FD
#endif

/* If Tcl can wait for a Unix file descriptor, define the EventHook() routine
   which uses this to handle Tcl events while the user is typing commands. */

#if FHANDLETYPE == TCL_UNIX_FD
#define WAIT_FOR_STDIN
#endif

#endif /* HAVE_CREATEFILEHANDLER */

/* Use OS native encoding for converting between Python strings and
   Tcl objects.
   On Windows use UTF-16 (or UTF-32 for 32-bit Tcl_UniChar) with the
   "surrogatepass" error handler for converting to/from Tcl Unicode objects.
   On Linux use UTF-8 with the "surrogateescape" error handler for converting
   to/from Tcl String objects. */
#ifdef MS_WINDOWS
#define USE_TCL_UNICODE 1
#else
#define USE_TCL_UNICODE 0
#endif

#if PY_LITTLE_ENDIAN
#define NATIVE_BYTEORDER -1
#else
#define NATIVE_BYTEORDER 1
#endif

#ifdef MS_WINDOWS
#include <conio.h>
#define WAIT_FOR_STDIN

static PyObject *
_get_tcl_lib_path(void)
{
    static PyObject *tcl_library_path = NULL;
    static int already_checked = 0;

    if (already_checked == 0) {
        struct stat stat_buf;
        int stat_return_value;
        PyObject *prefix;

        (void) _PySys_GetOptionalAttrString("base_prefix", &prefix);
        if (prefix == NULL) {
            return NULL;
        }

        /* Check expected location for an installed Python first */
        tcl_library_path = PyUnicode_FromString("\\tcl\\tcl" TCL_VERSION);
        if (tcl_library_path == NULL) {
            Py_DECREF(prefix);
            return NULL;
        }
        tcl_library_path = PyUnicode_Concat(prefix, tcl_library_path);
        Py_DECREF(prefix);
        if (tcl_library_path == NULL) {
            return NULL;
        }
        stat_return_value = _Py_stat(tcl_library_path, &stat_buf);
        if (stat_return_value == -2) {
            return NULL;
        }
        if (stat_return_value == -1) {
            /* install location doesn't exist, reset errno and see if
               we're a repository build */
            errno = 0;
#ifdef Py_TCLTK_DIR
            tcl_library_path = PyUnicode_FromString(
                                    Py_TCLTK_DIR "\\lib\\tcl" TCL_VERSION);
            if (tcl_library_path == NULL) {
                return NULL;
            }
            stat_return_value = _Py_stat(tcl_library_path, &stat_buf);
            if (stat_return_value == -2) {
                return NULL;
            }
            if (stat_return_value == -1) {
                /* tcltkDir for a repository build doesn't exist either,
                   reset errno and leave Tcl to its own devices */
                errno = 0;
                tcl_library_path = NULL;
            }
#else
            tcl_library_path = NULL;
#endif
        }
        already_checked = 1;
    }
    return tcl_library_path;
}
#endif /* MS_WINDOWS */

/* The threading situation is complicated.  Tcl is not thread-safe, except
   when configured with --enable-threads.

   So we need to use a lock around all uses of Tcl.  Previously, the
   Python interpreter lock was used for this.  However, this causes
   problems when other Python threads need to run while Tcl is blocked
   waiting for events.

   To solve this problem, a separate lock for Tcl is introduced.
   Holding it is incompatible with holding Python's interpreter lock.
   The following four macros manipulate both locks together.

   ENTER_TCL and LEAVE_TCL are brackets, just like
   Py_BEGIN_ALLOW_THREADS and Py_END_ALLOW_THREADS.  They should be
   used whenever a call into Tcl is made that could call an event
   handler, or otherwise affect the state of a Tcl interpreter.  These
   assume that the surrounding code has the Python interpreter lock;
   inside the brackets, the Python interpreter lock has been released
   and the lock for Tcl has been acquired.

   Sometimes, it is necessary to have both the Python lock and the Tcl
   lock.  (For example, when transferring data from the Tcl
   interpreter result to a Python string object.)  This can be done by
   using different macros to close the ENTER_TCL block: ENTER_OVERLAP
   reacquires the Python lock (and restores the thread state) but
   doesn't release the Tcl lock; LEAVE_OVERLAP_TCL releases the Tcl
   lock.

   By contrast, ENTER_PYTHON and LEAVE_PYTHON are used in Tcl event
   handlers when the handler needs to use Python.  Such event handlers
   are entered while the lock for Tcl is held; the event handler
   presumably needs to use Python.  ENTER_PYTHON releases the lock for
   Tcl and acquires the Python interpreter lock, restoring the
   appropriate thread state, and LEAVE_PYTHON releases the Python
   interpreter lock and re-acquires the lock for Tcl.  It is okay for
   ENTER_TCL/LEAVE_TCL pairs to be contained inside the code between
   ENTER_PYTHON and LEAVE_PYTHON.

   These locks expand to several statements and brackets; they should
   not be used in branches of if statements and the like.

   If Tcl is threaded, this approach won't work anymore. The Tcl
   interpreter is only valid in the thread that created it, and all Tk
   activity must happen in this thread, also. That means that the
   mainloop must be invoked in the thread that created the
   interpreter. Invoking commands from other threads is possible;
   _tkinter will queue an event for the interpreter thread, which will
   then execute the command and pass back the result. If the main
   thread is not in the mainloop, and invoking commands causes an
   exception; if the main loop is running but not processing events,
   the command invocation will block.

   In addition, for a threaded Tcl, a single global tcl_tstate won't
   be sufficient anymore, since multiple Tcl interpreters may
   simultaneously dispatch in different threads. So we use the Tcl TLS
   API.

*/

static PyThread_type_lock tcl_lock = 0;

#ifdef TCL_THREADS
static Tcl_ThreadDataKey state_key;
typedef PyThreadState *ThreadSpecificData;
#define tcl_tstate \
    (*(PyThreadState**)Tcl_GetThreadData(&state_key, sizeof(PyThreadState*)))
#else
static PyThreadState *tcl_tstate = NULL;
#endif

#define ENTER_TCL \
    { PyThreadState *tstate = PyThreadState_Get(); \
      Py_BEGIN_ALLOW_THREADS \
      if(tcl_lock)PyThread_acquire_lock(tcl_lock, 1); \
      tcl_tstate = tstate;

#define LEAVE_TCL \
    tcl_tstate = NULL; \
    if(tcl_lock)PyThread_release_lock(tcl_lock); \
    Py_END_ALLOW_THREADS}

#define ENTER_OVERLAP \
    Py_END_ALLOW_THREADS

#define LEAVE_OVERLAP_TCL \
    tcl_tstate = NULL; if(tcl_lock)PyThread_release_lock(tcl_lock); }

#define ENTER_PYTHON \
    { PyThreadState *tstate = tcl_tstate; tcl_tstate = NULL; \
      if(tcl_lock) \
        PyThread_release_lock(tcl_lock); \
      PyEval_RestoreThread((tstate)); }

#define LEAVE_PYTHON \
    { PyThreadState *tstate = PyEval_SaveThread(); \
      if(tcl_lock)PyThread_acquire_lock(tcl_lock, 1); \
      tcl_tstate = tstate; }

#define CHECK_TCL_APPARTMENT \
    if (((TkappObject *)self)->threaded && \
        ((TkappObject *)self)->thread_id != Tcl_GetCurrentThread()) { \
        PyErr_SetString(PyExc_RuntimeError, \
                        "Calling Tcl from different apartment"); \
        return 0; \
    }

#ifndef FREECAST
#define FREECAST (char *)
#endif

/**** Tkapp Object Declaration ****/

static PyObject *Tkapp_Type;

typedef struct {
    PyObject_HEAD
    Tcl_Interp *interp;
    int wantobjects;
    int threaded; /* True if tcl_platform[threaded] */
    Tcl_ThreadId thread_id;
    int dispatching;
    PyObject *trace;
    /* We cannot include tclInt.h, as this is internal.
       So we cache interesting types here. */
    const Tcl_ObjType *OldBooleanType;
    const Tcl_ObjType *BooleanType;
    const Tcl_ObjType *ByteArrayType;
    const Tcl_ObjType *DoubleType;
    const Tcl_ObjType *IntType;
    const Tcl_ObjType *WideIntType;
    const Tcl_ObjType *BignumType;
    const Tcl_ObjType *ListType;
    const Tcl_ObjType *StringType;
    const Tcl_ObjType *UTF32StringType;
} TkappObject;

#define Tkapp_Interp(v) (((TkappObject *) (v))->interp)


/**** Error Handling ****/

static PyObject *Tkinter_TclError;
static int quitMainLoop = 0;
static int errorInCmd = 0;
static PyObject *excInCmd;


static PyObject *Tkapp_UnicodeResult(TkappObject *);

static PyObject *
Tkinter_Error(TkappObject *self)
{
    PyObject *res = Tkapp_UnicodeResult(self);
    if (res != NULL) {
        PyErr_SetObject(Tkinter_TclError, res);
        Py_DECREF(res);
    }
    return NULL;
}



/**** Utils ****/

static int Tkinter_busywaitinterval = 20;

#ifndef MS_WINDOWS

/* Millisecond sleep() for Unix platforms. */

static void
Sleep(int milli)
{
    /* XXX Too bad if you don't have select(). */
    struct timeval t;
    t.tv_sec = milli/1000;
    t.tv_usec = (milli%1000) * 1000;
    select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
}
#endif /* MS_WINDOWS */

/* Wait up to 1s for the mainloop to come up. */

static int
WaitForMainloop(TkappObject* self)
{
    int i;
    for (i = 0; i < 10; i++) {
        if (self->dispatching)
            return 1;
        Py_BEGIN_ALLOW_THREADS
        Sleep(100);
        Py_END_ALLOW_THREADS
    }
    if (self->dispatching)
        return 1;
    PyErr_SetString(PyExc_RuntimeError, "main thread is not in main loop");
    return 0;
}



#define ARGSZ 64



static PyObject *
unicodeFromTclStringAndSize(const char *s, Py_ssize_t size)
{
    PyObject *r = PyUnicode_DecodeUTF8(s, size, NULL);
    if (r != NULL || !PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
        return r;
    }

    char *buf = NULL;
    PyErr_Clear();
    /* Tcl encodes null character as \xc0\x80.
       https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8 */
    if (memchr(s, '\xc0', size)) {
        char *q;
        const char *e = s + size;
        q = buf = (char *)PyMem_Malloc(size);
        if (buf == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        while (s != e) {
            if (s + 1 != e && s[0] == '\xc0' && s[1] == '\x80') {
                *q++ = '\0';
                s += 2;
            }
            else
                *q++ = *s++;
        }
        s = buf;
        size = q - s;
    }
    r = PyUnicode_DecodeUTF8(s, size, "surrogateescape");
    if (buf != NULL) {
        PyMem_Free(buf);
    }
    if (r == NULL || PyUnicode_KIND(r) == PyUnicode_1BYTE_KIND) {
        return r;
    }

    /* In CESU-8 non-BMP characters are represented as a surrogate pair,
       like in UTF-16, and then each surrogate code point is encoded in UTF-8.
       https://en.wikipedia.org/wiki/CESU-8 */
    Py_ssize_t len = PyUnicode_GET_LENGTH(r);
    Py_ssize_t i, j;
    /* All encoded surrogate characters start with \xED. */
    i = PyUnicode_FindChar(r, 0xdcED, 0, len, 1);
    if (i == -2) {
        Py_DECREF(r);
        return NULL;
    }
    if (i == -1) {
        return r;
    }
    Py_UCS4 *u = PyUnicode_AsUCS4Copy(r);
    Py_DECREF(r);
    if (u == NULL) {
        return NULL;
    }
    Py_UCS4 ch;
    for (j = i; i < len; i++, u[j++] = ch) {
        Py_UCS4 ch1, ch2, ch3, high, low;
        /* Low surrogates U+D800 - U+DBFF are encoded as
           \xED\xA0\x80 - \xED\xAF\xBF. */
        ch1 = ch = u[i];
        if (ch1 != 0xdcED) continue;
        ch2 = u[i + 1];
        if (!(0xdcA0 <= ch2 && ch2 <= 0xdcAF)) continue;
        ch3 = u[i + 2];
        if (!(0xdc80 <= ch3 && ch3 <= 0xdcBF)) continue;
        high = 0xD000 | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F);
        assert(Py_UNICODE_IS_HIGH_SURROGATE(high));
        /* High surrogates U+DC00 - U+DFFF are encoded as
           \xED\xB0\x80 - \xED\xBF\xBF. */
        ch1 = u[i + 3];
        if (ch1 != 0xdcED) continue;
        ch2 = u[i + 4];
        if (!(0xdcB0 <= ch2 && ch2 <= 0xdcBF)) continue;
        ch3 = u[i + 5];
        if (!(0xdc80 <= ch3 && ch3 <= 0xdcBF)) continue;
        low = 0xD000 | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F);
        assert(Py_UNICODE_IS_HIGH_SURROGATE(high));
        ch = Py_UNICODE_JOIN_SURROGATES(high, low);
        i += 5;
    }
    r = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, u, j);
    PyMem_Free(u);
    return r;
}

static PyObject *
unicodeFromTclString(const char *s)
{
    return unicodeFromTclStringAndSize(s, strlen(s));
}

static PyObject *
unicodeFromTclObj(TkappObject *tkapp, Tcl_Obj *value)
{
    Tcl_Size len;
#if USE_TCL_UNICODE
    if (value->typePtr != NULL && tkapp != NULL &&
        (value->typePtr == tkapp->StringType ||
         value->typePtr == tkapp->UTF32StringType))
    {
        int byteorder = NATIVE_BYTEORDER;
        const Tcl_UniChar *u = Tcl_GetUnicodeFromObj(value, &len);
        if (sizeof(Tcl_UniChar) == 2)
            return PyUnicode_DecodeUTF16((const char *)u, len * 2,
                                         "surrogatepass", &byteorder);
        else if (sizeof(Tcl_UniChar) == 4)
            return PyUnicode_DecodeUTF32((const char *)u, len * 4,
                                         "surrogatepass", &byteorder);
        else
            Py_UNREACHABLE();
    }
#endif /* USE_TCL_UNICODE */
    const char *s = Tcl_GetStringFromObj(value, &len);
    return unicodeFromTclStringAndSize(s, len);
}

/*[clinic input]
module _tkinter
class _tkinter.tkapp "TkappObject *" "&Tkapp_Type_spec"
class _tkinter.Tcl_Obj "PyTclObject *" "&PyTclObject_Type_spec"
class _tkinter.tktimertoken "TkttObject *" "&Tktt_Type_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b1ebf15c162ee229]*/

/**** Tkapp Object ****/

#if TK_MAJOR_VERSION >= 9
int Tcl_AppInit(Tcl_Interp *);
#endif

#ifndef WITH_APPINIT
int
Tcl_AppInit(Tcl_Interp *interp)
{
    const char * _tkinter_skip_tk_init;

    if (Tcl_Init(interp) == TCL_ERROR) {
        PySys_WriteStderr("Tcl_Init error: %s\n", Tcl_GetStringResult(interp));
        return TCL_ERROR;
    }

    _tkinter_skip_tk_init = Tcl_GetVar(interp,
                    "_tkinter_skip_tk_init", TCL_GLOBAL_ONLY);
    if (_tkinter_skip_tk_init != NULL &&
                    strcmp(_tkinter_skip_tk_init, "1") == 0) {
        return TCL_OK;
    }

    if (Tk_Init(interp) == TCL_ERROR) {
        PySys_WriteStderr("Tk_Init error: %s\n", Tcl_GetStringResult(interp));
        return TCL_ERROR;
    }

    return TCL_OK;
}
#endif /* !WITH_APPINIT */




/* Initialize the Tk application; see the `main' function in
 * `tkMain.c'.
 */

static void EnableEventHook(void); /* Forward */
static void DisableEventHook(void); /* Forward */

static TkappObject *
Tkapp_New(const char *screenName, const char *className,
          int interactive, int wantobjects, int wantTk, int sync,
          const char *use)
{
    TkappObject *v;
    char *argv0;

    v = PyObject_New(TkappObject, (PyTypeObject *) Tkapp_Type);
    if (v == NULL)
        return NULL;

    v->interp = Tcl_CreateInterp();
    v->wantobjects = wantobjects;
    v->threaded = Tcl_GetVar2Ex(v->interp, "tcl_platform", "threaded",
                                TCL_GLOBAL_ONLY) != NULL;
    v->thread_id = Tcl_GetCurrentThread();
    v->dispatching = 0;
    v->trace = NULL;

#ifndef TCL_THREADS
    if (v->threaded) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Tcl is threaded but _tkinter is not");
        Py_DECREF(v);
        return 0;
    }
#endif
    if (v->threaded && tcl_lock) {
        /* If Tcl is threaded, we don't need the lock. */
        PyThread_free_lock(tcl_lock);
        tcl_lock = NULL;
    }

    v->OldBooleanType = Tcl_GetObjType("boolean");
    {
        Tcl_Obj *value;
        int boolValue;

        /* Tcl 8.5 "booleanString" type is not registered
           and is renamed to "boolean" in Tcl 9.0.
           Based on approach suggested at
           https://core.tcl-lang.org/tcl/info/3bb3bcf2da5b */
        value = Tcl_NewStringObj("true", -1);
        Tcl_GetBooleanFromObj(NULL, value, &boolValue);
        v->BooleanType = value->typePtr;
        Tcl_DecrRefCount(value);

        // "bytearray" type is not registered in Tcl 9.0
        value = Tcl_NewByteArrayObj(NULL, 0);
        v->ByteArrayType = value->typePtr;
        Tcl_DecrRefCount(value);
    }
    v->DoubleType = Tcl_GetObjType("double");
    /* TIP 484 suggests retrieving the "int" type without Tcl_GetObjType("int")
       since it is no longer registered in Tcl 9.0. But even though Tcl 8.7
       only uses the "wideInt" type on platforms with 32-bit long, it still has
       a registered "int" type, which FromObj() should recognize just in case. */
    v->IntType = Tcl_GetObjType("int");
    if (v->IntType == NULL) {
        Tcl_Obj *value = Tcl_NewIntObj(0);
        v->IntType = value->typePtr;
        Tcl_DecrRefCount(value);
    }
    v->WideIntType = Tcl_GetObjType("wideInt");
    v->BignumType = Tcl_GetObjType("bignum");
    v->ListType = Tcl_GetObjType("list");
    v->StringType = Tcl_GetObjType("string");
    v->UTF32StringType = Tcl_GetObjType("utf32string");

    /* Delete the 'exit' command, which can screw things up */
    Tcl_DeleteCommand(v->interp, "exit");

    if (screenName != NULL)
        Tcl_SetVar2(v->interp, "env", "DISPLAY",
                    screenName, TCL_GLOBAL_ONLY);

    if (interactive)
        Tcl_SetVar(v->interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
    else
        Tcl_SetVar(v->interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

    /* This is used to get the application class for Tk 4.1 and up */
    argv0 = (char*)PyMem_Malloc(strlen(className) + 1);
    if (!argv0) {
        PyErr_NoMemory();
        Py_DECREF(v);
        return NULL;
    }

    strcpy(argv0, className);
    if (Py_ISUPPER(argv0[0]))
        argv0[0] = Py_TOLOWER(argv0[0]);
    Tcl_SetVar(v->interp, "argv0", argv0, TCL_GLOBAL_ONLY);
    PyMem_Free(argv0);

    if (! wantTk) {
        Tcl_SetVar(v->interp,
                        "_tkinter_skip_tk_init", "1", TCL_GLOBAL_ONLY);
    }

    /* some initial arguments need to be in argv */
    if (sync || use) {
        char *args;
        Py_ssize_t len = 0;

        if (sync)
            len += sizeof "-sync";
        if (use)
            len += strlen(use) + sizeof "-use ";  /* never overflows */

        args = (char*)PyMem_Malloc(len);
        if (!args) {
            PyErr_NoMemory();
            Py_DECREF(v);
            return NULL;
        }

        args[0] = '\0';
        if (sync)
            strcat(args, "-sync");
        if (use) {
            if (sync)
                strcat(args, " ");
            strcat(args, "-use ");
            strcat(args, use);
        }

        Tcl_SetVar(v->interp, "argv", args, TCL_GLOBAL_ONLY);
        PyMem_Free(args);
    }

#ifdef MS_WINDOWS
    {
        PyObject *str_path;
        PyObject *utf8_path;
        DWORD ret;

        ret = GetEnvironmentVariableW(L"TCL_LIBRARY", NULL, 0);
        if (!ret && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            str_path = _get_tcl_lib_path();
            if (str_path == NULL && PyErr_Occurred()) {
                return NULL;
            }
            if (str_path != NULL) {
                utf8_path = PyUnicode_AsUTF8String(str_path);
                if (utf8_path == NULL) {
                    return NULL;
                }
                Tcl_SetVar(v->interp,
                           "tcl_library",
                           PyBytes_AS_STRING(utf8_path),
                           TCL_GLOBAL_ONLY);
                Py_DECREF(utf8_path);
            }
        }
    }
#endif

    if (Tcl_AppInit(v->interp) != TCL_OK) {
        PyObject *result = Tkinter_Error(v);
        Py_DECREF((PyObject *)v);
        return (TkappObject *)result;
    }

    EnableEventHook();

    return v;
}


static void
Tkapp_ThreadSend(TkappObject *self, Tcl_Event *ev,
                 Tcl_Condition *cond, Tcl_Mutex *mutex)
{
    Py_BEGIN_ALLOW_THREADS;
    Tcl_MutexLock(mutex);
    Tcl_ThreadQueueEvent(self->thread_id, ev, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(self->thread_id);
    Tcl_ConditionWait(cond, mutex, NULL);
    Tcl_MutexUnlock(mutex);
    Py_END_ALLOW_THREADS
}


/** Tcl Eval **/

typedef struct {
    PyObject_HEAD
    Tcl_Obj *value;
    PyObject *string; /* This cannot cause cycles. */
} PyTclObject;

static PyObject *PyTclObject_Type;
#define PyTclObject_Check(v) Py_IS_TYPE(v, (PyTypeObject *) PyTclObject_Type)

static PyObject *
newPyTclObject(Tcl_Obj *arg)
{
    PyTclObject *self;
    self = PyObject_New(PyTclObject, (PyTypeObject *) PyTclObject_Type);
    if (self == NULL)
        return NULL;
    Tcl_IncrRefCount(arg);
    self->value = arg;
    self->string = NULL;
    return (PyObject*)self;
}

static void
PyTclObject_dealloc(PyObject *_self)
{
    PyTclObject *self = (PyTclObject *)_self;
    PyObject *tp = (PyObject *) Py_TYPE(self);
    Tcl_DecrRefCount(self->value);
    Py_XDECREF(self->string);
    PyObject_Free(self);
    Py_DECREF(tp);
}

/* Like _str, but create Unicode if necessary. */
PyDoc_STRVAR(PyTclObject_string__doc__,
"the string representation of this object, either as str or bytes");

static PyObject *
PyTclObject_string(PyObject *_self, void *ignored)
{
    PyTclObject *self = (PyTclObject *)_self;
    if (!self->string) {
        self->string = unicodeFromTclObj(NULL, self->value);
        if (!self->string)
            return NULL;
    }
    return Py_NewRef(self->string);
}

static PyObject *
PyTclObject_str(PyObject *_self)
{
    PyTclObject *self = (PyTclObject *)_self;
    if (self->string) {
        return Py_NewRef(self->string);
    }
    /* XXX Could cache result if it is non-ASCII. */
    return unicodeFromTclObj(NULL, self->value);
}

static PyObject *
PyTclObject_repr(PyObject *_self)
{
    PyTclObject *self = (PyTclObject *)_self;
    PyObject *repr, *str = PyTclObject_str(_self);
    if (str == NULL)
        return NULL;
    repr = PyUnicode_FromFormat("<%s object: %R>",
                                self->value->typePtr->name, str);
    Py_DECREF(str);
    return repr;
}

static PyObject *
PyTclObject_richcompare(PyObject *self, PyObject *other, int op)
{
    int result;

    /* neither argument should be NULL, unless something's gone wrong */
    if (self == NULL || other == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* both arguments should be instances of PyTclObject */
    if (!PyTclObject_Check(self) || !PyTclObject_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (self == other)
        /* fast path when self and other are identical */
        result = 0;
    else
        result = strcmp(Tcl_GetString(((PyTclObject *)self)->value),
                        Tcl_GetString(((PyTclObject *)other)->value));
    Py_RETURN_RICHCOMPARE(result, 0, op);
}

PyDoc_STRVAR(get_typename__doc__, "name of the Tcl type");

static PyObject*
get_typename(PyObject *self, void* ignored)
{
    PyTclObject *obj = (PyTclObject *)self;
    return unicodeFromTclString(obj->value->typePtr->name);
}


static PyGetSetDef PyTclObject_getsetlist[] = {
    {"typename", get_typename, NULL, get_typename__doc__},
    {"string", PyTclObject_string, NULL,
     PyTclObject_string__doc__},
    {0},
};

static PyType_Slot PyTclObject_Type_slots[] = {
    {Py_tp_dealloc, PyTclObject_dealloc},
    {Py_tp_repr, PyTclObject_repr},
    {Py_tp_str, PyTclObject_str},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_richcompare, PyTclObject_richcompare},
    {Py_tp_getset, PyTclObject_getsetlist},
    {0, 0}
};

static PyType_Spec PyTclObject_Type_spec = {
    "_tkinter.Tcl_Obj",
    sizeof(PyTclObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    PyTclObject_Type_slots,
};


#if SIZE_MAX > INT_MAX
#define CHECK_STRING_LENGTH(s) do {                                     \
        if (s != NULL && strlen(s) >= INT_MAX) {                        \
            PyErr_SetString(PyExc_OverflowError, "string is too long"); \
            return NULL;                                                \
        } } while(0)
#else
#define CHECK_STRING_LENGTH(s)
#endif

static Tcl_Obj*
asBignumObj(PyObject *value)
{
    Tcl_Obj *result;
    int neg;
    PyObject *hexstr;
    const char *hexchars;
    mp_int bigValue;

    assert(PyLong_Check(value));
    neg = _PyLong_IsNegative((PyLongObject *)value);
    hexstr = _PyLong_Format(value, 16);
    if (hexstr == NULL)
        return NULL;
    hexchars = PyUnicode_AsUTF8(hexstr);
    if (hexchars == NULL) {
        Py_DECREF(hexstr);
        return NULL;
    }
    hexchars += neg + 2; /* skip sign and "0x" */
    if (mp_init(&bigValue) != MP_OKAY ||
        mp_read_radix(&bigValue, hexchars, 16) != MP_OKAY)
    {
        mp_clear(&bigValue);
        Py_DECREF(hexstr);
        PyErr_NoMemory();
        return NULL;
    }
    Py_DECREF(hexstr);
    bigValue.sign = neg ? MP_NEG : MP_ZPOS;
    result = Tcl_NewBignumObj(&bigValue);
    mp_clear(&bigValue);
    if (result == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    return result;
}

static Tcl_Obj*
AsObj(PyObject *value)
{
    Tcl_Obj *result;

    if (PyBytes_Check(value)) {
        if (PyBytes_GET_SIZE(value) >= INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "bytes object is too long");
            return NULL;
        }
        return Tcl_NewByteArrayObj((unsigned char *)PyBytes_AS_STRING(value),
                                   (int)PyBytes_GET_SIZE(value));
    }

    if (PyBool_Check(value))
        return Tcl_NewBooleanObj(PyObject_IsTrue(value));

    if (PyLong_CheckExact(value)) {
        int overflow;
        long longValue;
        Tcl_WideInt wideValue;
        longValue = PyLong_AsLongAndOverflow(value, &overflow);
        if (!overflow) {
            return Tcl_NewLongObj(longValue);
        }
        /* If there is an overflow in the long conversion,
           fall through to wideInt handling. */
        if (_PyLong_AsByteArray((PyLongObject *)value,
                                (unsigned char *)(void *)&wideValue,
                                sizeof(wideValue),
                                PY_LITTLE_ENDIAN,
                                /* signed */ 1,
                                /* with_exceptions */ 1) == 0) {
            return Tcl_NewWideIntObj(wideValue);
        }
        PyErr_Clear();
        /* If there is an overflow in the wideInt conversion,
           fall through to bignum handling. */
        return asBignumObj(value);
        /* If there is no wideInt or bignum support,
           fall through to default object handling. */
    }

    if (PyFloat_Check(value))
        return Tcl_NewDoubleObj(PyFloat_AS_DOUBLE(value));

    if (PyTuple_Check(value) || PyList_Check(value)) {
        Tcl_Obj **argv;
        Py_ssize_t size, i;

        size = PySequence_Fast_GET_SIZE(value);
        if (size == 0)
            return Tcl_NewListObj(0, NULL);
        if (!CHECK_SIZE(size, sizeof(Tcl_Obj *))) {
            PyErr_SetString(PyExc_OverflowError,
                            PyTuple_Check(value) ? "tuple is too long" :
                                                   "list is too long");
            return NULL;
        }
        argv = (Tcl_Obj **) PyMem_Malloc(((size_t)size) * sizeof(Tcl_Obj *));
        if (!argv) {
          PyErr_NoMemory();
          return NULL;
        }
        for (i = 0; i < size; i++)
          argv[i] = AsObj(PySequence_Fast_GET_ITEM(value,i));
        result = Tcl_NewListObj((int)size, argv);
        PyMem_Free(argv);
        return result;
    }

    if (PyUnicode_Check(value)) {
        Py_ssize_t size = PyUnicode_GET_LENGTH(value);
        if (size == 0) {
            return Tcl_NewStringObj("", 0);
        }
        if (!CHECK_SIZE(size, sizeof(Tcl_UniChar))) {
            PyErr_SetString(PyExc_OverflowError, "string is too long");
            return NULL;
        }
        if (PyUnicode_IS_ASCII(value) &&
            strlen(PyUnicode_DATA(value)) == (size_t)PyUnicode_GET_LENGTH(value))
        {
            return Tcl_NewStringObj((const char *)PyUnicode_DATA(value),
                                    (int)size);
        }

        PyObject *encoded;
#if USE_TCL_UNICODE
        if (sizeof(Tcl_UniChar) == 2)
            encoded = _PyUnicode_EncodeUTF16(value,
                    "surrogatepass", NATIVE_BYTEORDER);
        else if (sizeof(Tcl_UniChar) == 4)
            encoded = _PyUnicode_EncodeUTF32(value,
                    "surrogatepass", NATIVE_BYTEORDER);
        else
            Py_UNREACHABLE();
        if (!encoded) {
            return NULL;
        }
        size = PyBytes_GET_SIZE(encoded);
        if (size > INT_MAX) {
            Py_DECREF(encoded);
            PyErr_SetString(PyExc_OverflowError, "string is too long");
            return NULL;
        }
        result = Tcl_NewUnicodeObj((const Tcl_UniChar *)PyBytes_AS_STRING(encoded),
                                   (int)(size / sizeof(Tcl_UniChar)));
#else
        encoded = _PyUnicode_AsUTF8String(value, "surrogateescape");
        if (!encoded) {
            return NULL;
        }
        size = PyBytes_GET_SIZE(encoded);
        if (strlen(PyBytes_AS_STRING(encoded)) != (size_t)size) {
            /* The string contains embedded null characters.
             * Tcl needs a null character to be represented as \xc0\x80 in
             * the Modified UTF-8 encoding.  Otherwise the string can be
             * truncated in some internal operations.
             *
             * NOTE: stringlib_replace() could be used here, but optimizing
             * this obscure case isn't worth it unless stringlib_replace()
             * was already exposed in the C API for other reasons. */
            Py_SETREF(encoded,
                      PyObject_CallMethod(encoded, "replace", "y#y#",
                                          "\0", (Py_ssize_t)1,
                                          "\xc0\x80", (Py_ssize_t)2));
            if (!encoded) {
                return NULL;
            }
            size = PyBytes_GET_SIZE(encoded);
        }
        if (size > INT_MAX) {
            Py_DECREF(encoded);
            PyErr_SetString(PyExc_OverflowError, "string is too long");
            return NULL;
        }
        result = Tcl_NewStringObj(PyBytes_AS_STRING(encoded), (int)size);
#endif /* USE_TCL_UNICODE */
        Py_DECREF(encoded);
        return result;
    }

    if (PyTclObject_Check(value)) {
        return ((PyTclObject*)value)->value;
    }

    {
        PyObject *v = PyObject_Str(value);
        if (!v)
            return 0;
        result = AsObj(v);
        Py_DECREF(v);
        return result;
    }
}

static PyObject *
fromBoolean(TkappObject *tkapp, Tcl_Obj *value)
{
    int boolValue;
    if (Tcl_GetBooleanFromObj(Tkapp_Interp(tkapp), value, &boolValue) == TCL_ERROR)
        return Tkinter_Error(tkapp);
    return PyBool_FromLong(boolValue);
}

static PyObject*
fromWideIntObj(TkappObject *tkapp, Tcl_Obj *value)
{
        Tcl_WideInt wideValue;
        if (Tcl_GetWideIntFromObj(Tkapp_Interp(tkapp), value, &wideValue) == TCL_OK) {
            if (sizeof(wideValue) <= SIZEOF_LONG_LONG)
                return PyLong_FromLongLong(wideValue);
            return _PyLong_FromByteArray((unsigned char *)(void *)&wideValue,
                                         sizeof(wideValue),
                                         PY_LITTLE_ENDIAN,
                                         /* signed */ 1);
        }
        return NULL;
}

static PyObject*
fromBignumObj(TkappObject *tkapp, Tcl_Obj *value)
{
    mp_int bigValue;
    mp_err err;
#if USE_DEPRECATED_TOMMATH_API
    unsigned long numBytes;
#else
    size_t numBytes;
#endif
    unsigned char *bytes;
    PyObject *res;

    if (Tcl_GetBignumFromObj(Tkapp_Interp(tkapp), value, &bigValue) != TCL_OK)
        return Tkinter_Error(tkapp);
#if USE_DEPRECATED_TOMMATH_API
    numBytes = mp_unsigned_bin_size(&bigValue);
#else
    numBytes = mp_ubin_size(&bigValue);
#endif
    bytes = PyMem_Malloc(numBytes);
    if (bytes == NULL) {
        mp_clear(&bigValue);
        return PyErr_NoMemory();
    }
#if USE_DEPRECATED_TOMMATH_API
    err = mp_to_unsigned_bin_n(&bigValue, bytes, &numBytes);
#else
    err = mp_to_ubin(&bigValue, bytes, numBytes, NULL);
#endif
    if (err != MP_OKAY) {
        mp_clear(&bigValue);
        PyMem_Free(bytes);
        return PyErr_NoMemory();
    }
    res = _PyLong_FromByteArray(bytes, numBytes,
                                /* big-endian */ 0,
                                /* unsigned */ 0);
    PyMem_Free(bytes);
    if (res != NULL && bigValue.sign == MP_NEG) {
        PyObject *res2 = PyNumber_Negative(res);
        Py_SETREF(res, res2);
    }
    mp_clear(&bigValue);
    return res;
}

static PyObject*
FromObj(TkappObject *tkapp, Tcl_Obj *value)
{
    PyObject *result = NULL;
    Tcl_Interp *interp = Tkapp_Interp(tkapp);

    if (value->typePtr == NULL) {
        return unicodeFromTclObj(tkapp, value);
    }

    if (value->typePtr == tkapp->BooleanType ||
        value->typePtr == tkapp->OldBooleanType) {
        return fromBoolean(tkapp, value);
    }

    if (value->typePtr == tkapp->ByteArrayType) {
        Tcl_Size size;
        char *data = (char*)Tcl_GetByteArrayFromObj(value, &size);
        return PyBytes_FromStringAndSize(data, size);
    }

    if (value->typePtr == tkapp->DoubleType) {
        return PyFloat_FromDouble(value->internalRep.doubleValue);
    }

    if (value->typePtr == tkapp->IntType ||
        value->typePtr == tkapp->WideIntType) {
        result = fromWideIntObj(tkapp, value);
        if (result != NULL || PyErr_Occurred())
            return result;
        Tcl_ResetResult(interp);
        /* If there is an error in the wideInt conversion,
           fall through to bignum handling. */
    }

    if (value->typePtr == tkapp->IntType ||
        value->typePtr == tkapp->WideIntType ||
        value->typePtr == tkapp->BignumType) {
        return fromBignumObj(tkapp, value);
    }

    if (value->typePtr == tkapp->ListType) {
        Tcl_Size i, size;
        int status;
        PyObject *elem;
        Tcl_Obj *tcl_elem;

        status = Tcl_ListObjLength(interp, value, &size);
        if (status == TCL_ERROR)
            return Tkinter_Error(tkapp);
        result = PyTuple_New(size);
        if (!result)
            return NULL;
        for (i = 0; i < size; i++) {
            status = Tcl_ListObjIndex(interp, value, i, &tcl_elem);
            if (status == TCL_ERROR) {
                Py_DECREF(result);
                return Tkinter_Error(tkapp);
            }
            elem = FromObj(tkapp, tcl_elem);
            if (!elem) {
                Py_DECREF(result);
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, elem);
        }
        return result;
    }

    if (value->typePtr == tkapp->StringType ||
        value->typePtr == tkapp->UTF32StringType)
    {
        return unicodeFromTclObj(tkapp, value);
    }

    if (tkapp->BignumType == NULL &&
        strcmp(value->typePtr->name, "bignum") == 0) {
        /* bignum type is not registered in Tcl */
        tkapp->BignumType = value->typePtr;
        return fromBignumObj(tkapp, value);
    }

    return newPyTclObject(value);
}

/* This mutex synchronizes inter-thread command calls. */
TCL_DECLARE_MUTEX(call_mutex)

typedef struct Tkapp_CallEvent {
    Tcl_Event ev;            /* Must be first */
    TkappObject *self;
    PyObject *args;
    int flags;
    PyObject **res;
    PyObject **exc;
    Tcl_Condition *done;
} Tkapp_CallEvent;

static void
Tkapp_CallDeallocArgs(Tcl_Obj** objv, Tcl_Obj** objStore, Tcl_Size objc)
{
    Tcl_Size i;
    for (i = 0; i < objc; i++)
        Tcl_DecrRefCount(objv[i]);
    if (objv != objStore)
        PyMem_Free(objv);
}

/* Convert Python objects to Tcl objects. This must happen in the
   interpreter thread, which may or may not be the calling thread. */

static Tcl_Obj**
Tkapp_CallArgs(PyObject *args, Tcl_Obj** objStore, Tcl_Size *pobjc)
{
    Tcl_Obj **objv = objStore;
    Py_ssize_t objc = 0, i;
    if (args == NULL)
        /* do nothing */;

    else if (!(PyTuple_Check(args) || PyList_Check(args))) {
        objv[0] = AsObj(args);
        if (objv[0] == NULL)
            goto finally;
        objc = 1;
        Tcl_IncrRefCount(objv[0]);
    }
    else {
        objc = PySequence_Fast_GET_SIZE(args);

        if (objc > ARGSZ) {
            if (!CHECK_SIZE(objc, sizeof(Tcl_Obj *))) {
                PyErr_SetString(PyExc_OverflowError,
                                PyTuple_Check(args) ? "tuple is too long" :
                                                      "list is too long");
                return NULL;
            }
            objv = (Tcl_Obj **)PyMem_Malloc(((size_t)objc) * sizeof(Tcl_Obj *));
            if (objv == NULL) {
                PyErr_NoMemory();
                objc = 0;
                goto finally;
            }
        }

        for (i = 0; i < objc; i++) {
            PyObject *v = PySequence_Fast_GET_ITEM(args, i);
            if (v == Py_None) {
                objc = i;
                break;
            }
            objv[i] = AsObj(v);
            if (!objv[i]) {
                /* Reset objc, so it attempts to clear
                   objects only up to i. */
                objc = i;
                goto finally;
            }
            Tcl_IncrRefCount(objv[i]);
        }
    }
    *pobjc = (Tcl_Size)objc;
    return objv;
finally:
    Tkapp_CallDeallocArgs(objv, objStore, (Tcl_Size)objc);
    return NULL;
}

/* Convert the results of a command call into a Python string. */

static PyObject *
Tkapp_UnicodeResult(TkappObject *self)
{
    return unicodeFromTclObj(self, Tcl_GetObjResult(self->interp));
}


/* Convert the results of a command call into a Python objects. */

static PyObject *
Tkapp_ObjectResult(TkappObject *self)
{
    PyObject *res = NULL;
    Tcl_Obj *value = Tcl_GetObjResult(self->interp);
    if (self->wantobjects) {
        /* Not sure whether the IncrRef is necessary, but something
           may overwrite the interpreter result while we are
           converting it. */
        Tcl_IncrRefCount(value);
        res = FromObj(self, value);
        Tcl_DecrRefCount(value);
    } else {
        res = unicodeFromTclObj(self, value);
    }
    return res;
}

static int
Tkapp_Trace(TkappObject *self, PyObject *args)
{
    if (args == NULL) {
        return 0;
    }
    if (self->trace) {
        PyObject *res = PyObject_CallObject(self->trace, args);
        if (res == NULL) {
            Py_DECREF(args);
            return 0;
        }
        Py_DECREF(res);
    }
    Py_DECREF(args);
    return 1;
}

#define TRACE(_self, ARGS) do {                 \
        if ((_self)->trace && !Tkapp_Trace((_self), Py_BuildValue ARGS)) { \
            return NULL;                        \
        }                                       \
    } while (0)

/* Tkapp_CallProc is the event procedure that is executed in the context of
   the Tcl interpreter thread. Initially, it holds the Tcl lock, and doesn't
   hold the Python lock. */

static int
Tkapp_CallProc(Tcl_Event *evPtr, int flags)
{
    Tkapp_CallEvent *e = (Tkapp_CallEvent *)evPtr;
    Tcl_Obj *objStore[ARGSZ];
    Tcl_Obj **objv;
    Tcl_Size objc;
    int i;
    ENTER_PYTHON
    if (e->self->trace && !Tkapp_Trace(e->self, PyTuple_Pack(1, e->args))) {
        objv = NULL;
    }
    else {
        objv = Tkapp_CallArgs(e->args, objStore, &objc);
    }
    if (!objv) {
        *(e->exc) = PyErr_GetRaisedException();
        *(e->res) = NULL;
    }
    LEAVE_PYTHON
    if (!objv)
        goto done;
    i = Tcl_EvalObjv(e->self->interp, objc, objv, e->flags);
    ENTER_PYTHON
    if (i == TCL_ERROR) {
        *(e->res) = Tkinter_Error(e->self);
    }
    else {
        *(e->res) = Tkapp_ObjectResult(e->self);
    }
    if (*(e->res) == NULL) {
        *(e->exc) = PyErr_GetRaisedException();
    }
    LEAVE_PYTHON

    Tkapp_CallDeallocArgs(objv, objStore, objc);
done:
    /* Wake up calling thread. */
    Tcl_MutexLock(&call_mutex);
    Tcl_ConditionNotify(e->done);
    Tcl_MutexUnlock(&call_mutex);
    return 1;
}


/* This is the main entry point for calling a Tcl command.
   It supports three cases, with regard to threading:
   1. Tcl is not threaded: Must have the Tcl lock, then can invoke command in
      the context of the calling thread.
   2. Tcl is threaded, caller of the command is in the interpreter thread:
      Execute the command in the calling thread. Since the Tcl lock will
      not be used, we can merge that with case 1.
   3. Tcl is threaded, caller is in a different thread: Must queue an event to
      the interpreter thread. Allocation of Tcl objects needs to occur in the
      interpreter thread, so we ship the PyObject* args to the target thread,
      and perform processing there. */

static PyObject *
Tkapp_Call(PyObject *selfptr, PyObject *args)
{
    Tcl_Obj *objStore[ARGSZ];
    Tcl_Obj **objv = NULL;
    Tcl_Size objc;
    PyObject *res = NULL;
    TkappObject *self = (TkappObject*)selfptr;
    int flags = TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL;

    /* If args is a single tuple, replace with contents of tuple */
    if (PyTuple_GET_SIZE(args) == 1) {
        PyObject *item = PyTuple_GET_ITEM(args, 0);
        if (PyTuple_Check(item))
            args = item;
    }
    if (self->threaded && self->thread_id != Tcl_GetCurrentThread()) {
        /* We cannot call the command directly. Instead, we must
           marshal the parameters to the interpreter thread. */
        Tkapp_CallEvent *ev;
        Tcl_Condition cond = NULL;
        PyObject *exc;
        if (!WaitForMainloop(self))
            return NULL;
        ev = (Tkapp_CallEvent*)attemptckalloc(sizeof(Tkapp_CallEvent));
        if (ev == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        ev->ev.proc = Tkapp_CallProc;
        ev->self = self;
        ev->args = args;
        ev->res = &res;
        ev->exc = &exc;
        ev->done = &cond;

        Tkapp_ThreadSend(self, (Tcl_Event*)ev, &cond, &call_mutex);

        if (res == NULL) {
            if (exc) {
                PyErr_SetRaisedException(exc);
            }
            else {
                PyErr_SetObject(Tkinter_TclError, exc);
            }
        }
        Tcl_ConditionFinalize(&cond);
    }
    else
    {
        TRACE(self, ("(O)", args));

        int i;
        objv = Tkapp_CallArgs(args, objStore, &objc);
        if (!objv)
            return NULL;

        ENTER_TCL

        i = Tcl_EvalObjv(self->interp, objc, objv, flags);

        ENTER_OVERLAP

        if (i == TCL_ERROR)
            Tkinter_Error(self);
        else
            res = Tkapp_ObjectResult(self);

        LEAVE_OVERLAP_TCL

        Tkapp_CallDeallocArgs(objv, objStore, objc);
    }
    return res;
}


/*[clinic input]
_tkinter.tkapp.eval

    script: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_eval_impl(TkappObject *self, const char *script)
/*[clinic end generated code: output=24b79831f700dea0 input=481484123a455f22]*/
{
    PyObject *res = NULL;
    int err;

    CHECK_STRING_LENGTH(script);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "eval", script));

    ENTER_TCL
    err = Tcl_Eval(Tkapp_Interp(self), script);
    ENTER_OVERLAP
    if (err == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = Tkapp_UnicodeResult(self);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.evalfile

    fileName: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_evalfile_impl(TkappObject *self, const char *fileName)
/*[clinic end generated code: output=63be88dcee4f11d3 input=873ab707e5e947e1]*/
{
    PyObject *res = NULL;
    int err;

    CHECK_STRING_LENGTH(fileName);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "source", fileName));

    ENTER_TCL
    err = Tcl_EvalFile(Tkapp_Interp(self), fileName);
    ENTER_OVERLAP
    if (err == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = Tkapp_UnicodeResult(self);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.record

    script: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_record_impl(TkappObject *self, const char *script)
/*[clinic end generated code: output=0ffe08a0061730df input=c0b0db5a21412cac]*/
{
    PyObject *res = NULL;
    int err;

    CHECK_STRING_LENGTH(script);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ssss))", "history", "add", script, "exec"));

    ENTER_TCL
    err = Tcl_RecordAndEval(Tkapp_Interp(self), script, TCL_NO_EVAL);
    ENTER_OVERLAP
    if (err == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = Tkapp_UnicodeResult(self);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.adderrorinfo

    msg: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_adderrorinfo_impl(TkappObject *self, const char *msg)
/*[clinic end generated code: output=52162eaca2ee53cb input=f4b37aec7c7e8c77]*/
{
    CHECK_STRING_LENGTH(msg);
    CHECK_TCL_APPARTMENT;

    ENTER_TCL
    Tcl_AddErrorInfo(Tkapp_Interp(self), msg);
    LEAVE_TCL

    Py_RETURN_NONE;
}



/** Tcl Variable **/

typedef PyObject* (*EventFunc)(TkappObject *, PyObject *, int);

TCL_DECLARE_MUTEX(var_mutex)

typedef struct VarEvent {
    Tcl_Event ev; /* must be first */
    TkappObject *self;
    PyObject *args;
    int flags;
    EventFunc func;
    PyObject **res;
    PyObject **exc;
    Tcl_Condition *cond;
} VarEvent;

/*[python]

class varname_converter(CConverter):
    type = 'const char *'
    converter = 'varname_converter'

[python]*/
/*[python checksum: da39a3ee5e6b4b0d3255bfef95601890afd80709]*/

static int
varname_converter(PyObject *in, void *_out)
{
    const char *s;
    const char **out = (const char**)_out;
    if (PyBytes_Check(in)) {
        if (PyBytes_GET_SIZE(in) > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "bytes object is too long");
            return 0;
        }
        s = PyBytes_AS_STRING(in);
        if (strlen(s) != (size_t)PyBytes_GET_SIZE(in)) {
            PyErr_SetString(PyExc_ValueError, "embedded null byte");
            return 0;
        }
        *out = s;
        return 1;
    }
    if (PyUnicode_Check(in)) {
        Py_ssize_t size;
        s = PyUnicode_AsUTF8AndSize(in, &size);
        if (s == NULL) {
            return 0;
        }
        if (size > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "string is too long");
            return 0;
        }
        if (strlen(s) != (size_t)size) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            return 0;
        }
        *out = s;
        return 1;
    }
    if (PyTclObject_Check(in)) {
        *out = Tcl_GetString(((PyTclObject *)in)->value);
        return 1;
    }
    PyErr_Format(PyExc_TypeError,
                 "must be str, bytes or Tcl_Obj, not %.50s",
                 Py_TYPE(in)->tp_name);
    return 0;
}


static void
var_perform(VarEvent *ev)
{
    *(ev->res) = ev->func(ev->self, ev->args, ev->flags);
    if (!*(ev->res)) {
        *(ev->exc) = PyErr_GetRaisedException();;
    }

}

static int
var_proc(Tcl_Event *evPtr, int flags)
{
    VarEvent *ev = (VarEvent *)evPtr;
    ENTER_PYTHON
    var_perform(ev);
    Tcl_MutexLock(&var_mutex);
    Tcl_ConditionNotify(ev->cond);
    Tcl_MutexUnlock(&var_mutex);
    LEAVE_PYTHON
    return 1;
}


static PyObject*
var_invoke(EventFunc func, PyObject *selfptr, PyObject *args, int flags)
{
    TkappObject *self = (TkappObject*)selfptr;
    if (self->threaded && self->thread_id != Tcl_GetCurrentThread()) {
        VarEvent *ev;
        PyObject *res, *exc;
        Tcl_Condition cond = NULL;

        /* The current thread is not the interpreter thread.  Marshal
           the call to the interpreter thread, then wait for
           completion. */
        if (!WaitForMainloop(self))
            return NULL;

        ev = (VarEvent*)attemptckalloc(sizeof(VarEvent));
        if (ev == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        ev->self = self;
        ev->args = args;
        ev->flags = flags;
        ev->func = func;
        ev->res = &res;
        ev->exc = &exc;
        ev->cond = &cond;
        ev->ev.proc = var_proc;
        Tkapp_ThreadSend(self, (Tcl_Event*)ev, &cond, &var_mutex);
        Tcl_ConditionFinalize(&cond);
        if (!res) {
            PyErr_SetObject((PyObject*)Py_TYPE(exc), exc);
            Py_DECREF(exc);
            return NULL;
        }
        return res;
    }
    /* Tcl is not threaded, or this is the interpreter thread. */
    return func(self, args, flags);
}

static PyObject *
SetVar(TkappObject *self, PyObject *args, int flags)
{
    const char *name1, *name2;
    PyObject *newValue;
    PyObject *res = NULL;
    Tcl_Obj *newval, *ok;

    switch (PyTuple_GET_SIZE(args)) {
    case 2:
        if (!PyArg_ParseTuple(args, "O&O:setvar",
                              varname_converter, &name1, &newValue))
            return NULL;
        /* XXX Acquire tcl lock??? */
        newval = AsObj(newValue);
        if (newval == NULL)
            return NULL;

        if (flags & TCL_GLOBAL_ONLY) {
            TRACE((TkappObject *)self, ("((ssssO))", "uplevel", "#0", "set",
                                        name1, newValue));
        }
        else {
            TRACE((TkappObject *)self, ("((ssO))", "set", name1, newValue));
        }

        ENTER_TCL
        ok = Tcl_SetVar2Ex(Tkapp_Interp(self), name1, NULL,
                           newval, flags);
        ENTER_OVERLAP
        if (!ok)
            Tkinter_Error(self);
        else {
            res = Py_NewRef(Py_None);
        }
        LEAVE_OVERLAP_TCL
        break;
    case 3:
        if (!PyArg_ParseTuple(args, "ssO:setvar",
                              &name1, &name2, &newValue))
            return NULL;
        CHECK_STRING_LENGTH(name1);
        CHECK_STRING_LENGTH(name2);

        /* XXX must hold tcl lock already??? */
        newval = AsObj(newValue);
        if (((TkappObject *)self)->trace) {
            if (flags & TCL_GLOBAL_ONLY) {
                TRACE((TkappObject *)self, ("((sssNO))", "uplevel", "#0", "set",
                                    PyUnicode_FromFormat("%s(%s)", name1, name2),
                                    newValue));
            }
            else {
                TRACE((TkappObject *)self, ("((sNO))", "set",
                                    PyUnicode_FromFormat("%s(%s)", name1, name2),
                                    newValue));
            }
        }

        ENTER_TCL
        ok = Tcl_SetVar2Ex(Tkapp_Interp(self), name1, name2, newval, flags);
        ENTER_OVERLAP
        if (!ok)
            Tkinter_Error(self);
        else {
            res = Py_NewRef(Py_None);
        }
        LEAVE_OVERLAP_TCL
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "setvar requires 2 to 3 arguments");
        return NULL;
    }
    return res;
}

static PyObject *
Tkapp_SetVar(PyObject *self, PyObject *args)
{
    return var_invoke(SetVar, self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalSetVar(PyObject *self, PyObject *args)
{
    return var_invoke(SetVar, self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
GetVar(TkappObject *self, PyObject *args, int flags)
{
    const char *name1, *name2=NULL;
    PyObject *res = NULL;
    Tcl_Obj *tres;

    if (!PyArg_ParseTuple(args, "O&|s:getvar",
                          varname_converter, &name1, &name2))
        return NULL;

    CHECK_STRING_LENGTH(name2);
    ENTER_TCL
    tres = Tcl_GetVar2Ex(Tkapp_Interp(self), name1, name2, flags);
    ENTER_OVERLAP
    if (tres == NULL) {
        Tkinter_Error(self);
    } else {
        if (self->wantobjects) {
            res = FromObj(self, tres);
        }
        else {
            res = unicodeFromTclObj(self, tres);
        }
    }
    LEAVE_OVERLAP_TCL
    return res;
}

static PyObject *
Tkapp_GetVar(PyObject *self, PyObject *args)
{
    return var_invoke(GetVar, self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalGetVar(PyObject *self, PyObject *args)
{
    return var_invoke(GetVar, self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
UnsetVar(TkappObject *self, PyObject *args, int flags)
{
    char *name1, *name2=NULL;
    int code;
    PyObject *res = NULL;

    if (!PyArg_ParseTuple(args, "s|s:unsetvar", &name1, &name2))
        return NULL;

    CHECK_STRING_LENGTH(name1);
    CHECK_STRING_LENGTH(name2);

    if (((TkappObject *)self)->trace) {
        if (flags & TCL_GLOBAL_ONLY) {
            if (name2) {
                TRACE((TkappObject *)self, ("((sssN))", "uplevel", "#0", "unset",
                                PyUnicode_FromFormat("%s(%s)", name1, name2)));
            }
            else {
                TRACE((TkappObject *)self, ("((ssss))", "uplevel", "#0", "unset", name1));
            }
        }
        else {
            if (name2) {
                TRACE((TkappObject *)self, ("((sN))", "unset",
                                PyUnicode_FromFormat("%s(%s)", name1, name2)));
            }
            else {
                TRACE((TkappObject *)self, ("((ss))", "unset", name1));
            }
        }
    }

    ENTER_TCL
    code = Tcl_UnsetVar2(Tkapp_Interp(self), name1, name2, flags);
    ENTER_OVERLAP
    if (code == TCL_ERROR)
        res = Tkinter_Error(self);
    else {
        res = Py_NewRef(Py_None);
    }
    LEAVE_OVERLAP_TCL
    return res;
}

static PyObject *
Tkapp_UnsetVar(PyObject *self, PyObject *args)
{
    return var_invoke(UnsetVar, self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalUnsetVar(PyObject *self, PyObject *args)
{
    return var_invoke(UnsetVar, self, args,
                      TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



/** Tcl to Python **/

/*[clinic input]
_tkinter.tkapp.getint

    arg: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_getint(TkappObject *self, PyObject *arg)
/*[clinic end generated code: output=88cf293fae307cfe input=034026997c5b91f8]*/
{
    char *s;
    Tcl_Obj *value;
    PyObject *result;

    if (PyLong_Check(arg)) {
        return Py_NewRef(arg);
    }

    if (PyTclObject_Check(arg)) {
        value = ((PyTclObject*)arg)->value;
        Tcl_IncrRefCount(value);
    }
    else {
        if (!PyArg_Parse(arg, "s:getint", &s))
            return NULL;
        CHECK_STRING_LENGTH(s);
        value = Tcl_NewStringObj(s, -1);
        if (value == NULL)
            return Tkinter_Error(self);
    }
    /* Don't use Tcl_GetInt() because it returns ambiguous result for value
       in ranges -2**32..-2**31-1 and 2**31..2**32-1 (on 32-bit platform).

       Prefer bignum because Tcl_GetWideIntFromObj returns ambiguous result for
       value in ranges -2**64..-2**63-1 and 2**63..2**64-1 (on 32-bit platform).
     */
    result = fromBignumObj(self, value);
    Tcl_DecrRefCount(value);
    if (result != NULL || PyErr_Occurred())
        return result;
    return Tkinter_Error(self);
}

/*[clinic input]
_tkinter.tkapp.getdouble

    arg: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_getdouble(TkappObject *self, PyObject *arg)
/*[clinic end generated code: output=c52b138bd8b956b9 input=22015729ce9ef7f8]*/
{
    char *s;
    double v;

    if (PyFloat_Check(arg)) {
        return Py_NewRef(arg);
    }

    if (PyNumber_Check(arg)) {
        return PyNumber_Float(arg);
    }

    if (PyTclObject_Check(arg)) {
        if (Tcl_GetDoubleFromObj(Tkapp_Interp(self),
                                 ((PyTclObject*)arg)->value,
                                 &v) == TCL_ERROR)
            return Tkinter_Error(self);
        return PyFloat_FromDouble(v);
    }

    if (!PyArg_Parse(arg, "s:getdouble", &s))
        return NULL;
    CHECK_STRING_LENGTH(s);
    if (Tcl_GetDouble(Tkapp_Interp(self), s, &v) == TCL_ERROR)
        return Tkinter_Error(self);
    return PyFloat_FromDouble(v);
}

/*[clinic input]
_tkinter.tkapp.getboolean

    arg: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_getboolean(TkappObject *self, PyObject *arg)
/*[clinic end generated code: output=726a9ae445821d91 input=7f11248ef8f8776e]*/
{
    char *s;
    int v;

    if (PyLong_Check(arg)) { /* int or bool */
        return PyBool_FromLong(!_PyLong_IsZero((PyLongObject *)arg));
    }

    if (PyTclObject_Check(arg)) {
        if (Tcl_GetBooleanFromObj(Tkapp_Interp(self),
                                  ((PyTclObject*)arg)->value,
                                  &v) == TCL_ERROR)
            return Tkinter_Error(self);
        return PyBool_FromLong(v);
    }

    if (!PyArg_Parse(arg, "s:getboolean", &s))
        return NULL;
    CHECK_STRING_LENGTH(s);
    if (Tcl_GetBoolean(Tkapp_Interp(self), s, &v) == TCL_ERROR)
        return Tkinter_Error(self);
    return PyBool_FromLong(v);
}

/*[clinic input]
_tkinter.tkapp.exprstring

    s: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_exprstring_impl(TkappObject *self, const char *s)
/*[clinic end generated code: output=beda323d3ed0abb1 input=fa78f751afb2f21b]*/
{
    PyObject *res = NULL;
    int retval;

    CHECK_STRING_LENGTH(s);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "expr", s));

    ENTER_TCL
    retval = Tcl_ExprString(Tkapp_Interp(self), s);
    ENTER_OVERLAP
    if (retval == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = Tkapp_UnicodeResult(self);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.exprlong

    s: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_exprlong_impl(TkappObject *self, const char *s)
/*[clinic end generated code: output=5d6a46b63c6ebcf9 input=11bd7eee0c57b4dc]*/
{
    PyObject *res = NULL;
    int retval;
    long v;

    CHECK_STRING_LENGTH(s);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "expr", s));

    ENTER_TCL
    retval = Tcl_ExprLong(Tkapp_Interp(self), s, &v);
    ENTER_OVERLAP
    if (retval == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = PyLong_FromLong(v);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.exprdouble

    s: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_exprdouble_impl(TkappObject *self, const char *s)
/*[clinic end generated code: output=ff78df1081ea4158 input=ff02bc11798832d5]*/
{
    PyObject *res = NULL;
    double v;
    int retval;

    CHECK_STRING_LENGTH(s);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "expr", s));

    ENTER_TCL
    retval = Tcl_ExprDouble(Tkapp_Interp(self), s, &v);
    ENTER_OVERLAP
    if (retval == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = PyFloat_FromDouble(v);
    LEAVE_OVERLAP_TCL
    return res;
}

/*[clinic input]
_tkinter.tkapp.exprboolean

    s: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_exprboolean_impl(TkappObject *self, const char *s)
/*[clinic end generated code: output=8b28038c22887311 input=c8c66022bdb8d5d3]*/
{
    PyObject *res = NULL;
    int retval;
    int v;

    CHECK_STRING_LENGTH(s);
    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((ss))", "expr", s));

    ENTER_TCL
    retval = Tcl_ExprBoolean(Tkapp_Interp(self), s, &v);
    ENTER_OVERLAP
    if (retval == TCL_ERROR)
        res = Tkinter_Error(self);
    else
        res = PyLong_FromLong(v);
    LEAVE_OVERLAP_TCL
    return res;
}



/*[clinic input]
_tkinter.tkapp.splitlist

    arg: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_splitlist(TkappObject *self, PyObject *arg)
/*[clinic end generated code: output=13b51d34386d36fb input=2b2e13351e3c0b53]*/
{
    char *list;
    Tcl_Size argc, i;
    const char **argv;
    PyObject *v;

    if (PyTclObject_Check(arg)) {
        Tcl_Size objc;
        Tcl_Obj **objv;
        if (Tcl_ListObjGetElements(Tkapp_Interp(self),
                                   ((PyTclObject*)arg)->value,
                                   &objc, &objv) == TCL_ERROR) {
            return Tkinter_Error(self);
        }
        if (!(v = PyTuple_New(objc)))
            return NULL;
        for (i = 0; i < objc; i++) {
            PyObject *s = FromObj(self, objv[i]);
            if (!s) {
                Py_DECREF(v);
                return NULL;
            }
            PyTuple_SET_ITEM(v, i, s);
        }
        return v;
    }
    if (PyTuple_Check(arg)) {
        return Py_NewRef(arg);
    }
    if (PyList_Check(arg)) {
        return PySequence_Tuple(arg);
    }

    if (!PyArg_Parse(arg, "et:splitlist", "utf-8", &list))
        return NULL;

    if (strlen(list) >= INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "string is too long");
        PyMem_Free(list);
        return NULL;
    }
    if (Tcl_SplitList(Tkapp_Interp(self), list,
                      &argc, &argv) == TCL_ERROR)  {
        PyMem_Free(list);
        return Tkinter_Error(self);
    }

    if (!(v = PyTuple_New(argc)))
        goto finally;

    for (i = 0; i < argc; i++) {
        PyObject *s = unicodeFromTclString(argv[i]);
        if (!s) {
            Py_SETREF(v, NULL);
            goto finally;
        }
        PyTuple_SET_ITEM(v, i, s);
    }

  finally:
    ckfree(FREECAST argv);
    PyMem_Free(list);
    return v;
}


/** Tcl Command **/

/* Client data struct */
typedef struct {
    TkappObject *self;
    PyObject *func;
} PythonCmd_ClientData;

static int
PythonCmd_Error(Tcl_Interp *interp)
{
    errorInCmd = 1;
    excInCmd = PyErr_GetRaisedException();
    LEAVE_PYTHON
    return TCL_ERROR;
}

/* This is the Tcl command that acts as a wrapper for Python
 * function or method.
 */
static int
PythonCmd(ClientData clientData, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[])
{
    PythonCmd_ClientData *data = (PythonCmd_ClientData *)clientData;
    PyObject *args, *res;
    int i;
    Tcl_Obj *obj_res;
    int objargs = data->self->wantobjects >= 2;

    ENTER_PYTHON

    /* Create argument tuple (objv1, ..., objvN) */
    if (!(args = PyTuple_New(objc - 1)))
        return PythonCmd_Error(interp);

    for (i = 0; i < (objc - 1); i++) {
        PyObject *s = objargs ? FromObj(data->self, objv[i + 1])
                              : unicodeFromTclObj(data->self, objv[i + 1]);
        if (!s) {
            Py_DECREF(args);
            return PythonCmd_Error(interp);
        }
        PyTuple_SET_ITEM(args, i, s);
    }

    res = PyObject_Call(data->func, args, NULL);
    Py_DECREF(args);

    if (res == NULL)
        return PythonCmd_Error(interp);

    obj_res = AsObj(res);
    if (obj_res == NULL) {
        Py_DECREF(res);
        return PythonCmd_Error(interp);
    }
    Tcl_SetObjResult(interp, obj_res);
    Py_DECREF(res);

    LEAVE_PYTHON

    return TCL_OK;
}


static void
PythonCmdDelete(ClientData clientData)
{
    PythonCmd_ClientData *data = (PythonCmd_ClientData *)clientData;

    ENTER_PYTHON
    Py_XDECREF(data->self);
    Py_XDECREF(data->func);
    PyMem_Free(data);
    LEAVE_PYTHON
}




TCL_DECLARE_MUTEX(command_mutex)

typedef struct CommandEvent{
    Tcl_Event ev;
    Tcl_Interp* interp;
    const char *name;
    int create;
    int *status;
    ClientData *data;
    Tcl_Condition *done;
} CommandEvent;

static int
Tkapp_CommandProc(Tcl_Event *evPtr, int flags)
{
    CommandEvent *ev = (CommandEvent *)evPtr;
    if (ev->create)
        *ev->status = Tcl_CreateObjCommand(
            ev->interp, ev->name, PythonCmd,
            ev->data, PythonCmdDelete) == NULL;
    else
        *ev->status = Tcl_DeleteCommand(ev->interp, ev->name);
    Tcl_MutexLock(&command_mutex);
    Tcl_ConditionNotify(ev->done);
    Tcl_MutexUnlock(&command_mutex);
    return 1;
}

/*[clinic input]
_tkinter.tkapp.createcommand

    name: str
    func: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_createcommand_impl(TkappObject *self, const char *name,
                                  PyObject *func)
/*[clinic end generated code: output=2a1c79a4ee2af410 input=255785cb70edc6a0]*/
{
    PythonCmd_ClientData *data;
    int err;

    CHECK_STRING_LENGTH(name);
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "command not callable");
        return NULL;
    }

    if (self->threaded && self->thread_id != Tcl_GetCurrentThread() &&
        !WaitForMainloop(self))
        return NULL;

    TRACE(self, ("((ss()O))", "proc", name, func));

    data = PyMem_NEW(PythonCmd_ClientData, 1);
    if (!data)
        return PyErr_NoMemory();
    Py_INCREF(self);
    data->self = self;
    data->func = Py_NewRef(func);
    if (self->threaded && self->thread_id != Tcl_GetCurrentThread()) {
        Tcl_Condition cond = NULL;
        CommandEvent *ev = (CommandEvent*)attemptckalloc(sizeof(CommandEvent));
        if (ev == NULL) {
            PyErr_NoMemory();
            PyMem_Free(data);
            return NULL;
        }
        ev->ev.proc = Tkapp_CommandProc;
        ev->interp = self->interp;
        ev->create = 1;
        ev->name = name;
        ev->data = (ClientData)data;
        ev->status = &err;
        ev->done = &cond;
        Tkapp_ThreadSend(self, (Tcl_Event*)ev, &cond, &command_mutex);
        Tcl_ConditionFinalize(&cond);
    }
    else
    {
        ENTER_TCL
        err = Tcl_CreateObjCommand(
            Tkapp_Interp(self), name, PythonCmd,
            (ClientData)data, PythonCmdDelete) == NULL;
        LEAVE_TCL
    }
    if (err) {
        PyErr_SetString(Tkinter_TclError, "can't create Tcl command");
        PyMem_Free(data);
        return NULL;
    }

    Py_RETURN_NONE;
}



/*[clinic input]
_tkinter.tkapp.deletecommand

    name: str
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_deletecommand_impl(TkappObject *self, const char *name)
/*[clinic end generated code: output=a67e8cb5845e0d2d input=53e9952eae1f85f5]*/
{
    int err;

    CHECK_STRING_LENGTH(name);

    TRACE(self, ("((sss))", "rename", name, ""));

    if (self->threaded && self->thread_id != Tcl_GetCurrentThread()) {
        Tcl_Condition cond = NULL;
        CommandEvent *ev;
        ev = (CommandEvent*)attemptckalloc(sizeof(CommandEvent));
        if (ev == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        ev->ev.proc = Tkapp_CommandProc;
        ev->interp = self->interp;
        ev->create = 0;
        ev->name = name;
        ev->status = &err;
        ev->done = &cond;
        Tkapp_ThreadSend(self, (Tcl_Event*)ev, &cond,
                         &command_mutex);
        Tcl_ConditionFinalize(&cond);
    }
    else
    {
        ENTER_TCL
        err = Tcl_DeleteCommand(self->interp, name);
        LEAVE_TCL
    }
    if (err == -1) {
        PyErr_SetString(Tkinter_TclError, "can't delete Tcl command");
        return NULL;
    }
    Py_RETURN_NONE;
}



#ifdef HAVE_CREATEFILEHANDLER
/** File Handler **/

typedef struct _fhcdata {
    PyObject *func;
    PyObject *file;
    int id;
    struct _fhcdata *next;
} FileHandler_ClientData;

static FileHandler_ClientData *HeadFHCD;

static FileHandler_ClientData *
NewFHCD(PyObject *func, PyObject *file, int id)
{
    FileHandler_ClientData *p;
    p = PyMem_NEW(FileHandler_ClientData, 1);
    if (p != NULL) {
        p->func = Py_XNewRef(func);
        p->file = Py_XNewRef(file);
        p->id = id;
        p->next = HeadFHCD;
        HeadFHCD = p;
    }
    return p;
}

static void
DeleteFHCD(int id)
{
    FileHandler_ClientData *p, **pp;

    pp = &HeadFHCD;
    while ((p = *pp) != NULL) {
        if (p->id == id) {
            *pp = p->next;
            Py_XDECREF(p->func);
            Py_XDECREF(p->file);
            PyMem_Free(p);
        }
        else
            pp = &p->next;
    }
}

static void
FileHandler(ClientData clientData, int mask)
{
    FileHandler_ClientData *data = (FileHandler_ClientData *)clientData;
    PyObject *func, *file, *res;

    ENTER_PYTHON
    func = data->func;
    file = data->file;

    res = PyObject_CallFunction(func, "Oi", file, mask);
    if (res == NULL) {
        errorInCmd = 1;
        excInCmd = PyErr_GetRaisedException();
    }
    Py_XDECREF(res);
    LEAVE_PYTHON
}

/*[clinic input]
_tkinter.tkapp.createfilehandler

    file: object
    mask: int
    func: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_createfilehandler_impl(TkappObject *self, PyObject *file,
                                      int mask, PyObject *func)
/*[clinic end generated code: output=f73ce82de801c353 input=84943a5286e47947]*/
{
    FileHandler_ClientData *data;
    int tfile;

    CHECK_TCL_APPARTMENT;

    tfile = PyObject_AsFileDescriptor(file);
    if (tfile < 0)
        return NULL;
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "bad argument list");
        return NULL;
    }

    TRACE(self, ("((ssiiO))", "#", "createfilehandler", tfile, mask, func));

    data = NewFHCD(func, file, tfile);
    if (data == NULL)
        return NULL;

    /* Ought to check for null Tcl_File object... */
    ENTER_TCL
    Tcl_CreateFileHandler(tfile, mask, FileHandler, (ClientData) data);
    LEAVE_TCL
    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.tkapp.deletefilehandler

    file: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_deletefilehandler(TkappObject *self, PyObject *file)
/*[clinic end generated code: output=b53cc96ebf9476fd input=abbec19d66312e2a]*/
{
    int tfile;

    CHECK_TCL_APPARTMENT;

    tfile = PyObject_AsFileDescriptor(file);
    if (tfile < 0)
        return NULL;

    TRACE(self, ("((ssi))", "#", "deletefilehandler", tfile));

    DeleteFHCD(tfile);

    /* Ought to check for null Tcl_File object... */
    ENTER_TCL
    Tcl_DeleteFileHandler(tfile);
    LEAVE_TCL
    Py_RETURN_NONE;
}
#endif /* HAVE_CREATEFILEHANDLER */


/**** Tktt Object (timer token) ****/

static PyObject *Tktt_Type;

typedef struct {
    PyObject_HEAD
    Tcl_TimerToken token;
    PyObject *func;
} TkttObject;

/*[clinic input]
_tkinter.tktimertoken.deletetimerhandler

[clinic start generated code]*/

static PyObject *
_tkinter_tktimertoken_deletetimerhandler_impl(TkttObject *self)
/*[clinic end generated code: output=bd7fe17f328cfa55 input=40bd070ff85f5cf3]*/
{
    TkttObject *v = self;
    PyObject *func = v->func;

    if (v->token != NULL) {
        /* TRACE(...) */
        Tcl_DeleteTimerHandler(v->token);
        v->token = NULL;
    }
    if (func != NULL) {
        v->func = NULL;
        Py_DECREF(func);
        Py_DECREF(v); /* See Tktt_New() */
    }
    Py_RETURN_NONE;
}

static TkttObject *
Tktt_New(PyObject *func)
{
    TkttObject *v;

    v = PyObject_New(TkttObject, (PyTypeObject *) Tktt_Type);
    if (v == NULL)
        return NULL;

    v->token = NULL;
    v->func = Py_NewRef(func);

    /* Extra reference, deleted when called or when handler is deleted */
    return (TkttObject*)Py_NewRef(v);
}

static void
Tktt_Dealloc(PyObject *self)
{
    TkttObject *v = (TkttObject *)self;
    PyObject *func = v->func;
    PyObject *tp = (PyObject *) Py_TYPE(self);

    Py_XDECREF(func);

    PyObject_Free(self);
    Py_DECREF(tp);
}

static PyObject *
Tktt_Repr(PyObject *self)
{
    TkttObject *v = (TkttObject *)self;
    return PyUnicode_FromFormat("<tktimertoken at %p%s>",
                                v,
                                v->func == NULL ? ", handler deleted" : "");
}

/** Timer Handler **/

static void
TimerHandler(ClientData clientData)
{
    TkttObject *v = (TkttObject *)clientData;
    PyObject *func = v->func;
    PyObject *res;

    if (func == NULL)
        return;

    v->func = NULL;

    ENTER_PYTHON

    res = PyObject_CallNoArgs(func);
    Py_DECREF(func);
    Py_DECREF(v); /* See Tktt_New() */

    if (res == NULL) {
        errorInCmd = 1;
        excInCmd = PyErr_GetRaisedException();
    }
    else
        Py_DECREF(res);

    LEAVE_PYTHON
}

/*[clinic input]
_tkinter.tkapp.createtimerhandler

    milliseconds: int
    func: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_createtimerhandler_impl(TkappObject *self, int milliseconds,
                                       PyObject *func)
/*[clinic end generated code: output=2da5959b9d031911 input=ba6729f32f0277a5]*/
{
    TkttObject *v;

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "bad argument list");
        return NULL;
    }

    CHECK_TCL_APPARTMENT;

    TRACE(self, ("((siO))", "after", milliseconds, func));

    v = Tktt_New(func);
    if (v) {
        v->token = Tcl_CreateTimerHandler(milliseconds, TimerHandler,
                                          (ClientData)v);
    }

    return (PyObject *) v;
}


/** Event Loop **/

/*[clinic input]
_tkinter.tkapp.mainloop

    threshold: int = 0
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_mainloop_impl(TkappObject *self, int threshold)
/*[clinic end generated code: output=0ba8eabbe57841b0 input=036bcdcf03d5eca0]*/
{
    PyThreadState *tstate = PyThreadState_Get();

    CHECK_TCL_APPARTMENT;
    self->dispatching = 1;

    quitMainLoop = 0;
    while (Tk_GetNumMainWindows() > threshold &&
           !quitMainLoop &&
           !errorInCmd)
    {
        int result;

        if (self->threaded) {
            /* Allow other Python threads to run. */
            ENTER_TCL
            result = Tcl_DoOneEvent(0);
            LEAVE_TCL
        }
        else {
            Py_BEGIN_ALLOW_THREADS
            if(tcl_lock)PyThread_acquire_lock(tcl_lock, 1);
            tcl_tstate = tstate;
            result = Tcl_DoOneEvent(TCL_DONT_WAIT);
            tcl_tstate = NULL;
            if(tcl_lock)PyThread_release_lock(tcl_lock);
            if (result == 0)
                Sleep(Tkinter_busywaitinterval);
            Py_END_ALLOW_THREADS
        }

        if (PyErr_CheckSignals() != 0) {
            self->dispatching = 0;
            return NULL;
        }
        if (result < 0)
            break;
    }
    self->dispatching = 0;
    quitMainLoop = 0;

    if (errorInCmd) {
        errorInCmd = 0;
        PyErr_SetRaisedException(excInCmd);
        excInCmd = NULL;
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.tkapp.dooneevent

    flags: int = 0
    /

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_dooneevent_impl(TkappObject *self, int flags)
/*[clinic end generated code: output=27c6b2aa464cac29 input=6542b928e364b793]*/
{
    int rv;

    ENTER_TCL
    rv = Tcl_DoOneEvent(flags);
    LEAVE_TCL
    return PyLong_FromLong(rv);
}

/*[clinic input]
_tkinter.tkapp.quit
[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_quit_impl(TkappObject *self)
/*[clinic end generated code: output=7f21eeff481f754f input=e03020dc38aff23c]*/
{
    quitMainLoop = 1;
    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.tkapp.interpaddr
[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_interpaddr_impl(TkappObject *self)
/*[clinic end generated code: output=6caaae3273b3c95a input=2dd32cbddb55a111]*/
{
    return PyLong_FromVoidPtr(Tkapp_Interp(self));
}

/*[clinic input]
_tkinter.tkapp.loadtk
[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_loadtk_impl(TkappObject *self)
/*[clinic end generated code: output=e9e10a954ce46d2a input=b5e82afedd6354f0]*/
{
    Tcl_Interp *interp = Tkapp_Interp(self);
    const char * _tk_exists = NULL;
    int err;

    /* We want to guard against calling Tk_Init() multiple times */
    CHECK_TCL_APPARTMENT;
    ENTER_TCL
    err = Tcl_Eval(Tkapp_Interp(self), "info exists     tk_version");
    ENTER_OVERLAP
    if (err == TCL_ERROR) {
        /* This sets an exception, but we cannot return right
           away because we need to exit the overlap first. */
        Tkinter_Error(self);
    } else {
        _tk_exists = Tcl_GetStringResult(Tkapp_Interp(self));
    }
    LEAVE_OVERLAP_TCL
    if (err == TCL_ERROR) {
        return NULL;
    }
    if (_tk_exists == NULL || strcmp(_tk_exists, "1") != 0)     {
        if (Tk_Init(interp)             == TCL_ERROR) {
            Tkinter_Error(self);
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
Tkapp_WantObjects(PyObject *self, PyObject *args)
{

    int wantobjects = -1;
    if (!PyArg_ParseTuple(args, "|i:wantobjects", &wantobjects))
        return NULL;
    if (wantobjects == -1)
        return PyLong_FromLong(((TkappObject*)self)->wantobjects);
    ((TkappObject*)self)->wantobjects = wantobjects;

    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.tkapp.settrace

    func: object
    /

Set the tracing function.
[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_settrace(TkappObject *self, PyObject *func)
/*[clinic end generated code: output=847f6ebdf46e84fa input=31b260d46d3d018a]*/
{
    if (func == Py_None) {
        func = NULL;
    }
    else {
        Py_INCREF(func);
    }
    Py_XSETREF(self->trace, func);
    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.tkapp.gettrace

Get the tracing function.
[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_gettrace_impl(TkappObject *self)
/*[clinic end generated code: output=d4e2ba7d63e77bb5 input=ac2aea5be74e8c4c]*/
{
    PyObject *func = self->trace;
    if (!func) {
        func = Py_None;
    }
    Py_INCREF(func);
    return func;
}

/*[clinic input]
_tkinter.tkapp.willdispatch

[clinic start generated code]*/

static PyObject *
_tkinter_tkapp_willdispatch_impl(TkappObject *self)
/*[clinic end generated code: output=0e3f46d244642155 input=d88f5970843d6dab]*/
{
    self->dispatching = 1;

    Py_RETURN_NONE;
}


/**** Tkapp Type Methods ****/

static void
Tkapp_Dealloc(PyObject *self)
{
    PyObject *tp = (PyObject *) Py_TYPE(self);
    /*CHECK_TCL_APPARTMENT;*/
    ENTER_TCL
    Tcl_DeleteInterp(Tkapp_Interp(self));
    LEAVE_TCL
    Py_XDECREF(((TkappObject *)self)->trace);
    PyObject_Free(self);
    Py_DECREF(tp);
    DisableEventHook();
}



/**** Tkinter Module ****/

typedef struct {
    PyObject* tuple;
    Py_ssize_t size; /* current size */
    Py_ssize_t maxsize; /* allocated size */
} FlattenContext;

static int
_bump(FlattenContext* context, Py_ssize_t size)
{
    /* expand tuple to hold (at least) size new items.
       return true if successful, false if an exception was raised */

    Py_ssize_t maxsize = context->maxsize * 2;  /* never overflows */

    if (maxsize < context->size + size)
        maxsize = context->size + size;  /* never overflows */

    context->maxsize = maxsize;

    return _PyTuple_Resize(&context->tuple, maxsize) >= 0;
}

static int
_flatten1(FlattenContext* context, PyObject* item, int depth)
{
    /* add tuple or list to argument tuple (recursively) */

    Py_ssize_t i, size;

    if (depth > 1000) {
        PyErr_SetString(PyExc_ValueError,
                        "nesting too deep in _flatten");
        return 0;
    } else if (PyTuple_Check(item) || PyList_Check(item)) {
        size = PySequence_Fast_GET_SIZE(item);
        /* preallocate (assume no nesting) */
        if (context->size + size > context->maxsize &&
            !_bump(context, size))
            return 0;
        /* copy items to output tuple */
        for (i = 0; i < size; i++) {
            PyObject *o = PySequence_Fast_GET_ITEM(item, i);
            if (PyList_Check(o) || PyTuple_Check(o)) {
                if (!_flatten1(context, o, depth + 1))
                    return 0;
            } else if (o != Py_None) {
                if (context->size + 1 > context->maxsize &&
                    !_bump(context, 1))
                    return 0;
                PyTuple_SET_ITEM(context->tuple,
                                 context->size++, Py_NewRef(o));
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "argument must be sequence");
        return 0;
    }
    return 1;
}

/*[clinic input]
_tkinter._flatten

    item: object
    /

[clinic start generated code]*/

static PyObject *
_tkinter__flatten(PyObject *module, PyObject *item)
/*[clinic end generated code: output=cad02a3f97f29862 input=6b9c12260aa1157f]*/
{
    FlattenContext context;

    context.maxsize = PySequence_Size(item);
    if (context.maxsize < 0)
        return NULL;
    if (context.maxsize == 0)
        return PyTuple_New(0);

    context.tuple = PyTuple_New(context.maxsize);
    if (!context.tuple)
        return NULL;

    context.size = 0;

    if (!_flatten1(&context, item, 0)) {
        Py_XDECREF(context.tuple);
        return NULL;
    }

    if (_PyTuple_Resize(&context.tuple, context.size))
        return NULL;

    return context.tuple;
}

/*[clinic input]
_tkinter.create

    screenName: str(accept={str, NoneType}) = None
    baseName: str = ""
    className: str = "Tk"
    interactive: bool = False
    wantobjects: int = 0
    wantTk: bool = True
        if false, then Tk_Init() doesn't get called
    sync: bool = False
        if true, then pass -sync to wish
    use: str(accept={str, NoneType}) = None
        if not None, then pass -use to wish
    /

[clinic start generated code]*/

static PyObject *
_tkinter_create_impl(PyObject *module, const char *screenName,
                     const char *baseName, const char *className,
                     int interactive, int wantobjects, int wantTk, int sync,
                     const char *use)
/*[clinic end generated code: output=e3315607648e6bb4 input=7e382ba431bed537]*/
{
    /* XXX baseName is not used anymore;
     * try getting rid of it. */
    CHECK_STRING_LENGTH(screenName);
    CHECK_STRING_LENGTH(baseName);
    CHECK_STRING_LENGTH(className);
    CHECK_STRING_LENGTH(use);

    return (PyObject *) Tkapp_New(screenName, className,
                                  interactive, wantobjects, wantTk,
                                  sync, use);
}

/*[clinic input]
_tkinter.setbusywaitinterval

    new_val: int
    /

Set the busy-wait interval in milliseconds between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.

It should be set to a divisor of the maximum time between frames in an animation.
[clinic start generated code]*/

static PyObject *
_tkinter_setbusywaitinterval_impl(PyObject *module, int new_val)
/*[clinic end generated code: output=42bf7757dc2d0ab6 input=deca1d6f9e6dae47]*/
{
    if (new_val < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "busywaitinterval must be >= 0");
        return NULL;
    }
    Tkinter_busywaitinterval = new_val;
    Py_RETURN_NONE;
}

/*[clinic input]
_tkinter.getbusywaitinterval -> int

Return the current busy-wait interval between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.
[clinic start generated code]*/

static int
_tkinter_getbusywaitinterval_impl(PyObject *module)
/*[clinic end generated code: output=23b72d552001f5c7 input=a695878d2d576a84]*/
{
    return Tkinter_busywaitinterval;
}

#include "clinic/_tkinter.c.h"

static PyMethodDef Tktt_methods[] =
{
    _TKINTER_TKTIMERTOKEN_DELETETIMERHANDLER_METHODDEF
    {NULL, NULL}
};

static PyType_Slot Tktt_Type_slots[] = {
    {Py_tp_dealloc, Tktt_Dealloc},
    {Py_tp_repr, Tktt_Repr},
    {Py_tp_methods, Tktt_methods},
    {0, 0}
};

static PyType_Spec Tktt_Type_spec = {
    "_tkinter.tktimertoken",
    sizeof(TkttObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    Tktt_Type_slots,
};


/**** Tkapp Method List ****/

static PyMethodDef Tkapp_methods[] =
{
    _TKINTER_TKAPP_WILLDISPATCH_METHODDEF
    {"wantobjects",            Tkapp_WantObjects, METH_VARARGS},
    _TKINTER_TKAPP_SETTRACE_METHODDEF
    _TKINTER_TKAPP_GETTRACE_METHODDEF
    {"call",                   Tkapp_Call, METH_VARARGS},
    _TKINTER_TKAPP_EVAL_METHODDEF
    _TKINTER_TKAPP_EVALFILE_METHODDEF
    _TKINTER_TKAPP_RECORD_METHODDEF
    _TKINTER_TKAPP_ADDERRORINFO_METHODDEF
    {"setvar",                 Tkapp_SetVar, METH_VARARGS},
    {"globalsetvar",       Tkapp_GlobalSetVar, METH_VARARGS},
    {"getvar",       Tkapp_GetVar, METH_VARARGS},
    {"globalgetvar",       Tkapp_GlobalGetVar, METH_VARARGS},
    {"unsetvar",     Tkapp_UnsetVar, METH_VARARGS},
    {"globalunsetvar",     Tkapp_GlobalUnsetVar, METH_VARARGS},
    _TKINTER_TKAPP_GETINT_METHODDEF
    _TKINTER_TKAPP_GETDOUBLE_METHODDEF
    _TKINTER_TKAPP_GETBOOLEAN_METHODDEF
    _TKINTER_TKAPP_EXPRSTRING_METHODDEF
    _TKINTER_TKAPP_EXPRLONG_METHODDEF
    _TKINTER_TKAPP_EXPRDOUBLE_METHODDEF
    _TKINTER_TKAPP_EXPRBOOLEAN_METHODDEF
    _TKINTER_TKAPP_SPLITLIST_METHODDEF
    _TKINTER_TKAPP_CREATECOMMAND_METHODDEF
    _TKINTER_TKAPP_DELETECOMMAND_METHODDEF
    _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
    _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
    _TKINTER_TKAPP_CREATETIMERHANDLER_METHODDEF
    _TKINTER_TKAPP_MAINLOOP_METHODDEF
    _TKINTER_TKAPP_DOONEEVENT_METHODDEF
    _TKINTER_TKAPP_QUIT_METHODDEF
    _TKINTER_TKAPP_INTERPADDR_METHODDEF
    _TKINTER_TKAPP_LOADTK_METHODDEF
    {NULL,                     NULL}
};

static PyType_Slot Tkapp_Type_slots[] = {
    {Py_tp_dealloc, Tkapp_Dealloc},
    {Py_tp_methods, Tkapp_methods},
    {0, 0}
};


static PyType_Spec Tkapp_Type_spec = {
    "_tkinter.tkapp",
    sizeof(TkappObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    Tkapp_Type_slots,
};

static PyMethodDef moduleMethods[] =
{
    _TKINTER__FLATTEN_METHODDEF
    _TKINTER_CREATE_METHODDEF
    _TKINTER_SETBUSYWAITINTERVAL_METHODDEF
    _TKINTER_GETBUSYWAITINTERVAL_METHODDEF
    {NULL,                 NULL}
};

#ifdef WAIT_FOR_STDIN

static int stdin_ready = 0;

#ifndef MS_WINDOWS
static void
MyFileProc(void *clientData, int mask)
{
    stdin_ready = 1;
}
#endif

static PyThreadState *event_tstate = NULL;

static int
EventHook(void)
{
#ifndef MS_WINDOWS
    int tfile;
#endif
    PyEval_RestoreThread(event_tstate);
    stdin_ready = 0;
    errorInCmd = 0;
#ifndef MS_WINDOWS
    tfile = fileno(stdin);
    Tcl_CreateFileHandler(tfile, TCL_READABLE, MyFileProc, NULL);
#endif
    while (!errorInCmd && !stdin_ready) {
        int result;
#ifdef MS_WINDOWS
        if (_kbhit()) {
            stdin_ready = 1;
            break;
        }
#endif
        Py_BEGIN_ALLOW_THREADS
        if(tcl_lock)PyThread_acquire_lock(tcl_lock, 1);
        tcl_tstate = event_tstate;

        result = Tcl_DoOneEvent(TCL_DONT_WAIT);

        tcl_tstate = NULL;
        if(tcl_lock)PyThread_release_lock(tcl_lock);
        if (result == 0)
            Sleep(Tkinter_busywaitinterval);
        Py_END_ALLOW_THREADS

        if (result < 0)
            break;
    }
#ifndef MS_WINDOWS
    Tcl_DeleteFileHandler(tfile);
#endif
    if (errorInCmd) {
        errorInCmd = 0;
        PyErr_SetRaisedException(excInCmd);
        excInCmd = NULL;
        PyErr_Print();
    }
    PyEval_SaveThread();
    return 0;
}

#endif

static void
EnableEventHook(void)
{
#ifdef WAIT_FOR_STDIN
    if (PyOS_InputHook == NULL) {
        event_tstate = PyThreadState_Get();
        PyOS_InputHook = EventHook;
    }
#endif
}

static void
DisableEventHook(void)
{
#ifdef WAIT_FOR_STDIN
    if (Tk_GetNumMainWindows() == 0 && PyOS_InputHook == EventHook) {
        PyOS_InputHook = NULL;
    }
#endif
}


static struct PyModuleDef _tkintermodule = {
    PyModuleDef_HEAD_INIT,
    "_tkinter",
    NULL,
    -1,
    moduleMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__tkinter(void)
{
    PyObject *m, *uexe, *cexe;

    tcl_lock = PyThread_allocate_lock();
    if (tcl_lock == NULL)
        return NULL;

    m = PyModule_Create(&_tkintermodule);
    if (m == NULL)
        return NULL;
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(m, Py_MOD_GIL_NOT_USED);
#endif

    Tkinter_TclError = PyErr_NewException("_tkinter.TclError", NULL, NULL);
    if (PyModule_AddObjectRef(m, "TclError", Tkinter_TclError)) {
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddIntConstant(m, "READABLE", TCL_READABLE)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "WRITABLE", TCL_WRITABLE)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "EXCEPTION", TCL_EXCEPTION)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "WINDOW_EVENTS", TCL_WINDOW_EVENTS)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "FILE_EVENTS", TCL_FILE_EVENTS)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "TIMER_EVENTS", TCL_TIMER_EVENTS)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "IDLE_EVENTS", TCL_IDLE_EVENTS)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "ALL_EVENTS", TCL_ALL_EVENTS)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "DONT_WAIT", TCL_DONT_WAIT)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddStringConstant(m, "TK_VERSION", TK_VERSION)) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddStringConstant(m, "TCL_VERSION", TCL_VERSION)) {
        Py_DECREF(m);
        return NULL;
    }

    Tkapp_Type = PyType_FromSpec(&Tkapp_Type_spec);
    if (PyModule_AddObjectRef(m, "TkappType", Tkapp_Type)) {
        Py_DECREF(m);
        return NULL;
    }

    Tktt_Type = PyType_FromSpec(&Tktt_Type_spec);
    if (PyModule_AddObjectRef(m, "TkttType", Tktt_Type)) {
        Py_DECREF(m);
        return NULL;
    }

    PyTclObject_Type = PyType_FromSpec(&PyTclObject_Type_spec);
    if (PyModule_AddObjectRef(m, "Tcl_Obj", PyTclObject_Type)) {
        Py_DECREF(m);
        return NULL;
    }


    /* This helps the dynamic loader; in Unicode aware Tcl versions
       it also helps Tcl find its encodings. */
    (void) _PySys_GetOptionalAttrString("executable", &uexe);
    if (uexe && PyUnicode_Check(uexe)) {   // sys.executable can be None
        cexe = PyUnicode_EncodeFSDefault(uexe);
        Py_DECREF(uexe);
        if (cexe) {
#ifdef MS_WINDOWS
            int set_var = 0;
            PyObject *str_path;
            wchar_t *wcs_path;
            DWORD ret;

            ret = GetEnvironmentVariableW(L"TCL_LIBRARY", NULL, 0);

            if (!ret && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
                str_path = _get_tcl_lib_path();
                if (str_path == NULL && PyErr_Occurred()) {
                    Py_DECREF(cexe);
                    Py_DECREF(m);
                    return NULL;
                }
                if (str_path != NULL) {
                    wcs_path = PyUnicode_AsWideCharString(str_path, NULL);
                    if (wcs_path == NULL) {
                        Py_DECREF(cexe);
                        Py_DECREF(m);
                        return NULL;
                    }
                    SetEnvironmentVariableW(L"TCL_LIBRARY", wcs_path);
                    set_var = 1;
                }
            }

            Tcl_FindExecutable(PyBytes_AS_STRING(cexe));

            if (set_var) {
                SetEnvironmentVariableW(L"TCL_LIBRARY", NULL);
                PyMem_Free(wcs_path);
            }
#else
            Tcl_FindExecutable(PyBytes_AS_STRING(cexe));
#endif /* MS_WINDOWS */
        }
        Py_XDECREF(cexe);
    }
    else {
        Py_XDECREF(uexe);
    }

    if (PyErr_Occurred()) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

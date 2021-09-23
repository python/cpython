
/* Readline interface for tokenizer.c and [raw_]input() in bltinmodule.c.
   By default, or when stdin is not a tty device, we have a super
   simple my_readline function using fgets.
   Optionally, we can use the GNU readline library.
   my_readline() has a different return value from GNU readline():
   - NULL if an interrupt occurred or if an error occurred
   - a malloc'ed empty string if EOF was read
   - a malloc'ed string ending in \n normally
*/

#include "Python.h"
#include "pycore_pystate.h"   // _PyThreadState_GET()
#ifdef MS_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"
#endif /* MS_WINDOWS */


PyThreadState* _PyOS_ReadlineTState = NULL;

static PyThread_type_lock _PyOS_ReadlineLock = NULL;

int (*PyOS_InputHook)(void) = NULL;

/* This function restarts a fgets() after an EINTR error occurred
   except if _PyOS_InterruptOccurred() returns true. */

static int
my_fgets(PyThreadState* tstate, char *buf, int len, FILE *fp)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    _Py_BEGIN_SUPPRESS_IPH
    handle = (HANDLE)_get_osfhandle(fileno(fp));
    _Py_END_SUPPRESS_IPH

    /* bpo-40826: fgets(fp) does crash if fileno(fp) is closed */
    if (handle == INVALID_HANDLE_VALUE) {
        return -1; /* EOF */
    }
#endif

    while (1) {
        if (PyOS_InputHook != NULL) {
            (void)(PyOS_InputHook)();
        }

        errno = 0;
        clearerr(fp);
        char *p = fgets(buf, len, fp);
        if (p != NULL) {
            return 0; /* No error */
        }
        int err = errno;

#ifdef MS_WINDOWS
        /* Ctrl-C anywhere on the line or Ctrl-Z if the only character
           on a line will set ERROR_OPERATION_ABORTED. Under normal
           circumstances Ctrl-C will also have caused the SIGINT handler
           to fire which will have set the event object returned by
           _PyOS_SigintEvent. This signal fires in another thread and
           is not guaranteed to have occurred before this point in the
           code.

           Therefore: check whether the event is set with a small timeout.
           If it is, assume this is a Ctrl-C and reset the event. If it
           isn't set assume that this is a Ctrl-Z on its own and drop
           through to check for EOF.
        */
        if (GetLastError()==ERROR_OPERATION_ABORTED) {
            HANDLE hInterruptEvent = _PyOS_SigintEvent();
            switch (WaitForSingleObjectEx(hInterruptEvent, 10, FALSE)) {
            case WAIT_OBJECT_0:
                ResetEvent(hInterruptEvent);
                return 1; /* Interrupt */
            case WAIT_FAILED:
                return -2; /* Error */
            }
        }
#endif /* MS_WINDOWS */

        if (feof(fp)) {
            clearerr(fp);
            return -1; /* EOF */
        }

#ifdef EINTR
        if (err == EINTR) {
            PyEval_RestoreThread(tstate);
            int s = PyErr_CheckSignals();
            PyEval_SaveThread();

            if (s < 0) {
                return 1;
            }
            /* try again */
            continue;
        }
#endif

        if (_PyOS_InterruptOccurred(tstate)) {
            return 1; /* Interrupt */
        }
        return -2; /* Error */
    }
    /* NOTREACHED */
}

#ifdef MS_WINDOWS
/* Readline implementation using ReadConsoleW */

extern char _get_console_type(HANDLE handle);

char *
_PyOS_WindowsConsoleReadline(PyThreadState *tstate, HANDLE hStdIn)
{
    static wchar_t wbuf_local[1024 * 16];
    const DWORD chunk_size = 1024;

    DWORD n_read, total_read, wbuflen, u8len;
    wchar_t *wbuf;
    char *buf = NULL;
    int err = 0;

    n_read = (DWORD)-1;
    total_read = 0;
    wbuf = wbuf_local;
    wbuflen = sizeof(wbuf_local) / sizeof(wbuf_local[0]) - 1;
    while (1) {
        if (PyOS_InputHook != NULL) {
            (void)(PyOS_InputHook)();
        }
        if (!ReadConsoleW(hStdIn, &wbuf[total_read], wbuflen - total_read, &n_read, NULL)) {
            err = GetLastError();
            goto exit;
        }
        if (n_read == (DWORD)-1 && (err = GetLastError()) == ERROR_OPERATION_ABORTED) {
            break;
        }
        if (n_read == 0) {
            int s;
            err = GetLastError();
            if (err != ERROR_OPERATION_ABORTED)
                goto exit;
            err = 0;
            HANDLE hInterruptEvent = _PyOS_SigintEvent();
            if (WaitForSingleObjectEx(hInterruptEvent, 100, FALSE)
                    == WAIT_OBJECT_0) {
                ResetEvent(hInterruptEvent);
                PyEval_RestoreThread(tstate);
                s = PyErr_CheckSignals();
                PyEval_SaveThread();
                if (s < 0) {
                    goto exit;
                }
            }
            break;
        }

        total_read += n_read;
        if (total_read == 0 || wbuf[total_read - 1] == L'\n') {
            break;
        }
        wbuflen += chunk_size;
        if (wbuf == wbuf_local) {
            wbuf[total_read] = '\0';
            wbuf = (wchar_t*)PyMem_RawMalloc(wbuflen * sizeof(wchar_t));
            if (wbuf) {
                wcscpy_s(wbuf, wbuflen, wbuf_local);
            }
            else {
                PyEval_RestoreThread(tstate);
                PyErr_NoMemory();
                PyEval_SaveThread();
                goto exit;
            }
        }
        else {
            wchar_t *tmp = PyMem_RawRealloc(wbuf, wbuflen * sizeof(wchar_t));
            if (tmp == NULL) {
                PyEval_RestoreThread(tstate);
                PyErr_NoMemory();
                PyEval_SaveThread();
                goto exit;
            }
            wbuf = tmp;
        }
    }

    if (wbuf[0] == '\x1a') {
        buf = PyMem_RawMalloc(1);
        if (buf) {
            buf[0] = '\0';
        }
        else {
            PyEval_RestoreThread(tstate);
            PyErr_NoMemory();
            PyEval_SaveThread();
        }
        goto exit;
    }

    u8len = WideCharToMultiByte(CP_UTF8, 0,
                                wbuf, total_read,
                                NULL, 0,
                                NULL, NULL);
    buf = PyMem_RawMalloc(u8len + 1);
    if (buf == NULL) {
        PyEval_RestoreThread(tstate);
        PyErr_NoMemory();
        PyEval_SaveThread();
        goto exit;
    }

    u8len = WideCharToMultiByte(CP_UTF8, 0,
                                wbuf, total_read,
                                buf, u8len,
                                NULL, NULL);
    buf[u8len] = '\0';

exit:
    if (wbuf != wbuf_local) {
        PyMem_RawFree(wbuf);
    }

    if (err) {
        PyEval_RestoreThread(tstate);
        PyErr_SetFromWindowsErr(err);
        PyEval_SaveThread();
    }
    return buf;
}

#endif


/* Readline implementation using fgets() */

char *
PyOS_StdioReadline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt)
{
    size_t n;
    char *p, *pr;
    PyThreadState *tstate = _PyOS_ReadlineTState;
    assert(tstate != NULL);

#ifdef MS_WINDOWS
    if (!Py_LegacyWindowsStdioFlag && sys_stdin == stdin) {
        HANDLE hStdIn, hStdErr;

        hStdIn = _Py_get_osfhandle_noraise(fileno(sys_stdin));
        hStdErr = _Py_get_osfhandle_noraise(fileno(stderr));

        if (_get_console_type(hStdIn) == 'r') {
            fflush(sys_stdout);
            if (prompt) {
                if (_get_console_type(hStdErr) == 'w') {
                    wchar_t *wbuf;
                    int wlen;
                    wlen = MultiByteToWideChar(CP_UTF8, 0, prompt, -1,
                            NULL, 0);
                    if (wlen) {
                        wbuf = PyMem_RawMalloc(wlen * sizeof(wchar_t));
                        if (wbuf == NULL) {
                            PyEval_RestoreThread(tstate);
                            PyErr_NoMemory();
                            PyEval_SaveThread();
                            return NULL;
                        }
                        wlen = MultiByteToWideChar(CP_UTF8, 0, prompt, -1,
                                wbuf, wlen);
                        if (wlen) {
                            DWORD n;
                            fflush(stderr);
                            /* wlen includes null terminator, so subtract 1 */
                            WriteConsoleW(hStdErr, wbuf, wlen - 1, &n, NULL);
                        }
                        PyMem_RawFree(wbuf);
                    }
                } else {
                    fprintf(stderr, "%s", prompt);
                    fflush(stderr);
                }
            }
            clearerr(sys_stdin);
            return _PyOS_WindowsConsoleReadline(tstate, hStdIn);
        }
    }
#endif

    fflush(sys_stdout);
    if (prompt) {
        fprintf(stderr, "%s", prompt);
    }
    fflush(stderr);

    n = 0;
    p = NULL;
    do {
        size_t incr = (n > 0) ? n + 2 : 100;
        if (incr > INT_MAX) {
            PyMem_RawFree(p);
            PyEval_RestoreThread(tstate);
            PyErr_SetString(PyExc_OverflowError, "input line too long");
            PyEval_SaveThread();
            return NULL;
        }
        pr = (char *)PyMem_RawRealloc(p, n + incr);
        if (pr == NULL) {
            PyMem_RawFree(p);
            PyEval_RestoreThread(tstate);
            PyErr_NoMemory();
            PyEval_SaveThread();
            return NULL;
        }
        p = pr;
        int err = my_fgets(tstate, p + n, (int)incr, sys_stdin);
        if (err == 1) {
            // Interrupt
            PyMem_RawFree(p);
            return NULL;
        } else if (err != 0) {
            // EOF or error
            p[n] = '\0';
            break;
        }
        n += strlen(p + n);
    } while (p[n-1] != '\n');

    pr = (char *)PyMem_RawRealloc(p, n+1);
    if (pr == NULL) {
        PyMem_RawFree(p);
        PyEval_RestoreThread(tstate);
        PyErr_NoMemory();
        PyEval_SaveThread();
        return NULL;
    }
    return pr;
}


/* By initializing this function pointer, systems embedding Python can
   override the readline function.

   Note: Python expects in return a buffer allocated with PyMem_Malloc. */

char *(*PyOS_ReadlineFunctionPointer)(FILE *, FILE *, const char *) = NULL;


/* Interface used by tokenizer.c and bltinmodule.c */

char *
PyOS_Readline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt)
{
    char *rv, *res;
    size_t len;

    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyOS_ReadlineTState == tstate) {
        PyErr_SetString(PyExc_RuntimeError,
                        "can't re-enter readline");
        return NULL;
    }


    if (PyOS_ReadlineFunctionPointer == NULL) {
        PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;
    }

    if (_PyOS_ReadlineLock == NULL) {
        _PyOS_ReadlineLock = PyThread_allocate_lock();
        if (_PyOS_ReadlineLock == NULL) {
            PyErr_SetString(PyExc_MemoryError, "can't allocate lock");
            return NULL;
        }
    }

    _PyOS_ReadlineTState = tstate;
    Py_BEGIN_ALLOW_THREADS
    PyThread_acquire_lock(_PyOS_ReadlineLock, 1);

    /* This is needed to handle the unlikely case that the
     * interpreter is in interactive mode *and* stdin/out are not
     * a tty.  This can happen, for example if python is run like
     * this: python -i < test1.py
     */
    if (!isatty (fileno (sys_stdin)) || !isatty (fileno (sys_stdout)))
        rv = PyOS_StdioReadline (sys_stdin, sys_stdout, prompt);
    else
        rv = (*PyOS_ReadlineFunctionPointer)(sys_stdin, sys_stdout,
                                             prompt);
    Py_END_ALLOW_THREADS

    PyThread_release_lock(_PyOS_ReadlineLock);

    _PyOS_ReadlineTState = NULL;

    if (rv == NULL)
        return NULL;

    len = strlen(rv) + 1;
    res = PyMem_Malloc(len);
    if (res != NULL) {
        memcpy(res, rv, len);
    }
    else {
        PyErr_NoMemory();
    }
    PyMem_RawFree(rv);

    return res;
}

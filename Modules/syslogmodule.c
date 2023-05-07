/***********************************************************
Copyright 1994 by Lance Ellinghouse,
Cathedral City, California Republic, United States of America.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Lance Ellinghouse
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

LANCE ELLINGHOUSE DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL LANCE ELLINGHOUSE BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/******************************************************************

Revision history:

2010/04/20 (Sean Reifschneider)
  - Use basename(sys.argv[0]) for the default "ident".
  - Arguments to openlog() are now keyword args and are all optional.
  - syslog() calls openlog() if it hasn't already been called.

1998/04/28 (Sean Reifschneider)
  - When facility not specified to syslog() method, use default from openlog()
    (This is how it was claimed to work in the documentation)
  - Potential resource leak of o_ident, now cleaned up in closelog()
  - Minor comment accuracy fix.

95/06/29 (Steve Clift)
  - Changed arg parsing to use PyArg_ParseTuple.
  - Added PyErr_Clear() call(s) where needed.
  - Fix core dumps if user message contains format specifiers.
  - Change openlog arg defaults to match normal syslog behavior.
  - Plug memory leak in openlog().
  - Fix setlogmask() to return previous mask value.

******************************************************************/

/* syslog module */

#include "Python.h"
#include "osdefs.h"               // SEP

#include <syslog.h>

/*[clinic input]
module syslog
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=478f4ac94a1d4cae]*/

#include "clinic/syslogmodule.c.h"

/*  only one instance, only one syslog, so globals should be ok,
 *  these fields are writable from the main interpreter only. */
static PyObject *S_ident_o = NULL;  // identifier, held by openlog()
static char S_log_open = 0;

static inline int
is_main_interpreter(void)
{
    return (PyInterpreterState_Get() == PyInterpreterState_Main());
}

static PyObject *
syslog_get_argv(void)
{
    /* Figure out what to use for as the program "ident" for openlog().
     * This swallows exceptions and continues rather than failing out,
     * because the syslog module can still be used because openlog(3)
     * is optional.
     */

    Py_ssize_t argv_len, scriptlen;
    PyObject *scriptobj;
    Py_ssize_t slash;
    PyObject *argv = PySys_GetObject("argv");

    if (argv == NULL) {
        return(NULL);
    }

    argv_len = PyList_Size(argv);
    if (argv_len == -1) {
        PyErr_Clear();
        return(NULL);
    }
    if (argv_len == 0) {
        return(NULL);
    }

    scriptobj = PyList_GetItem(argv, 0);
    if (scriptobj == NULL) {
        PyErr_Clear();
        return NULL;
    }
    if (!PyUnicode_Check(scriptobj)) {
        return(NULL);
    }
    scriptlen = PyUnicode_GET_LENGTH(scriptobj);
    if (scriptlen == 0) {
        return(NULL);
    }

    slash = PyUnicode_FindChar(scriptobj, SEP, 0, scriptlen, -1);
    if (slash == -2) {
        PyErr_Clear();
        return NULL;
    }
    if (slash != -1) {
        return PyUnicode_Substring(scriptobj, slash + 1, scriptlen);
    } else {
        Py_INCREF(scriptobj);
        return(scriptobj);
    }
}


/*[clinic input]
syslog.openlog

    ident: unicode = NULL
    logoption as logopt: long = 0
    facility: long(c_default="LOG_USER") = LOG_USER

Set logging options of subsequent syslog() calls.
[clinic start generated code]*/

static PyObject *
syslog_openlog_impl(PyObject *module, PyObject *ident, long logopt,
                    long facility)
/*[clinic end generated code: output=5476c12829b6eb75 input=8a987a96a586eee7]*/
{
    // Since the sys.openlog changes the process level state of syslog library,
    // this operation is only allowed for the main interpreter.
    if (!is_main_interpreter()) {
        PyErr_SetString(PyExc_RuntimeError, "subinterpreter can't use syslog.openlog()");
        return NULL;
    }

    const char *ident_str = NULL;

    if (ident) {
        Py_INCREF(ident);
    }
    else {
        /* get sys.argv[0] or NULL if we can't for some reason  */
        ident = syslog_get_argv();
    }

    /* At this point, ident should be INCREF()ed.  openlog(3) does not
     * make a copy, and syslog(3) later uses it.  We can't garbagecollect it.
     * If NULL, just let openlog figure it out (probably using C argv[0]).
     */
    if (ident) {
        ident_str = PyUnicode_AsUTF8(ident);
        if (ident_str == NULL) {
            Py_DECREF(ident);
            return NULL;
        }
    }
    if (PySys_Audit("syslog.openlog", "Oll", ident ? ident : Py_None, logopt, facility) < 0) {
        Py_DECREF(ident);
        return NULL;
    }

    openlog(ident_str, logopt, facility);
    S_log_open = 1;
    Py_XSETREF(S_ident_o, ident);

    Py_RETURN_NONE;
}



/*[clinic input]
syslog.syslog

    [
    priority: int(c_default="LOG_INFO") = LOG_INFO
    ]

    message: str

    /

Send the string message to the system logger.
[clinic start generated code]*/

static PyObject *
syslog_syslog_impl(PyObject *module, int group_left_1, int priority,
                   const char *message)
/*[clinic end generated code: output=c3dbc73445a0e078 input=ac83d92b12ea3d4e]*/
{
    if (PySys_Audit("syslog.syslog", "is", priority, message) < 0) {
        return NULL;
    }

    /*  if log is not opened, open it now  */
    if (!S_log_open) {
        if (!is_main_interpreter()) {
            PyErr_SetString(PyExc_RuntimeError, "subinterpreter can't use syslog.syslog() "
                                                "until the syslog is opened by the main interpreter");
            return NULL;
        }
        PyObject *openlog_ret = syslog_openlog_impl(module, NULL, 0, LOG_USER);
        if (openlog_ret == NULL) {
            return NULL;
        }
        Py_DECREF(openlog_ret);
    }

    /* Incref ident, because it can be decrefed if syslog.openlog() is
     * called when the GIL is released.
     */
    PyObject *ident = Py_XNewRef(S_ident_o);
#ifdef __APPLE__
    // gh-98178: On macOS, libc syslog() is not thread-safe
    syslog(priority, "%s", message);
#else
    Py_BEGIN_ALLOW_THREADS;
    syslog(priority, "%s", message);
    Py_END_ALLOW_THREADS;
#endif
    Py_XDECREF(ident);
    Py_RETURN_NONE;
}


/*[clinic input]
syslog.closelog

Reset the syslog module values and call the system library closelog().
[clinic start generated code]*/

static PyObject *
syslog_closelog_impl(PyObject *module)
/*[clinic end generated code: output=97890a80a24b1b84 input=fb77a54d447acf07]*/
{
    // Since the sys.closelog changes the process level state of syslog library,
    // this operation is only allowed for the main interpreter.
    if (!is_main_interpreter()) {
        PyErr_SetString(PyExc_RuntimeError, "sunbinterpreter can't use syslog.closelog()");
        return NULL;
    }

    if (PySys_Audit("syslog.closelog", NULL) < 0) {
        return NULL;
    }
    if (S_log_open) {
        closelog();
        Py_CLEAR(S_ident_o);
        S_log_open = 0;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
syslog.setlogmask -> long

    maskpri: long
    /

Set the priority mask to maskpri and return the previous mask value.
[clinic start generated code]*/

static long
syslog_setlogmask_impl(PyObject *module, long maskpri)
/*[clinic end generated code: output=d6ed163917b434bf input=adff2c2b76c7629c]*/
{
    if (PySys_Audit("syslog.setlogmask", "l", maskpri) < 0) {
        return -1;
    }

    return setlogmask(maskpri);
}

/*[clinic input]
syslog.LOG_MASK -> long

    pri: long
    /

Calculates the mask for the individual priority pri.
[clinic start generated code]*/

static long
syslog_LOG_MASK_impl(PyObject *module, long pri)
/*[clinic end generated code: output=c4a5bbfcc74c7c94 input=534829cb7fb5f7d2]*/
{
    return LOG_MASK(pri);
}

/*[clinic input]
syslog.LOG_UPTO -> long

    pri: long
    /

Calculates the mask for all priorities up to and including pri.
[clinic start generated code]*/

static long
syslog_LOG_UPTO_impl(PyObject *module, long pri)
/*[clinic end generated code: output=9eab083c90601d7e input=5e906d6c406b7458]*/
{
    return LOG_UPTO(pri);
}

/* List of functions defined in the module */

static PyMethodDef syslog_methods[] = {
    SYSLOG_OPENLOG_METHODDEF
    SYSLOG_CLOSELOG_METHODDEF
    SYSLOG_SYSLOG_METHODDEF
    SYSLOG_SETLOGMASK_METHODDEF
    SYSLOG_LOG_MASK_METHODDEF
    SYSLOG_LOG_UPTO_METHODDEF
    {NULL,              NULL,                   0}
};


static int
syslog_exec(PyObject *module)
{
#define ADD_INT_MACRO(module, macro)                                  \
    do {                                                              \
        if (PyModule_AddIntConstant(module, #macro, macro) < 0) {     \
            return -1;                                                \
        }                                                             \
    } while (0)
    /* Priorities */
    ADD_INT_MACRO(module, LOG_EMERG);
    ADD_INT_MACRO(module, LOG_ALERT);
    ADD_INT_MACRO(module, LOG_CRIT);
    ADD_INT_MACRO(module, LOG_ERR);
    ADD_INT_MACRO(module, LOG_WARNING);
    ADD_INT_MACRO(module, LOG_NOTICE);
    ADD_INT_MACRO(module, LOG_INFO);
    ADD_INT_MACRO(module, LOG_DEBUG);

    /* openlog() option flags */
    ADD_INT_MACRO(module, LOG_PID);
    ADD_INT_MACRO(module, LOG_CONS);
    ADD_INT_MACRO(module, LOG_NDELAY);
#ifdef LOG_ODELAY
    ADD_INT_MACRO(module, LOG_ODELAY);
#endif
#ifdef LOG_NOWAIT
    ADD_INT_MACRO(module, LOG_NOWAIT);
#endif
#ifdef LOG_PERROR
    ADD_INT_MACRO(module, LOG_PERROR);
#endif

    /* Facilities */
    ADD_INT_MACRO(module, LOG_KERN);
    ADD_INT_MACRO(module, LOG_USER);
    ADD_INT_MACRO(module, LOG_MAIL);
    ADD_INT_MACRO(module, LOG_DAEMON);
    ADD_INT_MACRO(module, LOG_AUTH);
    ADD_INT_MACRO(module, LOG_LPR);
    ADD_INT_MACRO(module, LOG_LOCAL0);
    ADD_INT_MACRO(module, LOG_LOCAL1);
    ADD_INT_MACRO(module, LOG_LOCAL2);
    ADD_INT_MACRO(module, LOG_LOCAL3);
    ADD_INT_MACRO(module, LOG_LOCAL4);
    ADD_INT_MACRO(module, LOG_LOCAL5);
    ADD_INT_MACRO(module, LOG_LOCAL6);
    ADD_INT_MACRO(module, LOG_LOCAL7);

#ifndef LOG_SYSLOG
#define LOG_SYSLOG              LOG_DAEMON
#endif
#ifndef LOG_NEWS
#define LOG_NEWS                LOG_MAIL
#endif
#ifndef LOG_UUCP
#define LOG_UUCP                LOG_MAIL
#endif
#ifndef LOG_CRON
#define LOG_CRON                LOG_DAEMON
#endif

    ADD_INT_MACRO(module, LOG_SYSLOG);
    ADD_INT_MACRO(module, LOG_CRON);
    ADD_INT_MACRO(module, LOG_UUCP);
    ADD_INT_MACRO(module, LOG_NEWS);

#ifdef LOG_AUTHPRIV
    ADD_INT_MACRO(module, LOG_AUTHPRIV);
#endif

    return 0;
}

static PyModuleDef_Slot syslog_slots[] = {
    {Py_mod_exec, syslog_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

/* Initialization function for the module */

static struct PyModuleDef syslogmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "syslog",
    .m_size = 0,
    .m_methods = syslog_methods,
    .m_slots = syslog_slots,
};

PyMODINIT_FUNC
PyInit_syslog(void)
{
    return PyModuleDef_Init(&syslogmodule);
}

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

/*  only one instance, only one syslog, so globals should be ok  */
static PyObject *S_ident_o = NULL;                      /*  identifier, held by openlog()  */
static char S_log_open = 0;


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
    if (!PyUnicode_Check(scriptobj)) {
        return(NULL);
    }
    scriptlen = PyUnicode_GET_LENGTH(scriptobj);
    if (scriptlen == 0) {
        return(NULL);
    }

    slash = PyUnicode_FindChar(scriptobj, SEP, 0, scriptlen, -1);
    if (slash == -2)
        return NULL;
    if (slash != -1) {
        return PyUnicode_Substring(scriptobj, slash + 1, scriptlen);
    } else {
        Py_INCREF(scriptobj);
        return(scriptobj);
    }

    return(NULL);
}


static PyObject *
syslog_openlog(PyObject * self, PyObject * args, PyObject *kwds)
{
    long logopt = 0;
    long facility = LOG_USER;
    PyObject *new_S_ident_o = NULL;
    static char *keywords[] = {"ident", "logoption", "facility", 0};
    const char *ident = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                          "|Ull:openlog", keywords, &new_S_ident_o, &logopt, &facility))
        return NULL;

    if (new_S_ident_o) {
        Py_INCREF(new_S_ident_o);
    }

    /*  get sys.argv[0] or NULL if we can't for some reason  */
    if (!new_S_ident_o) {
        new_S_ident_o = syslog_get_argv();
    }

    Py_XDECREF(S_ident_o);
    S_ident_o = new_S_ident_o;

    /* At this point, S_ident_o should be INCREF()ed.  openlog(3) does not
     * make a copy, and syslog(3) later uses it.  We can't garbagecollect it
     * If NULL, just let openlog figure it out (probably using C argv[0]).
     */
    if (S_ident_o) {
        ident = PyUnicode_AsUTF8(S_ident_o);
        if (ident == NULL)
            return NULL;
    }

    if (PySys_Audit("syslog.openlog", "sll", ident, logopt, facility) < 0) {
        return NULL;
    }

    openlog(ident, logopt, facility);
    S_log_open = 1;

    Py_RETURN_NONE;
}


static PyObject *
syslog_syslog(PyObject * self, PyObject * args)
{
    PyObject *message_object;
    const char *message;
    int   priority = LOG_INFO;

    if (!PyArg_ParseTuple(args, "iU;[priority,] message string",
                          &priority, &message_object)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "U;[priority,] message string",
                              &message_object))
            return NULL;
    }

    message = PyUnicode_AsUTF8(message_object);
    if (message == NULL)
        return NULL;

    if (PySys_Audit("syslog.syslog", "is", priority, message) < 0) {
        return NULL;
    }

    /*  if log is not opened, open it now  */
    if (!S_log_open) {
        PyObject *openargs;

        /* Continue even if PyTuple_New fails, because openlog(3) is optional.
         * So, we can still do logging in the unlikely event things are so hosed
         * that we can't do this tuple.
         */
        if ((openargs = PyTuple_New(0))) {
            PyObject *openlog_ret = syslog_openlog(self, openargs, NULL);
            Py_XDECREF(openlog_ret);
            Py_DECREF(openargs);
        }
    }

    Py_BEGIN_ALLOW_THREADS;
    syslog(priority, "%s", message);
    Py_END_ALLOW_THREADS;
    Py_RETURN_NONE;
}

static PyObject *
syslog_closelog(PyObject *self, PyObject *unused)
{
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

static PyObject *
syslog_setlogmask(PyObject *self, PyObject *args)
{
    long maskpri, omaskpri;

    if (!PyArg_ParseTuple(args, "l;mask for priority", &maskpri))
        return NULL;
    if (PySys_Audit("syslog.setlogmask", "(O)", args ? args : Py_None) < 0) {
        return NULL;
    }
    omaskpri = setlogmask(maskpri);
    return PyLong_FromLong(omaskpri);
}

static PyObject *
syslog_log_mask(PyObject *self, PyObject *args)
{
    long mask;
    long pri;
    if (!PyArg_ParseTuple(args, "l:LOG_MASK", &pri))
        return NULL;
    mask = LOG_MASK(pri);
    return PyLong_FromLong(mask);
}

static PyObject *
syslog_log_upto(PyObject *self, PyObject *args)
{
    long mask;
    long pri;
    if (!PyArg_ParseTuple(args, "l:LOG_UPTO", &pri))
        return NULL;
    mask = LOG_UPTO(pri);
    return PyLong_FromLong(mask);
}

/* List of functions defined in the module */

static PyMethodDef syslog_methods[] = {
    {"openlog",         (PyCFunction)(void(*)(void)) syslog_openlog,           METH_VARARGS | METH_KEYWORDS},
    {"closelog",        syslog_closelog,        METH_NOARGS},
    {"syslog",          syslog_syslog,          METH_VARARGS},
    {"setlogmask",      syslog_setlogmask,      METH_VARARGS},
    {"LOG_MASK",        syslog_log_mask,        METH_VARARGS},
    {"LOG_UPTO",        syslog_log_upto,        METH_VARARGS},
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
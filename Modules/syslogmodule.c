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

#include <syslog.h>

/*  only one instance, only one syslog, so globals should be ok  */
static PyObject *S_ident_o = NULL;			/*  identifier, held by openlog()  */


static PyObject * 
syslog_openlog(PyObject * self, PyObject * args)
{
	long logopt = 0;
	long facility = LOG_USER;


	Py_XDECREF(S_ident_o);
	if (!PyArg_ParseTuple(args,
			      "S|ll;ident string [, logoption [, facility]]",
			      &S_ident_o, &logopt, &facility))
		return NULL;

	/* This is needed because openlog() does NOT make a copy
	 * and syslog() later uses it.. cannot trash it.
	 */
	Py_INCREF(S_ident_o);

	openlog(PyString_AsString(S_ident_o), logopt, facility);

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject * 
syslog_syslog(PyObject * self, PyObject * args)
{
	char *message;
	int   priority = LOG_INFO;

	if (!PyArg_ParseTuple(args, "is;[priority,] message string",
			      &priority, &message)) {
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "s;[priority,] message string",
				      &message))
			return NULL;
	}

	syslog(priority, "%s", message);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * 
syslog_closelog(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":closelog"))
		return NULL;
	closelog();
	Py_XDECREF(S_ident_o);
	S_ident_o = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * 
syslog_setlogmask(PyObject *self, PyObject *args)
{
	long maskpri, omaskpri;

	if (!PyArg_ParseTuple(args, "l;mask for priority", &maskpri))
		return NULL;
	omaskpri = setlogmask(maskpri);
	return PyInt_FromLong(omaskpri);
}

static PyObject * 
syslog_log_mask(PyObject *self, PyObject *args)
{
	long mask;
	long pri;
	if (!PyArg_ParseTuple(args, "l:LOG_MASK", &pri))
		return NULL;
	mask = LOG_MASK(pri);
	return PyInt_FromLong(mask);
}

static PyObject * 
syslog_log_upto(PyObject *self, PyObject *args)
{
	long mask;
	long pri;
	if (!PyArg_ParseTuple(args, "l:LOG_UPTO", &pri))
		return NULL;
	mask = LOG_UPTO(pri);
	return PyInt_FromLong(mask);
}

/* List of functions defined in the module */

static PyMethodDef syslog_methods[] = {
	{"openlog",	syslog_openlog,		METH_VARARGS},
	{"closelog",	syslog_closelog,	METH_VARARGS},
	{"syslog",	syslog_syslog,		METH_VARARGS},
	{"setlogmask",	syslog_setlogmask,	METH_VARARGS},
	{"LOG_MASK",	syslog_log_mask,	METH_VARARGS},
	{"LOG_UPTO",	syslog_log_upto,	METH_VARARGS},
	{NULL,		NULL,			0}
};

/* helper function for initialization function */

static void
ins(PyObject *d, char *s, long x)
{
	PyObject *v = PyInt_FromLong(x);
	if (v) {
		PyDict_SetItemString(d, s, v);
		Py_DECREF(v);
	}
}

/* Initialization function for the module */

DL_EXPORT(void)
initsyslog(void)
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("syslog", syslog_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);

	/* Priorities */
	ins(d, "LOG_EMERG",	LOG_EMERG);
	ins(d, "LOG_ALERT",	LOG_ALERT);
	ins(d, "LOG_CRIT",	LOG_CRIT);
	ins(d, "LOG_ERR",	LOG_ERR);
	ins(d, "LOG_WARNING",	LOG_WARNING);
	ins(d, "LOG_NOTICE",	LOG_NOTICE);
	ins(d, "LOG_INFO",	LOG_INFO);
	ins(d, "LOG_DEBUG",	LOG_DEBUG);

	/* openlog() option flags */
	ins(d, "LOG_PID",	LOG_PID);
	ins(d, "LOG_CONS",	LOG_CONS);
	ins(d, "LOG_NDELAY",	LOG_NDELAY);
#ifdef LOG_NOWAIT
	ins(d, "LOG_NOWAIT",	LOG_NOWAIT);
#endif
#ifdef LOG_PERROR
	ins(d, "LOG_PERROR",	LOG_PERROR);
#endif

	/* Facilities */
	ins(d, "LOG_KERN",	LOG_KERN);
	ins(d, "LOG_USER",	LOG_USER);
	ins(d, "LOG_MAIL",	LOG_MAIL);
	ins(d, "LOG_DAEMON",	LOG_DAEMON);
	ins(d, "LOG_AUTH",	LOG_AUTH);
	ins(d, "LOG_LPR",	LOG_LPR);
	ins(d, "LOG_LOCAL0",	LOG_LOCAL0);
	ins(d, "LOG_LOCAL1",	LOG_LOCAL1);
	ins(d, "LOG_LOCAL2",	LOG_LOCAL2);
	ins(d, "LOG_LOCAL3",	LOG_LOCAL3);
	ins(d, "LOG_LOCAL4",	LOG_LOCAL4);
	ins(d, "LOG_LOCAL5",	LOG_LOCAL5);
	ins(d, "LOG_LOCAL6",	LOG_LOCAL6);
	ins(d, "LOG_LOCAL7",	LOG_LOCAL7);

#ifndef LOG_SYSLOG
#define LOG_SYSLOG		LOG_DAEMON
#endif
#ifndef LOG_NEWS
#define LOG_NEWS		LOG_MAIL
#endif
#ifndef LOG_UUCP
#define LOG_UUCP		LOG_MAIL
#endif
#ifndef LOG_CRON
#define LOG_CRON		LOG_DAEMON
#endif

	ins(d, "LOG_SYSLOG",	LOG_SYSLOG);
	ins(d, "LOG_CRON",	LOG_CRON);
	ins(d, "LOG_UUCP",	LOG_UUCP);
	ins(d, "LOG_NEWS",	LOG_NEWS);
}

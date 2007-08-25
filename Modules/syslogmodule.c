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
	PyObject *new_S_ident_o;
	const char *ident;

	if (!PyArg_ParseTuple(args,
			      "U|ll;ident string [, logoption [, facility]]",
			      &new_S_ident_o, &logopt, &facility))
		return NULL;

	/* This is needed because openlog() does NOT make a copy
	 * and syslog() later uses it.. cannot trash it.
	 */
	Py_XDECREF(S_ident_o);
	S_ident_o = new_S_ident_o;
	Py_INCREF(S_ident_o);

	ident = PyUnicode_AsString(S_ident_o);
	if (ident == NULL)
		return NULL;
	openlog(ident, logopt, facility);

	Py_INCREF(Py_None);
	return Py_None;
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

	message = PyUnicode_AsString(message_object);
	if (message == NULL)
		return NULL;
	syslog(priority, "%s", message);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * 
syslog_closelog(PyObject *self, PyObject *unused)
{
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
	{"closelog",	syslog_closelog,	METH_NOARGS},
	{"syslog",	syslog_syslog,		METH_VARARGS},
	{"setlogmask",	syslog_setlogmask,	METH_VARARGS},
	{"LOG_MASK",	syslog_log_mask,	METH_VARARGS},
	{"LOG_UPTO",	syslog_log_upto,	METH_VARARGS},
	{NULL,		NULL,			0}
};

/* Initialization function for the module */

PyMODINIT_FUNC
initsyslog(void)
{
	PyObject *m;

	/* Create the module and add the functions */
	m = Py_InitModule("syslog", syslog_methods);
	if (m == NULL)
		return;

	/* Add some symbolic constants to the module */

	/* Priorities */
	PyModule_AddIntConstant(m, "LOG_EMERG",	  LOG_EMERG);
	PyModule_AddIntConstant(m, "LOG_ALERT",	  LOG_ALERT);
	PyModule_AddIntConstant(m, "LOG_CRIT",	  LOG_CRIT);
	PyModule_AddIntConstant(m, "LOG_ERR",	  LOG_ERR);
	PyModule_AddIntConstant(m, "LOG_WARNING", LOG_WARNING);
	PyModule_AddIntConstant(m, "LOG_NOTICE",  LOG_NOTICE);
	PyModule_AddIntConstant(m, "LOG_INFO",	  LOG_INFO);
	PyModule_AddIntConstant(m, "LOG_DEBUG",	  LOG_DEBUG);

	/* openlog() option flags */
	PyModule_AddIntConstant(m, "LOG_PID",	  LOG_PID);
	PyModule_AddIntConstant(m, "LOG_CONS",	  LOG_CONS);
	PyModule_AddIntConstant(m, "LOG_NDELAY",  LOG_NDELAY);
#ifdef LOG_NOWAIT
	PyModule_AddIntConstant(m, "LOG_NOWAIT",  LOG_NOWAIT);
#endif
#ifdef LOG_PERROR
	PyModule_AddIntConstant(m, "LOG_PERROR",  LOG_PERROR);
#endif

	/* Facilities */
	PyModule_AddIntConstant(m, "LOG_KERN",	  LOG_KERN);
	PyModule_AddIntConstant(m, "LOG_USER",	  LOG_USER);
	PyModule_AddIntConstant(m, "LOG_MAIL",	  LOG_MAIL);
	PyModule_AddIntConstant(m, "LOG_DAEMON",  LOG_DAEMON);
	PyModule_AddIntConstant(m, "LOG_AUTH",	  LOG_AUTH);
	PyModule_AddIntConstant(m, "LOG_LPR",	  LOG_LPR);
	PyModule_AddIntConstant(m, "LOG_LOCAL0",  LOG_LOCAL0);
	PyModule_AddIntConstant(m, "LOG_LOCAL1",  LOG_LOCAL1);
	PyModule_AddIntConstant(m, "LOG_LOCAL2",  LOG_LOCAL2);
	PyModule_AddIntConstant(m, "LOG_LOCAL3",  LOG_LOCAL3);
	PyModule_AddIntConstant(m, "LOG_LOCAL4",  LOG_LOCAL4);
	PyModule_AddIntConstant(m, "LOG_LOCAL5",  LOG_LOCAL5);
	PyModule_AddIntConstant(m, "LOG_LOCAL6",  LOG_LOCAL6);
	PyModule_AddIntConstant(m, "LOG_LOCAL7",  LOG_LOCAL7);

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

	PyModule_AddIntConstant(m, "LOG_SYSLOG",  LOG_SYSLOG);
	PyModule_AddIntConstant(m, "LOG_CRON",	  LOG_CRON);
	PyModule_AddIntConstant(m, "LOG_UUCP",	  LOG_UUCP);
	PyModule_AddIntConstant(m, "LOG_NEWS",	  LOG_NEWS);
}

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

95/06/29 (Steve Clift)
  - Changed arg parsing to use PyArg_ParseTuple.
  - Added PyErr_Clear() call(s) where needed.
  - Fix core dumps if user message contains format specifiers.
  - Change openlog arg defaults to match normal syslog behaviour.
  - Plug memory leak in openlog().
  - Fix setlogmask() to return previous mask value.

******************************************************************/

/* syslog module */

#include "Python.h"

#include <syslog.h>

static PyObject * 
syslog_openlog(self, args)
     PyObject * self;
     PyObject * args;
{
  long logopt = 0;
  long facility = LOG_USER;

  static PyObject *ident_o = NULL;

  Py_XDECREF(ident_o);
  if (!PyArg_ParseTuple(args, "S|ll;ident string [, logoption [, facility]]",
			&ident_o, &logopt, &facility)) {
	return NULL;
  }
  Py_INCREF(ident_o); /* This is needed because openlog() does NOT make a copy
		         and syslog() later uses it.. cannot trash it. */

  openlog(PyString_AsString(ident_o), logopt, facility);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
syslog_syslog(self, args)
     PyObject * self;
     PyObject * args;
{
  char *message;
  int   priority = LOG_INFO | LOG_USER;

  if (!PyArg_ParseTuple(args, "is;[priority,] message string",
			&priority, &message)) {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "s;[priority,] message string", &message)) {
      return NULL;
    }
  }
  syslog(priority, "%s", message);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
syslog_closelog(self, args)
     PyObject * self;
     PyObject * args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	closelog();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * 
syslog_setlogmask(self, args)
     PyObject * self;
     PyObject * args;
{
  long maskpri, omaskpri;

  if (!PyArg_ParseTuple(args,"l;mask for priority",&maskpri))
    return NULL;
  omaskpri = setlogmask(maskpri);
  return PyInt_FromLong(omaskpri);
}

static PyObject * 
syslog_log_mask(self, args)
     PyObject * self;
     PyObject * args;
{
  long mask;
  long pri;
  if (!PyArg_ParseTuple(args,"l",&pri))
    return NULL;
  mask = LOG_MASK(pri);
  return PyInt_FromLong(mask);
}

static PyObject * 
syslog_log_upto(self, args)
     PyObject * self;
     PyObject * args;
{
  long mask;
  long pri;
  if (!PyArg_ParseTuple(args,"l",&pri))
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

/* Initialization function for the module */

#define DICT_SET_INT(d, s, x) \
	PyDict_SetItemString(d, s, PyInt_FromLong((long) (x)))

void
initsyslog()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("syslog", syslog_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);

	/* Priorities */
	DICT_SET_INT(d, "LOG_EMERG",	LOG_EMERG);
	DICT_SET_INT(d, "LOG_ALERT",	LOG_ALERT);
	DICT_SET_INT(d, "LOG_CRIT",	LOG_CRIT);
	DICT_SET_INT(d, "LOG_ERR",	LOG_ERR);
	DICT_SET_INT(d, "LOG_WARNING",	LOG_WARNING);
	DICT_SET_INT(d, "LOG_NOTICE",	LOG_NOTICE);
	DICT_SET_INT(d, "LOG_INFO",	LOG_INFO);
	DICT_SET_INT(d, "LOG_DEBUG",	LOG_DEBUG);

	/* openlog() option flags */
	DICT_SET_INT(d, "LOG_PID",	LOG_PID);
	DICT_SET_INT(d, "LOG_CONS",	LOG_CONS);
	DICT_SET_INT(d, "LOG_NDELAY",	LOG_NDELAY);
	DICT_SET_INT(d, "LOG_NOWAIT",	LOG_NOWAIT);
#ifdef LOG_PERROR
	DICT_SET_INT(d, "LOG_PERROR",	LOG_PERROR);
#endif

	/* Facilities */
	DICT_SET_INT(d, "LOG_KERN",	LOG_KERN);
	DICT_SET_INT(d, "LOG_USER",	LOG_USER);
	DICT_SET_INT(d, "LOG_MAIL",	LOG_MAIL);
	DICT_SET_INT(d, "LOG_DAEMON",	LOG_DAEMON);
	DICT_SET_INT(d, "LOG_AUTH",	LOG_AUTH);
	DICT_SET_INT(d, "LOG_LPR",	LOG_LPR);
#ifdef LOG_NEWS
	DICT_SET_INT(d, "LOG_NEWS",	LOG_NEWS);
#else
	DICT_SET_INT(d, "LOG_NEWS",	LOG_MAIL);
#endif
#ifdef LOG_UUCP
	DICT_SET_INT(d, "LOG_UUCP",	LOG_UUCP);
#else
	DICT_SET_INT(d, "LOG_UUCP",	LOG_MAIL);
#endif
#ifdef LOG_CRON
	DICT_SET_INT(d, "LOG_CRON",	LOG_CRON);
#else
	DICT_SET_INT(d, "LOG_CRON",	LOG_DAEMON);
#endif
	DICT_SET_INT(d, "LOG_LOCAL0",	LOG_LOCAL0);
	DICT_SET_INT(d, "LOG_LOCAL1",	LOG_LOCAL1);
	DICT_SET_INT(d, "LOG_LOCAL2",	LOG_LOCAL2);
	DICT_SET_INT(d, "LOG_LOCAL3",	LOG_LOCAL3);
	DICT_SET_INT(d, "LOG_LOCAL4",	LOG_LOCAL4);
	DICT_SET_INT(d, "LOG_LOCAL5",	LOG_LOCAL5);
	DICT_SET_INT(d, "LOG_LOCAL6",	LOG_LOCAL6);
	DICT_SET_INT(d, "LOG_LOCAL7",	LOG_LOCAL7);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module syslog");
}

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

/* syslog module */

#include "Python.h"

#include <syslog.h>

static PyObject * 
syslog_openlog(self, args)
     PyObject * self;
     PyObject * args;
{
  char *ident = "";
  PyObject * ident_o;
  long logopt = LOG_PID;
  long facility = LOG_USER;
  if (!PyArg_Parse(args, "(Sll);ident string, logoption, facility", &ident_o, &logopt, &facility))
    if (!PyArg_Parse(args, "(Sl);ident string, logoption", &ident_o, &logopt))
      if (!PyArg_Parse(args, "S;ident string", &ident_o))
	return NULL;
  Py_INCREF(ident_o); /* This is needed because openlog() does NOT make a copy
		      and syslog() later uses it.. cannot trash it. */
  ident = PyString_AsString(ident_o);
  openlog(ident,logopt,facility);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
syslog_syslog(self, args)
     PyObject * self;
     PyObject * args;
{
  int   priority = LOG_INFO;
  char *message;

  if (!PyArg_Parse(args,"(is);priority, message string",&priority,&message))
    if (!PyArg_Parse(args,"s;message string",&message))
      return NULL;
  syslog(priority, message);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
syslog_closelog(self, args)
     PyObject * self;
     PyObject * args;
{
	if (!PyArg_NoArgs(args))
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
  long maskpri;
  if (!PyArg_Parse(args,"l;mask for priority",&maskpri))
    return NULL;
  setlogmask(maskpri);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
syslog_log_mask(self, args)
     PyObject * self;
     PyObject * args;
{
  long mask;
  long pri;
  if (!PyArg_Parse(args,"l",&pri))
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
  if (!PyArg_Parse(args,"l",&pri))
    return NULL;
  mask = LOG_UPTO(pri);
  return PyInt_FromLong(mask);
}

/* List of functions defined in the module */

static PyMethodDef syslog_methods[] = {
	{"openlog",		(PyCFunction)syslog_openlog},
	{"closelog",		(PyCFunction)syslog_closelog},
	{"syslog",              (PyCFunction)syslog_syslog},
	{"setlogmask",          (PyCFunction)syslog_setlogmask},
	{"LOG_MASK",            (PyCFunction)syslog_log_mask},
	{"LOG_UPTO",            (PyCFunction)syslog_log_upto},
	{NULL,		NULL}		/* sentinel */
};

/* Initialization function for the module */

void
initsyslog()
{
	PyObject *m, *d, *x;

	/* Create the module and add the functions */
	m = Py_InitModule("syslog", syslog_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	x = PyInt_FromLong(LOG_EMERG);
	PyDict_SetItemString(d, "LOG_EMERG", x);
	x = PyInt_FromLong(LOG_ALERT);
	PyDict_SetItemString(d, "LOG_ALERT", x);
	x = PyInt_FromLong(LOG_CRIT);
	PyDict_SetItemString(d, "LOG_CRIT", x);
	x = PyInt_FromLong(LOG_ERR);
	PyDict_SetItemString(d, "LOG_ERR", x);
	x = PyInt_FromLong(LOG_WARNING);
	PyDict_SetItemString(d, "LOG_WARNING", x);
	x = PyInt_FromLong(LOG_NOTICE);
	PyDict_SetItemString(d, "LOG_NOTICE", x);
	x = PyInt_FromLong(LOG_INFO);
	PyDict_SetItemString(d, "LOG_INFO", x);
	x = PyInt_FromLong(LOG_DEBUG);
	PyDict_SetItemString(d, "LOG_DEBUG", x);
	x = PyInt_FromLong(LOG_PID);
	PyDict_SetItemString(d, "LOG_PID", x);
	x = PyInt_FromLong(LOG_CONS);
	PyDict_SetItemString(d, "LOG_CONS", x);
	x = PyInt_FromLong(LOG_NDELAY);
	PyDict_SetItemString(d, "LOG_NDELAY", x);
	x = PyInt_FromLong(LOG_NOWAIT);
	PyDict_SetItemString(d, "LOG_NOWAIT", x);
	x = PyInt_FromLong(LOG_KERN);
	PyDict_SetItemString(d, "LOG_KERN", x);
	x = PyInt_FromLong(LOG_USER);
	PyDict_SetItemString(d, "LOG_USER", x);
	x = PyInt_FromLong(LOG_MAIL);
	PyDict_SetItemString(d, "LOG_MAIL", x);
	x = PyInt_FromLong(LOG_DAEMON);
	PyDict_SetItemString(d, "LOG_DAEMON", x);
	x = PyInt_FromLong(LOG_LPR);
	PyDict_SetItemString(d, "LOG_LPR", x);
	x = PyInt_FromLong(LOG_LOCAL0);
	PyDict_SetItemString(d, "LOG_LOCAL0", x);
	x = PyInt_FromLong(LOG_LOCAL1);
	PyDict_SetItemString(d, "LOG_LOCAL1", x);
	x = PyInt_FromLong(LOG_LOCAL2);
	PyDict_SetItemString(d, "LOG_LOCAL2", x);
	x = PyInt_FromLong(LOG_LOCAL3);
	PyDict_SetItemString(d, "LOG_LOCAL3", x);
	x = PyInt_FromLong(LOG_LOCAL4);
	PyDict_SetItemString(d, "LOG_LOCAL4", x);
	x = PyInt_FromLong(LOG_LOCAL5);
	PyDict_SetItemString(d, "LOG_LOCAL5", x);
	x = PyInt_FromLong(LOG_LOCAL6);
	PyDict_SetItemString(d, "LOG_LOCAL6", x);
	x = PyInt_FromLong(LOG_LOCAL7);
	PyDict_SetItemString(d, "LOG_LOCAL7", x);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module syslog");
}

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* SGI module -- random SGI-specific things */

#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static PyObject *
sgi_nap(self, args)
	PyObject *self;
	PyObject *args;
{
	long ticks;
	if (!PyArg_Parse(args, "l", &ticks))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	sginap(ticks);
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}

extern char *_getpty(int *, int, mode_t, int);

static PyObject *
sgi__getpty(self, args)
	PyObject *self;
	PyObject *args;
{
	int oflag;
	int mode;
	int nofork;
	char *name;
	int fildes;
	if (!PyArg_Parse(args, "(iii)", &oflag, &mode, &nofork))
		return NULL;
	errno = 0;
	name = _getpty(&fildes, oflag, (mode_t)mode, nofork);
	if (name == NULL) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	return Py_BuildValue("(si)", name, fildes);
}

static PyMethodDef sgi_methods[] = {
	{"nap",		sgi_nap},
	{"_getpty",	sgi__getpty},
	{NULL,		NULL}		/* sentinel */
};


void
initsgi()
{
	Py_InitModule("sgi", sgi_methods);
}

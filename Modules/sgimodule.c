/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

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

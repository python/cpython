
/* SGI module -- random SGI-specific things */

#include "Python.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static PyObject *
sgi_nap(PyObject *self, PyObject *args)
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
sgi__getpty(PyObject *self, PyObject *args)
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
initsgi(void)
{
	Py_InitModule("sgi", sgi_methods);
}
